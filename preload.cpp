#include "c_sockfd_manager.hpp"
#include <mutex>
//#include <sys/types.h>
#include <sys/socket.h>

static c_sockfd_manager sockfd_manager;

int socket(int domain, int type, int protocol) noexcept {
	try {
		if (type == SOCK_DGRAM && domain == AF_INET6)
			return sockfd_manager.add_udp_descriptor(domain, type, protocol);
		else if (type == SOCK_STREAM && domain == AF_INET6) {
			return  sockfd_manager.add_tcp_descriptor(domain, type, protocol);
		}
	} catch (const std::exception &) {
		return -1;
	}

	return socket(domain, type, protocol);
}

ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen) {
	return 0;
}

ssize_t sendto(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen) {
	return 0;
}
