#pragma once

#include <queue>
#include <thread>
#include "CSession.h"
#include <map>
#include <functional>
#include "const.h"

using FuncCallBack = std::function<void(std::shared_ptr<CSession>, const short& msg_id, const std::string& msg_data)>;

class LogicSystem
{
public:
	~LogicSystem();
	void PostMsgToQue(std::shared_ptr<LogicNode> msg);
	static LogicSystem& GetInstance();

	LogicSystem(const LogicSystem&) = delete;
	LogicSystem operator=(const LogicSystem&) = delete;

private:
	LogicSystem();
	void DealMsg();
	void RegisterCallBacks();
	void HelloWorldCallBack(std::shared_ptr<CSession> session, const short& msg_id, const std::string& msg_data);

	std::thread _worker_threads;
	std::queue<std::shared_ptr<LogicNode>> _msg_que;
	std::mutex _mutex;
	std::condition_variable _consume;
	bool _is_stop;
	std::map<short, FuncCallBack> _func_callbacks;
};

