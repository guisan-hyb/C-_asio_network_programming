#pragma once

#include <boost/unordered_map.hpp>

class Connection;

class ConnectionMgr
{
public:
	static ConnectionMgr& GetInstance();
	void AddConnection(std::shared_ptr<Connection> conptr);
	void RmvConnection(std::string& uuid);

	ConnectionMgr(const ConnectionMgr&) = delete;
	ConnectionMgr& operator=(const ConnectionMgr&) = delete;

private:
	ConnectionMgr();

	boost::unordered_map<std::string, std::shared_ptr<Connection>> _con_maps;
};

