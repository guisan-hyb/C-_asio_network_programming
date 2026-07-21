#include <iostream>
#include <boost/asio.hpp>
#include <string>

const int MAX_LENGTH = 1024;

int main() {
	try{
		boost::asio::io_context ioc;
		boost::asio::ip::tcp::endpoint remote_ep(boost::asio::ip::make_address("127.0.0.1"), 10086);
		boost::asio::ip::tcp::socket socket(ioc);
		boost::system::error_code error;
		socket.connect(remote_ep, error);

		if (error) {
			std::cerr << "connect failed, code is: " << error.value() 
				<< " error msg is " << error.message() << std::endl;
			return 0;
		}

		std::cout << "Enter message:";
		char request[MAX_LENGTH];
		std::cin.getline(request, MAX_LENGTH);
		std::size_t request_length = strlen(request);
		boost::asio::write(socket, boost::asio::buffer(request, request_length));

		char reply[MAX_LENGTH];
		std::size_t reply_length = boost::asio::read(socket, boost::asio::buffer(reply, request_length));
		std::cout << "reply is: " << std::string(reply, reply_length) << std::endl;
		getchar();
	}
	catch (const std::exception& e){
		std::cerr << "Exception: " << e.what() << std::endl;
	}

	return 0;
}