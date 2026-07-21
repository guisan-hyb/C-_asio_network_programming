#define _CRT_SECURE_NO_WARNINGS

#include <boost/asio/co_spawn.hpp> //启动协程的函数
#include <boost/asio/detached.hpp> //分离，主线程启动多个协程
#include <boost/asio/io_context.hpp> //协程的调度需要一个执行器
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/write.hpp>
#include <iostream>


// 协程函数1：处理具体的 Echo 逻辑
boost::asio::awaitable<void> echo(boost::asio::ip::tcp::socket socket) {
	try {
		char data[1024];
		for (;;) {
			// 异步读：没数据时挂起，有数据时恢复，n 是读取到的字节数
			std::size_t n = co_await socket.async_read_some(boost::asio::buffer(data),
				boost::asio::use_awaitable);
			
			// 异步写：将收到的数据原样发回去。发完才恢复
			co_await boost::asio::async_write(socket, boost::asio::buffer(data),
				boost::asio::use_awaitable);
		}
	}
	catch (std::exception& e) {
		// 当客户端断开连接时，async_read_some 会抛出 EOF 异常
		// 捕获异常后，函数结束，socket 局部变量析构，底层连接自动关闭
		std::printf("echo Exception: %s\n", e.what());
	}
}


// 协程函数2：监听并接受新连接
boost::asio::awaitable<void> listener() {
	//获取当前协程所在的执行器，本质上是获取底层的 io_context
	auto executor = co_await boost::asio::this_coro::executor; //异步查询调度器

	// 一步到位：Open + Bind + Listen，在 10086 端口监听 IPv4 连接
	boost::asio::ip::tcp::acceptor acceptor(executor, { boost::asio::ip::tcp::v4(),10086 });

	for (;;) {
		// 【魔法发生的地方】
		// 异步等待新连接。如果没有连接，co_await 会让当前协程挂起，
		// io_context 的线程可以去干别的事（比如处理其他客户端的读写）。
		// 当新连接到来，协程在这里恢复，返回代表客户端的 socket。
		boost::asio::ip::tcp::socket socket = co_await acceptor.async_accept(boost::asio::use_awaitable);//可以看作是 当前协程被挂起等待accept信号，时间片分给其他协程，主线程一直是在运行的
		
		// 为这个新客户端启动一个独立的 echo 协程
		// co_spawn 相当于“开枝散叶”，创建一个新的并发协程任务
		// detached 表示我们不关心这个新协程什么时候结束，它在后台独立运行
		boost::asio::co_spawn(executor, echo(std::move(socket)), boost::asio::detached);
	}
}

int main() {
	try {
		// 构造 io_context，参数 1 表示暗示只跑 1 个线程
		boost::asio::io_context ioc(1);

		// 信号集：监听 Ctrl+C (SIGINT) 和 kill 命令 (SIGTERM)
		boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);
		// 异步等待信号，收到信号后停止 io_context 的事件循环
		signals.async_wait([&](auto, auto) {
			ioc.stop();
		});

		// 【启动入口】
		// 把 listener 协程挂载到 io_context 上，开始运行
		boost::asio::co_spawn(ioc, listener(), boost::asio::detached);

		// 阻塞主线程，驱动 io_context 的事件循环
		// 只有收到 SIGINT/SIGTERM 导致 stop() 被调用时，run() 才会返回
		ioc.run();
	}
	catch (std::exception& e) {
		std::cout << "Exception: " << e.what() << std::endl;
	}

	return 0;
}