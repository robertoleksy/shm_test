#ifndef C_SOCKFD_MANAGER_HPP
#define C_SOCKFD_MANAGER_HPP

#include "c_turbosocket.hpp"
#include <map>

class c_sockfd_manager {
	public:
		int add_udp_descriptor(int domain, int type, int protocol);
		int add_tcp_descriptor(int domain, int type, int protocol);
		int close(int fd);
		c_turbosocket &get_turbosocket_for_descriptor(int fd);
	private:
		std::map<int, c_turbosocket> m_udp_descriptors;
		std::map<int, c_turbosocket> m_tcp_descriptors;

};

#endif // C_SOCKFD_MANAGER_HPP
