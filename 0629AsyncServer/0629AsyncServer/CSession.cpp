#include "CSession.h"
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <iostream>
#include "CServer.h"

CSession::CSession(boost::asio::io_context& ioc, CServer* server)
	: _socket(ioc), _server(server), _b_close(false), _b_head_parse(false)
{
	memset(_data, '\0', MAX_LENGTH);
	boost::uuids::uuid a_uuid = boost::uuids::random_generator()();
	_uuid = boost::uuids::to_string(a_uuid);
	_recv_head_node = std::make_shared<MsgNode>(HEAD_LENGTH);
}

CSession::~CSession()
{
	std::cout << "CSession destructed" << std::endl;
}

boost::asio::ip::tcp::socket& CSession::Socket()
{
	return _socket;
}

void CSession::Start()
{
	_socket.async_read_some(boost::asio::buffer(_data, MAX_LENGTH),
		std::bind(&CSession::handle_read, this, std::placeholders::_1, std::placeholders::_2,
			SharedSelf()));
}

std::string& CSession::GetUuid()
{
	return _uuid;
}

void CSession::Send(char* msg, int max_length)
{
	bool isPending = false;//是否还有未发送完毕的数据
	std::lock_guard<std::mutex> lock(_send_lock);
	if (_send_que.size() > 0) {
		isPending = true;
	}

	_send_que.push(std::make_shared<MsgNode>(msg, max_length));

	if (isPending) return;
	
	auto& msgnode = _send_que.front();
	boost::asio::async_write(_socket, boost::asio::buffer(msgnode->_data, msgnode->_total_len),
		std::bind(&CSession::handle_write, this, std::placeholders::_1, SharedSelf()));
}

void CSession::Close() {
	_socket.close();
	_b_close = true;
}

std::shared_ptr<CSession> CSession::SharedSelf()
{
	return shared_from_this();
}

//ASIO 层面：bytes_transferred 永远是已经完成的传输量（读到了多少，写入了多少）
//业务层面：由于 TCP 是流式协议，没有消息边界，我们必须自己切分消息。
//切分后剩下的部分，在代码逻辑里被称为“未处理字节”，它是用 bytes_transferred 减去你消费掉的字节算出来的。
//千万不要把协议解析层的状态和 ASIO I / O 层的状态搞混了！

void CSession::handle_read(const boost::system::error_code& error, std::size_t bytes_transferred, std::shared_ptr<CSession> _self_shared)
{
	if (!error) {
		int copy_len = 0;//已经处理的字节数, _data 缓冲区的读指针向后移动
		while (bytes_transferred > 0) { //未处理字节数 = bytes_transferred (ASIO给的已读字节数) - 已解析字节数 (代码算出来的)
			//A. 处理消息头
			//进入这个分支，说明我们还没有拿到一个完整的消息头
			if (!_b_head_parse) { 
				//A1. 收到的数据不足头部大小 (半包)
				if (bytes_transferred + _recv_head_node->_cur_len < HEAD_LENGTH) {
					//1. 把新读到的数据全塞进头部节点 _recv_head_node 里，更新 _cur_len
					//2. 然后直接 return 并发起下一次 async_read_some，等更多数据来
					memcpy(_recv_head_node->_data + _recv_head_node->_cur_len, _data + copy_len, bytes_transferred);
					_recv_head_node->_cur_len += bytes_transferred;
					memset(_data, '\0', MAX_LENGTH);
					_socket.async_read_some(boost::asio::buffer(_data, MAX_LENGTH),
						std::bind(&CSession::handle_read, this, std::placeholders::_1, std::placeholders::_2,
							SharedSelf()));
					return;
				}


				//A2. 数据够凑齐头了（甚至还有多余的）
				//1. 算出还差多少字节凑齐头：head_remain = HEAD_LENGTH - _recv_head_node->_cur_len
				//2. 把差的部分从 _data 拷贝到 _recv_head_node
				//3. 移动游标，扣减剩余字节数
				//4. 解析头部，拿到消息体长度 data_len。做合法性校验（不能超过 MAX_LENGTH）
				//5. 创建消息体节点 _recv_msg_node
				int head_remain = HEAD_LENGTH - _recv_head_node->_cur_len;//头部剩余未复制的长度
				memcpy(_recv_head_node->_data + _recv_head_node->_cur_len, _data + copy_len, head_remain);

				copy_len += head_remain;
				bytes_transferred -= head_remain;// 此时 bytes_transferred 变成了“剩余未处理”

				short data_len = 0;//获取头部数据
				memcpy(&data_len, _recv_head_node->_data, HEAD_LENGTH);
				std::cout << "data_len is " << data_len << std::endl;

				if (data_len > MAX_LENGTH) {
					std::cout << "invalid data length is " << data_len << std::endl;
					_server->ClearSession(_uuid);
					return;
				}

				_recv_msg_node = std::make_shared<MsgNode>(data_len);

				//A2-1. 剩下的数据不够一个消息体（又是半包）
				//把剩下的 bytes_transferred 全塞进消息体节点，
				//设置 _b_head_parse = true（告诉下次回调：头已经处理完了，直接处理消息体），然后 return 等新数据
				if (bytes_transferred < data_len) {
					memcpy(_recv_msg_node->_data + _recv_msg_node->_cur_len, _data + copy_len, bytes_transferred);
					_recv_msg_node->_cur_len += bytes_transferred;
					memset(_data, '\0', MAX_LENGTH);
					_socket.async_read_some(boost::asio::buffer(_data, MAX_LENGTH),
						std::bind(&CSession::handle_read, this, std::placeholders::_1, std::placeholders::_2,
							SharedSelf()));

					_b_head_parse = true;//头部处理完成，下次回调不需要再处理头部
					return;
				}

				//A2-2. 剩下的数据足够一个消息体（粘包，一口气读完）
				//拷贝确切的 data_len 长度到消息体节点。再次更新游标和剩余字节数。打印/发送消息。
				//重置状态：_b_head_parse = false，清空头节点。
				//如果 bytes_transferred 还大于 0，说明 _data 里还藏着下一个包！通过 continue 回到 while 循环开头继续解析
				memcpy(_recv_msg_node->_data + _recv_msg_node->_cur_len, _data + copy_len, data_len);
				_recv_msg_node->_cur_len += data_len;
				copy_len += data_len;
				bytes_transferred -= data_len;
				_recv_msg_node->_data[_recv_msg_node->_total_len] = '\0';
				std::cout << "receive data is " << _recv_msg_node->_data << std::endl;

				Send(_recv_msg_node->_data, _recv_msg_node->_total_len);//此处可以调用Send发送测试

				//继续轮询剩余未处理数据
				_b_head_parse = false;
				_recv_head_node->Clear();
				if (bytes_transferred <= 0) {
					memset(_data, '\0', MAX_LENGTH);
					_socket.async_read_some(boost::asio::buffer(_data, MAX_LENGTH),
						std::bind(&CSession::handle_read, this, std::placeholders::_1, std::placeholders::_2,
							SharedSelf()));
				}

				continue;
			}


			//B. 处理消息体 (else 对应 _b_head_parse == true)
			//进入这个分支，说明在上一次（或上面 A2-1 逻辑中）头部已经解析完了，
			//并且拿到了消息体长度，但消息体没凑齐，现在继续凑

			//计算还差多少：remain_msg = _recv_msg_node->_total_len - _recv_msg_node->_cur_len
			int remain_msg = _recv_msg_node->_total_len - _recv_msg_node->_cur_len;
			
			//B1. 新数据依然不够（半包）
			//全塞进去，更新 _cur_len，return 等新数据
			if (bytes_transferred < remain_msg) {
				memcpy(_recv_msg_node->_data + _recv_msg_node->_cur_len, _data + copy_len, bytes_transferred);
				_recv_msg_node->_cur_len += bytes_transferred;
				memset(_data, '\0', MAX_LENGTH);
				_socket.async_read_some(boost::asio::buffer(_data, MAX_LENGTH),
					std::bind(&CSession::handle_read, this, std::placeholders::_1, std::placeholders::_2,
						SharedSelf()));
				return;
			}

			//B2. 新数据够了，把消息体补全
			//拷贝确切的 remain_msg。更新游标，扣减 bytes_transferred。打印/发送。
			//重置状态：_b_head_parse = false，清空头节点。如果还有剩余字节，continue 回到 while 解析下一个包头
			memcpy(_recv_msg_node->_data + _recv_msg_node->_cur_len, _data + copy_len, remain_msg);
			_recv_msg_node->_cur_len += remain_msg;
			bytes_transferred -= remain_msg;
			copy_len += remain_msg;
			_recv_msg_node->_data[_recv_msg_node->_total_len] = '\0';
			std::cout << "receive data is " << _recv_msg_node->_data << std::endl;

			Send(_recv_msg_node->_data, _recv_msg_node->_total_len);// 此处可以调用Send发送测试

			//继续轮询剩余未处理数据
			_b_head_parse = false;
			_recv_head_node->Clear();
			if (bytes_transferred <= 0) {
				memset(_data, '\0', MAX_LENGTH);
				_socket.async_read_some(boost::asio::buffer(_data, MAX_LENGTH),
					std::bind(&CSession::handle_read, this, std::placeholders::_1, std::placeholders::_2,
						SharedSelf()));
				return;
			}
			continue;
		}
	}
	else {
		std::cout << "handle read failed, error is: " << error.what() << std::endl;
		_server->ClearSession(_uuid);
	}
}

void CSession::handle_write(const boost::system::error_code& error, std::shared_ptr<CSession> _self_shared)
{
	if (!error) {
		std::lock_guard<std::mutex> lock(_send_lock);
		_send_que.pop();
		if (!_send_que.empty()) {
			auto& msgnode = _send_que.front();
			boost::asio::async_write(_socket, boost::asio::buffer(msgnode->_data, msgnode->_total_len),
				std::bind(&CSession::handle_write, this, std::placeholders::_1, _self_shared));
		}
	}
	else {
		std::cout << "handle write failed, error is: " << error.what() << std::endl;
		_server->ClearSession(_uuid);
	}
}

