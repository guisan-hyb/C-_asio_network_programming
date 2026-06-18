#pragma once

#include <boost/asio.hpp>
#include <memory>
#include <iostream>
#include <queue>


//封装一个Node结构，用来管理要发送和接收的数据
const int RECVSIZE = 1024;
class MsgNode {
public:
	MsgNode(const char* msg, int total_len) //构造写节点
		: _total_len(total_len), _cur_len(0)
	{
		_msg = new char[RECVSIZE];
		memcpy(_msg, msg, total_len);
	}

	MsgNode(int total_len) :_total_len(total_len), _cur_len(0) { //构造读节点
		_msg = new char[RECVSIZE];
	}

	~MsgNode() {
		delete[] _msg;
	}

public:
	int _total_len;
	int _cur_len;
	char* _msg;
};



//这个session类表示服务器处理客户端连接的管理类
class Session
{
public:
	Session(std::shared_ptr<boost::asio::ip::tcp::socket>& socket);
	void Connect(const boost::asio::ip::tcp::endpoint& ep);

	//先写一个经典的错误版本
	void WriteCallBackErr(const boost::system::error_code& ec, std::size_t bytes_transferred,
		std::shared_ptr<MsgNode> msg_node);
	void WriteToSocketErr(const std::string& buf);


	//正确版本
	void WriteCallBack(const boost::system::error_code& ec, std::size_t bytes_transferred);
	void WriteToSocket(const std::string& buf);


	void WriteAllCallBack(const boost::system::error_code& ec, std::size_t bytes_transferred);
	void WriteAllToSocket(const std::string& buf);
	


	void ReadCallBack(const boost::system::error_code& ec, std::size_t bytes_transferred);
	void ReadFromSocket();

	void ReadAllCallBack(const boost::system::error_code& ec, std::size_t bytes_transferred);
	void ReadAllFromSocket();

private:
	std::shared_ptr<boost::asio::ip::tcp::socket> _socket;

	std::shared_ptr<MsgNode> _send_node;
	std::queue<std::shared_ptr<MsgNode>> _send_queue;
	bool _send_pending;//是否有未发送完的数据

	std::shared_ptr<MsgNode> _recv_node;
	bool _recv_pending;
};


