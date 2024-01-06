#pragma once

#include <map>
#include "../layer2/layer2.h"	// use l2 sap

#pragma pack (push, 1)
typedef struct IP_hdr_t {
    // these are meta info
    unsigned int version : 4;   // ipv4 or ipv6
    unsigned int ihl : 4;       // ip header length
    char tos;   	            // type of service, indicates how to process the packet
    short total_length;         // total length of the packet

    // thess are for fragmentation
    short identification;       // a single IP datagram would be diveded into several fragments, this id indicates which datagram it belongs to.
    unsigned int unused_flag : 1;
    unsigned int DF_flag : 1;
    unsigned int MORE_flag : 1;
    unsigned int frag_offset : 13;  // sequence of the fragment

    // belows are for routing control
    char ttl;           // time to live, prevent the packet from looping
    char protocol;      // Tcp, udp .etc
    short checksum;     // check the integrity of the packet
    unsigned int src_ip;
    unsigned int dst_ip;
} IP_hdr;

typedef struct Route_t {
    Route_t(const char* mask, IP gw_ip, Nic* gw_intf, bool is_direct);
    Route_t();
    char _mask[4];
	IP   _gw_ip;
	Nic* _gw_intf;
    bool _is_direct;    // if true, gw_intf and gw_ip are null,
                        // L2 would get inft by compare the sub net ip of dest ip and intf ip
} Route;
#pragma pack (pop)

class Layer3 {
public:
    Layer3(Node* node) : _node(node) {}

	void		promote_to_layer3(Nic* receiver, char* pkt, unsigned pktSize, unsigned tos);

	void		demote_to_layer3(IP dest, char* pkt, unsigned pktSize, char protocal);

    bool        get_longest_prefix_match_route(IP dest_ip, Route& rt) const;

    void        add_route(IP dest_subnet, const char* mask, IP gw_ip, Nic* gw_intf, bool is_direct);

private:
    void        _layer3_recvPkt(Nic* receiver, char* pkt, unsigned pktSize);

    bool        _is_self_ping(IP dest_ip);

private:
    typedef struct Route_CMP_t {
        bool operator()(const IP& lhs, const IP& rhs) const {
			return lhs.ip < rhs.ip;
		}
	} Route_CMP;
	std::map<IP, Route, Route_CMP>	_routeTable;    // subnet ip : route

    Node*       _node;
};