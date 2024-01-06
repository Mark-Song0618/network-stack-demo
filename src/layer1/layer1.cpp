#include "layer1.h"
#include <winsock2.h>
#include <iostream>
#include "../utils/utils.h"

unsigned Layer1::_currPort = 40000;

Layer1::Layer1()
	:_udpPort(_currPort++)
{
	_sockfd = socket(AF_INET,	/*IPV4 协议族*/
		SOCK_DGRAM,/*UDP*/
		0	/*默认协议*/);
	if (_sockfd < 0) {
		log(1, "create udp socket failed");
		return;
	}

	struct sockaddr_in addr = {};
	addr.sin_family = AF_INET;					/*IPV4协议族， 和socket保持一致*/
	addr.sin_port = htons(_udpPort);			/*端口号*/
	addr.sin_addr.s_addr = htonl(INADDR_ANY);	/*监听所有地址*/

	if (bind(_sockfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
		// report error;
		return;
	}
}