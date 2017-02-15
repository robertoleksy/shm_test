#include "c_sockfd_manager.hpp"

int c_sockfd_manager::add_udp_descriptor() {
	int descriptor = std::min(get_first_free_descritpor(m_udp_descriptors), get_first_free_descritpor(m_tcp_descriptors));
	m_udp_descriptors.emplace(std::make_pair(descriptor, c_turbosocket()));
	return descriptor;
}

int c_sockfd_manager::get_first_free_descritpor(const std::map<int, c_turbosocket> &map) {
	int min_descriptor = 3;
	auto it = map.cbegin();
	auto end_it = map.cend();
	while (it != end_it) {
		if (it->first > min_descriptor) break;
		++it;
		++min_descriptor;
	}
	return min_descriptor;
}
