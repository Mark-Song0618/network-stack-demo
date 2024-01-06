#include "layer5.h"
#include "../Infrastructure/infrastructure.h"
#include <iostream>
#include <string>
#include <sstream>

void
Layer5::sendMsg(const char* msg, unsigned int size, unsigned char* dest_ip)
{
	IP dest;
	for (int i = 0; i < 4; ++i) {
		dest.ip[i] = dest_ip[i];
	}
	char *pkt = (char*)calloc(1, size);
	memcpy(pkt, msg, size);

	std::stringstream logStr;
	logStr << "Node " << _node->getNodeId() << " send msg: " << "\n\t"
			  <<  msg      << "\n"
			  << "to : " <<"\n\t"
			  << dest << std::endl;
	log(5, logStr.str().c_str());

	_node->demote_to_layer3(dest, pkt, size, 0);
}

void
Layer5::receiveMsg(const char* msg, unsigned int size,  unsigned int src_ip)
{
	std::stringstream logStr;
	logStr << "Node :" << _node->getNodeId() << " received msg: " << "\n\t"
		   <<  msg      << "\n"
	       << "from : " <<"\n\t"
		   << std::to_string(src_ip >> 24 & 0xff) << "."
		   << std::to_string(src_ip >> 16 & 0xff) << "."
		   << std::to_string(src_ip >> 8 & 0xff) << "."
		   << std::to_string(src_ip >> 0 & 0xff) << std::endl;
	log(5, logStr.str().c_str());
}