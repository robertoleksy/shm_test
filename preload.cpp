#include "c_turbosocket.hpp"
#include <mutex>



ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen) {
	static c_turbosocket turbosocket;
	std::once_flag flag;
	std::call_once(flag, []() {
		turbosocket.connect_as_client();
	});

	return 0;
}

ssize_t sendto(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen) {
	return 0;
}
