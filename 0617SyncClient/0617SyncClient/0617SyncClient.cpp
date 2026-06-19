#include <boost/asio.hpp>
#include <iostream>

const int MAX_LENGTH = 1024;

int main() {
	try {
		//创建上下文服务
		boost::asio::io_context ioc;
		//构造endpoint
		boost::asio::ip::tcp::endpoint remote_ep(boost::asio::ip::make_address("127.0.0.1"), 10086);

		boost::asio::ip::tcp::socket sock(ioc);
		boost::system::error_code error = boost::asio::error::host_not_found;
		sock.connect(remote_ep, error);
		if (error) {
			std::cout << "connect failed,code is " << error.value()
				<< " error msg is " << error.message() << std::endl;
		}

		std::cout << "Enter message: ";
		char request[MAX_LENGTH];
		std::cin.getline(request, MAX_LENGTH);
		size_t request_lenth = strlen(request);
		boost::asio::write(sock, boost::asio::buffer(request, request_lenth));

		char reply[MAX_LENGTH];
		size_t reply_length = boost::asio::read(sock, boost::asio::buffer(reply, request_lenth));
		std::cout << "Reply is: ";
		std::cout.write(reply, reply_length);
		std::cout << '\n';
	}
	catch (std::exception& e) { //等价于boost库里的那个系统异常
		std::cerr << "Exception: " << e.what() << std::endl;
	}

	return 0;
}