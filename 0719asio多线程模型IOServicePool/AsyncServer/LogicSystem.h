#pragma once

#include "Singleton.h"
#include <queue>
#include <thread>
#include "CSession.h"
#include <map>
#include <functional>
#include "const.h"
#include <nlohmann/json.hpp>

using FunCallBack = std::function<void(std::shared_ptr<CSession> session, const short& msg_id, const std::string& msg_data)>;

class LogicSystem : public Singleton<LogicSystem>
{
	friend class Singleton<LogicSystem>;
public:
	~LogicSystem();
	void PostMsgToQue(std::shared_ptr<LogicNode> msg);

private:
	LogicSystem();
	void RegisterCallBacks();
	void HelloWorldCallBack(std::shared_ptr<CSession>, const short& msg_id, const std::string& msg_data);
	void DealMsg(); // 线程调用的一个函数

	std::queue<std::shared_ptr<LogicNode>> _msg_que;
	std::mutex _mutex;
	std::condition_variable _consume;
	std::thread _worker_thread;
	bool _b_stop;
	std::map<short, FunCallBack> _func_callback;
};

