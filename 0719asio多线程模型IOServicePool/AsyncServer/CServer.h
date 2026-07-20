#pragma once

#include <boost/asio.hpp>
#include <memory>
#include <map>
#include <string>

class CSession;

class CServer
{
public:
	CServer(boost::asio::io_context& ioc, short port_num);

	void ClearSession(std::string& uuid);

private:
	void StartAccept();
	void HandleAccept(const boost::system::error_code& error, std::shared_ptr<CSession> new_session);

	boost::asio::ip::tcp::acceptor _acceptor;
	boost::asio::io_context& _ioc;
	short _port_num;
	std::map<std::string, std::shared_ptr<CSession>> _sessions;
};

