#include "c_sockfd_manager.hpp"
#include <sys/socket.h>
#include <dlfcn.h>

int c_sockfd_manager::add_udp_descriptor(int domain, int type, int protocol) {
	std::cout << "add udp socket" << std::endl;
	// XXX if (domain != AF_INET6) throw std::invalid_argument("domain is not AF_INET6");
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
	// XXX if (domain != AF_INET6) throw std::invalid_argument("domain is not AF_INET6");
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
