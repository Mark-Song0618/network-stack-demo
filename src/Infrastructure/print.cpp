#include "infrastructure.h"
#include <iostream>

void 
Topo::print() const
{
	for (auto& node : _nodes) {
		node.second->print();
	}

	for (auto net : _etherNets) {
		net.second->print();
	}
}

void
EtherNet::print() const
{
	std::cout << "EtherNet: " << _id << " { " << std::endl;
	for (auto nic : _nics) {
		std::cout << "\tNode: " << nic->getOwner().lock()->getNodeId() << " nic: " << nic->getNicId() << std::endl;
	}
	std::cout <<"}" << std::endl;
}

void
Node::print() const
{
	std::cout << "Node: " << _id << " { " << std::endl;
	for (auto nic : _interfaces) {
		nic->print();
	}
	std::cout << "}" << std::endl;
}

void
Nic::print() const
{
	std::cout << "Nic: " << _id << " { " << std::endl;
	std::cout << "\tMAC: " << _nwProps._macAddr << std::endl;
	if (is_IP_configured()) {
		std::cout << "\tIP: " << _nwProps._ipAddr << std::endl;
		std::cout << "\tMask: " << std::to_string((unsigned)_nwProps._mask[3]) << "."
								<< std::to_string((unsigned)_nwProps._mask[2]) << "."
								<< std::to_string((unsigned)_nwProps._mask[1]) << "."
								<< std::to_string((unsigned)_nwProps._mask[0]) << std::endl;
	}
	std::cout << "}" << std::endl;
}