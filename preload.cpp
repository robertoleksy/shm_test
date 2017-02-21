#include "c_sockfd_manager.hpp"
#include <dlfcn.h>
#include <iostream>
#include <mutex>
#include <sys/socket.h>

static c_sockfd_manager sockfd_manager;

int socket(int domain, int type, int protocol) noexcept {
	std::cout << "fake socket()" << std::endl;
	try {
		if (type == SOCK_DGRAM && domain == AF_INET) // XXX AF_INET6
			return sockfd_manager.add_udp_descriptor(domain, type, protocol);
		else if (type == SOCK_STREAM && domain == AF_INET) { // XXX AF_INET6
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
	return 0;
}

ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen) {

	return 0;
}

ssize_t sendto(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen) {
	std::cout << "fake sendto" << std::endl;
	void * m_buf = nullptr;
	size_t m_buf_size = 0;
	c_turbosocket &turbosock_ref = sockfd_manager.get_turbosocket_for_descriptor(sockfd);
	std::tie<void *, size_t>(m_buf, m_buf_size) = turbosock_ref.get_buffer_for_write();
	if (len > m_buf_size) return -1;
	std::memcpy(m_buf, buf, len);
	turbosock_ref.send();
	return len;
}
