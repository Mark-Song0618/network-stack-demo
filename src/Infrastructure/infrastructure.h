/*
	Layer1 is the physical layer.
	It is responsible for the physical connection between nodes.
	This program is focus on the layers beyond the physical layer.So I will use
	the UDP to simulate the physical layer.
*/


#pragma once

#include <list>
#include <set>
#include <map>
#include <memory>
#include <thread>
#include <mutex>

#include "../utils/utils.h"
#include "../layer1/layer1.h"
#include "../layer2/layer2.h"
#include "../layer3/layer3.h"
#include "../layer4/layer4.h"
#include "../layer5/layer5.h"

class Nic;
class Node;
class EtherNet;
class Topo;

typedef unsigned EtherID;
typedef unsigned NodeID;
typedef unsigned NicID;

template<typename T> using SPtr = std::shared_ptr<T>;
template<typename T> using UPtr = std::unique_ptr<T>;
template<typename T> using WPtr = std::weak_ptr<T>;

typedef struct NW_props_t{
	// L2
	Mac			_macAddr;
	L2MODE		_mode;
	unsigned	_vlans[MAX_VLAN_FIELDS];
	bool		_is_ipAddr_config_backup;

	// L3
	bool		  _is_ipAddr_config;
	IP			  _ipAddr;
	unsigned char _mask[4];
} NW_props;

class Nic {
public:
	Nic(SPtr<Node>, NicID);
	
	void		print() const;

	void		config_ip(unsigned ip, unsigned mask);

	void		setEtherId(EtherID id) { _etherId = id; }

	void		intf_receive_kt(char* pkt, unsigned pktSize);

	void		sendPkt(char* pkt, unsigned pktSize);	// send pkt to EtherNet which it is connected to

	L2MODE		getWorkingMode() const { return _nwProps._mode; }

	unsigned	getVlanId() const { return _nwProps._vlans[0]; }

	bool		is_vlan_accessable(unsigned vid) const;

	bool		is_IP_configured() const { return _nwProps._is_ipAddr_config; }

	bool		is_same_subnet(const IP& ip) const;

	const IP&	getIp() const { return _nwProps._ipAddr; }

	unsigned char const*	getMask()const { return _nwProps._mask; }

	const Mac&	getMac() const { return _nwProps._macAddr; }


	// layer1 simulation query api
	int			getSockfd() { return _layer1->getSockFd(); }

	unsigned	getPort() { return _layer1->getPort(); }

	WPtr<Node>	getOwner() { return _owner; }

	NicID		getNicId() { return _id; }

private:
	WPtr<Node>			_owner;

	EtherID				_etherId;
	
	NicID 				_id;
	
	UPtr<Layer1>		_layer1;
	
	NW_props			_nwProps;
};

class Node {
public:
	Node(Topo* owner);

	void					print() const;

	NodeID					getNodeId() const { return _id; }

	SPtr<EtherNet>			getEtherNet(EtherID id);

	SPtr<Nic>				addInterface();

	std::list<SPtr<Nic>>	getInterfaces() { return _interfaces; }

	void					connect_to_ethernet(EtherID, SPtr<Nic>, bool cfgIp = false);

	void					receive_pkt_from_intf(Nic*, char*, unsigned);

	void					promote_to_layer2(Nic* receiver, char* pkt, unsigned size) { _layer2->promote_to_layer2(receiver, pkt, size); }

	void					demote_to_layer2(IP nexthoop, Nic* out, char* pkt, unsigned size, short protocal)	{ _layer2->demote_to_layer2(nexthoop, out, pkt, size, protocal); }
	
	void					promote_to_layer3(Nic* receiver, char* pkt, unsigned pktSize, unsigned type) { _layer3->promote_to_layer3(receiver, pkt, pktSize, type); }

	void					demote_to_layer3(IP dest, char* pkt, unsigned pktSize, char protocal)	{ _layer3->demote_to_layer3(dest, pkt, pktSize, protocal); }

	void					sendMsg(const char* msg, unsigned int size, unsigned char* dest_ip) { _layer5->sendMsg(msg, size, dest_ip); }

	void					receiveMsg(const char* msg, unsigned int size,  unsigned int src_ip) { _layer5->receiveMsg(msg, size, src_ip); }

	void					add_route(IP dest_subnet, const char* mask, IP gw_ip, Nic* gw_intf, bool is_direct) { _layer3->add_route(dest_subnet, mask, gw_ip, gw_intf, is_direct); }

private:
	UPtr<Layer2>			_layer2;

	UPtr<Layer3>			_layer3;

	UPtr<Layer4>			_layer4;

	UPtr<Layer5>			_layer5;

	static NodeID			_currId;

	Topo*					_topo;

	NodeID					_id;

	std::list<SPtr<Nic>>	_interfaces;
};

// An ethernet is a network.
// The center of the network is a bus or hub, which is connected with many nic.
// The nics are identified by their mac.
// The messages are broadcast on the bus, and each nic would pick their own message with the mac header. 
class EtherNet {
	friend class Topo;
public:
	void			print() const;

	void			register_intf(SPtr<Nic> nic) { _nics.push_back(nic); }

	EtherID			getNetId() const { return _id; }

	void			sendPkt(Nic* sender, char* pkt, unsigned pktSize);

	void			allocate_ip(unsigned& ip, unsigned& mask);

	unsigned		get_subnet_ip();

	unsigned		get_mask();

	EtherNet();

private:
	static EtherID			_currId;
	EtherID					_id;
	std::list<SPtr<Nic>>	_nics;
};


class Topo {
public:
	Topo();

	void			print() const;

	EtherID			createEtherNet();

	NodeID			createNode();

	SPtr<EtherNet>	getEtherNet(EtherID id) { return { _etherNets[id] }; }

	SPtr<Node>		getNode(NodeID id) { return { _nodes[id] }; }

	void			start_pkt_receive_thread();

	void			stop_pkt_receive_thread();

private:
	void			_pkt_receive_func();

private:
	std::map<EtherID, SPtr<EtherNet>>	_etherNets;
	std::map<NodeID, SPtr<Node>>		_nodes;
	bool								_running;
	std::thread  						_pkt_receive_thread;
};