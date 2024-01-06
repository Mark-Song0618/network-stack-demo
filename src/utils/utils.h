#pragma once

#define NAMESPACE_BEGIN(name) namespace name {
#define NAMESPACE_END(name) }

#define VLAN_8021Q_PROTO	0x8100
#define ARP_MSG				806
#define ETH_IP				0x0800
#define ICMP_PRO			0x01
#define ICMP_ECHO_REQ		0x08
#define ICMP_ECHO_REPLY		0x00
#define IP_IN_IP			0x04
#define MTCP				0x14
#define USERAPP1			0x15
#define ARP_BROAD_REQ		1
#define ARP_REPLY			2
#define MAX_PACKET_SIZE		2048
#define LOCAL_HOST_IP		{127,0,0,1}
#define MAX_VLAN_FIELDS 128

#define BROADCASTADDR  {0xFF};

void int_to_str(unsigned ip, char[4]);

void str_to_int(unsigned ip, char[4]);

void log(unsigned stack_level, const char* msg, unsigned indent_level = 0);