#include "CServer.h"
#include <iostream>
#include "CSession.h"

CServer::CServer(boost::asio::io_context& ioc, unsigned short port_num)
	: _ioc(ioc), _port_num(port_num), _acceptor(ioc, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port_num))
{
	std::cout << "Server start success, on port: " << port_num << std::endl;
	start_accept();
}

void CServer::ClearSession(std::string& uuid)
{
	_sessions.erase(uuid);
}

void CServer::start_accept()
{
	auto new_session = std::make_shared<CSession>(_ioc, this);
	_acceptor.async_accept(new_session->Socket(), std::bind(&CServer::handle_accept, this
		, new_session, std::placeholders::_1));
}

void CServer::handle_accept(std::shared_ptr<CSession> new_session, const boost::system::error_code& error)
{
	if (!error) {
		new_session->Start();
		_sessions[new_session->GetUuid()] = new_session;
	}
	else {
		std::cout << "session accept failed, error is " << error.message() << std::endl;
	}

	start_accept();
}
