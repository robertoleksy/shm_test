#include "c_sockfd_manager.hpp"
#include <sys/socket.h>
#include <dlfcn.h>

int c_sockfd_manager::add_udp_descriptor(int domain, int type, int protocol) {
	std::cout << "add udp socket" << std::endl;
	//if (domain != AF_INET6) throw std::invalid_argument("domain is not AF_INET6");
	if (type != SOCK_DGRAM) throw std::invalid_argument("type is not SOCK_DGRAM");
	int(*original_socket)(int, int, int);
	original_socket = reinterpret_cast<decltype (original_socket)>(dlsym(RTLD_NEXT, "socket"));
	int descriptor = original_socket(domain, type, protocol);
	std::cout << "original socket return descriptor " << descriptor << std::endl;
	if (descriptor != -1) {
		m_udp_descriptors.emplace(std::make_pair(descriptor, c_turbosocket()));
		m_udp_descriptors.at(descriptor).connect_as_client();
	}
	return descriptor;
}

int c_sockfd_manager::add_tcp_descriptor(int domain, int type, int protocol) {
	std::cout << "add tcp socket" << std::endl;
	//if (domain != AF_INET6) throw std::invalid_argument("domain is not AF_INET6");
	if (type != SOCK_STREAM) throw std::invalid_argument("type is not SOCK_STREAM");
	int descriptor = ::socket(domain, type, protocol);
	if (descriptor != -1) {
		m_tcp_descriptors.emplace(std::make_pair(descriptor, c_turbosocket()));
		m_tcp_descriptors.at(descriptor).connect_as_client();
	}
	return descriptor;
}

int c_sockfd_manager::close(int fd) {
	int(*original_close)(int);
	original_close = reinterpret_cast<decltype(original_close)>(dlsym(RTLD_NEXT, "close"));
	int ret = original_close(fd);
	if (ret == -1) return ret; // close error
	if (m_tcp_descriptors.find(fd) != m_tcp_descriptors.end()) {
		m_tcp_descriptors.erase(fd);
	} else if (m_udp_descriptors.find(fd) != m_udp_descriptors.end()) {
		m_udp_descriptors.erase(fd);
	}
	return ret;
}

c_turbosocket &c_sockfd_manager::get_turbosocket_for_descriptor(int fd) {
	if (m_tcp_descriptors.find(fd) != m_tcp_descriptors.end()) {
		return m_tcp_descriptors.at(fd);
	} else if (m_udp_descriptors.find(fd) != m_udp_descriptors.end()) {
		return m_udp_descriptors.at(fd);
	}
	throw std::invalid_argument("not found descriptor " + std::to_string(fd));
}
