#define _CRT_SECURE_NO_WARNINGS

#include <boost/asio.hpp>
#include <nlohmann/json.hpp>
#include <iostream>
#include <string>

#define MAX_LENGTH 1024*2
#define HEAD_LENGTH 2

using json = nlohmann::json;

int main() {
	try {
		boost::asio::io_context ioc;
		boost::asio::ip::tcp::endpoint remote_point(boost::asio::ip::make_address("127.0.0.1"), 10086);
		boost::asio::ip::tcp::socket sock(ioc);
		boost::system::error_code error;

		sock.connect(remote_point, error);

		if (error) {
			std::cout << "connect failed, code is: " << error.value() <<
				" error message is: " << error.message() << std::endl;
			return 0;
		}

		json root;
		root["id"] = 1001;
		root["data"] = "hello world";
		std::string request = root.dump();
		std::size_t request_length = request.length();
		char send_data[MAX_LENGTH] = { 0 };

		//转换为网络字节序
		int request_host_length = boost::asio::detail::socket_ops::host_to_network_short(request_length);
		memcpy(send_data, &request_host_length, HEAD_LENGTH);
		memcpy(send_data + HEAD_LENGTH, request.data(), request_length);
		boost::asio::write(sock, boost::asio::buffer(send_data, request_length + 2));

		std::cout << "begin to receive..." << std::endl;
		char reply_head[HEAD_LENGTH];
		std::size_t reply_length = boost::asio::read(sock, boost::asio::buffer(reply_head, HEAD_LENGTH));
		short msglen = 0;
		memcpy(&msglen, reply_head, HEAD_LENGTH);
		//转为本机字节序
		msglen = boost::asio::detail::socket_ops::network_to_host_short(msglen);
		char msg[MAX_LENGTH] = { 0 };
		std::size_t msg_lenth = boost::asio::read(sock, boost::asio::buffer(msg, msglen));

		root = json::parse(std::string(msg, msglen));//解析收到的网络字节流

		// 检查字段是否存在并打印
		if (root.contains("id") && root.contains("data")) {
			std::cout << "msg id is " << root["id"].get<int>()
				<< " msg is " << root["data"].get<std::string>() << std::endl;
		}
		else {
			std::cerr << "收到的数据缺少必要字段" << std::endl;
		}
	}
	catch (std::exception& e) {
		std::cerr << "Exception: " << e.what() << std::endl;
	}

	return 0;
}