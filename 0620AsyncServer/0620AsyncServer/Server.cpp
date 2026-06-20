#include "Server.h"
#include "Session.h"
#include <iostream>

Server::Server(boost::asio::io_context& ioc, unsigned short port_num)
	: _ioc(ioc),_acceptor(ioc,boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(),port_num))
{
	std::cout << "Server start success, on port: " << port_num << std::endl;
	start_accept();
}

void Server::ClearSession(std::string& uuid)
{
	_sessions.erase(uuid);
}

void Server::start_accept()
{
	std::shared_ptr<Session> new_session = std::make_shared<Session>(_ioc, this);
	_acceptor.async_accept(new_session->Socket(),
		std::bind(&Server::handle_accept, this, new_session, std::placeholders::_1));

	//注：这个右括号执行完毕后，这个new_session会被释放掉吗？
	//答案是不会，bind绑定是拷贝，传值，会增加引用计数
}

void Server::handle_accept(std::shared_ptr<Session> new_session, const boost::system::error_code& error)
{
	if (!error) {
		new_session->Start();

		//_sessions.insert({ new_session->GetUuid(),new_session });
		//_sessions[new_session->GetUuid()] = new_session;
		_sessions.insert(std::make_pair(new_session->GetUuid(), new_session));

	}
	else {
				
	}

	start_accept();
}
