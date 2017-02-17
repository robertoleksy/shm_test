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

int c_sockfd_manager::close(int fd) {
	int ret = close(fd);
	if (ret == -1) return ret; // close error
	if (m_tcp_descriptors.find(fd) != m_tcp_descriptors.end()) {
		m_tcp_descriptors.erase(fd);
	} else if (m_udp_descriptors.find(fd) != m_udp_descriptors.end()) {
		m_udp_descriptors.erase(fd);
	}
	return ret;
}
