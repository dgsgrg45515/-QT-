#pragma once
#include <memory>
#include <mutex>
#include <unordered_map>
#include "Singleton.h"
#include "CSession.h"
#include "RedisMgr.h"
class CSession;
class UserMgr :public Singleton<UserMgr>
{
	friend class Singleton<UserMgr>;
public:
	~UserMgr() {
		_uid_to_session.clear();
	}
	std::shared_ptr<CSession> GetSession(int uid) {
		std::lock_guard<std::mutex> lock(_session_mtx);
		auto iter = _uid_to_session.find(uid);
		if (iter == _uid_to_session.end()) {
			return nullptr;
		}
		return iter->second;
	}
	void SetUserSession(int uid, std::shared_ptr<CSession> session) {
		std::lock_guard<std::mutex> lock(_session_mtx);
		_uid_to_session[uid] = session;
	}
	void RmvUserSession(int uid,std::string session_id) {
		std::lock_guard<std::mutex> lock(_session_mtx);
		auto iter = _uid_to_session.find(uid);
		if (iter == _uid_to_session.end()) {
			return;
		}
		auto session_id_ = iter->second->GetSessionId();
		//不相等说明是其他地方登录了
		if (session_id_ != session_id) {
			return;
		}
		_uid_to_session.erase(uid);
	}
private:
	UserMgr() {
		
	}
	std::mutex _session_mtx;
	std::unordered_map<int , std::shared_ptr<CSession>> _uid_to_session;
};

