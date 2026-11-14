#include "HttpConnection.h"
#include "LogicSystem.h"
#include <cctype>

namespace {

bool FromHex(unsigned char character, unsigned char& value) {
    if (character >= 'A' && character <= 'F') {
        value = character - 'A' + 10;
        return true;
    }
    if (character >= 'a' && character <= 'f') {
        value = character - 'a' + 10;
        return true;
    }
    if (character >= '0' && character <= '9') {
        value = character - '0';
        return true;
    }
    return false;
}

bool UrlDecode(const std::string& input, std::string& output) {
    output.clear();
    output.reserve(input.size());
    for (std::size_t index = 0; index < input.size(); ++index) {
        if (input[index] == '+') {
            output += ' ';
            continue;
        }
        if (input[index] != '%') {
            output += input[index];
            continue;
        }
        if (index + 2 >= input.size()) {
            return false;
        }
        unsigned char high = 0;
        unsigned char low = 0;
        if (!FromHex(static_cast<unsigned char>(input[index + 1]), high) ||
            !FromHex(static_cast<unsigned char>(input[index + 2]), low)) {
            return false;
        }
        output += static_cast<char>(high * 16 + low);
        index += 2;
    }
    return true;
}

}

HttpConnection::HttpConnection(boost::asio::io_context& ioc)
    : _socket(ioc) {
}



void HttpConnection::Start() {
    auto self = shared_from_this();
    deadline_.expires_after(std::chrono::seconds(60));
    CheckDeadline();
    http::async_read(_socket, _buffer, _request,
        [self](beast::error_code ec, std::size_t bytes_transferred) {
            try {
                if (ec) {
                    std::cout << "http read err: " << ec.message() << std::endl;
                    return;
                }
                boost::ignore_unused(bytes_transferred);

                self->HandleReq();
            }
            catch (std::exception& exp) {
                std::cout << "exception: " << exp.what() << std::endl;
                self->_response = {};
                self->_response.version(self->_request.version());
                self->_response.keep_alive(false);
                self->_response.result(http::status::ok);
                self->_response.set(http::field::content_type, "application/json");
                Json::Value root;
                root["error"] = ErrorCodes::RPCFailed;
                root["message"] = exp.what();
                beast::ostream(self->_response.body()) << root.toStyledString();
                self->WriteResponse();
            }
        });
}

void HttpConnection::HandleReq() {
    _response.version(_request.version());
    _response.keep_alive(false);


    if (_request.method() == http::verb::get) {
        if (!PreParseGetParamze()) {
            _response.result(http::status::bad_request);
            _response.set(http::field::content_type, "text/plain");
            beast::ostream(_response.body()) << "invalid URL encoding\r\n";
            WriteResponse();
            return;
        }
        _response.result(http::status::ok);
        bool success = LogicSystem::GetInstance()->HandleGet(_get_url, shared_from_this());
        if (!success) {
            _response.result(http::status::not_found);
            _response.set(http::field::content_type, "text/plain");
            beast::ostream(_response.body()) << "url not found\r\n";
            WriteResponse();
            return;
        }

        _response.set(http::field::server, "GateServer");
        WriteResponse();
        return;
    }
    if (_request.method()==http::verb::post)
    {   

        auto path = std::string(_request.target());
        const auto query_position = path.find('?');
        if (query_position != std::string::npos) {
            path.erase(query_position);
        }
        _response.result(http::status::ok);
        bool success = LogicSystem::GetInstance()->HandlePost(path, shared_from_this());
        if (!success)
        {
            _response.result(http::status::not_found);
            _response.set(http::field::content_type, "text/plain");
            beast::ostream(_response.body()) << "url not found\r\n";
            WriteResponse();
            return;
        }
        _response.set(http::field::server, "GateServer");
        WriteResponse();
        return;
    }
    _response.result(http::status::bad_request);
    _response.set(http::field::content_type, "text/plain");
    beast::ostream(_response.body()) << "unsupported request method\r\n";
    WriteResponse();
}

void HttpConnection::WriteResponse() {
    auto self = shared_from_this();
    _response.content_length(_response.body().size());

    http::async_write(_socket, _response,
        [self](beast::error_code ec, std::size_t) {
            self->_socket.shutdown(tcp::socket::shutdown_send, ec);
            self->deadline_.cancel();
        });
}

void HttpConnection::CheckDeadline() {
    auto self = shared_from_this();
    deadline_.async_wait([self](beast::error_code ec) {
        if (!ec) {
            self->_socket.close(ec);
        }
        });
}

bool HttpConnection::PreParseGetParamze() {
    _get_params.clear();
    auto uri = _request.target();
    auto query_pos = uri.find('?');
    if (query_pos == std::string::npos) {
        _get_url = uri;
        return true;
    }

    _get_url = uri.substr(0, query_pos);
    std::string query_string = uri.substr(query_pos + 1);
    size_t pos = 0;
    while ((pos = query_string.find('&')) != std::string::npos) {
        auto pair = query_string.substr(0, pos);
        size_t eq_pos = pair.find('=');
        if (eq_pos != std::string::npos) {
            std::string key;
            std::string value;
            if (!UrlDecode(pair.substr(0, eq_pos), key) ||
                !UrlDecode(pair.substr(eq_pos + 1), value)) {
                return false;
            }
            _get_params[key] = value;
        }
        query_string.erase(0, pos + 1);
    }
    if (!query_string.empty()) {
        size_t eq_pos = query_string.find('=');
        if (eq_pos != std::string::npos) {
            std::string key;
            std::string value;
            if (!UrlDecode(query_string.substr(0, eq_pos), key) ||
                !UrlDecode(query_string.substr(eq_pos + 1), value)) {
                return false;
            }
            _get_params[key] = value;
        }
    }
    return true;
}
