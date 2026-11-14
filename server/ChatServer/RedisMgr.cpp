#include "RedisMgr.h"
#include "configMgr.h"
#include <vector>

namespace {

enum class RedisReplyType {
    SimpleString,
    Error,
    Integer,
    BulkString,
    Nil
};

struct RedisReply {
    RedisReplyType type = RedisReplyType::Error;
    std::string value;
    long long integer = 0;
};

struct RedisResult {
    RedisResult() = default;
    RedisResult(RedisReply value) : valid(true), reply(std::move(value)) {}

    explicit operator bool() const { return valid; }
    const RedisReply* operator->() const { return &reply; }

    bool valid = false;
    RedisReply reply;
};

std::string EncodeCommand(const std::vector<std::string>& arguments) {
    std::string command = "*" + std::to_string(arguments.size()) + "\r\n";
    for (const auto& argument : arguments) {
        command += "$" + std::to_string(argument.size()) + "\r\n";
        command.append(argument.data(), argument.size());
        command += "\r\n";
    }
    return command;
}

class RedisConnection {
public:
    RedisConnection(std::string host, unsigned short port, std::string password)
        : host_(std::move(host)), port_(port), password_(std::move(password)), socket_(ioc_) {
    }

    RedisResult Execute(const std::vector<std::string>& arguments) {
        try {
            if (!EnsureConnected()) {
                return {};
            }
            auto reply = ExecuteRaw(arguments);
            if (!reply) {
                Close();
            }
            return reply;
        }
        catch (const std::exception& exception) {
            std::cerr << "Redis command failed: " << exception.what() << std::endl;
            Close();
            return {};
        }
    }

    void Close() {
        boost::system::error_code error;
        socket_.close(error);
        response_buffer_.consume(response_buffer_.size());
    }

private:
    bool EnsureConnected() {
        if (socket_.is_open()) {
            return true;
        }

        tcp::resolver resolver(ioc_);
        const auto endpoints = resolver.resolve(host_, std::to_string(port_));
        boost::asio::connect(socket_, endpoints);
        if (!password_.empty()) {
            const auto reply = ExecuteRaw({ "AUTH", password_ });
            if (!reply || reply->type == RedisReplyType::Error) {
                Close();
                return false;
            }
        }
        return true;
    }

    RedisResult ExecuteRaw(const std::vector<std::string>& arguments) {
        const auto command = EncodeCommand(arguments);
        boost::asio::write(socket_, boost::asio::buffer(command));
        return ReadReply();
    }

    std::string ReadLine() {
        boost::asio::read_until(socket_, response_buffer_, "\r\n");
        std::istream input(&response_buffer_);
        std::string line;
        std::getline(input, line);
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        return line;
    }

    RedisResult ReadReply() {
        const auto line = ReadLine();
        if (line.empty()) {
            return {};
        }

        RedisReply reply;
        const auto payload = line.substr(1);
        switch (line.front()) {
        case '+':
            reply.type = RedisReplyType::SimpleString;
            reply.value = payload;
            return reply;
        case '-':
            reply.type = RedisReplyType::Error;
            reply.value = payload;
            return reply;
        case ':':
            reply.type = RedisReplyType::Integer;
            reply.integer = std::stoll(payload);
            return reply;
        case '$': {
            const auto length = std::stoll(payload);
            if (length == -1) {
                reply.type = RedisReplyType::Nil;
                return reply;
            }
            if (length < 0) {
                return {};
            }

            const auto required = static_cast<std::size_t>(length) + 2;
            if (response_buffer_.size() < required) {
                boost::asio::read(socket_, response_buffer_,
                    boost::asio::transfer_exactly(required - response_buffer_.size()));
            }
            std::istream input(&response_buffer_);
            reply.value.resize(static_cast<std::size_t>(length));
            if (length > 0) {
                input.read(&reply.value[0], static_cast<std::streamsize>(length));
            }
            char suffix[2]{};
            input.read(suffix, 2);
            if (suffix[0] != '\r' || suffix[1] != '\n') {
                return {};
            }
            reply.type = RedisReplyType::BulkString;
            return reply;
        }
        default:
            return {};
        }
    }

    std::string host_;
    unsigned short port_;
    std::string password_;
    boost::asio::io_context ioc_;
    tcp::socket socket_;
    boost::asio::streambuf response_buffer_;
};

}

class RedisConPool {
public:
    RedisConPool(std::size_t pool_size, const std::string& host, unsigned short port,
        const std::string& password) {
        for (std::size_t index = 0; index < pool_size; ++index) {
            connections_.push(std::make_shared<RedisConnection>(host, port, password));
        }
    }

    RedisResult Execute(const std::vector<std::string>& arguments) {
        auto connection = GetConnection();
        if (!connection) {
            return {};
        }
        const auto reply = connection->Execute(arguments);
        ReturnConnection(std::move(connection));
        return reply;
    }

    void Close() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (stopped_) {
            return;
        }
        stopped_ = true;
        while (!connections_.empty()) {
            connections_.front()->Close();
            connections_.pop();
        }
        condition_.notify_all();
    }

private:
    std::shared_ptr<RedisConnection> GetConnection() {
        std::unique_lock<std::mutex> lock(mutex_);
        condition_.wait(lock, [this] { return stopped_ || !connections_.empty(); });
        if (stopped_) {
            return nullptr;
        }
        auto connection = std::move(connections_.front());
        connections_.pop();
        return connection;
    }

    void ReturnConnection(std::shared_ptr<RedisConnection> connection) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (stopped_) {
            connection->Close();
            return;
        }
        connections_.push(std::move(connection));
        condition_.notify_one();
    }

    std::queue<std::shared_ptr<RedisConnection>> connections_;
    std::mutex mutex_;
    std::condition_variable condition_;
    bool stopped_ = false;
};

namespace {

bool IsSuccessfulInteger(const RedisResult& reply) {
    return reply && reply->type == RedisReplyType::Integer && reply->integer > 0;
}

}

RedisMgr::RedisMgr() {
    auto& config = ConfigMgr::Inst();
    const auto host = config["Redis"]["Host"];
    const auto port_text = config["Redis"]["Port"];
    auto password = config["Redis"]["Password"];
    if (password.empty()) {
        password = config["Redis"]["Passwd"];
    }

    const auto port_number = std::strtoul(port_text.c_str(), nullptr, 10);
    if (host.empty() || port_number == 0 || port_number > 65535) {
        throw std::runtime_error("Invalid Redis host or port");
    }
    con_pool_ = std::make_unique<RedisConPool>(5, host,
        static_cast<unsigned short>(port_number), password);
}

RedisMgr::~RedisMgr() {
    Close();
}

bool RedisMgr::Get(const std::string& key, std::string& value) {
    const auto reply = con_pool_->Execute({ "GET", key });
    if (!reply || reply->type != RedisReplyType::BulkString) {
        return false;
    }
    value = reply->value;
    return true;
}

bool RedisMgr::Set(const std::string& key, const std::string& value) {
    const auto reply = con_pool_->Execute({ "SET", key, value });
    return reply && reply->type == RedisReplyType::SimpleString && reply->value == "OK";
}

bool RedisMgr::Auth(const std::string& password) {
    const auto reply = con_pool_->Execute({ "AUTH", password });
    return reply && reply->type == RedisReplyType::SimpleString && reply->value == "OK";
}

bool RedisMgr::LPush(const std::string& key, const std::string& value) {
    return IsSuccessfulInteger(con_pool_->Execute({ "LPUSH", key, value }));
}

bool RedisMgr::LPop(const std::string& key, std::string& value) {
    const auto reply = con_pool_->Execute({ "LPOP", key });
    if (!reply || reply->type != RedisReplyType::BulkString) {
        return false;
    }
    value = reply->value;
    return true;
}

bool RedisMgr::RPush(const std::string& key, const std::string& value) {
    return IsSuccessfulInteger(con_pool_->Execute({ "RPUSH", key, value }));
}

bool RedisMgr::RPop(const std::string& key, std::string& value) {
    const auto reply = con_pool_->Execute({ "RPOP", key });
    if (!reply || reply->type != RedisReplyType::BulkString) {
        return false;
    }
    value = reply->value;
    return true;
}

bool RedisMgr::HSet(const std::string& key, const std::string& hash_key, const std::string& value) {
    const auto reply = con_pool_->Execute({ "HSET", key, hash_key, value });
    return reply && reply->type == RedisReplyType::Integer;
}

bool RedisMgr::HSet(const char* key, const char* hash_key, const char* value,
    std::size_t value_length) {
    return HSet(key, hash_key, std::string(value, value_length));
}

std::string RedisMgr::HGet(const std::string& key, const std::string& hash_key) {
    const auto reply = con_pool_->Execute({ "HGET", key, hash_key });
    return reply && reply->type == RedisReplyType::BulkString ? reply->value : std::string{};
}

bool RedisMgr::HDel(const std::string& key, const std::string& hash_key) {
    return IsSuccessfulInteger(con_pool_->Execute({ "HDEL", key, hash_key }));
}

bool RedisMgr::Del(const std::string& key) {
    return IsSuccessfulInteger(con_pool_->Execute({ "DEL", key }));
}

bool RedisMgr::ExistsKey(const std::string& key) {
    return IsSuccessfulInteger(con_pool_->Execute({ "EXISTS", key }));
}

void RedisMgr::Close() {
    if (con_pool_) {
        con_pool_->Close();
    }
}

