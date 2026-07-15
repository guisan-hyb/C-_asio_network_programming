#define _CRT_SECURE_NO_WARNINGS

#include <boost/asio.hpp>
#include <iostream>
#include <cstdint>

//为保证字节序一致性，网络传输使用网络字节序，也就是大端模式

int main() {
	uint32_t host_long_value = 0x12345678;
	uint16_t host_short_value = 0x5678;

	//需要注意的是，在使用这些函数时，应该确保输入参数和返回结果都是无符号整数类型，否则可能会出现错误
	uint32_t network_long_value = boost::asio::detail::socket_ops::host_to_network_long(host_long_value);
	uint16_t network_short_value = boost::asio::detail::socket_ops::host_to_network_short(host_short_value);

	std::cout << "Host long value: 0x" << std::hex << host_long_value << std::endl;
	std::cout << "Network long value: 0x" << std::hex << network_long_value << std::endl;
	std::cout << "Host short value: 0x" << std::hex << host_short_value << std::endl;
	std::cout << "Network short value: 0x" << std::hex << network_short_value << std::endl;

	return 0;
}