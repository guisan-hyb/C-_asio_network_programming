#include "Session.h"

void Session::Start()
{
	//memset(_data, '\0', max_length);
	_socket.async_read_some(boost::asio::buffer(_data, max_length),
		std::bind(&Session::handle_read, this, std::placeholders::_1, std::placeholders::_2));
}

void Session::handle_read(const boost::system::error_code& error, std::size_t bytes_transferred)
{
	if (!error) {
		std::cout << "server receive data is " << _data << std::endl;
		//这里写的是一个 echo 的server
		boost::asio::async_write(_socket, boost::asio::buffer(_data, bytes_transferred),
			std::bind(&Session::handle_write, this, std::placeholders::_1));
		//这个bind可以用lambda函数替代：[this](const auto& error) { handle_write(error); }
	}
	else {
		std::cout << "read error" << std::endl;
		delete this;//把this销毁，表示连接断开
		//这种写法有隐患，但由于这种echo是半双工，所以不会出错
	}
}

void Session::handle_write(const boost::system::error_code& error)
{
	if (!error) {
		memset(_data, '\0', max_length);
		_socket.async_read_some(boost::asio::buffer(_data, max_length),
			std::bind(&Session::handle_read, this, std::placeholders::_1, std::placeholders::_2));
	}
	else {
		std::cout << "write error" << std::endl;
		delete this;
	}
}




Server::Server(boost::asio::io_context& ioc, unsigned short port)
	: _ioc(ioc),_acceptor(ioc,boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(),port))
{
	std::cout << "Server start success, on port: " << port << std::endl;
	start_accept();
}

void Server::start_accept()
{
	Session* new_session = new Session(_ioc);
	_acceptor.async_accept(new_session->Socket(),
		std::bind(&Server::handle_accept, this, new_session, std::placeholders::_1));
}

void Server::handle_accept(Session* new_session, const boost::system::error_code& error)
{
	if (!error) {
		new_session->Start();
	}
	else {
		delete new_session;
	}

	start_accept();
	//事件驱动的循环，而非 函数调用的递归，因此不会出现栈溢出问题
}
