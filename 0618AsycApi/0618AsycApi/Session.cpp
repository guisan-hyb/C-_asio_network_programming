#include "Session.h"

Session::Session(std::shared_ptr<boost::asio::ip::tcp::socket>& socket)
	: _socket(socket), _send_pending(false), _recv_pending(false)
{

}

void Session::Connect(const boost::asio::ip::tcp::endpoint& ep)
{
	_socket->connect(ep);
}

void Session::WriteCallBackErr(const boost::system::error_code& ec, std::size_t bytes_transferred, 
	std::shared_ptr<MsgNode> msg_node)
{
	if (bytes_transferred + msg_node->_cur_len < msg_node->_total_len) {
		_send_node->_cur_len += bytes_transferred;
		this->_socket->async_write_some(boost::asio::buffer(_send_node->_msg + _send_node->_cur_len, _send_node->_total_len - _send_node->_cur_len),
			std::bind(&Session::WriteCallBackErr, this, std::placeholders::_1, std::placeholders::_2, _send_node));
	}
}

void Session::WriteToSocketErr(const std::string& buf)
{
	_send_node = std::make_shared<MsgNode>(buf.data(), buf.length());
	//异步发送数据，因为异步所以不会一下发送完
	this->_socket->async_write_some(boost::asio::buffer(_send_node->_msg, _send_node->_total_len),
		std::bind(&Session::WriteCallBackErr, this, std::placeholders::_1, std::placeholders::_2, _send_node));
}



void Session::WriteCallBack(const boost::system::error_code& ec, std::size_t bytes_transferred)
{
	//优先检查错误码
	if (ec.value() != 0) {
		std::cout << "Error, code is " << ec.value() << ". Message is " << ec.message() << std::endl;
		return;
	}

	//检查是否确实发送完毕 (使用 async_write 理论上到这里一定是发完了，但做断言更安全)
	auto& send_data = _send_queue.front();//取出队首元素即当前未发送的数据
	send_data->_cur_len += bytes_transferred;
	//数据未发送完， 则继续发送
	if (send_data->_cur_len < send_data->_total_len) {
		this->_socket->async_write_some(boost::asio::buffer(send_data->_msg + send_data->_cur_len,
			send_data->_total_len - send_data->_cur_len),
			std::bind(&Session::WriteCallBack, this, std::placeholders::_1, std::placeholders::_2)
		);
		return;
	}

	//如果发送完，则pop出队首元素
	_send_queue.pop();

	//如果队列为空，则说明所有数据都发送完,将pending设置为false
	if (_send_queue.empty()) {
		_send_pending = false;
	}

	//如果队列不是空，则继续将队首元素发送
	if (!_send_queue.empty()) {
		auto& next_data = _send_queue.front();
		this->_socket->async_write_some(boost::asio::buffer(next_data->_msg + next_data->_cur_len,
			next_data->_total_len - next_data->_cur_len),
			std::bind(&Session::WriteCallBack, this, std::placeholders::_1, std::placeholders::_2));
		//其实buffer结构中可以不用使用cur_len，因为这是新一轮的数据，之前从未被发送: 
		/*boost::asio::buffer(next_data->_msg,
			next_data->_total_len);*/
		//这里写上是为了熟悉一下异步发送的时候要做好数据的地址偏移
	}
}

void Session::WriteToSocket(const std::string& buf)
{
	//插入发送队列
	_send_queue.emplace(std::make_shared<MsgNode>(buf.data(), buf.length()));
	if (_send_pending) { //pending状态说明上一次有未发送完的数据
		return;
	}

	//异步发送数据
	this->_socket->async_write_some(boost::asio::buffer(buf),
		std::bind(&Session::WriteCallBack, this, std::placeholders::_1, std::placeholders::_2));
	_send_pending = true;
}

void Session::WriteAllCallBack(const boost::system::error_code& ec, std::size_t bytes_transferred)
{
	if (ec.value() != 0) {
		std::cout << "Error occurred! Error code = " << ec.value() 
			<< ". Message: " << ec.message() << std::endl;
		return;
	}

	_send_queue.pop();
	if (_send_queue.empty()) {
		_send_pending = false;
	}

	if (!_send_queue.empty()) {
		auto& next_data = _send_queue.front();
		this->_socket->async_send(boost::asio::buffer(next_data->_msg + next_data->_cur_len,
			next_data->_total_len - next_data->_cur_len),
			std::bind(&Session::WriteAllCallBack, std::placeholders::_1, std::placeholders::_2));
	}
}

void Session::WriteAllToSocket(const std::string& buf)
{
	_send_queue.emplace(std::make_shared<MsgNode>(buf.data(), buf.length()));
	if (_send_pending) {
		return;
	}

	this->_socket->async_send(boost::asio::buffer(buf),
		std::bind(&Session::WriteAllCallBack, this, std::placeholders::_1, std::placeholders::_2));
	_send_pending = true;
}

void Session::ReadCallBack(const boost::system::error_code& ec, std::size_t bytes_transferred)
{
	if (ec.value() != 0) {
		std::cout << "Error occurred! Error code = " << ec.value()
			<< ". Message: " << ec.message() << std::endl;
		return;
	}

	_recv_node->_cur_len += bytes_transferred;
	//没有读完继续
	if (_recv_node->_cur_len < _recv_node->_total_len) {
		this->_socket->async_read_some(boost::asio::buffer(_recv_node->_msg + _recv_node->_cur_len,
			_recv_node->_total_len - _recv_node->_cur_len),
			std::bind(&Session::ReadCallBack, this, std::placeholders::_1, std::placeholders::_2));
		return;
	}

	//将数据投递到队列里交给逻辑线程处理，此处略去
	//如果读完了则将标记置为false
	_recv_pending = false;
	//指针置空
	_recv_node = nullptr;
}

void Session::ReadFromSocket()
{
	if (_recv_pending) {
		return;
	}

	_recv_node = std::make_shared<MsgNode>(RECVSIZE);
	this->_socket->async_read_some(boost::asio::buffer(_recv_node->_msg, _recv_node->_total_len),
		std::bind(&Session::ReadCallBack, this, std::placeholders::_1, std::placeholders::_2));
	_recv_pending = true;
}

void Session::ReadAllCallBack(const boost::system::error_code& ec, std::size_t bytes_transferred)
{
	if (ec.value() != 0) {
		std::cout << "Error occurred! Error code = " << ec.value()
			<< ". Message: " << ec.message() << std::endl;
		return;
	}

	_recv_node->_cur_len += bytes_transferred;
	_recv_node = nullptr;
	_recv_pending = false;
}

void Session::ReadAllFromSocket()
{
	if (_recv_pending) {
		return;
	}

	_recv_node = std::make_shared<MsgNode>(RECVSIZE);
	this->_socket->async_receive(boost::asio::buffer(_recv_node->_msg, _recv_node->_total_len),
		std::bind(&Session::ReadAllCallBack, this, std::placeholders::_1, std::placeholders::_2));
	_recv_pending = true;
}


//读的时候推荐async_read_some
//写的时候推荐async_send
