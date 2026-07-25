#include "Connection.h"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <iostream>
#include "ConnectionMgr.h"

Connection::Connection(net::io_context& ioc)
	: _ioc(ioc), _ws_ptr(std::make_unique<beast::websocket::stream<beast::tcp_stream>>(net::make_strand(ioc)))
{
	boost::uuids::random_generator generator;
	boost::uuids::uuid uuid = generator();
	_uuid = boost::uuids::to_string(uuid);
}

std::string& Connection::GetUuid()
{
	return _uuid;
}

net::ip::tcp::socket& Connection::GetSocket()
{
	return boost::beast::get_lowest_layer(*_ws_ptr).socket();
}

void Connection::AsyncAccept()
{
	auto self = shared_from_this();
	_ws_ptr->async_accept([self](boost::system::error_code ec) {
		try {
			if (!ec) {
				ConnectionMgr::GetInstance().AddConnection(self);
				self->Start();
			}
			else {
				std::cerr << "websocket accept failed, error is: " << ec.what() << std::endl;
			}
		}
		catch (std::exception& e) {
			std::cout << "websocket async accept exception is: " << e.what() << std::endl;
		}
	});
}

void Connection::Start()
{
	auto self = shared_from_this();
	_ws_ptr->async_read(_recv_buffer, [self](boost::system::error_code ec, std::size_t buffer_bytes) {
		try {
			if (!ec) {
				self->_ws_ptr->text(self->_ws_ptr->got_text());
				std::string recv_data = boost::beast::buffers_to_string(self->_recv_buffer.data());
				self->_recv_buffer.consume(self->_recv_buffer.size());
				std::cout << "websocket receive msg is: " << recv_data << std::endl;

				self->AsyncSend(recv_data);
				self->Start();
			}
			else {
				std::cerr << "websocket read failed, error is: " << ec.what() << std::endl;
				ConnectionMgr::GetInstance().RmvConnection(self->GetUuid());
			}
		}
		catch (std::exception& e) {
			std::cerr << "Exception: " << e.what() << std::endl;
			ConnectionMgr::GetInstance().RmvConnection(self->GetUuid());
		}
	});
}

void Connection::AsyncSend(std::string& msg)  
{  
    std::unique_lock<std::mutex> lock(_send_mutex);  
    int cur_que_size = _send_que.size();  
    _send_que.push(msg);  
    if (cur_que_size > 0) {  
        return;  
    }  

    auto& msgnode = _send_que.front();  
    lock.unlock();  

    auto self = shared_from_this();  
    _ws_ptr->async_write(net::buffer(msgnode),  
        [self](boost::system::error_code ec, std::size_t nsize) {  
            try {  
                if (!ec) {  
                    std::lock_guard<std::mutex> lk(self->_send_mutex);  
                    self->_send_que.pop();  
                    if (!self->_send_que.empty()) {  
                        auto& msgnode = self->_send_que.front();  
                        self->_ws_ptr->async_write(net::buffer(msgnode),  
                            [self, &msgnode](boost::system::error_code ec, std::size_t nsize) {  
                                self->AsyncSend(msgnode);  
                        });  
                    }  
                }  
                else {  
                    std::cerr << "async write error: " << ec.message() << std::endl;  
                    ConnectionMgr::GetInstance().RmvConnection(self->GetUuid());  
                }  
            }  
            catch (const std::exception& e) {  
                std::cerr << "Exception: " << e.what() << std::endl;  
                ConnectionMgr::GetInstance().RmvConnection(self->GetUuid());  
            }  
        });  
}


//AsyncSendÐÞ¸Ä
//void Connection::AsyncSend(std::string msg)
//{
//	{
//		std::lock_guard<std::mutex> lck_gurad(_send_mtx);
//		int que_len = _send_que.size();
//		_send_que.push(msg);
//		if (que_len > 0) {
//			return;
//		}
//	}
//
//	SendCallBack(std::move(msg));
//}
//
//
//void Connection::SendCallBack(std::string msg)
//{
//	auto self = shared_from_this();
//	_ws_ptr->async_write(boost::asio::buffer(msg.c_str(), msg.length()),
//		[self](error_code  err, std::size_t  nsize) {
//			try {
//				if (err) {
//					std::cout << "async send err is " << err.what() << std::endl;
//					ConnectionMgr::GetInstance().RmvConnection(self->_uuid);
//					return;
//				}
//
//				std::string send_msg;
//				{
//					std::lock_guard<std::mutex> lck_gurad(self->_send_mtx);
//					self->_send_que.pop();
//					if (self->_send_que.empty()) {
//						return;
//					}
//
//					send_msg = self->_send_que.front();
//				}
//
//				self->SendCallBack(std::move(send_msg));
//			}
//			catch (std::exception& exp) {
//				std::cout << "async send exception is " << exp.what() << std::endl;
//				ConnectionMgr::GetInstance().RmvConnection(self->_uuid);
//			}
//		});
//}

