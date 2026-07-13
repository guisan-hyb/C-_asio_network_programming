#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <boost/asio.hpp>
#include <thread>
#include <chrono>

#define MAX_LENGTH 1024*2
#define HEAD_LENGTH 2

int main() {
	try {
		boost::asio::io_context ioc;
		boost::asio::ip::tcp::endpoint remote_point(boost::asio::ip::make_address("127.0.0.1"), 10086);

		boost::asio::ip::tcp::socket sock(ioc);
		boost::system::error_code error;

		sock.connect(remote_point, error);
		if (error) {
			std::cout << "connect failed, error code is: " << error.value() << std::endl
				<< "error msg is: " << error.message() << std::endl;
			return 0;
		}

		std::thread send_thread([&sock] {
			for (;;) {
				std::this_thread::sleep_for(std::chrono::milliseconds(2));
				const char* request = "hello world!";
				std::size_t request_length = strlen(request);
				char send_data[MAX_LENGTH] = { 0 };
				memcpy(send_data, &request_length, HEAD_LENGTH);
				memcpy(send_data + HEAD_LENGTH, request, request_length);
				boost::asio::write(sock, boost::asio::buffer(send_data, request_length + 2));
			}
		});

		std::thread recv_thread([&sock] {
			for (;;) {
				std::this_thread::sleep_for(std::chrono::milliseconds(2));
				std::cout << "begin to receive..." << std::endl;

				char reply_head[HEAD_LENGTH];
				std::size_t reply_length = boost::asio::read(sock, boost::asio::buffer(reply_head, HEAD_LENGTH));
				short msglen = 0;
				memcpy(&msglen, reply_head, HEAD_LENGTH);

				char msg[MAX_LENGTH] = { 0 };
				std::size_t msg_length = boost::asio::read(sock, boost::asio::buffer(msg, msglen));

				std::cout << "Reply is: ";
				std::cout.write(msg, msglen) << std::endl;
				std::cout << "Reply len is " << msglen;
				std::cout << '\n';
			}
		});

		send_thread.join();
		recv_thread.join();
	}
	catch (std::exception& e) {
		std::cerr << "Exception: " << e.what() << std::endl;
	}

	return 0;
}