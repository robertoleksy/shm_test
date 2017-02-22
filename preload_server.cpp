#include "c_sockfd_manager.hpp"
#include "c_turbosocket.hpp"
#include <atomic>
#include <boost/asio.hpp>
#include <map>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

enum class e_ipv6_proto_type : unsigned short {
	eIPv6_TCP=6,
	eIPv6_UDP=17,
	eIPv6_ICMP=58
};

class c_endpoint final {
	public:
		c_endpoint(e_ipv6_proto_type proto, unsigned short port, const boost::asio::ip::address_v6 &address);
		bool operator < (const c_endpoint &rhs) const;
	private:
		e_ipv6_proto_type m_proto;
		unsigned short m_port;
		boost::asio::ip::address_v6 m_address;
};

c_endpoint::c_endpoint(e_ipv6_proto_type proto, unsigned short port, const boost::asio::ip::address_v6 &address)
:
	m_proto(proto),
	m_port(port),
	m_address(address)
{
}

bool c_endpoint::operator < (const c_endpoint &rhs) const {
	if (this->m_proto < rhs.m_proto) return true;
	if (this->m_port < rhs.m_port) return true;
	if (this->m_address < rhs.m_address) return true;
	return false;
}

/********************************************************************************/

class c_endpoint_manager final {
	public:
		c_endpoint_manager();
	private:
		std::mutex m_socket_id_map_mutex;
		std::map<uint64_t, std::shared_ptr<c_turbosocket>> m_socket_id_map; ///< for all sockets (binded and not binded)
		std::map<c_endpoint, std::shared_ptr<c_turbosocket>> m_udp_socket_map; ///< only for binded sockets
		std::atomic<bool> m_stop_flag;
		std::thread m_connection_thread;
		void connection_wait_loop(); ///< wait for new shm segment name from msg queue (other thread)

		std::thread m_bind_thread;
		void bind_wait_loop();
};

c_endpoint_manager::c_endpoint_manager()
:
	m_stop_flag(false),
	m_connection_thread([this](){connection_wait_loop();}),
	m_bind_thread([this](){bind_wait_loop();})
{
}

void c_endpoint_manager::connection_wait_loop() {
	while (!m_stop_flag) {
		c_turbosocket turbosocket;
		bool new_connection = turbosocket.timed_wait_for_connection();
		if (!new_connection) continue;
		uint64_t id = turbosocket.id();
		std::lock_guard<std::mutex> lg(m_socket_id_map_mutex);
		m_socket_id_map.emplace(std::make_pair(id, std::make_shared<c_turbosocket>(std::move(turbosocket))));
	}
}

void c_endpoint_manager::bind_wait_loop() {
	while (!m_stop_flag) {
		const size_t size_of_serialized_data = 22;
		const std::vector<unsigned char> serialized(size_of_serialized_data);

	}
}











int main() {

}


