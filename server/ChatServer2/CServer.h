#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>

#include "const.h"
#include "CSession.h"

class CServer
{
public:
	CServer(boost::asio::io_context& io_context, short port);

	void ClearSession(const std::string& uuid);
	std::shared_ptr<CSession> GetSession(std::string uid);
	bool  CheckValid(std::string uid);
private:
	void StartAccept();
	void HandleAccept(std::shared_ptr<CSession> new_session, const boost::system::error_code& error);

	boost::asio::io_context& _io_context;
	short _port;
	tcp::acceptor _acceptor;
	std::unordered_map<std::string, std::shared_ptr<CSession>> _sessions;
	std::mutex _mutex;
};
