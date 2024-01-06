#include <iostream>
#include "../infrastructure/infrastructure.h"
#include "../utils/utils.h"

int main () {
	// create net topology
	Topo topo;
	auto netId0		 = topo.createEtherNet();
	auto net0		 = topo.getEtherNet(netId0);
	auto netId1		 = topo.createEtherNet();
	auto net1		 = topo.getEtherNet(netId1);
	auto netId2		 = topo.createEtherNet();
	auto net2		 = topo.getEtherNet(netId2);
	auto netId3		 = topo.createEtherNet();
	auto net3		 = topo.getEtherNet(netId3);
	auto netId4		 = topo.createEtherNet();
	auto net4		 = topo.getEtherNet(netId4);
	auto netId5		 = topo.createEtherNet();
	auto net5		 = topo.getEtherNet(netId5);

	auto nodeId0	= topo.createNode();
	auto node0      = topo.getNode(nodeId0);
	auto nodeId1	= topo.createNode();
	auto node1      = topo.getNode(nodeId1);
	auto nodeId2	= topo.createNode();
	auto node2      = topo.getNode(nodeId2);
	auto nodeId3	= topo.createNode();
	auto node3      = topo.getNode(nodeId3);
	auto nodeId4	= topo.createNode();
	auto node4      = topo.getNode(nodeId4);
	auto nodeId5	= topo.createNode();
	auto node5      = topo.getNode(nodeId5);
	auto nodeId6	= topo.createNode();
	auto node6      = topo.getNode(nodeId6);
	auto nodeId7	= topo.createNode();
	auto node7      = topo.getNode(nodeId7);

	SPtr<Nic>   nic0   = node0->addInterface();
	SPtr<Nic>   nic1   = node1->addInterface();
	SPtr<Nic>   nic2_0 = node2->addInterface();
	SPtr<Nic>   nic2_1 = node2->addInterface();
	SPtr<Nic>   nic2_2 = node2->addInterface();
	SPtr<Nic>   nic3   = node3->addInterface();
	SPtr<Nic>   nic4_0 = node4->addInterface();
	SPtr<Nic>   nic4_1 = node4->addInterface();
	SPtr<Nic>   nic5_0 = node5->addInterface();
	SPtr<Nic>   nic5_1 = node5->addInterface();
	SPtr<Nic>   nic6_0 = node6->addInterface();
	SPtr<Nic>   nic6_1 = node6->addInterface();
	SPtr<Nic>   nic7   = node7->addInterface();

	// net0
	node0->connect_to_ethernet(netId0, nic0, true);
	node1->connect_to_ethernet(netId0, nic1, true);
	node2->connect_to_ethernet(netId0, nic2_0, true);

	// net1
	node2->connect_to_ethernet(netId1, nic2_1, true);
	node3->connect_to_ethernet(netId1, nic3, true);

	// net2
	node2->connect_to_ethernet(netId2, nic2_2);
	node4->connect_to_ethernet(netId2, nic4_0);

	// net3
	node4->connect_to_ethernet(netId3, nic4_1);
	node5->connect_to_ethernet(netId3, nic5_0);
	
	// net4
	node5->connect_to_ethernet(netId4, nic5_1);
	node6->connect_to_ethernet(netId4, nic6_0);
	// net5
	node6->connect_to_ethernet(netId5, nic6_1);
	node7->connect_to_ethernet(netId5, nic7);

	// route learning
	char ip0[4] = {0};
	char ip1[4] = {0};
	char mask0[4] = {0};
	char mask1[4] = {0};
	int_to_str(net0->get_subnet_ip(), ip0);
	int_to_str(net1->get_subnet_ip(), ip1);
	int_to_str(net0->get_mask(), mask0);
	int_to_str(net1->get_mask(), mask1);
	node0->add_route({ip0 }, mask0, {}, nullptr, true);
	node0->add_route({ip1}, mask1, nic2_0->getIp(), nic0.get(), false);
	node2->add_route({ip0}, mask0, {}, nullptr, true);
	node2->add_route({ip1}, mask1, {}, nullptr, true);

	// check
	topo.print();

	topo.start_pkt_receive_thread();
	IP ip_node1 = nic1->getIp();
	IP ip_node3 = nic3->getIp();
	node0->sendMsg("hello, Mark1", 12, ip_node1.ip);
	node0->sendMsg("hello, Mark3", 12, ip_node3.ip);
	
	std::this_thread::sleep_for(std::chrono::seconds(5));
	topo.stop_pkt_receive_thread();
	return 0;
}