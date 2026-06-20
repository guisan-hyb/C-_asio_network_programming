#pragma once

#include <boost/asio.hpp>
#include <map>
#include <memory>

class Session;

class Server
{
public:
	Server(boost::asio::io_context& ioc, unsigned short port_num);

	void ClearSession(std::string& uuid);

private:
	void start_accept();
	void handle_accept(std::shared_ptr<Session> new_session, const boost::system::error_code& error);

	boost::asio::io_context& _ioc;
	boost::asio::ip::tcp::acceptor _acceptor;
	std::map<std::string, std::shared_ptr<Session>>_sessions;
};

