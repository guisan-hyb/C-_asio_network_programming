#pragma once
#include <boost/asio.hpp>

namespace net = boost::asio;

class WebSocketServer
{
public:
	WebSocketServer(net::io_context& ioc, unsigned short port);
	void StartAccept();// tcp层面接收连接

	WebSocketServer(const WebSocketServer&) = delete;
	WebSocketServer& operator=(const WebSocketServer&) = delete;

private:
	net::ip::tcp::acceptor _acceptor;
	net::io_context& _ioc;
};

