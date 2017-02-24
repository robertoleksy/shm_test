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
	int ret = original_bind(sockfd, addr, addrlen);
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
		const std::vector<unsigned char> serialized = data.serialize();
		std::cout << "create queue\n";
		std::cout << "serialize size " << serialized.size() << "\n";
		message_queue bind_queue(open_or_create, "turbosocket_bind_queue", 20, serialized.size()); // sizeof serialized always the same
		std::cout << "sending via queue\n";
		bind_queue.send(serialized.data(), serialized.size(), 0);
	};// lambda

	if (it_tcp != m_tcp_descriptors.end()) { // TODO
		std::cout << "send bind request via tcp queue\n";
		send_bind_request(it_tcp->second, reinterpret_cast<const struct sockaddr_in6 *>(addr));
	} else if (it_udp != m_udp_descriptors.end()) {
		std::cout << "send bind request via udp queue\n";
		send_bind_request(it_udp->second, reinterpret_cast<const struct sockaddr_in6 *>(addr));
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
	ret.address.from_string(addr_str);
	return ret;
}

std::vector<unsigned char> bind_data::serialize() {
	std::vector<unsigned char> ret;
	for (int i = sizeof(turbosocket_id) - 1; i >=0; i--) {
		unsigned char byte = static_cast<unsigned char>(turbosocket_id >> i);
		ret.push_back(byte);
	}
	ret.push_back(port & 0xFF);
	ret.push_back(port >> 8);
	boost::asio::ip::address_v6::bytes_type ip_as_bytes = address.to_bytes();
	for (const auto & byte : ip_as_bytes)
		ret.emplace_back(byte);
	return ret;
}

void bind_data::deserialize(std::vector<unsigned char> buf) {
	turbosocket_id = 0;
	for (size_t i = 0; i < sizeof(turbosocket_id); i++) {
		turbosocket_id += static_cast<unsigned char>((buf.at(i) << (i * 8)));
	}
	port = buf.at(4);
	port += buf.at(5) << 8;
	boost::asio::ip::address_v6::bytes_type ip_bytes;
	auto it = (buf.begin() + 6);
	for (size_t i = 0; i < ip_bytes.size(); i++) {
		ip_bytes.at(i) = *it;
		++it;
	}
	address = boost::asio::ip::address_v6(ip_bytes);
}
