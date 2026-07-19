#define _CRT_SECURE_NO_WARNINGS

#include <boost/asio.hpp>
#include <iostream>
#include <nlohmann/json.hpp>

const int MAX_LENGTH = 1024 * 2;
const int HEAD_ID_LEN = 2;
const int HEAD_DATA_LEN = 2;
const int HEAD_TOTAL_LEN = 4;

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

		int msg_id = 1001;
		int msg_id_host = boost::asio::detail::socket_ops::host_to_network_short(msg_id);
		memcpy(send_data, &msg_id_host, HEAD_ID_LEN);

		int request_length_host = boost::asio::detail::socket_ops::host_to_network_short(request_length);
		memcpy(send_data + HEAD_ID_LEN, &request_length_host, HEAD_DATA_LEN);

		memcpy(send_data + HEAD_TOTAL_LEN, request.data(), request_length);

		boost::asio::write(sock, boost::asio::buffer(send_data, request_length + HEAD_TOTAL_LEN));


		std::cout << "begin to receive..." << std::endl;

		char reply_head[HEAD_TOTAL_LEN] = { 0 };
		std::size_t reply_length = boost::asio::read(sock, boost::asio::buffer(reply_head, HEAD_TOTAL_LEN));

		int msgid = 0;
		memcpy(&msgid, reply_head, HEAD_ID_LEN);
		msgid = boost::asio::detail::socket_ops::network_to_host_short(msgid);

		int msglen = 0;
		memcpy(&msglen, reply_head + HEAD_ID_LEN, HEAD_DATA_LEN);
		msglen = boost::asio::detail::socket_ops::network_to_host_short(msglen);

		char reply_data[MAX_LENGTH] = { 0 };
		std::size_t msg_length = boost::asio::read(sock, boost::asio::buffer(reply_data, msglen));
		json reader;
		reader = json::parse(std::string(reply_data, msg_length));
		std::cout << "msg id is: " << root["id"].get<short>() << " msg is: " << root["data"] << std::endl;
	}
	catch (std::exception& e) {
		std::cerr << "Exception: " << e.what() << std::endl;
	}

	return 0;
}