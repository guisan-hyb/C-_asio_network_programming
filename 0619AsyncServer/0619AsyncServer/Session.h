#pragma once

#include <iostream>
#include <boost/asio.hpp>
#include <cstring>

class Session
{
public:
	Session(boost::asio::io_context& ioc): _socket(ioc) {
		memset(_data, '\0', max_length);
	}

	boost::asio::ip::tcp::socket& Socket() {
		return _socket;
	}

	void Start();

private:
	void handle_read(const boost::system::error_code& error, std::size_t bytes_transferred);//读的回调函数
	void handle_write(const boost::system::error_code& error);//写的回调函数

	boost::asio::ip::tcp::socket _socket;
	enum {max_length = 1024};//enum 在类内创建一个编译时常量，给_data使用
	char _data[max_length];
};



class Server {
public:
	Server(boost::asio::io_context& ioc, unsigned short port);

private:
	void start_accept();
	void handle_accept(Session* new_session, const boost::system::error_code& error);

	boost::asio::io_context& _ioc;
	boost::asio::ip::tcp::acceptor _acceptor;
};

