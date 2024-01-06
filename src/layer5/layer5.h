#pragma once
#include "../utils/utils.h"

class Node;

class Layer5{
public:
	Layer5(Node* owner) : _node(owner) {}

	void	sendMsg(const char* msg, unsigned int size, unsigned char* dest_ip);

	void	receiveMsg(const char* msg, unsigned int size, unsigned int src_ip);

private:
	Node* _node;
};