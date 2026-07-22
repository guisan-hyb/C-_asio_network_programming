#include "LogicSystem.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

LogicSystem::LogicSystem() : _is_stop(false) {
	RegisterCallBacks();
	_worker_threads = std::thread(&LogicSystem::DealMsg, this);
}

LogicSystem::~LogicSystem() {
	_is_stop = true;
	_consume.notify_one();
	_worker_threads.join();
}

void LogicSystem::PostMsgToQue(std::shared_ptr<LogicNode> msg)
{
	std::unique_lock<std::mutex> unique_lk(_mutex);
	_msg_que.push(msg);
	if (_msg_que.size() == 1) {
		_consume.notify_one();
	}
}

void LogicSystem::DealMsg() {
	for (;;) {
		std::unique_lock<std::mutex> unique_lk(_mutex);
		while (!_is_stop && _msg_que.empty()) {
			_consume.wait(unique_lk);
		}

		if (_is_stop) {
			while (!_msg_que.empty()) {
				auto msgnode = _msg_que.front();
				std::cout << "recv msg id is: " << msgnode->_recvnode->GetMsgId() << std::endl;
				auto call_back_iter = _func_callbacks.find(msgnode->_recvnode->GetMsgId());
				if (call_back_iter == _func_callbacks.end()) {
					_msg_que.pop();
					continue;
				}

				call_back_iter->second(msgnode->_session, msgnode->_recvnode->GetMsgId(),
					std::string(msgnode->_recvnode->_data, msgnode->_recvnode->_total_len));
				_msg_que.pop();
			}
			break;
		}

		//没有停服并且说明队列中有数据
		auto msgnode = _msg_que.front();
		std::cout << "recv msg id is: " << msgnode->_recvnode->GetMsgId() << std::endl;
		auto call_back_iter = _func_callbacks.find(msgnode->_recvnode->GetMsgId());
		if (call_back_iter == _func_callbacks.end()) {
			_msg_que.pop();
			continue;
		}

		call_back_iter->second(msgnode->_session, msgnode->_recvnode->GetMsgId(),
			std::string(msgnode->_recvnode->_data, msgnode->_recvnode->_total_len));
		_msg_que.pop();
	}
}

void LogicSystem::RegisterCallBacks()
{
	_func_callbacks[MSG_HELLO_WORLD] = std::bind(&LogicSystem::HelloWorldCallBack, this,
		std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
}

void LogicSystem::HelloWorldCallBack(std::shared_ptr<CSession> session, const short& msg_id, const std::string& msg_data)
{
	json root = json::parse(msg_data);
	std::cout << "receive msg id: " << root["id"].get<short>() << " msg data is: " << root["data"].get<std::string>();
	root["data"] = "server has received msg, msg data is: " + root["data"].get<std::string>();
	std::string return_str = root.dump();

	session->Send(return_str, msg_id);
}

LogicSystem& LogicSystem::GetInstance() {
	static LogicSystem instance;
	return instance;
}

