#include "c_sockfd_manager.hpp"
#include <sys/socket.h>
#include <dlfcn.h>

int c_sockfd_manager::add_udp_descriptor(int domain, int type, int protocol) {
	std::cout << "add udp socket" << std::endl;
	if (domain != AF_INET6) throw std::invalid_argument("domain is not AF_INET6");
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
	if (domain != AF_INET6) throw std::invalid_argument("domain is not AF_INET6");
	if (type != SOCK_STREAM) throw std::invalid_argument("type is not SOCK_STREAM");
	int(*original_socket)(int, int, int);
	original_socket = reinterpret_cast<decltype (original_socket)>(dlsym(RTLD_NEXT, "socket"));
	int descriptor = original_socket(domain, type, protocol);
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

int c_sockfd_manager::bind(int sockfd, const sockaddr *addr, socklen_t addrlen) {
	int(*original_bind)(int, const struct sockaddr *, socklen_t);
	original_bind = reinterpret_cast<decltype(original_bind)>(dlsym(RTLD_NEXT, "bind"));

	std::cout << "original bind" << std::endl;
	int ret = 0; // XXX original_bind(sockfd, addr, addrlen);
	if (ret == -1) {
		std::cout << "original bind error\n";
		return ret;
	}
	std::cout << "finding file descriptor in turbosocket\n";
	auto it_tcp = m_tcp_descriptors.find(sockfd);
	auto it_udp = m_udp_descriptors.find(sockfd);

	auto send_bind_request = [this](c_turbosocket &turbosocket, const struct sockaddr_in6 *addr) {
		using namespace boost::interprocess;
		bind_data data = generate_bind_data(turbosocket.id(), addr);
		std::cout << "generated bind data: " << data.address << " port " << data.port << std::endl;
		std::cout << "create queue\n";
		message_queue bind_queue(open_or_create, "turbosocket_bind_queue", 20, 16);
		std::cout << "sending via queue\n";
		auto ip_as_bytes = data.address.to_bytes();
		std::cout << "send address " << data.address << "\n";
		bind_queue.send(ip_as_bytes.data(), ip_as_bytes.size(), 0);
		std::cout << "send port " << data.port << "\n";
		bind_queue.send(&data.port, sizeof(data.port), 0);
		std::cout << "send tubbosocket id " << data.turbosocket_id << "\n";
		bind_queue.send(&data.turbosocket_id, sizeof(data.turbosocket_id), 0);
		std::cout << "end of send\n";
		std::cout << "sended data\n";
		std::cout << "bind ip " << data.address << "\n";
		std::cout << "port " << data.port << "\n";
		std::cout << "turbosocket id " << data.turbosocket_id << "\n";
	};// lambda

	if (it_tcp != m_tcp_descriptors.end()) { // TODO
		std::cout << "send bind request via tcp queue\n";
		send_bind_request(it_tcp->second, reinterpret_cast<const struct sockaddr_in6 *>(addr));
	} else if (it_udp != m_udp_descriptors.end()) {
		std::cout << "send bind request via udp queue\n";
		send_bind_request(it_udp->second, reinterpret_cast<const struct sockaddr_in6 *>(addr));
	} else {
		std::cout << "not found descriptor " << sockfd << '\n';
	}
	std::cout << "end of fake bind\n";
	return ret;
}

bind_data c_sockfd_manager::generate_bind_data(uint64_t turbosocket_id, const sockaddr_in6 *addr) {
	bind_data ret;
	ret.turbosocket_id = turbosocket_id;
	ret.port = htons(addr->sin6_port);
	char addr_str[INET6_ADDRSTRLEN];
	inet_ntop(AF_INET6, &(addr->sin6_addr), addr_str, INET6_ADDRSTRLEN);
	std::cout << "generate bind data address " << addr_str << "\n";
	//ret.address.from_string(addr_str);
	boost::asio::ip::address_v6::bytes_type ip_bytes;
	std::copy(&(addr->sin6_addr.s6_addr[0]), &(addr->sin6_addr.s6_addr[16]), ip_bytes.begin());
	ret.address = boost::asio::ip::address_v6(ip_bytes);
	std::cout << "generated addres as asio object " << ret.address << "\n";
	return ret;
}
