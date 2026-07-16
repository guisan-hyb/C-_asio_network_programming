#include "CServer.h"
#include <iostream>
#include "CSession.h"

CServer::CServer(boost::asio::io_context& ioc, short port_num)
	: _ioc(ioc),_port_num(port_num),_acceptor(ioc, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(),port_num))
{
	std::cout << "Server start success, listen on port: " << port_num << std::endl;
	StartAccept();
}

void CServer::ClearSession(std::string& uuid)
{
	if (!_sessions.count(uuid)) {
		_sessions.erase(uuid);
	}
}

void CServer::StartAccept()
{
	auto new_session = std::make_shared<CSession>(_ioc, this);

	_acceptor.async_accept(new_session->GetSocket(),
		std::bind(&CServer::handleAccept, this, std::placeholders::_1, new_session));
}

void CServer::handleAccept(const boost::system::error_code& error, std::shared_ptr<CSession> new_session)
{
	if (!error) {
		new_session->Start();
		_sessions[new_session->GetUuid()] = new_session;
	}
	else {
		std::cout << "session accept failed, error is" << error.message() << std::endl;
	}

	StartAccept();
}
