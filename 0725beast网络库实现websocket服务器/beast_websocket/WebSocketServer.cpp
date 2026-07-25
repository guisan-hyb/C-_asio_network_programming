#include "WebSocketServer.h"
#include <iostream>
#include "Connection.h"

WebSocketServer::WebSocketServer(net::io_context& ioc, unsigned short port)
	: _ioc(ioc), _acceptor{ioc,{net::ip::tcp::v4(), port}}
{
	std::cout << "Server start on port: " << port << std::endl;
}

void WebSocketServer::StartAccept()
{
	auto con_ptr = std::make_shared<Connection>(_ioc);
	_acceptor.async_accept(con_ptr->GetSocket(), [con_ptr, this](boost::system::error_code ec) {
		try {
			if (!ec) {
				con_ptr->AsyncAccept();// 协议升级成webSocket
			}
			else {
				std::cerr << "acceptor async_accept failed, err: " << ec.message() << std::endl;
			}

			StartAccept();
		}
		catch (const std::exception& e) {
			std::cerr << "Exception: " << e.what() << std::endl;
		}
	});
}
