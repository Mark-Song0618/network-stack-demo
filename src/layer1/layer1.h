#pragma once

class Layer1 {
public:
	Layer1();

	unsigned		getPort() const { return _udpPort; }

	int				getSockFd() const { return _sockfd; }

private:
	static unsigned _currPort;	// current available udp port number
	unsigned		_udpPort;
	int				_sockfd;
};