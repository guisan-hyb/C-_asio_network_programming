#define _CRT_SECURE_NO_WARNINGS
#include "endPoint.h"
#include <boost/asio.hpp>
#include <iostream>

//socket的监听和连接

//一.端点的创建

//如果我们是客户端，我们可以通过对端的ip和端口构造一个endpoint，用这个endpoint和其通信
int client_end_point()
{
	std::string raw_ip_address = "127.4.8.1";
	unsigned short port_num = 3333;
	boost::system::error_code ec;//错误码，转换时若出现问题，我们可以通过错误码判断
	boost::asio::ip::address ip_address = boost::asio::ip::make_address(raw_ip_address, ec);

	if (ec.value() != 0) { //有错误
		std::cout << "Failed to parse the IP address.Error = " << ec.value()
			<< " .Message is " << ec.message() << std::endl;
		return ec.value();
	}

	boost::asio::ip::tcp::endpoint ep(ip_address, port_num);//把ip地址和端口号转换成端点

	return 0;
}

//如果是服务端，则只需根据本地地址绑定就可以生成endpoint
int server_end_point()
{
	unsigned short port_num = 3333;
	boost::asio::ip::address ip_address = boost::asio::ip::address_v6::any();//服务器本地地址
	boost::asio::ip::tcp::endpoint ep(ip_address, port_num);

	return 0;
}



//二.创建socket
//创建socket分为4步，创建上下文iocontext，选择协议，生成socket，打开socket

// 通信的socket
int create_tcp_socket()
{
	//写法一：构造 + 打开
	
	//io_context 是整个 Asio 体系的“大脑”和“调度中心”
	boost::asio::io_context ioc;// 负责监听操作系统的通知、触发回调函数
	boost::asio::ip::tcp protocol = boost::asio::ip::tcp::v4();
	boost::asio::ip::tcp::socket sock(ioc);
	boost::system::error_code ec;
	sock.open(protocol, ec);
	if (ec.value() != 0) { //有错误
		std::cout << "Failed to open the socket! Error code = " << ec.value()
			<< ". Message: " << ec.message() << std::endl;
		return ec.value();
	}

	//写法二：构造时打开
	//boost::asio::ip::tcp::socket sock2(ioc, boost::asio::ip::tcp::v4());

	return 0;
}

//如果是服务端，我们还需要生成一个acceptor类型的socket，用来接收新的连接
int create_acceptor_socket()
{
	//写法一：空壳构造，需要手动调用 open() -> bind() -> listen()
	//boost::asio::io_context ioc;
	//boost::asio::ip::tcp::acceptor acceptor(ioc);

	//写法二：显示Open，需要继续调用 bind() 和 listen()
	/*boost::asio::io_context ioc2;
	boost::asio::ip::tcp protocol = boost::asio::ip::tcp::v6();
	boost::asio::ip::tcp::acceptor acceptor(ioc2);
	boost::system::error_code ec;
	acceptor.open(protocol, ec);
	if (ec.value() != 0) {
		std::cout << "Failed to open the acceptor socket!"
			<< "Error code = " << ec.value() << ". Message : " << ec.message();
		return ec.value();
	}*/

	//写法三：一步到位，执行了open(),bind(),listen()
	boost::asio::io_context ioc3;
	boost::asio::ip::tcp::acceptor a(ioc3, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), 3333));

	return 0;
}



//三.绑定acceptor

//对于acceptor类型的socket，服务器要将其绑定到指定的端点,所有连接这个端点的连接都可以被接收到
int bind_acceptor_socket()
{
	unsigned short port_num = 3333;
	boost::asio::ip::tcp::endpoint ep(boost::asio::ip::address_v4::any(), port_num);
	boost::asio::io_context ioc;
	boost::asio::ip::tcp::acceptor acceptor(ioc, ep.protocol());
	boost::system::error_code ec;
	acceptor.bind(ep, ec);
	if (ec.value() != 0) {
		std::cout << "Failed to bind the acceptor socket."
			<< "Error code = " << ec.value() << ". Message : " << ec.message() << std::endl;
		return ec.value();
	}

	return 0;
}



//四.连接指定的端点

//作为客户端可以连接服务器指定的端点进行连接
int connect_to_end() //重点
{
	std::string raw_ip_address = "192.168.1.124";//服务器地址
	unsigned short port_num = 3333;//服务器端口号
	try {
		boost::asio::ip::tcp::endpoint ep(boost::asio::ip::make_address(raw_ip_address), port_num);
		boost::asio::io_context ioc;
		boost::asio::ip::tcp::socket sock(ioc, ep.protocol());
		sock.connect(ep);//只接受单个端点
	}
	catch (boost::system::system_error& e) {
		std::cout << "Error occured! Error code = " << e.code() << ". Message : " << e.what() << std::endl;

		return e.code().value();
	}

	return 0;
}

//如果只知道域名呢
int dns_connect_to_end()
{
	std::string host = "jianchi";
	std::string port_num = "3333";

	try {
		boost::asio::io_context ioc;
		boost::asio::ip::tcp::resolver resolver(ioc); // 创建解析器对象

		boost::asio::ip::tcp::resolver::results_type endpoints = resolver.resolve(host, port_num, boost::asio::ip::tcp::resolver::numeric_service);
		//auto endpoints = resolver.resolve(host, port_num); 等价写法

		boost::asio::ip::tcp::socket socket(ioc);

		//在基于域名进行客户端连接时，永远推荐使用 asio::connect(socket, endpoints)
		//如果使用socket.connect()，需要手动遍历
		boost::asio::connect(socket, endpoints);//接收一个端点序列
	}
	catch (const boost::system::system_error& e) {
		std::cout << "Error code: " << e.code().value()
			<< ". Message: " << e.what() << std::endl;
	}

	return 0;
}



//五.服务器接收连接

//当有客户端连接时，服务器需要接收连接
int accept_new_connection() //重点
{
	const int BACKLOG_SIZE = 30;//监听队列的大小/缓冲区队列的大小；看似是30，按照tcp算法上，其实是30*2 = 60
	//最多能缓冲60个来不及处理的连接
	unsigned short port_num = 3333;
	boost::asio::ip::tcp::endpoint ep(boost::asio::ip::address_v4::any(), port_num);//服务器本地的地址+端口
	boost::asio::io_context ioc;
	try {
		boost::asio::ip::tcp::acceptor acceptor(ioc, ep.protocol());
		acceptor.bind(ep);
		acceptor.listen(BACKLOG_SIZE);

		boost::asio::ip::tcp::socket sock(ioc);//服务器再创建一个socket，用于与客户端通信
		acceptor.accept(sock);//把接收到的新连接交给socket处理
	}
	catch (boost::system::system_error& e) {
		std::cout << "Error occured! Error code = " << e.code().value()
			<< ". Message: " << e.what();

		return e.code().value();
	}

	return 0;
}






//buffer结构

void use_const_buffer()
{
	std::string buf = "hello world";
	boost::asio::const_buffer asio_buf(buf.data(), buf.length());
	std::vector<boost::asio::const_buffer> buffers_sequence;
	buffers_sequence.emplace_back(asio_buf);

	//伪代码：
	//asio.send(buffers_sequence);
}

//上面这个写法太复杂，可以直接用buffer函数转化为send需要的参数类型
//boost::asio::buffer() 是一个重载函数，它可以根据你传入的参数自动推导并生成合适的缓冲区对象
void use_buffer_str()
{
	boost::asio::const_buffer asio_buf = boost::asio::buffer("hello world");
}

//如果要传的是数组呢
void use_buffer_array()
{
	const size_t BUF_SIZE_BYTES = 20;
	std::unique_ptr<char[]> buf(new char[BUF_SIZE_BYTES]);
	auto input_buf = boost::asio::buffer(static_cast<void*>(buf.get()), BUF_SIZE_BYTES);
}

//对于流式操作，我们可以用streambuf，将输入输出流和streambuf绑定，可以实现流式输入和输出。
void use_stream_buffer()
{
	boost::asio::streambuf buf;
	std::ostream output(&buf);
	output << "Message1\nMessage2";
	std::istream input(&buf);

	std::string message1;
	std::getline(input, message1);
}







//同步读写

//同步写 write_some
//boost::asio提供了几种同步写的api，write_some可以每次向指定的空间写入固定的字节数，如果写缓冲区满了，就只写一部分，返回写入的字节数
void write_to_socket(boost::asio::ip::tcp::socket& sock) {
	std::string buf = "hello world";
	std::size_t total_bytes_written = 0;//表示我们总共写了多少数据

	//循环发送（重要）
	while (total_bytes_written != buf.length()) {
		//write_some 返回每次写入的字节数
		total_bytes_written += sock.write_some(boost::asio::buffer(buf.data() + total_bytes_written, buf.length() - total_bytes_written));
		//参数1：指针偏移    参数2：剩余长度
	}
}

//同步写 write_some
//客户端连接端口并发送
int send_data_by_write_some()
{
	std::string raw_ip_address = "192.168.3.11";
	unsigned short port_num = 3333;
	try {
		boost::asio::ip::tcp::endpoint ep(boost::asio::ip::make_address(raw_ip_address), port_num);
		boost::asio::io_context ioc;
		boost::asio::ip::tcp::socket sock(ioc, ep.protocol());
		sock.connect(ep);
		write_to_socket(sock);//调用上面那个我自己写的函数
	}
	catch (boost::system::system_error& e) {
		std::cout << "Error occured! Error code = " << e.code().value()
			<< ". Message: " << e.what();
		return e.code().value();
	}

	return 0;
}

//同步写 send
//客户端连接端口并发送
//write_some使用起来比较麻烦，需要多次调用，asio提供了send函数。
//send函数会一次性将buffer中的内容发送给对端，如果有部分字节因为发送缓冲区满无法发送，则阻塞等待，直到发送缓冲区可用，则继续发送完成。
int send_data_by_send()
{
	std::string raw_ip_address = "192.168.3.11";
	unsigned short port_num = 3333;
	try {
		boost::asio::ip::tcp::endpoint ep(boost::asio::ip::make_address(raw_ip_address), port_num);
		boost::asio::io_context ioc;
		boost::asio::ip::tcp::socket sock(ioc, ep.protocol());
		sock.connect(ep);

		std::string buf = "hello world";
		int send_length = sock.send(boost::asio::buffer(buf.data(), buf.length()));
		if (send_length <= 0) {
			std::cout << "send failed" << std::endl;
			return 0;
		}
	}
	catch (boost::system::system_error& e) {
		std::cout << "Error occured! Error code = " << e.code().value()
			<< ". Message: " << e.what();
		return e.code().value();
	}

	return 0;
}

//同步写 write
//类似send方法，asio还提供了一个write函数，可以一次性将所有数据发送给对端，
//如果发送缓冲区满了则阻塞，直到发送缓冲区可用，将数据发送完成。
int send_data_by_write()
{
	std::string raw_ip_address = "192.168.3.11";
	unsigned short port_num = 3333;
	try {
		boost::asio::ip::tcp::endpoint ep(boost::asio::ip::make_address(raw_ip_address), port_num);
		boost::asio::io_context ioc;
		boost::asio::ip::tcp::socket sock(ioc, ep.protocol());
		sock.connect(ep);

		std::string buf = "hello world";
		int send_length = boost::asio::write(sock, boost::asio::buffer(buf.data(), buf.length()));
		if (send_length <= 0) {
			std::cout << "send failed" << std::endl;
			return 0;
		}
	}
	catch (boost::system::system_error& e) {
		std::cout << "Error occured! Error code = " << e.code().value()
			<< ". Message: " << e.what();
		return e.code().value();
	}

	return 0;
}


//同步读 read_some
//同步读和同步写类似，提供了读取指定字节数的接口read_some
std::string read_from_socket(boost::asio::ip::tcp::socket& sock)
{
	const unsigned char MESSAGE_SIZE = 7;
	char buf[MESSAGE_SIZE];
	std::size_t total_bytes_read = 0;
	while (total_bytes_read != MESSAGE_SIZE) {
		total_bytes_read += sock.read_some(boost::asio::buffer(buf + total_bytes_read, MESSAGE_SIZE - total_bytes_read));
	}

	return std::string(buf, total_bytes_read);
}

//作为socket怎么把读的过程串联起来呢
int read_data_by_read_some()
{
	std::string raw_ip_address = "127.0.0.1";
	unsigned short port_num = 3333;
	try {
		boost::asio::ip::tcp::endpoint ep(boost::asio::ip::make_address(raw_ip_address), port_num);
		boost::asio::io_context ioc;
		boost::asio::ip::tcp::socket sock(ioc, ep.protocol());
		sock.connect(ep);
		read_from_socket(sock);//调用上面写的函数
	}
	catch (boost::system::system_error& e) {
		std::cout << "Error occured! Error code = " << e.code().value()
			<< ". Message: " << e.what();
		return e.code().value();
	}

	return 0;
}

//同步读receive
//可以一次性同步接收对方发送的数据
int read_data_by_receive()
{
	std::string raw_ip_address = "192.168.1.11";
	unsigned short port_num = 3333;
	try {
		boost::asio::ip::tcp::endpoint ep(boost::asio::ip::make_address(raw_ip_address), port_num);
		boost::asio::io_context ioc;
		boost::asio::ip::tcp::socket sock(ioc, ep.protocol());
		sock.connect(ep);

		const unsigned char BUFF_SIZE = 7;
		char buffer_receive[BUFF_SIZE];
		int receive_length = sock.receive(boost::asio::buffer(buffer_receive, BUFF_SIZE));
		if (receive_length <= 0) {
			std::cout << "receive failed" << std::endl;
			return 0;
		}
	}
	catch (boost::system::system_error& e) {
		std::cout << "Error occured! Error code = " << e.code().value()
			<< ". Message: " << e.what();
		return e.code().value();
	}
	
	return 0;
}

//同步读read
//可以一次性同步读取对方发送的数据
int read_data_by_read()
{
	std::string raw_ip_address = "192.168.1.11";
	unsigned short port_num = 3333;
	try {
		boost::asio::ip::tcp::endpoint ep(boost::asio::ip::make_address(raw_ip_address), port_num);
		boost::asio::io_context ioc;
		boost::asio::ip::tcp::socket sock(ioc, ep.protocol());
		sock.connect(ep);

		const unsigned char BUFF_SIZE = 7;
		char buffer_receive[BUFF_SIZE];
		int receive_length = boost::asio::read(sock, boost::asio::buffer(buffer_receive, BUFF_SIZE));
		if (receive_length <= 0) {
			std::cout << "receive failed" << std::endl;
			return 0;
		}
	}
	catch (boost::system::system_error& e) {
		std::cout << "Error occured! Error code = " << e.code().value()
			<< ". Message: " << e.what();
		return e.code().value();
	}

	return 0;
}

//读取直到指定字符
//我们可以一直读取，直到读取指定字符结束
std::string read_data_by_until(boost::asio::ip::tcp::socket& sock)
{
	boost::asio::streambuf buf;
	boost::asio::read_until(sock, buf, '\n');//读到换行符结束
	std::string message;
	std::istream input_stream(&buf);
	std::getline(input_stream, message);

	return message;
}


