#pragma once
#include <boost/asio.hpp>
#include <memory>
#include <string>
#include <queue>
#include <mutex>

#define MAX_LENGTH 1024*2
#define HEAD_LENGTH 2 //头部长度，2字节

class CServer;


//消息节点，采用 消息长度+消息内容 的方式
class MsgNode {
	friend class CSession;
public:
	MsgNode(char* msg, short max_len) : _total_len(max_len + HEAD_LENGTH), _cur_len(0) { //发送数据时构造发送信息的节点
		_data = new char[_total_len + 1]();
		memcpy(_data, &max_len, HEAD_LENGTH);
		memcpy(_data + HEAD_LENGTH, msg, max_len);
		_data[_total_len] = '\0';
	}

	MsgNode(short max_len) : _total_len(max_len), _cur_len(0) { //接收对端数据时构造接收节点
		_data = new char[_total_len + 1]();
	}

	~MsgNode() {
		delete[] _data;
	}

	void Clear() {
		memset(_data, '\0', _total_len);
		_cur_len = 0;
	}

private:
	short _cur_len;
	short _total_len;
	char* _data;
};


class CSession : public std::enable_shared_from_this<CSession>
{
public:
	CSession(boost::asio::io_context& ioc, CServer* server);

	~CSession();

	boost::asio::ip::tcp::socket& Socket();

	void Start();

	std::string& GetUuid();

	void Send(char* msg, int max_length);

	void Close();

	std::shared_ptr<CSession> SharedSelf();

private:
	void handle_read(const boost::system::error_code& error, std::size_t bytes_transferred,
		std::shared_ptr<CSession> _self_shared);
	void handle_write(const boost::system::error_code& error,
		std::shared_ptr<CSession> _self_shared);

	boost::asio::ip::tcp::socket _socket;
	char _data[MAX_LENGTH];// 接收缓冲区
	CServer* _server;
	std::string _uuid;
	bool _b_close;
	std::queue<std::shared_ptr<MsgNode>> _send_que; // 发送缓冲区
	std::mutex _send_lock;


	//收到的消息结构
	std::shared_ptr<MsgNode> _recv_msg_node;
	//收到的头部结构
	std::shared_ptr<MsgNode> _recv_head_node;
	//头部是否解析完成
	bool _b_head_parse;
};

