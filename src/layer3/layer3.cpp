#include "layer3.h"
#include "../Infrastructure/infrastructure.h"
#include "../utils/utils.h"
#include <iostream>
#include <sstream>

Route_t::Route_t( const char* mask, IP gw_ip, Nic* gw_intf, bool is_direct)
	:_gw_ip(gw_ip),
	_gw_intf(gw_intf),
	_is_direct(is_direct)
{
	for (int i = 0; i < 4; ++i) {
		_mask[i] = mask[i];
	}
}

Route_t::Route_t()
	:_gw_intf(nullptr), _is_direct(false)
{
	for (int i = 0; i < 4; ++i) {
		_mask[i] = 0x00;
		_gw_ip.ip[i] = 0x00;
	}
}

void	
Layer3::promote_to_layer3(Nic* receiver, char* pkt, unsigned pktSize, unsigned protocal)
{
	switch (protocal) {
	case ETH_IP:
	case IP_IN_IP:
		_layer3_recvPkt(receiver, pkt, pktSize);
	default:
		break;
	}
}

void
Layer3::demote_to_layer3(IP dest, char* pkt, unsigned pktSize, char protocal)
{
	// 1. prepare ip pkt
	// src_ip, dest_ip, ttl, protocol, total_length are required
	IP_hdr ipHdr = {};
	for (int i = 0; i < 4; ++i) {
		ipHdr.dst_ip|= dest.ip[i] << (i * 8);
	}
	ipHdr.ttl = 10;
	ipHdr.protocol = protocal;
	ipHdr.ihl = sizeof(IP_hdr) / 4;
	ipHdr.total_length = (short)ipHdr.ihl + (short)(pktSize / 4) + (short)(!!(pktSize % 4));

	// 2. lookup route table for dest ip
	Route route;
	if (!get_longest_prefix_match_route(dest, route)) {
		// no route to dest_ip, drop it
		return;
	}
	char local_ip[4] = LOCAL_HOST_IP;
	for (int i = 0; i < 4; ++i) {
		if (route._is_direct) {
			ipHdr.src_ip |= local_ip[i] << (i * 8);
		}
	}

	char* ipPkt = (char*)calloc(1, ipHdr.total_length * 4);
	if (!ipPkt) {
		// bad alloc
		return;
	}
	memcpy(ipPkt, &ipHdr, ipHdr.ihl * 4);
	memcpy(ipPkt + ipHdr.ihl * 4, pkt, pktSize);

	// 3. demote to layer2 for forwarding
	if (route._is_direct) {
		std::stringstream logStr;
		logStr << "demote to layer2 for direct forwarding" << std::endl;
		logStr << "\tFrom Node: " << _node->getNodeId() << " to: " << dest << std::endl;	
		log(3, logStr.str().c_str());
		_node->demote_to_layer2(dest, nullptr, ipPkt, ipHdr.total_length * 4, ETH_IP);
	}
	else {
		std::stringstream logStr;
		logStr << "demote to layer2 for gateway forwarding" << std::endl;
		logStr << "\tFrom Node: " << _node->getNodeId() << " through "
			   << route._gw_ip << " to: "
			   << dest << std::endl;
		log(3, logStr.str().c_str());

		_node->demote_to_layer2(route._gw_ip, route._gw_intf, ipPkt, ipHdr.total_length * 4, ETH_IP);
	}

	free(ipPkt);
}

bool
Layer3::get_longest_prefix_match_route(IP dest_ip, Route& rt) const
{
	unsigned matched_size = 0;
	for (auto route : _routeTable) {
		unsigned dest_subnet = 0;
		unsigned route_subnet = 0;
		unsigned mask_size = 0;
		for (int i = 0; i < 4; ++i) {
			dest_subnet  |= (dest_ip.ip[i] & route.second._mask[i]) << (i * 8);
			route_subnet |= (route.first.ip[i] & route.second._mask[i]) << (i * 8);
			mask_size += route.second._mask[i] << (i*8);
		}
		if (dest_subnet == route_subnet && mask_size > matched_size) {
			matched_size = mask_size;
			rt = route.second;
		}
	}

	return matched_size != 0;
}

void    
Layer3::_layer3_recvPkt(Nic* receiver, char* pkt, unsigned pktSize)
{
	EtherHdr* eHdr = (EtherHdr*)pkt;
	IP_hdr* ipHdr = (IP_hdr*)(eHdr->payload);
	IP dest_ip = { ipHdr->dst_ip };
	Route route;
	if (get_longest_prefix_match_route(dest_ip, route)) {
		// no route to dest_ip, drop it
		return;
	}
	// case1:destinated to self, accept it and promote to layer4/layer5
	if (_is_self_ping(dest_ip)) {
		switch (ipHdr->protocol) {
		case MTCP:
			break;
		case USERAPP1:
			break;
		case ICMP_PRO:
			break;
		case IP_IN_IP:
			break;
		default:
			_node->receiveMsg(eHdr->payload + ipHdr->ihl * 4, (ipHdr->total_length - ipHdr->ihl) * 4, (unsigned)(ipHdr->src_ip));
			break;
		}
		return;
	}
	// case2:destinated to subnet, forward it
	if (route._is_direct) {
		_node->demote_to_layer2(dest_ip, nullptr, pkt, pktSize, ETH_IP);
		return;
	}
	// case3:destinated to other subnet, forward it to gateway by looking up  route table
	if (!route._is_direct) {
		--ipHdr->ttl;
		_node->demote_to_layer2(route._gw_ip, route._gw_intf, pkt, pktSize, ETH_IP);
	}
}

bool  
Layer3::_is_self_ping(IP dest_ip)
{
	for (auto& intf : _node->getInterfaces()) {
		if (intf->is_same_subnet(dest_ip)) {
			return true;
		}
	}
	char local_ip[4] = LOCAL_HOST_IP;
	return !strncmp((char*)dest_ip.ip, local_ip, 4);
}

void   
Layer3::add_route(IP dest_subnet, const char* mask, IP gw_ip, Nic* gw_intf, bool is_direct) 
{
	_routeTable[dest_subnet] = { mask, gw_ip, gw_intf, is_direct };
}