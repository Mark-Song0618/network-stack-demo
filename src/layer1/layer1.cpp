#include "layer1.h"
#include <winsock2.h>
#include <iostream>
#include "../utils/utils.h"

unsigned Layer1::_currPort = 40000;

Layer1::Layer1()
	:_udpPort(_currPort++)
{
	_sockfd = socket(AF_INET,	/*IPV4 Э����*/
		SOCK_DGRAM,/*UDP*/
		0	/*Ĭ��Э��*/);
	if (_sockfd < 0) {
		log(1, "create udp socket failed");
		return;
	}

	struct sockaddr_in addr = {};
	addr.sin_family = AF_INET;					/*IPV4Э���壬 ��socket����һ��*/
	addr.sin_port = htons(_udpPort);			/*�˿ں�*/
	addr.sin_addr.s_addr = htonl(INADDR_ANY);	/*�������е�ַ*/

	if (bind(_sockfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
		// report error;
		return;
	}
}