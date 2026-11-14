#pragma once
#include "const.h"
#include <thread>
#include <jdbc/mysql_driver.h>
#include <jdbc/mysql_connection.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <cppconn/exception.h>
#include <cppconn/connection.h>
#include <cppconn/driver.h>
using namespace std;

class SqlConnection {
public:
	SqlConnection(sql::Connection* con, int64_t lasttime) :_con(con), _last_oper_time(lasttime) {}
	unique_ptr<sql::Connection> _con;
	int64_t _last_oper_time;
};

class MySqlPool {
public:
	MySqlPool(const std::string& host, const std::string& port, const std::string& user, const std::string& pass, const std::string& schema, int poolSize)
		: host_(host), port_(port), user_(user), pass_(pass), schema_(schema), poolSize_(poolSize), b_stop_(false), _fail_count(0) {
		try {
			for (int i = 0; i < poolSize_; ++i) {
				sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
				std::string url = "tcp://" + host_ + ":" + port_;
				std::cout << "mysql url=" << url << std::endl;
				auto* con = driver->connect(sql::SQLString(url), sql::SQLString(user_), sql::SQLString(pass_));
				con->setSchema(schema_);
				// ��ȡ��ǰʱ���
				auto currentTime = std::chrono::system_clock::now().time_since_epoch();
				// ��ʱ���ת��Ϊ��
				long long timestamp = std::chrono::duration_cast<std::chrono::seconds>(currentTime).count();
				pool_.push(std::make_unique<SqlConnection>(con, timestamp));
			}

			_check_thread = std::thread([this]() {
				while (!b_stop_) {
					checkConnectionPro();
					std::this_thread::sleep_for(std::chrono::seconds(60));
				}
				});

			_check_thread.detach();
		}
		catch (sql::SQLException& e) {
			// �����쳣
			std::cout << "mysql pool init failed, error is " << e.what() << std::endl;
			_fail_count = poolSize_;
			cond_.notify_all();
		}
		catch (const std::exception& e) {
			std::cout << "mysql pool init failed, exception is " << e.what() << std::endl;
			_fail_count = poolSize_;
			cond_.notify_all();
		}
	}

	void checkConnectionPro() {
		// 1)�ȶ�ȡ��Ŀ�괦������
		size_t targetCount;
		{
			std::lock_guard<std::mutex> guard(mutex_);
			targetCount = pool_.size();
		}

		//2 ��ǰ�Ѿ�����������
		size_t processed = 0;

		//3 ʱ���
		auto now = std::chrono::system_clock::now().time_since_epoch();
		long long timestamp = std::chrono::duration_cast<std::chrono::seconds>(now).count();

		while (processed < targetCount) {
			std::unique_ptr<SqlConnection> con;
			{
				std::lock_guard<std::mutex> guard(mutex_);
				if (pool_.empty()) {
					break;
				}
				con = std::move(pool_.front());
				pool_.pop();
			}

			bool healthy = true;
			//�����������/�����߼�
			if (timestamp - con->_last_oper_time >= 5) {
				try {
					std::unique_ptr<sql::Statement> stmt(con->_con->createStatement());
					stmt->executeQuery("SELECT 1");
					con->_last_oper_time = timestamp;
				}
				catch (sql::SQLException& e) {
					std::cout << "Error keeping connection alive: " << e.what() << std::endl;
					healthy = false;
					_fail_count++;
				}

			}

			if (healthy)
			{
				std::lock_guard<std::mutex> guard(mutex_);
				pool_.push(std::move(con));
				cond_.notify_one();
			}

			++processed;
		}

		while (_fail_count > 0) {
			auto b_res = reconnect(timestamp);
			if (b_res) {
				_fail_count--;
			}
			else {
				break;
			}
		}
	}

	bool reconnect(long long timestamp) {
		try {

			sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
			std::string url = "tcp://" + host_ + ":" + port_;
			std::cout << "mysql url=" << url << std::endl;
			auto* con = driver->connect(sql::SQLString(url), sql::SQLString(user_), sql::SQLString(pass_));
			con->setSchema(schema_);

			auto newCon = std::make_unique<SqlConnection>(con, timestamp);
			{
				std::lock_guard<std::mutex> guard(mutex_);
				pool_.push(std::move(newCon));
			}

			std::cout << "mysql connection reconnect success" << std::endl;
			return true;

		}
		catch (sql::SQLException& e) {
			std::cout << "Reconnect failed, error is " << e.what() << std::endl;
			return false;
		}
		catch (const std::exception& e) {
			std::cout << "Reconnect failed, exception is " << e.what() << std::endl;
			return false;
		}
	}

	void checkConnection() {
		std::lock_guard<std::mutex> guard(mutex_);
		int poolsize = pool_.size();
		// ��ȡ��ǰʱ���
		auto currentTime = std::chrono::system_clock::now().time_since_epoch();
		// ��ʱ���ת��Ϊ��
		long long timestamp = std::chrono::duration_cast<std::chrono::seconds>(currentTime).count();
		for (int i = 0; i < poolsize; i++) {
			auto con = std::move(pool_.front());
			pool_.pop();
			Defer defer([this, &con]() {
				pool_.push(std::move(con));
				});

			if (timestamp - con->_last_oper_time < 5) {
				continue;
			}

			try {
				std::unique_ptr<sql::Statement> stmt(con->_con->createStatement());
				stmt->executeQuery("SELECT 1");
				con->_last_oper_time = timestamp;
				//std::cout << "execute timer alive query , cur is " << timestamp << std::endl;
			}
			catch (sql::SQLException& e) {
				std::cout << "Error keeping connection alive: " << e.what() << std::endl;
				// ���´������Ӳ��滻�ɵ�����
				sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
				std::string url = "tcp://" + host_ + ":" + port_;
				std::cout << "mysql reconnect url=" << url << std::endl;
				auto* newcon = driver->connect(sql::SQLString(url), sql::SQLString(user_), sql::SQLString(pass_));
				newcon->setSchema(schema_);
				con->_con.reset(newcon);
				con->_last_oper_time = timestamp;
			}
		}
	}

	std::unique_ptr<SqlConnection> getConnection() {
		std::unique_lock<std::mutex> lock(mutex_);
		cond_.wait(lock, [this] {
			if (b_stop_) {
				return true;
			}
			return !pool_.empty() || _fail_count > 0; });
		if (b_stop_ || pool_.empty()) {
			return nullptr;
		}
		std::unique_ptr<SqlConnection> con(std::move(pool_.front()));
		pool_.pop();
		return con;
	}

	void returnConnection(std::unique_ptr<SqlConnection> con) {
		std::unique_lock<std::mutex> lock(mutex_);
		if (b_stop_) {
			return;
		}
		pool_.push(std::move(con));
		cond_.notify_one();
	}

	void Close() {
		b_stop_ = true;
		cond_.notify_all();
	}

	~MySqlPool() {
		std::unique_lock<std::mutex> lock(mutex_);
		while (!pool_.empty()) {
			pool_.pop();
		}
	}

private:
	std::string host_;
	std::string port_;
	std::string user_;
	std::string pass_;
	std::string schema_;
	int poolSize_;
	std::queue<std::unique_ptr<SqlConnection>> pool_;
	std::mutex mutex_;
	std::condition_variable cond_;
	std::atomic<bool> b_stop_;
	std::thread _check_thread;
	std::atomic<int> _fail_count;
};

struct UserInfo {
	std::string name;
	std::string pwd;
	int uid;
	std::string email;
};

class MysqlDao
{
public:
	MysqlDao();
	~MysqlDao();
	int RegUser(const std::string& name, const std::string& email, const std::string& pwd);
	int RegUserTransaction(const std::string& name, const std::string& email, const std::string& pwd);
	bool CheckEmail(const std::string& name, const std::string& email);
	bool UpdatePwd(const std::string& name, const std::string& newpwd);
	bool CheckPwd(const std::string& name, const std::string& pwd, UserInfo& userInfo);
	bool TestProcedure(const std::string& email, int& uid, string& name);
	bool IsAvailable();
private:
	std::unique_ptr<MySqlPool> pool_;
	std::string host_;
	std::string port_;
	std::string user_;
	std::string pass_;
	std::string schema_;
};


