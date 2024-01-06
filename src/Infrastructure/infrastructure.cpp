#include "infrastructure.h"
#include <winsock2.h>
#include <iostream>
#include <cmath>
#include <sstream>


Nic::Nic(SPtr<Node> owner, NicID id)
	: _owner(owner), _id(id), _etherId(0), _nwProps({})
{
	_layer1 = std::make_unique<Layer1>();

	long long mac = ((owner->getNodeId() << 16) | id);
	for (int i = 0; i < 6; ++i) {
		_nwProps._macAddr.mac[i] = mac & 0xff;
		mac >>= 8;
	}
}

void
Nic::intf_receive_kt(char* pkt, unsigned pktSize)
{
	if (!_owner.lock()) {
		// error out
		return;
	}
	pkt[pktSize] = '\0';
	std::stringstream logStr;
	logStr  << "Node: " << _owner.lock()->getNodeId()
			<< " Nic: " << _id
			<< " received:" << pktSize << " bytes"
			<< std::endl;
	log(1, logStr.str().c_str());

	_owner.lock()->receive_pkt_from_intf(this, pkt, pktSize);
}

void
Nic::sendPkt(char* pkt, unsigned pktSize)
{
	if (_owner.lock()) {
		_owner.lock()->getEtherNet(_etherId)->sendPkt(this, pkt, pktSize);
	}
}

bool
Nic::is_vlan_accessable(unsigned vid) const
{
	for (int i = 0; i < MAX_VLAN_FIELDS; ++i)
	{
		if (_nwProps._vlans[i] == vid) {
			return true;
		}
	}
	return false;
}

bool 
Nic::is_same_subnet(const IP& ip) const
{
	char nic_subnet[4] = {0}, ip_subnet[4] = {0};
	for (int i = 0; i < 4; ++i) {
		nic_subnet[i] = getIp().ip[i] & _nwProps._mask[i];
		ip_subnet[i]  = ip.ip[i] & _nwProps._mask[i];
	}
	return !strncmp(nic_subnet, ip_subnet, 4);
}

void
Nic::config_ip(unsigned ip, unsigned mask)
{
	_nwProps._is_ipAddr_config = true;
	for (int i = 0; i < 4; ++i) {
		_nwProps._ipAddr.ip[i] = (ip >> (i * 8)) & 0xff;
		_nwProps._mask[i] = (mask >> (i * 8)) & 0xff;
	}
}

NodeID Node::_currId = 0;

Node::Node(Topo* owner) : _topo(owner), _id(_currId++) 
{
	_layer2 = std::make_unique<Layer2>(this);
	_layer3 = std::make_unique<Layer3>(this);
	_layer4 = std::make_unique<Layer4>();
	_layer5 = std::make_unique<Layer5>(this);
}

SPtr<Nic>
Node::addInterface()
{
	SPtr<Node> sp = _topo->getNode(_id);
	_interfaces.emplace_back(std::make_shared<Nic>(sp, _interfaces.size()));
	return _interfaces.back();
}

SPtr<EtherNet>
Node::getEtherNet(EtherID id)
{
	return _topo->getEtherNet(id);
}

void
Node::connect_to_ethernet(EtherID id, SPtr<Nic> nic, bool config_ip)
{
	SPtr<EtherNet> net = getEtherNet(id);
	nic->setEtherId(id);
	net->register_intf(nic);
	if (config_ip) {
		unsigned ip, mask;
		net->allocate_ip(ip, mask);
		nic->config_ip(ip, mask);
	}
}

void
Node::receive_pkt_from_intf(Nic* receiver, char* pkt, unsigned pktSize)
{
	if (_layer2->IsAcceptablePacket(receiver, pkt)) {
		// todo: print log
		if (receiver->getWorkingMode() == ACCESS ||
			receiver->getWorkingMode() == TRUNK) {
			// handover to l2 switch
			_layer2->switch_receive_pkt(receiver, pkt, pktSize);
		}
		else {
			_layer2->promote_to_layer2(receiver, pkt, pktSize);
		}
	}
	else {
		// drop
		// todo: print log
	}
}

EtherID EtherNet::_currId = 0;

EtherNet::EtherNet() : _id(_currId++) {}

void
EtherNet::sendPkt(Nic* sender, char* pkt, unsigned pktSize)
{
	// send pkt to all interfaces, except sender
	for (auto& nic : _nics) {
		if (nic.get() != sender) {
			struct sockaddr_in dest = {};
			dest.sin_family = AF_INET;
			dest.sin_port = htons(nic->getPort());
			dest.sin_addr.s_addr = inet_addr("127.0.0.1");
			int send_sock = socket(AF_INET, SOCK_DGRAM, 0);
			int bytesSent = sendto(send_sock, pkt, pktSize, 0, (struct sockaddr*)&dest, sizeof(dest));
			if (bytesSent < 0) {
				std::stringstream logStr;
				logStr << "sendto failed with error: " << WSAGetLastError() << std::endl;
				log(1, logStr.str().c_str());
			}
			closesocket(send_sock);
		}
	}
}

void
EtherNet::allocate_ip(unsigned& ip, unsigned& mask) {
	ip = 0, mask = 0;
	unsigned nicId = _nics.size();
	unsigned maskLen = (unsigned)std::floor(std::log2(_id)) + 1;
	ip |= (_id << (32 - maskLen));	// set net id
	ip |= nicId;	// set host id
	mask = 0xFFFFFFFF << (32 - maskLen);
}

unsigned
EtherNet::get_subnet_ip() {
	unsigned ip = 0;
	unsigned maskLen = (unsigned)std::floor(std::log2(_id)) + 1;
	ip |= (_id << (32 - maskLen));	// set net id
	return ip;
}

unsigned
EtherNet::get_mask()
{
	unsigned mask = 0;
	unsigned maskLen = (unsigned)std::floor(std::log2(_id)) + 1;
	mask = 0xFFFFFFFF << (32 - maskLen);
	return mask;
}

Topo::Topo()
	: _running(false)
{
	std::once_flag flag;
	std::call_once(flag, []() {
		WSADATA wsaData;
		int rt = WSAStartup(MAKEWORD(2, 2), &wsaData);
		});
}

EtherID
Topo::createEtherNet()
{
	auto net = std::make_shared<EtherNet>();
	EtherID id = net->getNetId();
	_etherNets.insert({ id, net });
	return id;
}

NodeID
Topo::createNode()
{
	auto node = std::make_shared<Node>(this);
	NodeID id = node->getNodeId();
	_nodes.insert({ id, node });
	return id;
}

void
Topo::start_pkt_receive_thread()
{
	_running = true;
	_pkt_receive_thread = std::thread(&Topo::_pkt_receive_func, this);
}

void
Topo::_pkt_receive_func()
{
	// 1. collect udp fds
	fd_set udp_fds = { 0 }, active_fds = { 0 };
	int max_fd = 0;

	for (auto& nPair : _nodes) {
		std::list<SPtr<Nic>>& nics = nPair.second->getInterfaces();
		for (auto& nic : nics) {
			int fd = nic->getSockfd();
			if (fd > 0) {
				FD_SET(fd, &udp_fds);
				if (fd > max_fd) {
					max_fd = fd;
				}
			}
		}
	}

	// 2. receive udp packets to interfaces
	char recvBuffer[MAX_PACKET_SIZE] = {};
	struct sockaddr fromAddr = {0};
	int addrLen = sizeof(sockaddr);
	struct timeval timeout = {0};
	timeout.tv_sec = 1;
	timeout.tv_usec = 0;
	while (_running) {
		memcpy(&active_fds, &udp_fds, sizeof(udp_fds));
		int activeCnt = select(max_fd + 1, &active_fds, NULL, NULL, &timeout);
		if (activeCnt <= 0) {
			continue;
		}
		for (auto& nPair : _nodes) {
			for (auto& nic : nPair.second->getInterfaces()) {
				if (FD_ISSET(nic->getSockfd(), &active_fds)) {
					memset(recvBuffer, '\0', MAX_PACKET_SIZE);
					int recvLen = recvfrom(nic->getSockfd(), recvBuffer, MAX_PACKET_SIZE, 0, (sockaddr*)(&fromAddr), &addrLen);
					if (recvLen > 0) {
						nic->intf_receive_kt(recvBuffer, recvLen);
					}
					else if (recvLen < 0) {
						std::stringstream logStr;
						logStr << "recvfrom failed with error: " << WSAGetLastError() << std::endl;
						log(1, logStr.str().c_str());
					}
				}
			}
		}
	}
}

void
Topo::stop_pkt_receive_thread()
{
	_running = false;
	if (_pkt_receive_thread.joinable()) {
		_pkt_receive_thread.join();
	}
}