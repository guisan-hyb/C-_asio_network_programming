#pragma once

#include <boost/asio.hpp>
#include "Server.h"

class Server;

class Session : public std::enable_shared_from_this<Session> //CRTP
{
public:
	Session(boost::asio::io_context& ioc, Server* server);

	boost::asio::ip::tcp::socket& Socket();

	void Start();

	std::string& GetUuid();

private:
	void handle_read(const boost::system::error_code& error, std::size_t bytes_transferred,
		std::shared_ptr<Session> _self_shared);
	void handle_write(const boost::system::error_code& error, std::shared_ptr<Session> _self_shared);

	boost::asio::ip::tcp::socket _socket;
	enum { max_length = 1024 };
	char _data[max_length];
	Server* _server;
	std::string _uuid;
};

