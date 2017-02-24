#include "c_sockfd_manager.hpp"
#include <dlfcn.h>
#include <iostream>
#include <mutex>
#include <sys/socket.h>

static c_sockfd_manager sockfd_manager;


int socket(int domain, int type, int protocol) noexcept {
	std::cout << "fake socket()" << std::endl;
	try {
		if (type == SOCK_DGRAM && domain == AF_INET6) {
			return sockfd_manager.add_udp_descriptor(domain, type, protocol);
		}
		else if (type == SOCK_STREAM && domain == AF_INET6) {
			return  sockfd_manager.add_tcp_descriptor(domain, type, protocol);
		}
	} catch (const std::exception &) {
		return -1;
	}

	int(*original_socket)(int, int, int);
	original_socket = reinterpret_cast<decltype (original_socket)>(dlsym(RTLD_NEXT, "socket"));
	return original_socket(domain, type, protocol);
}

int close(int fd) {
	std::cout << "fake close" << std::endl;
	return sockfd_manager.close(fd);
}

int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
	std::cout << "fake bind" << std::endl;
	return sockfd_manager.bind(sockfd, addr, addrlen);
}

ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen) {
	return -1;
	std::cout << "fake receivefrom" << std::endl;
	void *m_buf = nullptr;
	size_t m_buf_size = 0;
	try {
		c_turbosocket &turbosock_ref = sockfd_manager.get_turbosocket_for_descriptor(sockfd);
		std::tie<void *, size_t>(m_buf, m_buf_size) = turbosock_ref.get_buffer_for_read_from_client();
		if (len < m_buf_size) {
			turbosock_ref.received_from_client();
			return -1;
		}
		std::memcpy(buf, m_buf, m_buf_size);
		turbosock_ref.received_from_client();
		return static_cast<ssize_t>(m_buf_size);
	} catch (const std::invalid_argument &) {
		ssize_t(*original_recvfrom)(int, void *, size_t, int, struct sockaddr *, socklen_t *);
		original_recvfrom = reinterpret_cast<decltype (original_recvfrom)>(dlsym(RTLD_NEXT, "receiveform"));
		return original_recvfrom(sockfd, buf, len, flags, src_addr, addrlen);
	}
}

ssize_t sendto(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen) {
	std::cout << "fake sendto" << std::endl;
	void *m_buf = nullptr;
	size_t m_buf_size = 0;
	try {
		c_turbosocket &turbosock_ref = sockfd_manager.get_turbosocket_for_descriptor(sockfd);
		std::cout << "get buffer for write to server\n";
		std::tie<void *, size_t>(m_buf, m_buf_size) = turbosock_ref.get_buffer_for_write_to_server();
/*		if (len > m_buf_size) { // TODO
			turbosock_ref.send();
			std::cout << "sendto ret -1: len > m_buf_size" << std::endl;
			return -1;
		}*/
		std::memcpy(m_buf, buf, len);
		assert(dest_addr->sa_family == AF_INET6);
		const sockaddr_in6 *addr = reinterpret_cast<const sockaddr_in6 *>(dest_addr);
		turbosock_ref.send_to_server(len, addr->sin6_addr.s6_addr, addr->sin6_port);
		return static_cast<ssize_t>(len);
	} catch (const std::invalid_argument &) { // not found descriptor in sockfd_manager
		ssize_t(*original_sendto)(int, const void *, size_t, int, const struct sockaddr *, socklen_t);
		original_sendto = reinterpret_cast<decltype (original_sendto)>(dlsym(RTLD_NEXT, "sendto"));
		return original_sendto(sockfd, buf, len, flags, dest_addr, addrlen);
	}
}
