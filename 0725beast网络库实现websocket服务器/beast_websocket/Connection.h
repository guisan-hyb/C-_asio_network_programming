#pragma once

#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <memory>
#include <mutex>
#include <queue>
#include <string>

namespace net = boost::asio;
namespace beast = boost::beast;
namespace websocket = beast::websocket;

class Connection : public std::enable_shared_from_this<Connection>
{
public:
	Connection(net::io_context& ioc);
	std::string& GetUuid();
	net::ip::tcp::socket& GetSocket();
	void AsyncAccept();
	void Start();
	void AsyncSend(std::string& msg);

private:
	std::unique_ptr<beast::websocket::stream<beast::tcp_stream>> _ws_ptr;
	std::string _uuid;
	net::io_context& _ioc;
	beast::flat_buffer _recv_buffer;
	std::queue<std::string> _send_que;
	std::mutex _send_mutex;
};

