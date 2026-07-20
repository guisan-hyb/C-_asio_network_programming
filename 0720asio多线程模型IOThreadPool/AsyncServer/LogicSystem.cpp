#include "LogicSystem.h"
#include <nlohmann/json.hpp>

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

void LogicSystem::RegisterCallBacks()
{
	_func_callback[MSG_HELLO_WORLD] = std::bind(&LogicSystem::HelloWorldCallBack, this,
		std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
}

void LogicSystem::HelloWorldCallBack(std::shared_ptr<CSession> session, const short& msg_id, const std::string& msg_data)
{
	json root = json::parse(msg_data);
	std::cout << "receive msg id: " << root["id"].get<short>() << " msg data: " << root["data"].get<std::string>();
	root["data"] = "server has received msg, msg data is: " + root["data"].get<std::string>();
	std::string return_str = root.dump();

	session->Send(return_str, msg_id);
}

void LogicSystem::DealMsg()
{
	for (;;) {
		std::unique_lock<std::mutex> unique_lk(_mutex);

		while (!_b_stop && _msg_que.empty()) {
			_consume.wait(unique_lk);
		}

		if (_b_stop) {
			while (!_msg_que.empty()) {
				auto& msgnode = _msg_que.front();
				std::cout << "recv msg id: " << msgnode->_recvnode->GetMsgId() << std::endl;
				auto call_back_iter = _func_callback.find(msgnode->_recvnode->GetMsgId());
				if (call_back_iter == _func_callback.end()) {
					_msg_que.pop();
					continue;
				}

				call_back_iter->second(msgnode->_session, msgnode->_recvnode->GetMsgId(),
					std::string(msgnode->_recvnode->_data, msgnode->_recvnode->_total_len));
				_msg_que.pop();
			}
			break;
		}

		auto& msgnode = _msg_que.front();
		std::cout << "recv msg id: " << msgnode->_recvnode->GetMsgId() << std::endl;
		auto call_back_iter = _func_callback.find(msgnode->_recvnode->GetMsgId());
		if (call_back_iter == _func_callback.end()) {
			_msg_que.pop();
			continue;
		}

		call_back_iter->second(msgnode->_session, msgnode->_recvnode->GetMsgId(),
			std::string(msgnode->_recvnode->_data, msgnode->_recvnode->_total_len));
		_msg_que.pop();
	}
}

LogicSystem::LogicSystem() 
	: _b_stop(false)
{
	RegisterCallBacks();
	_worker_thread = std::thread(&LogicSystem::DealMsg, this);
}
