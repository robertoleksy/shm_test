#include "c_sockfd_manager.hpp"
#include <sys/socket.h>

int c_sockfd_manager::add_udp_descriptor(int domain, int type, int protocol) {
	if (domain != AF_INET6) throw std::invalid_argument("domain is not AF_INET6");
	if (type != SOCK_DGRAM) throw std::invalid_argument("type is not SOCK_DGRAM");
	int descriptor = socket(domain, type, protocol);
	if (descriptor != -1)
		m_udp_descriptors.emplace(std::make_pair(descriptor, c_turbosocket()));
	return descriptor;
}

int c_sockfd_manager::add_tcp_descriptor(int domain, int type, int protocol) {
	if (domain != AF_INET6) throw std::invalid_argument("domain is not AF_INET6");
	if (type != SOCK_STREAM) throw std::invalid_argument("type is not SOCK_STREAM");
	int descriptor = socket(domain, type, protocol);
	if (descriptor != -1)
		m_tcp_descriptors.emplace(std::make_pair(descriptor, c_turbosocket()));
	return descriptor;
}
