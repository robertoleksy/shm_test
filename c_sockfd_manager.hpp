#ifndef C_SOCKFD_MANAGER_HPP
#define C_SOCKFD_MANAGER_HPP

#include "c_turbosocket.hpp"
#include <boost/asio.hpp>
#include <boost/interprocess/ipc/message_queue.hpp>
#include <map>

struct bind_data {
	uint64_t turbosocket_id;
	boost::asio::ip::address_v6 address;
	unsigned short port;
	std::vector<unsigned char> serialize();
	void deserialize(std::vector<unsigned char> buf);
};

class c_sockfd_manager {
	public:
		c_sockfd_manager();
		int add_udp_descriptor(int domain, int type, int protocol);
		int add_tcp_descriptor(int domain, int type, int protocol);
		int close(int fd);
		c_turbosocket &get_turbosocket_for_descriptor(int fd);
	private:
		std::map<int, c_turbosocket> m_udp_descriptors;
		std::map<int, c_turbosocket> m_tcp_descriptors;
		boost::interprocess::message_queue m_bind_queue; // for bind notification

};

#endif // C_SOCKFD_MANAGER_HPP
