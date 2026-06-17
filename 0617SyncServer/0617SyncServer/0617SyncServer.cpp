#include <boost/asio.hpp>
#include <iostream>
#include <set>
#include <memory>
#include <thread>
#include <cstring>

const int MAX_LENGTH = 1024;
using socket_ptr = std::shared_ptr<boost::asio::ip::tcp::socket>;
std::set<std::shared_ptr<std::thread>> thread_set;


//创建session函数，该函数为服务器处理客户端请求，每当我们获取客户端连接后就调用该函数。
//在session函数里里进行echo方式的读写，所谓echo就是应答式的处理
void session(socket_ptr sock) { //这个函数相当于服务器处理客户端一个连接的读和写
	try {
		for (;;) {
			char data[MAX_LENGTH];
			memset(data, '\0', MAX_LENGTH);
			boost::system::error_code error;

			//size_t legnth = boost::asio::read(sock, boost::asio::buffer(data, MAX_LENGTH), error);
			//这种写法服务器要接收1024字节才停止，如果客户端只发了几个字节，服务器就会一直阻塞在这，十分不友好

			size_t length = sock->read_some(boost::asio::buffer(data, MAX_LENGTH), error);
			if (error == boost::asio::error::eof) { // eof 表示对端关闭的错误
				std::cout << "connection closed by peer" << std::endl;
				break;
			}
			else if (error) {
				throw boost::system::system_error(error);
			}

			std::cout << "receive from " << sock->remote_endpoint().address().to_string() << std::endl;
			std::cout << "receive message is " << data << std::endl;
			//把数据回传给对方
			boost::asio::write(*sock, boost::asio::buffer(data, MAX_LENGTH));
		}
	}
	catch (std::exception& e) {
		std::cerr << "Exception in thread: " << e.what() << '\n' << std::endl;
	}
}

void server(boost::asio::io_context& io_context, unsigned short port) {
	boost::asio::ip::tcp::acceptor a(io_context,
		boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port));
	for (;;) {
		socket_ptr socket(new boost::asio::ip::tcp::socket(io_context));
		a.accept(*socket);

		//这个线程做 session 这样的工作，session的参数是socket
		auto t = std::make_shared<std::thread>(session, socket);
		//有可能这个线程还没来得及跑完，循环就结束了，t生命周期结束，智能指针被析构
		//因此要用 shared_ptr 引用计数的特性，这里把它放到集合里面
		thread_set.insert(t);
	}
}

int main() {
	try {
		boost::asio::io_context ioc;
		server(ioc, 10086);
		//主线程一定要等到子线程退出才退出
		for (auto& t : thread_set) {
			t->join();
		}
	}
	catch (std::exception& e) {
		std::cerr << "Exception " << e.what() << std::endl;
	}

	return 0;
}