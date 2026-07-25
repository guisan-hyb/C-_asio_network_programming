#include "ConnectionMgr.h"
#include "Connection.h"

ConnectionMgr& ConnectionMgr::GetInstance()
{
	static ConnectionMgr instance;
	return instance;
}

void ConnectionMgr::AddConnection(std::shared_ptr<Connection> conptr)
{
	_con_maps[conptr->GetUuid()] = conptr;
}

void ConnectionMgr::RmvConnection(std::string& uuid)
{
	_con_maps.erase(uuid);
}

ConnectionMgr::ConnectionMgr() {

}