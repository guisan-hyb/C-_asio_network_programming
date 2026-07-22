#include "CServer.h"
#include <iostream>
#include "CSession.h"
#include "AsioIOServicePool.h"

CServer::CServer(boost::asio::io_context& ioc, short port)
	: _ioc(ioc), _port(port), _acceptor(ioc, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port))
{
	std::cout << "Server start success, listen on port : " << _port << std::endl;
	StartAccept();
}

CServer::~CServer()
{

}

void CServer::ClearSession(std::string& uuid)
{
	std::lock_guard<std::mutex> lock(_mutex);
	if (_sessions.find(uuid) == _sessions.end()) {
		_sessions.erase(uuid);
	}
}

void CServer::StartAccept()
{
	auto& io_context = AsioIOServicePool::GetInstance().GetIOService();
	auto new_session = std::make_shared<CSession>(io_context, this);
	_acceptor.async_accept(new_session->GetSocket(),
		std::bind(&CServer::HandleAccept, this, std::placeholders::_1, new_session));
}

void CServer::HandleAccept(const boost::system::error_code& error, std::shared_ptr<CSession> new_session)
{
	try {
		if (!error) {
			new_session->Start();
			std::lock_guard<std::mutex> lock(_mutex);
			_sessions[new_session->GetUuid()] = new_session;
		}
		else {
			std::cerr << "session accept failed, error is: " << error.what() << std::endl;
		}

		StartAccept();
	}
	catch (std::exception& e) {
		std::cerr << "Exception: " << e.what() << std::endl;
	}
}
