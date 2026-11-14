#pragma once
#include "const.h"
#include <condition_variable>
#include <functional>
#include <json/json.h>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>
#include "CSession.h"
#include "StatusGrpcClient.h"
#include "data.h"
#include "MysqlMgr.h"
#include "Singleton.h"
#include "RedisMgr.h"


typedef function<void(shared_ptr<CSession>, const short& msg_id, const string& msg_data)> FuncCallBack;
class LogicSystem:public Singleton<LogicSystem> {
    friend class Singleton<LogicSystem>;
public:
    ~LogicSystem();
    void PostMsgToQue(std::shared_ptr<LogicNode> msg);
private:
    LogicSystem();
    void RegisterCallBacks();
    void DealMsg();
    bool isPureDigit(const std::string& str);
    void LoginHandler(std::shared_ptr<CSession> session, const short& msg_id, const std::string& msg_data);
    void SearchInfo(std::shared_ptr<CSession>, const short& msg_id, const std::string& msg_data);
    void AddFriendApply(std::shared_ptr<CSession>, const short& msg_id, const std::string& msg_data);
    void AuthFriendApply(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data);
    bool GetBaseInfo(std::string base_key, int uid, std::shared_ptr<UserInfo>& userinfo);
    void GetUserByUid(std::string uid_str, Json::Value& rtvalue);
    void GetUserByName(std::string name, Json::Value& rtvalue);
    bool GetFriendApplyInfo(int to_uid, std::vector<std::shared_ptr<ApplyInfo>>& list);
    bool GetFriendList(int self_uid, std::vector<std::shared_ptr<UserInfo>>& user_info);
    void DealChatTextMsg(std::shared_ptr<CSession> session, const short& msg_id, const string& msg_data);
    std::thread _worker_thread;
    std::queue<std::shared_ptr<LogicNode>> _msg_que;
    std::mutex _mutex;
    std::condition_variable _consume;
    bool _b_stop;
    std::map<short, FuncCallBack> _fun_callbacks;
};
