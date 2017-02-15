#ifndef C_SOCKFD_MANAGER_HPP
#define C_SOCKFD_MANAGER_HPP

#include "c_turbosocket.hpp"
#include <map>

class c_sockfd_manager {
	public:
		int add_udp_descriptor();
		int add_tcp_descriptor();
	private:
		int get_first_free_descritpor(const std::map<int, c_turbosocket> &map);
		std::map<int, c_turbosocket> m_udp_descriptors;
		std::map<int, c_turbosocket> m_tcp_descriptors;
};

#endif // C_SOCKFD_MANAGER_HPP
