#include "MysqlDao.h"
#include "configMgr.h"
#include "const.h"
#include <mysql.h>
#include <cstdlib>

using namespace std;

namespace {
std::shared_ptr<UserInfo> BuildUserInfo(MYSQL_ROW row, int uidFallback = 0) {
	if (row == nullptr) {
		return nullptr;
	}

	auto user_ptr = std::make_shared<UserInfo>();
	user_ptr->uid = row[0] ? std::atoi(row[0]) : uidFallback;
	user_ptr->name = row[1] ? row[1] : "";
	user_ptr->email = row[2] ? row[2] : "";
	user_ptr->pwd = row[3] ? row[3] : "";
	user_ptr->nick = row[4] ? row[4] : user_ptr->name;
	user_ptr->desc = row[5] ? row[5] : "";
	user_ptr->sex = row[6] ? std::atoi(row[6]) : 0;
	user_ptr->icon = row[7] ? row[7] : "";
	return user_ptr;
}

MYSQL* OpenMysqlConnection(const std::string& host, const std::string& port,
	const std::string& user, const std::string& password, const std::string& schema) {
	MYSQL* mysql = mysql_init(nullptr);
	if (mysql == nullptr) {
		return nullptr;
	}

	if (mysql_real_connect(mysql, host.c_str(), user.c_str(), password.c_str(), schema.c_str(),
		static_cast<unsigned int>(std::stoul(port)), nullptr, 0) == nullptr) {
		std::cerr << "mysql_real_connect failed: " << mysql_error(mysql) << std::endl;
		mysql_close(mysql);
		return nullptr;
	}
	return mysql;
}
}

MysqlDao::MysqlDao()
{
	auto& cfg = ConfigMgr::Inst();
	const auto& host = cfg["Mysql"]["Host"];
	const auto& port = cfg["Mysql"]["Port"];
	const auto& pwd = cfg["Mysql"]["Passwd"];
	const auto& schema = cfg["Mysql"]["Schema"];
	const auto& user = cfg["Mysql"]["User"];
	host_ = host;
	port_ = port;
	user_ = user;
	pass_ = pwd;
	schema_ = schema;
	std::cout << "mysql host=" << host << " port=" << port << " schema=" << schema << " user=" << user << std::endl;
	pool_.reset(new MySqlPool(host, port, user, pwd, schema, 5));
}

MysqlDao::~MysqlDao() {
	pool_->Close();
}

int MysqlDao::RegUser(const std::string& name, const std::string& email, const std::string& pwd)
{
	auto con = pool_->getConnection();
	try {
		if (con == nullptr) {
			return false;
		}
		// ׼�����ô洢����
		unique_ptr < sql::PreparedStatement > stmt(con->_con->prepareStatement("CALL reg_user(?,?,?,@result)"));
		// �����������
		stmt->setString(1, name);
		stmt->setString(2, email);
		stmt->setString(3, pwd);

		// ����PreparedStatement��ֱ��֧��ע�����������������Ҫʹ�ûỰ������������������ȡ���������ֵ

		  // ִ�д洢����
		stmt->execute();
		// ����洢���������˻Ự��������������ʽ��ȡ���������ֵ�������������ִ��SELECT��ѯ����ȡ����
	   // ���磬����洢����������һ���Ự����@result���洢������������������ȡ��
		unique_ptr<sql::Statement> stmtResult(con->_con->createStatement());
		unique_ptr<sql::ResultSet> res(stmtResult->executeQuery("SELECT @result AS result"));
		if (res->next()) {
			int result = res->getInt("result");
			cout << "Result: " << result << endl;
			pool_->returnConnection(std::move(con));
			return result;
		}
		pool_->returnConnection(std::move(con));
		return -1;
	}
	catch (sql::SQLException& e) {
		pool_->returnConnection(std::move(con));
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return -1;
	}
}

int MysqlDao::RegUserTransaction(const std::string& name, const std::string& email, const std::string& pwd)
{
	auto con = pool_->getConnection();
	if (con == nullptr) {
		return false;
	}

	Defer defer([this, &con] {
		pool_->returnConnection(std::move(con));
		});

	try {
		//��ʼ����
		con->_con->setAutoCommit(false);
		//ִ�е�һ�����ݿ����������email�����û�
			// ׼����ѯ���

		std::unique_ptr<sql::PreparedStatement> pstmt_email(con->_con->prepareStatement("SELECT 1 FROM user WHERE email = ?"));

		// �󶨲���
		pstmt_email->setString(1, email);

		// ִ�в�ѯ
		std::unique_ptr<sql::ResultSet> res_email(pstmt_email->executeQuery());

		auto email_exist = res_email->next();
		if (email_exist) {
			con->_con->rollback();
			std::cout << "email " << email << " exist";
			return 0;
		}

		// ׼����ѯ�û����Ƿ��ظ�
		std::unique_ptr<sql::PreparedStatement> pstmt_name(con->_con->prepareStatement("SELECT 1 FROM user WHERE name = ?"));

		// �󶨲���
		pstmt_name->setString(1, name);

		// ִ�в�ѯ
		std::unique_ptr<sql::ResultSet> res_name(pstmt_name->executeQuery());

		auto name_exist = res_name->next();
		if (name_exist) {
			con->_con->rollback();
			std::cout << "name " << name << " exist";
			return 0;
		}

		// ׼�������û�id
		std::unique_ptr<sql::PreparedStatement> pstmt_upid(con->_con->prepareStatement("UPDATE user_id SET id = id + 1"));

		// ִ�и���
		pstmt_upid->executeUpdate();

		// ��ȡ���º�� id ֵ
		std::unique_ptr<sql::PreparedStatement> pstmt_uid(con->_con->prepareStatement("SELECT id FROM user_id"));
		std::unique_ptr<sql::ResultSet> res_uid(pstmt_uid->executeQuery());
		int newId = 0;
		// ���������
		if (res_uid->next()) {
			newId = res_uid->getInt("id");
		}
		else {
			std::cout << "select id from user_id failed" << std::endl;
			con->_con->rollback();
			return -1;
		}

		// ����user��Ϣ
		std::unique_ptr<sql::PreparedStatement> pstmt_insert(con->_con->prepareStatement("INSERT INTO user (uid, name, email, pwd, nick, icon) "
			"VALUES (?, ?, ?, ?,?,?)"));
		pstmt_insert->setInt(1, newId);
		pstmt_insert->setString(2, name);
		pstmt_insert->setString(3, email);
		pstmt_insert->setString(4, pwd);
		pstmt_insert->setString(5, name);
		pstmt_insert->setString(6, "");
		//ִ�в���
		pstmt_insert->executeUpdate();
		// �ύ����
		con->_con->commit();
		std::cout << "newuser insert into user success" << std::endl;
		return newId;
	}
	catch (sql::SQLException& e) {
		// ����������󣬻ع�����
		if (con) {
			con->_con->rollback();
		}
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return -1;
	}
}

bool MysqlDao::CheckEmail(const std::string& name, const std::string& email) {
	auto con = pool_->getConnection();
	try {
		if (con == nullptr) {
			return false;
		}

		// ׼����ѯ���
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("SELECT email FROM user WHERE name = ?"));

		// �󶨲���
		pstmt->setString(1, name);

		// ִ�в�ѯ
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

		// ���������
		while (res->next()) {
			std::cout << "Check Email: " << res->getString("email") << std::endl;
			if (email != res->getString("email")) {
				pool_->returnConnection(std::move(con));
				return false;
			}
			pool_->returnConnection(std::move(con));
			return true;
		}
		return false;
	}
	catch (sql::SQLException& e) {
		pool_->returnConnection(std::move(con));
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

bool MysqlDao::UpdatePwd(const std::string& name, const std::string& newpwd) {
	auto con = pool_->getConnection();
	try {
		if (con == nullptr) {
			return false;
		}

		// ׼����ѯ���
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("UPDATE user SET pwd = ? WHERE name = ?"));

		// �󶨲���
		pstmt->setString(2, name);
		pstmt->setString(1, newpwd);

		// ִ�и���
		int updateCount = pstmt->executeUpdate();

		std::cout << "Updated rows: " << updateCount << std::endl;
		pool_->returnConnection(std::move(con));
		return true;
	}
	catch (sql::SQLException& e) {
		pool_->returnConnection(std::move(con));
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

bool MysqlDao::CheckPwd(const std::string& email, const std::string& pwd, UserInfo& userInfo) {
	auto con = pool_->getConnection();
	if (con == nullptr) {
		return false;
	}

	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
		});

	try {
		// ׼��SQL���
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("SELECT * FROM user WHERE email = ?"));
		pstmt->setString(1, email); // ��username�滻Ϊ��Ҫ��ѯ���û���

		// ִ�в�ѯ
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		std::string origin_pwd = "";
		// ���������
		while (res->next()) {
			origin_pwd = res->getString("pwd");
			// �����ѯ��������
			std::cout << "Password: " << origin_pwd << std::endl;
			break;
		}

		if (pwd != origin_pwd) {
			return false;
		}
		userInfo.name = res->getString("name");
		userInfo.email = res->getString("email");
		userInfo.uid = res->getInt("uid");
		userInfo.pwd = origin_pwd;
		return true;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

bool MysqlDao::TestProcedure(const std::string& email, int& uid, string& name) {
	auto con = pool_->getConnection();
	try {
		if (con == nullptr) {
			return false;
		}

		Defer defer([this, &con]() {
			pool_->returnConnection(std::move(con));
			});

		unique_ptr < sql::PreparedStatement > stmt(con->_con->prepareStatement("CALL test_procedure(?,@userId,@userName)"));

		stmt->setString(1, email);


		stmt->execute();

		unique_ptr<sql::Statement> stmtResult(con->_con->createStatement());
		unique_ptr<sql::ResultSet> res(stmtResult->executeQuery("SELECT @userId AS uid"));
		if (!(res->next())) {
			return false;
		}

		uid = res->getInt("uid");
		cout << "uid: " << uid << endl;

		stmtResult.reset(con->_con->createStatement());
		res.reset(stmtResult->executeQuery("SELECT @userName AS name"));
		if (!(res->next())) {
			return false;
		}

		name = res->getString("name");
		cout << "name: " << name << endl;
		return true;

	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}
std::shared_ptr<UserInfo> MysqlDao::GetUser(int uid) {
	auto con = pool_->getConnection();
	if (con == nullptr) {
		MYSQL* mysql = mysql_init(nullptr);
		if (mysql == nullptr) {
			return nullptr;
		}

		Defer defer([mysql]() {
			mysql_close(mysql);
			});

		if (mysql_real_connect(mysql, host_.c_str(), user_.c_str(), pass_.c_str(), schema_.c_str(),
			static_cast<unsigned int>(std::stoul(port_)), nullptr, 0) == nullptr) {
			std::cerr << "mysql_real_connect failed: " << mysql_error(mysql) << std::endl;
			return nullptr;
		}

		std::string query = "SELECT uid,name,email,pwd,nick,`desc`,sex,icon FROM user WHERE uid=" + std::to_string(uid);
		if (mysql_query(mysql, query.c_str()) != 0) {
			std::cerr << "mysql_query failed: " << mysql_error(mysql) << std::endl;
			return nullptr;
		}

		MYSQL_RES* raw_res = mysql_store_result(mysql);
		if (raw_res == nullptr) {
			std::cerr << "mysql_store_result failed: " << mysql_error(mysql) << std::endl;
			return nullptr;
		}

		std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> res(raw_res, mysql_free_result);
		return BuildUserInfo(mysql_fetch_row(res.get()), uid);
	}

	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
		});

	try {
		// 准备SQL语句
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("SELECT uid,name,email,pwd,nick,`desc`,sex,icon FROM user WHERE uid = ?"));
		pstmt->setInt(1, uid); // 将uid替换为你要查询的uid

		// 执行查询
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		std::shared_ptr<UserInfo> user_ptr = nullptr;
		// 遍历结果集
		while (res->next()) {
			user_ptr.reset(new UserInfo);
			user_ptr->pwd = res->getString("pwd");
			user_ptr->email = res->getString("email");
			user_ptr->name = res->getString("name");
			user_ptr->nick = res->getString("nick");
			user_ptr->desc = res->getString("desc");
			user_ptr->sex = res->getInt("sex");
			user_ptr->icon = res->getString("icon");
			user_ptr->uid = uid;
			break;
		}
		return user_ptr;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return nullptr;
	}
}

std::shared_ptr<UserInfo> MysqlDao::GetUser(std::string name) {
	auto con = pool_->getConnection();
	if (con == nullptr) {
		MYSQL* mysql = mysql_init(nullptr);
		if (mysql == nullptr) {
			return nullptr;
		}

		Defer defer([mysql]() {
			mysql_close(mysql);
			});

		if (mysql_real_connect(mysql, host_.c_str(), user_.c_str(), pass_.c_str(), schema_.c_str(),
			static_cast<unsigned int>(std::stoul(port_)), nullptr, 0) == nullptr) {
			std::cerr << "mysql_real_connect failed: " << mysql_error(mysql) << std::endl;
			return nullptr;
		}

		std::string escaped_name;
		escaped_name.resize(name.size() * 2 + 1);
		auto escaped_len = mysql_real_escape_string(mysql, &escaped_name[0], name.c_str(), static_cast<unsigned long>(name.size()));
		escaped_name.resize(escaped_len);

		std::string query = "SELECT uid,name,email,pwd,nick,`desc`,sex,icon FROM user WHERE name='" + escaped_name + "'";
		if (mysql_query(mysql, query.c_str()) != 0) {
			std::cerr << "mysql_query failed: " << mysql_error(mysql) << std::endl;
			return nullptr;
		}

		MYSQL_RES* raw_res = mysql_store_result(mysql);
		if (raw_res == nullptr) {
			std::cerr << "mysql_store_result failed: " << mysql_error(mysql) << std::endl;
			return nullptr;
		}

		std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> res(raw_res, mysql_free_result);
		return BuildUserInfo(mysql_fetch_row(res.get()));
	}

	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
		});

	try {
		// 准备SQL语句
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("SELECT uid,name,email,pwd,nick,`desc`,sex,icon FROM user WHERE name = ?"));
		pstmt->setString(1, name); // 将uid替换为你要查询的uid

		// 执行查询
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		std::shared_ptr<UserInfo> user_ptr = nullptr;
		// 遍历结果集
		while (res->next()) {
			user_ptr.reset(new UserInfo);
			user_ptr->pwd = res->getString("pwd");
			user_ptr->email = res->getString("email");
			user_ptr->name = res->getString("name");
			user_ptr->nick = res->getString("nick");
			user_ptr->desc = res->getString("desc");
			user_ptr->sex = res->getInt("sex");
			user_ptr->uid = res->getInt("uid");
			user_ptr->icon = res->getString("icon");
			break;
		}
		return user_ptr;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return nullptr;
	}
}

bool MysqlDao::AddFriendApply(const int& from, const int& to) {
	auto con = pool_->getConnection();
	if (con == nullptr) {
		MYSQL* mysql = OpenMysqlConnection(host_, port_, user_, pass_, schema_);
		if (mysql == nullptr) {
			return false;
		}
		Defer close_mysql([mysql]() { mysql_close(mysql); });
		std::string query = "INSERT INTO friend_apply(from_uid,to_uid) VALUES(" +
			std::to_string(from) + "," + std::to_string(to) +
			") ON DUPLICATE KEY UPDATE from_uid=from_uid,to_uid=to_uid";
		if (mysql_query(mysql, query.c_str()) != 0) {
			std::cerr << "AddFriendApply failed: " << mysql_error(mysql) << std::endl;
			return false;
		}
		return true;
	}

	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
		});
	try {
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("INSERT INTO friend_apply(from_uid,to_uid) values(?,?)"
		"ON DUPLICATE KEY UPDATE from_uid = from_uid, to_uid = to_uid "));
		pstmt->setInt(1, from);
		pstmt->setInt(2, to);
		//执行更新
		int rowAffected = pstmt->executeUpdate();
		if (rowAffected < 0) {
			return false;
		}
		return true;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;

	}
}


bool MysqlDao::GetApplyList(int touid, std::vector<std::shared_ptr<ApplyInfo>>& applylist, int begin, int limit) {
	auto con = pool_->getConnection();
	if (con == nullptr) {
		MYSQL* mysql = OpenMysqlConnection(host_, port_, user_, pass_, schema_);
		if (mysql == nullptr) {
			return false;
		}
		Defer close_mysql([mysql]() { mysql_close(mysql); });
		std::string query = "SELECT apply.from_uid,apply.status,user.name,user.nick,user.sex "
			"FROM friend_apply AS apply JOIN user ON apply.from_uid=user.uid WHERE apply.to_uid=" +
			std::to_string(touid) + " AND apply.id>" + std::to_string(begin) +
			" ORDER BY apply.id ASC LIMIT " + std::to_string(limit);
		if (mysql_query(mysql, query.c_str()) != 0) {
			std::cerr << "GetApplyList failed: " << mysql_error(mysql) << std::endl;
			return false;
		}
		MYSQL_RES* raw_res = mysql_store_result(mysql);
		if (raw_res == nullptr) {
			std::cerr << "GetApplyList store result failed: " << mysql_error(mysql) << std::endl;
			return false;
		}
		std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> res(raw_res, mysql_free_result);
		while (auto row = mysql_fetch_row(res.get())) {
			auto apply_ptr = std::make_shared<ApplyInfo>(
				row[0] ? std::atoi(row[0]) : 0,
				row[2] ? row[2] : "", "", "",
				row[3] ? row[3] : "",
				row[4] ? std::atoi(row[4]) : 0,
				row[1] ? std::atoi(row[1]) : 0);
			applylist.push_back(apply_ptr);
		}
		return true;
	}

	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
		});


	try {
		// 准备SQL语句, 根据起始id和限制条数返回列表
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("select apply.from_uid, apply.status, user.name, "
			"user.nick, user.sex from friend_apply as apply join user on apply.from_uid = user.uid where apply.to_uid = ? "
			"and apply.id > ? order by apply.id ASC LIMIT ? "));

		pstmt->setInt(1, touid); // 将uid替换为你要查询的uid
		pstmt->setInt(2, begin); // 起始id
		pstmt->setInt(3, limit); //偏移量
		// 执行查询
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		// 遍历结果集
		while (res->next()) {
			auto name = res->getString("name");
			auto uid = res->getInt("from_uid");
			auto status = res->getInt("status");
			auto nick = res->getString("nick");
			auto sex = res->getInt("sex");
			auto apply_ptr = std::make_shared<ApplyInfo>(uid, name, "", "", nick, sex, status);
			applylist.push_back(apply_ptr);
		}
		return true;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

bool MysqlDao::AuthFriendApply(const int& from, const int& to) {
	auto con = pool_->getConnection();
	if (con == nullptr) {
		MYSQL* mysql = OpenMysqlConnection(host_, port_, user_, pass_, schema_);
		if (mysql == nullptr) {
			return false;
		}
		Defer close_mysql([mysql]() { mysql_close(mysql); });
		std::string query = "UPDATE friend_apply SET status=1 WHERE from_uid=" +
			std::to_string(to) + " AND to_uid=" + std::to_string(from);
		if (mysql_query(mysql, query.c_str()) != 0) {
			std::cerr << "AuthFriendApply failed: " << mysql_error(mysql) << std::endl;
			return false;
		}
		return true;
	}

	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
		});

	try {
		// 准备SQL语句
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("UPDATE friend_apply SET status = 1 "
			"WHERE from_uid = ? AND to_uid = ?"));
		//反过来的申请时from，验证时to
		pstmt->setInt(1, to); // from id
		pstmt->setInt(2, from);
		// 执行更新
		int rowAffected = pstmt->executeUpdate();
		if (rowAffected < 0) {
			return false;
		}
		return true;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}


	return true;
}

bool MysqlDao::AddFriend(const int& from, const int& to, std::string back_name) {
	auto con = pool_->getConnection();
	if (con == nullptr) {
		MYSQL* mysql = OpenMysqlConnection(host_, port_, user_, pass_, schema_);
		if (mysql == nullptr) {
			return false;
		}
		Defer close_mysql([mysql]() { mysql_close(mysql); });
		std::string escaped_back(back_name.size() * 2 + 1, '\0');
		auto escaped_len = mysql_real_escape_string(mysql, &escaped_back[0], back_name.c_str(),
			static_cast<unsigned long>(back_name.size()));
		escaped_back.resize(escaped_len);
		if (mysql_autocommit(mysql, 0) != 0) {
			return false;
		}
		std::string first = "INSERT IGNORE INTO friend(self_id,friend_id,back) VALUES(" +
			std::to_string(from) + "," + std::to_string(to) + ",'" + escaped_back + "')";
		std::string second = "INSERT IGNORE INTO friend(self_id,friend_id,back) VALUES(" +
			std::to_string(to) + "," + std::to_string(from) + ",'')";
		if (mysql_query(mysql, first.c_str()) != 0 || mysql_query(mysql, second.c_str()) != 0 ||
			mysql_commit(mysql) != 0) {
			std::cerr << "AddFriend failed: " << mysql_error(mysql) << std::endl;
			mysql_rollback(mysql);
			return false;
		}
		return true;
	}

	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
		});

	try {
		//开始事务
		con->_con->setAutoCommit(false);

		// 准备第一个SQL语句，插入认证方好友数据
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement(
			"INSERT IGNORE INTO friend(self_id, friend_id, back) VALUES (?, ?, ?)"
		));
		//反过来的申请时from, 验证时to
		pstmt->setInt(1, from); // from id
		pstmt->setInt(2, to);
		pstmt->setString(3, back_name);
		// 执行更新
		int rowAffected = pstmt->executeUpdate();
		if (rowAffected < 0) {
			con->_con->rollback();
			return false;
		}

		//准备第二个SQL语句，插入申请方好友数据
		std::unique_ptr<sql::PreparedStatement> pstmt2(con->_con->prepareStatement(
			"INSERT IGNORE INTO friend(self_id, friend_id, back)" "VALUES (?, ?, ?) "
		));
		//反过来的申请时from, 验证时to
		pstmt2->setInt(1, to); // from id
		pstmt2->setInt(2, from);
		pstmt2->setString(3, "");
		// 执行更新
		int rowAffected2 = pstmt2->executeUpdate();
		if (rowAffected2 < 0) {
			con->_con->rollback();
			return false;
		}

		// 提交事务
		con->_con->commit();
		std::cout << "addfriend insert friends success" << std::endl;

		return true;
	}
	catch (sql::SQLException& e) {
		// 如果发生错误, 回滚事务
		if (con) {
			con->_con->rollback();
		}
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}

	return true;
}

bool MysqlDao::GetFriendList(int self_uid, std::vector<std::shared_ptr<UserInfo>>& user_info_list) {
	auto con = pool_->getConnection();
	if (con == nullptr) {
		return false;
	}

	Defer defer([this, &con]() {
		pool_->returnConnection(std::move(con));
		});


	try {
		// 准备SQL语句, 根据起始id和限制条数返回列表
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("select * from friend where self_id = ? "));

		pstmt->setInt(1, self_uid); // 将uid替换为你要查询的uid

		// 执行查询
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		// 遍历结果集
		while (res->next()) {
			auto friend_id = res->getInt("friend_id");
			auto back = res->getString("back");
			//再一次查询friend_id对应的信息
			auto user_info = GetUser(friend_id);
			if (user_info == nullptr) {
				continue;
			}

			user_info->back = user_info->name;
			user_info_list.push_back(user_info);
		}
		return true;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}

	return true;
}


