#pragma once

#include <string>

class MsgNode
{
public:
	MsgNode(int max_len);

	~MsgNode();

	void Clear();

	char* _data;
	int _cur_len;
	int _total_len;
};

class RecvNode : public MsgNode {
public:
	RecvNode(int max_len, short msg_id);
	short GetMsgId();

private:
	short _msg_id;
};

class SendNode : public MsgNode {
public:
	SendNode(const char* msg, short max_len, short msg_id);
	short GetMsgId();

private:
	short _msg_id;
};