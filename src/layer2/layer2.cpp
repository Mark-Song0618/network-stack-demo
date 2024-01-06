#include "layer2.h"
#include "../Infrastructure/infrastructure.h"
#include <cassert>
#include <iostream>
#include <sstream>

bool
Layer2::IsAcceptablePacket(Nic* receiver, const char* pkt)
{
	unsigned pkt_vlan_id = _get_pkt_vlanId(pkt);
	L2MODE intfMode = receiver->getWorkingMode();
	if (intfMode == ACCESS) {
		/*
			If interface working on access mode:
				If frame is not tagged with vlanId, The frame would be accept and tagged with vlan Id.
				If frame is already tagged with the same vlanId of interface, it will be accept too.
				Otherwise, It will be dropped
		*/
		unsigned intf_vlan_id = receiver->getVlanId();
		if (!intf_vlan_id) {
			return false;	// drop
		}
		if (!pkt_vlan_id) {
			return true;
		}
		if (pkt_vlan_id && pkt_vlan_id == intf_vlan_id) {
			return true;
		}
	}

	if (intfMode == TRUNK) {
		/*
			If interface working on trunk mode:
				Only accept pkt if it is tagged with one of the vlan id of interface.
		*/
		if (pkt_vlan_id) {
			return receiver->is_vlan_accessable(pkt_vlan_id);
		}
	}

	if (intfMode == UNKNOWN && receiver->is_IP_configured()) {
		// Working on L3 mode, not Vlan Mode
		if (pkt_vlan_id) {
			return false;
		}
		Mac destMac = ((EtherHdr*)(pkt))->dest_mac;
		if (is_matched<Mac>(receiver->getMac(), destMac) || isBroadcastPkt(destMac)) {
			return true;
		}
	}

	return false;
}

bool
Layer2::isBroadcastPkt(const Mac& mac)
{
	char mac_bc[6] = BROADCASTADDR;
	return !strncmp((const char*)mac.mac, mac_bc, 6);
}

void
Layer2::process_arp_request(Nic* receiver, char* pkt)
{
	// 1.prepare arp reply msg
	ArpHdr* request = (ArpHdr*)(_get_ether_payload(pkt));
	if (request->dest_ip != receiver->getIp()) {
		return;
	}
	
	std::stringstream logStr;
	logStr << "Node: " << _node->getNodeId() << " process arp request of "
		   << request->dest_ip << " with Mac: "
		   << receiver->getMac() << std::endl;
	log(2, logStr.str().c_str());

	EtherHdr* ehdr = (EtherHdr*)calloc(1, sizeof(EtherHdr));
	ehdr->dest_mac = request->sender_mac;
	ehdr->src_mac = receiver->getMac();
	ehdr->type = ARP_MSG;
	ArpHdr* arp_hdr = (ArpHdr*)(_get_ether_payload((char*)ehdr));
	arp_hdr->hw_type= 1;
	arp_hdr->proto_type = 0x0800;
	arp_hdr->hw_addr_len = 6;
	arp_hdr->proto_addr_len = 4;
	arp_hdr->opcode = ARP_REPLY;
	arp_hdr->dest_ip = request->sender_ip;
	arp_hdr->dest_mac = request->sender_mac;
	arp_hdr->sender_ip = receiver->getIp();
	arp_hdr->sender_mac = receiver->getMac();

	// 2.send back to sender by receiver
	receiver->sendPkt((char*)ehdr, sizeof(EtherHdr));
	free(ehdr);
}

void
Layer2::process_arp_reply(Nic* receiver, char* pkt)
{
	// 1.update arp table
	ArpHdr* reply = (ArpHdr*)(_get_ether_payload(pkt));
	IP dest_ip = reply->sender_ip;
	Mac dest_mac = reply->sender_mac;
	_arpTable[dest_ip] = dest_mac;

	// 2.send packets of resolved ip
	for (Arp_pending& arp_pending : _arpPendingTable[dest_ip]) {
		std::stringstream logStr;
		logStr << "Node: " <<_node->getNodeId()
			<< " process arp reply£¬ send pending packet of " 
			<< dest_ip << std::endl;
		log(2, logStr.str().c_str());
		// 1. update dest mac of ether hdr
		EtherHdr* ehdr = (EtherHdr*)(arp_pending.packet);
		ehdr->dest_mac = dest_mac;
		// 2. send packet
		arp_pending.sender->sendPkt(arp_pending.packet, arp_pending.size);
		free(arp_pending.packet);
	}
	_arpPendingTable.erase(dest_ip);
}

unsigned
Layer2::_get_pkt_vlanId(const char* pkt)
{
	if (pkt == nullptr) {
		return 0;
	}
	vlan_8021q_header* vlan_hdr = (vlan_8021q_header*)(pkt + 2 * sizeof(Mac));
	if (vlan_hdr->tpid == 0x8100) {
		return vlan_hdr->tci_vid;
	}
	return 0;
}

unsigned
Layer2::_get_etherHdr_size(unsigned payloadSize)
{
	return sizeof(EtherHdr) + payloadSize - 1500;
}

void
Layer2::_set_ether_FCS(const char* pkt, unsigned fcs)
{
	unsigned vlanId = _get_pkt_vlanId(pkt);
	if (vlanId) {
		((Vlan_EtherHdr_t*)pkt)->fcs = fcs;
	}
	else {
		((EtherHdr*)pkt)->fcs = fcs;
	}
}

char*
Layer2::_get_ether_payload(char* pkt)
{
	return pkt + sizeof(Mac) * 2 + sizeof(short);
}

template <>
bool
Layer2::is_matched<IP>(const IP& ip1, const IP& ip2)
{
	return !strncmp((char*)ip1.ip, (char*)ip2.ip, 4);
}

template <>
bool
Layer2::is_matched<Mac>(const Mac& mac1, const Mac& mac2)
{
	return !strncmp((char*)mac1.mac, (char*)mac1.mac, 6);
}

void
Layer2::promote_to_layer2(Nic* receiver, char* pkt, unsigned size)
{
	if (receiver == nullptr || pkt == nullptr) {
		return;
	}
	EtherHdr *hdr = (EtherHdr*)pkt;
	// mac learning
	Mac sender = ((EtherHdr*)(pkt))->src_mac;
	_macTable.insert({ sender, receiver });
	// process pkt
	ArpHdr* arp_hdr = 0;
	switch (hdr->type) {
	case ARP_MSG:
		arp_hdr = (ArpHdr*)(_get_ether_payload(pkt));
		if (arp_hdr->opcode == ARP_BROAD_REQ) {
			process_arp_request(receiver, pkt);
		}
		else if (arp_hdr->opcode == ARP_REPLY) {
			process_arp_reply(receiver, pkt);
		}
		break;
	case ETH_IP:
		_node->promote_to_layer3(receiver, pkt, size, ETH_IP);
		break;
	default:
		break;
	}
}

void
Layer2::switch_receive_pkt(Nic* receiver, char* pkt, unsigned pktSize)
{
	// 1. mac learning
	Mac sender = ((EtherHdr*)(pkt))->src_mac;
	_macTable.insert({sender, receiver});

	// 2. pkt forwarding
	// 2.1 if pkt is broadcast pkt, send to all interface except the port which receive the pkt
	Mac destMac = ((EtherHdr*)(pkt))->dest_mac;
	if (!isBroadcastPkt(destMac)) {
		Nic* out = _macTable[destMac];
		if (out) {
			_switch_forward_pkt(out, pkt, pktSize);
			return;
		}
	} 

	// 2.2 if pkt is unicast pkt, or interface is not found,
	// send to all ports except the port which receive the pkt
	for (SPtr<Nic> intf : _node->getInterfaces()) {
		if (intf.get() != receiver) {
			_switch_forward_pkt(intf.get(), pkt, pktSize);
		}
	}
	return;
}

void
Layer2::_switch_forward_pkt(Nic* out, char* pkt, unsigned size)
{
	if (out == nullptr || pkt == nullptr) {
		return;
	}

	unsigned pkt_vid  = _get_pkt_vlanId(pkt);
	unsigned inft_vid = out->getVlanId();

	if (out->getWorkingMode() == ACCESS) {
		// 1. if interface is access mode:
		//		1.1 intf vid matches with pkt vid: send it
		//		1.2 inft vid and pkt vid are both not set : send it
		//		1.3 intf vid and pkt vid are both set but not match: drop it
		//		1.4 intf vid is set but pkt vid is not set: tag pkt with intf vid and send it
		if (inft_vid == pkt_vid) {
			out->sendPkt(pkt, size);
		}
		else if (inft_vid && !pkt_vid) {
			char* space = new char[size + sizeof(vlan_8021q_header)];
			memcpy(space + sizeof(vlan_8021q_header), pkt, size);
			Vlan_EtherHdr* tagged = _tag_pkt_with_vid((space + sizeof(vlan_8021q_header)), inft_vid);
			assert((char*)tagged == space);
			out->sendPkt((char*)tagged, sizeof(Vlan_EtherHdr));
			delete []tagged;
		}
	}
	else if (out->getWorkingMode() == TRUNK) {
		// 2. if interface is trunk mode, send pkt to interface only if vlan id of pkt matched one of interface's
		if (out->is_vlan_accessable(pkt_vid)) {
			out->sendPkt(pkt, size);
		}
	}
}

Vlan_EtherHdr*
Layer2::_tag_pkt_with_vid(char* pkt, unsigned vid)
{
	// caller should ensure the memory of pkt is enough to hold tagged pkt
	if (pkt == nullptr) {
		return nullptr;
	}
	Vlan_EtherHdr* tagged = (Vlan_EtherHdr*)(pkt - sizeof(vlan_8021q_header));
	memcpy(tagged, pkt, 2 * sizeof(Mac));
	vlan_8021q_header* vlan_hdr = (vlan_8021q_header*)(tagged + 2 * sizeof(Mac));
	vlan_hdr->tci_dei = 0;
	vlan_hdr->tci_pcp = 0;
	vlan_hdr->tci_vid = vid;
	vlan_hdr->tpid = VLAN_8021Q_PROTO;
	// other parts remain the same
}

void
Layer2::demote_to_layer2(IP nexthoop, Nic* out, char* pkt, unsigned size, short protocal)
{
	// 1. wrap with ether hdr
	EtherHdr* ehdr = (EtherHdr*)calloc(1, sizeof(EtherHdr));
	memcpy(ehdr->payload, pkt, size);	// todo: check size limit or split
	ehdr->type = protocal;

	// 2. forward pkt
	if (out) {
		// if out interface resolved, send pkt to next hoop through out interface
		_layer2_sendpkt(nexthoop, out, (char*)ehdr, sizeof(EtherHdr));
	}
	else if (_is_self_ping(nexthoop)) {
		// if it is self pin, bounce payload back to layer3
		_node->promote_to_layer3(out, pkt, size, protocal);
	}
	else {
		// forward pkt to subnet
		out = _get_intf_to_subnet(nexthoop);
		if (!out) {
			// error out
			free(ehdr);
			return;
		}
		_layer2_sendpkt(nexthoop, out, (char*)ehdr, sizeof(EtherHdr));
	}
	free(ehdr);
}

Nic* 
Layer2::_get_intf_to_subnet(IP ip)
{
	for (auto& intf : _node->getInterfaces()) {
		if (intf->is_same_subnet(ip)) {
			return intf.get();
		}
	}
	return nullptr;
}

void
Layer2::_layer2_sendpkt(IP nexthoop, Nic* out, char* pkt, unsigned size)
{
	std::stringstream logStr;
	logStr << "send packet to:" << nexthoop << std::endl;
		EtherHdr* ehdr = (EtherHdr*)pkt;
	ehdr->src_mac = out->getMac();
	if (_arpTable.find(nexthoop) != _arpTable.end()) {
		ehdr->dest_mac = _arpTable[nexthoop];
		logStr << "Mac resolved: " << ehdr->dest_mac << std::endl;
		out->sendPkt((char*)ehdr, sizeof(EtherHdr));
	}
	else {
		// arp pending
		char* space = (char*)calloc(1, size);
		memcpy(space, pkt, size);
		Arp_pending pending{ out, space, sizeof(EtherHdr) };
		_arpPendingTable[nexthoop].push_back(pending);
		logStr << "Mac not resolved, send arp request" << std::endl;
		send_arp_request(out, nexthoop);
	}
	log(2, logStr.str().c_str());
}

void	
Layer2::send_arp_request(Nic* sender, IP dest)
{
	size_t sz = sizeof(EtherHdr);
	EtherHdr* ehdr = (EtherHdr*)calloc(1, sz);
	ArpHdr* arp_hdr = (ArpHdr*)(ehdr->payload);
	ehdr->src_mac = sender->getMac();
	ehdr->type = ARP_MSG;

	arp_hdr->proto_type = 0x0800;
	arp_hdr->hw_addr_len = 6;
	arp_hdr->proto_addr_len = 4;
	arp_hdr->hw_type = 1;
	arp_hdr->opcode = ARP_BROAD_REQ;
	arp_hdr->dest_ip = dest;
	arp_hdr->sender_ip = sender->getIp();
	arp_hdr->sender_mac = sender->getMac();

	sender->sendPkt((char*)ehdr, sizeof(EtherHdr));
	free(ehdr);
}

bool
Layer2::_is_self_ping(IP nexthoop)
{
	for (auto& inft : _node->getInterfaces()) {
		if (nexthoop.ip == inft->getIp().ip) {
			return true;
		}
	}
	char local_ip[4] = LOCAL_HOST_IP;
	return !strncmp((char*)nexthoop.ip, local_ip, 4);
}

std::ostream& operator<<(std::ostream& os, const IP ip) {
	os << std::to_string(ip.ip[3]) << "."
		<< std::to_string(ip.ip[2]) << "."
		<< std::to_string(ip.ip[1]) << "."
		<< std::to_string(ip.ip[0]);
	return os;
}

std::ostream& operator<<(std::ostream& os, const Mac mac) {
	os << std::to_string(mac.mac[5]) << "."
		<< std::to_string(mac.mac[4]) << "."
		<< std::to_string(mac.mac[3]) << "."
		<< std::to_string(mac.mac[2]) << "."
		<< std::to_string(mac.mac[1]) << "."
		<< std::to_string(mac.mac[0]);
	return os;
}
