#include "LogicSystem.h"

using json = nlohmann::json;

LogicSystem::~LogicSystem()
{
	_b_stop = true;
	_consume.notify_one();
	_worker_thread.join();
}

void LogicSystem::PostMsgToQue(std::shared_ptr<LogicNode> msg)
{
	std::unique_lock<std::mutex> unique_lk(_mutex);
	_msg_que.push(msg);

	if (_msg_que.size() == 1) {
		_consume.notify_one();
	}
}

LogicSystem::LogicSystem() : _b_stop(false) {
	RegisterCallBacks();
	_worker_thread = std::thread(&LogicSystem::DealMsg, this);
}

void LogicSystem::RegisterCallBacks()
{
	_func_callback[MSG_HELLO_WORLD] = std::bind(&LogicSystem::HelloWorldCallBack, this,
		std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
}

void LogicSystem::HelloWorldCallBack(std::shared_ptr<CSession> session, const short& msg_id, const std::string& msg_data)
{
	json root = json::parse(msg_data);
	std::cout << "receive msg id: " << root["id"].get<short>() << " msg data is " << root["data"].get<std::string>();
	root["data"] = "server has received msg, msg data is: " + root["data"].get<std::string>();
	std::string return_str = root.dump();

	session->Send(return_str, msg_id);
}

void LogicSystem::DealMsg()
{
	for (;;) {
		std::unique_lock<std::mutex> unique_lk(_mutex);

		//判断队列为空则用条件变量等待
		while (_msg_que.empty() && !_b_stop) {
			_consume.wait(unique_lk);
		}

		//判断如果为关闭状态，取出逻辑队列所有数据及时处理并退出循环
		if (_b_stop) {
			while (!_msg_que.empty()) {
				auto msg_node = _msg_que.front();
				std::cout << "recv msg id is: " << msg_node->_recvnode->GetMsgId() << std::endl;
				auto call_back_iter = _func_callback.find(msg_node->_recvnode->GetMsgId());
				if (call_back_iter == _func_callback.end()) {
					_msg_que.pop();
					continue;
				}

				call_back_iter->second(msg_node->_session, msg_node->_recvnode->GetMsgId(),
					std::string(msg_node->_recvnode->_data, msg_node->_recvnode->_total_len));
				_msg_que.pop();
			}
			break;
		}

		//如果没有停服，且说明队列中有数据
		auto msg_node = _msg_que.front();
		std::cout << "recv msg id is: " << msg_node->_recvnode->GetMsgId() << std::endl;
		auto call_back_iter = _func_callback.find(msg_node->_recvnode->GetMsgId());
		if (call_back_iter == _func_callback.end()) {
			_msg_que.pop();
			continue;
		}

		call_back_iter->second(msg_node->_session, msg_node->_recvnode->GetMsgId(),
			std::string(msg_node->_recvnode->_data, msg_node->_recvnode->_total_len));
		_msg_que.pop();
	}
}

