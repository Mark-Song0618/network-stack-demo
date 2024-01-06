#include "utils.h"
#include <iostream>
#include <string>

void int_to_str(unsigned ip, char str[4]) {
	for (int i = 0; i < 4; i++) {
		str[i] = (ip >> (i * 8)) & 0xff;
	}
}

void str_to_int(unsigned ip, char str[4]) {
	for (int i = 0; i < 4; i++) {
		ip |= str[i] << (8 * i);
	}
}

void log(unsigned stack_level, const char* msg, unsigned indent_level)
{
	std::cout << std::endl;
	std::cout << "##LAYER" << std::to_string(stack_level) << std::endl;
	std::string indent(indent_level, '\t');
	std::cout << indent << msg << std::endl;
}
