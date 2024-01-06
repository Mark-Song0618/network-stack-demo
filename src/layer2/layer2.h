#pragma once

#include "../utils/utils.h"
#include <list>
#include <map>
#include <iostream>
#include <string>

class Nic;
class Node;

#pragma pack(push,1)
typedef struct IP_t
{
	IP_t(char ip_[4])
	{
		for (int i = 0; i < 4; ++i) {
			ip[i] = (unsigned char)ip_[i];
		}
	};

	IP_t(unsigned char ip_[4]) 
	{
		for (int i = 0; i < 4; ++i) {
			ip[i] = ip_[i];
		}
	};

	IP_t(unsigned int ip_)
	{
		for (int i = 0; i < 4; ++i) {
			ip[i] = (ip_ >> (i * 8)) & 0xff;
		}
	};

	IP_t() 
	{
		for (int i = 0; i < 4; ++i) {
			ip[i] = 0x00;
		}
	};

	bool operator==(const IP_t& ip1)
	{
		for (int i = 0; i < 4; ++i) {
			if (ip[i] != ip1.ip[i]) {
				return false;
			}
		}
		return true;
	}
	bool operator!=(const IP_t& ip1)
	{
		for (int i = 0; i < 4; ++i) {
			if (ip[i] != ip1.ip[i]) {
				return true;
			}
		}
		return false;
	}

	unsigned char ip[4];

} IP;

typedef struct Mac_t
{
	unsigned char mac[6];
} Mac;

std::ostream& operator<<(std::ostream& os, const IP ip);
std::ostream& operator<<(std::ostream& os, const Mac mac);

typedef struct ArpHdr_t
{
	short hw_type;			// 1 : ARPHRD_ETHER
	short proto_type;		// 0x0800 : ipv4
	char  hw_addr_len;		// 6(B) : mac
	char  proto_addr_len;	// 4(B) : ip
	short opcode;			// 1 : ARP_BROAD_REQ, 2 : ARP_REPLY
	Mac	  sender_mac;		// sender mac
	IP	  sender_ip;		// sender ip
	Mac   dest_mac;			// target mac; 0 for Arp request
	IP	  dest_ip;			// target ip
} ArpHdr;

// the max length of ether frame is 1518 bytes
// 12 bytes for src and dest mac
// 2 bytes for type
// 4 bytes for FCS
// the max payload length is 1500 
// but with Vlan tag, the max length of ether frame is 1522 bytes
typedef struct EtherHdr_t{
	Mac dest_mac;			// destination mac
	Mac src_mac;			// source mac
	short type;				// 0x0806 : ARP
	char payload[1500];		// arp packet
	unsigned int fcs;		// frame check sequence
} EtherHdr;

typedef struct vlan_8021q_header_t {
	uint16_t tpid;			// 0x8100
	short	 tci_pcp:3;		// priority code point
	short	 tci_dei:1;		// drop eligible indicator
	short	 tci_vid:12;	// vlan identifier
}vlan_8021q_header;

typedef struct Vlan_EtherHdr_t {
	Mac dest_mac;			// destination mac
	Mac src_mac;			// source mac
	vlan_8021q_header vlan;	// vlan tag
	short type;				// 0x0806 : ARP
	char payload[1500];		// arp packet
	unsigned int fcs;		// frame check sequence
} Vlan_EtherHdr;

#pragma pack(pop)

typedef struct Arp_pendint_t
{
	Nic* sender;
	char* packet;
	unsigned int size;
} Arp_pending;

enum L2MODE
{
	UNKNOWN,
	ACCESS,
	TRUNK,
};

class Layer2
{
public:
	Layer2(Node* owner) : _node(owner) {}

	bool		IsAcceptablePacket(Nic* receiver, const char* pkt);

	void		promote_to_layer2(Nic* receiver, char* pkt, unsigned size);

	void		switch_receive_pkt(Nic*, char*, unsigned);

	void		demote_to_layer2(IP nexthoop, Nic* out, char* pkt, unsigned size, short protocal);

	bool		isBroadcastPkt(const Mac& mac);

	void		process_arp_request(Nic* receiver, char* pkt);

	void		process_arp_reply(Nic* receiver, char* pkt);

	void		send_arp_request(Nic* sender, IP dest);

	template <typename T>
	bool		is_matched(const T&, const T&) { return false; }

	void		add_mac_entry(Mac mac, Nic* intf) { _macTable[mac] = intf; }

private:
	unsigned	_get_pkt_vlanId(const char* pkt);

	unsigned	_get_etherHdr_size(unsigned payloadSize);

	void		_set_ether_FCS(const char*pkt, unsigned size);

	char*		_get_ether_payload(char* pkt);

	bool		_is_self_ping(IP nexthoop);

	void		_switch_forward_pkt(Nic* out, char* pkt, unsigned size);

	void		_layer2_sendpkt(IP nexthoop, Nic* out, char* pkt, unsigned size);

	Nic*		_get_intf_to_subnet(IP);

	Vlan_EtherHdr* _tag_pkt_with_vid(char* pkt, unsigned vid);

private:
	typedef struct IP_CMP {
		bool operator() (IP ip1, IP ip2) const {
			return ip1.ip > ip2.ip;
		}
	};

	typedef struct MAC_CMP {
		bool operator() (Mac mac1, Mac mac2) const {
			return mac1.mac > mac2.mac;
		}
	};

	std::map<IP, Mac, IP_CMP> _arpTable;

	std::map<IP, std::list<Arp_pending>, IP_CMP> _arpPendingTable;

	std::map<Mac, Nic*, MAC_CMP>	_macTable;

	Node*				_node;	// back pointer to node
};

template <>
bool
Layer2::is_matched<IP>(const IP& ip1, const IP& ip2);

template <>
bool
Layer2::is_matched<Mac>(const Mac& mac1, const Mac& mac2);
