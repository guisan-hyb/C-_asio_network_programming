#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>

#include <chrono>
#include <ctime>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <nlohmann/json.hpp>

namespace beast = boost::beast;
namespace http = beast::http;
using json = nlohmann::json;
using tcp = boost::asio::ip::tcp;

namespace my_program_state {
	std::size_t request_count() { // 使用静态局部变量记录服务器启动以来处理的请求总数  每次调用自增 1
		static std::size_t count = 0;
		return ++count;
	}

	std::time_t now() { // 获取当前时间戳
		return std::time(0);
	}
}

class http_connection : public std::enable_shared_from_this<http_connection> {
public:
	http_connection(tcp::socket socket) : _socket(std::move(socket)) {

	}

	void Start() {
		read_request();
		check_deadline();
	}

private:
	void read_request() {
		auto self = shared_from_this();
		boost::beast::http::async_read(_socket, _buffer, _request,
			[self](boost::beast::error_code ec, std::size_t bytes_transferred) {
				boost::ignore_unused(bytes_transferred);
				if (!ec) {
					self->process_request();
				}
		});
	}

	void check_deadline() { // 超时控制
		auto self = shared_from_this();
		_deadline.async_wait([self](const boost::system::error_code ec) {
			if (!ec) {
				self->_socket.close();
			}
		});
	}

	void process_request() { // 请求路由
		_response.version(_request.version());
		_response.keep_alive(false);
		switch (_request.method())
		{
		case boost::beast::http::verb::get:
			_response.result(boost::beast::http::status::ok);
			_response.set(boost::beast::http::field::server, "Beast");
			create_response();
			break;

		case boost::beast::http::verb::post:
			_response.result(http::status::ok);
			_response.set(http::field::server, "Beast");
			create_post_response();
			break;

		default:
			_response.result(boost::beast::http::status::bad_request);
			_response.set(boost::beast::http::field::content_type, "text/plain");
			boost::beast::ostream(_response.body()) << "Invalid request-method '"
				<< std::string(_request.method_string()) << "'";
			break;
		}

		write_response();
	}

	void write_response() { // 发送响应
		auto self = shared_from_this();
		_response.content_length(_response.body().size());
		http::async_write(_socket, _response,
			[self](beast::error_code ec, std::size_t) {
				self->_socket.shutdown(tcp::socket::shutdown_send, ec);// 关闭发送端
				self->_deadline.cancel();
		});
	}

	void create_response() { // GET请求处理
		if (_request.target() == "/count") { // 返回 HTML 格式的请求计数
			_response.set(boost::beast::http::field::content_type, "text/html");
			boost::beast::ostream(_response.body())
				<< "<html>\n"
				<< "<head><title>Request count</title></head>\n"
				<< "<body>\n"
				<< "<h1>Request count</h1>\n"
				<< "<p>There have been "
				<< my_program_state::request_count()
				<< " requests so far.</p>\n"
				<< "</body>\n"
				<< "</html>\n";
		}
		else if (_request.target() == "/time") { // 返回 HTML 格式的时间戳
			_response.set(http::field::content_type, "text/html");
			boost::beast::ostream(_response.body())
				<< "<html>\n"
				<< "<head><title>Current time</title></head>\n"
				<< "<body>\n"
				<< "<h1>Current time</h1>\n"
				<< "<p>The current time is "
				<< my_program_state::now()
				<< " seconds since the epoch.</p>\n"
				<< "</body>\n"
				<< "</html>\n";
		}
		else {
			_response.result(boost::beast::http::status::not_found);
			_response.set(boost::beast::http::field::content_type, "text/plain");
			beast::ostream(_response.body()) << "File not found\r\n";
		}
	}

	void create_post_response() { // POST请求处理
		if (_request.target() == "/email") {
			auto& body = _request.body();
			auto body_str = beast::buffers_to_string(body.data());
			std::cout << "receive body is: " << body_str << std::endl;
			_response.set(http::field::content_type, "text/json");
			
			json root, src_root;
			try {
				src_root = json::parse(body_str);
			}
			catch (const json::parse_error& e) {
				std::cout << "Failed to parse JSON data! Error: " << e.what() << std::endl;
				root["error"] = 1001;
				std::string jsonstr = root.dump();
				beast::ostream(this->_response.body()) << jsonstr;
				return;
			}

			auto email = src_root["email"].get<std::string>();
			std::cout << "email is: " << email << std::endl;

			root["error"] = 0;
			root["email"] = src_root["email"];
			root["msg"] = "receive email post success";
			std::string jsonstr = root.dump();
			beast::ostream(_response.body()) << jsonstr;
		}
		else {
			_response.result(boost::beast::http::status::not_found);
			_response.set(boost::beast::http::field::content_type, "text/plain");
			beast::ostream(_response.body()) << "File not found\r\n";
		}
	}


	tcp::socket _socket;// 与客户端通信
	boost::beast::flat_buffer _buffer{ 8192 };// 读取数据的缓冲区
	boost::beast::http::request<boost::beast::http::dynamic_body> _request;// 请求对象
	boost::beast::http::response<boost::beast::http::dynamic_body> _response;// 响应对象
	//超时定时器，60秒
	boost::asio::steady_timer _deadline{
		_socket.get_executor(),std::chrono::seconds(60)
	};
};


void http_server(tcp::acceptor& acceptor, tcp::socket& socket) {
	acceptor.async_accept(socket, [&](boost::system::error_code ec) {
		if (!ec) {
			//这里创建 shared_ptr 启动，是因为使用 enable_shared_from_this 必须有一个 shared_ptr
			std::make_shared<http_connection>(std::move(socket))->Start();
		}
		//Boost.Asio 的 async_accept API 有一个硬性要求：传入的 peer socket 必须处于“未打开”状态，否则会报错
		//恰恰因为前面 std::move 把原来的 socket 清空了，此时传进来的 socket 正好是“未打开”的空壳！Asio 再次顺利地把下一个客户端连接装进这个空壳里
		http_server(acceptor, socket);
	});
}


int main() {
	try {
		auto const address = boost::asio::ip::make_address("127.0.0.1");
		unsigned short port = static_cast<unsigned short>(8080);
		boost::asio::io_context ioc{ 1 };// 推荐线程数: 1
		tcp::acceptor acceptor{ ioc,{address,port} };
		tcp::socket socket{ ioc };
		http_server(acceptor, socket);
		ioc.run();
	}
	catch (std::exception& e) {
		std::cerr << "Error: " << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return 0;
}