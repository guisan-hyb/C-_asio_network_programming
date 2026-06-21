#pragma once

#include <boost/asio.hpp>
#include <memory>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <queue>

class CServer;


class MsgNode // 数据节点，用来存储数据
{
	friend class CSession;
public:
	MsgNode(char* msg, int max_len) {
		_data = new char[max_len];
		memcpy(_data, msg, max_len);
	}

	~MsgNode() {
		delete[] _data;
	}

private:
	int _cur_len;//表示数据当前已处理的长度(已经发送的数据或者已经接收的数据长度)
	int _max_len;//表示数据的总长度。
	char* _data;//数据域
};


class CSession : public std::enable_shared_from_this<CSession>
{
public:
	CSession(boost::asio::io_context& ioc, CServer* server);

	boost::asio::ip::tcp::socket& Socket();

	void Start();

	std::string& GetUuid();

	void Send(char* msg, int max_length);

private:
	void handle_read(const boost::system::error_code& error, std::size_t bytes_transferred,
		std::shared_ptr<CSession> _self_shared);
	void handle_write(const boost::system::error_code& error, std::shared_ptr<CSession> _self_shared);


	boost::asio::ip::tcp::socket _socket;
	enum { max_length = 1024 };
	char _data[max_length];
	CServer* _server;
	std::string _uuid;

	std::queue<std::shared_ptr<MsgNode>> _send_que;
	std::mutex _send_lock;
};

