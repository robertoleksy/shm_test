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
		~c_endpoint_manager();
		/**
		 * handler will be called for each received data buffer
		 * The function signature of the handler must be
		 * void handler (void *buf, size_t buf_size)
		 */
		template <typename F>
		void foreach_read(F &&handler);
	private:
		std::mutex m_maps_mutex;
		std::map<uint64_t, std::shared_ptr<c_turbosocket>> m_socket_id_map; ///< for all sockets (binded and not binded)
		std::map<c_endpoint, std::shared_ptr<c_turbosocket>> m_udp_socket_map; ///< only for binded sockets
		std::atomic<bool> m_stop_flag;
		std::thread m_connection_thread;
		void connection_wait_loop(); ///< wait for new shm segment name from msg queue (other thread)

		std::thread m_bind_thread;
		void bind_wait_loop(); ///< called from other thread


};

c_endpoint_manager::c_endpoint_manager()
:
	m_stop_flag(false),
	m_connection_thread([this](){connection_wait_loop();}),
	m_bind_thread([this](){bind_wait_loop();})
{
}

c_endpoint_manager::~c_endpoint_manager() {
	m_stop_flag = true;
	m_connection_thread.join();
	m_bind_thread.join();
}

void c_endpoint_manager::connection_wait_loop() {
	while (!m_stop_flag) {
		c_turbosocket turbosocket;
		bool new_connection = turbosocket.timed_wait_for_connection();
		if (!new_connection) continue;
		uint64_t id = turbosocket.id();
		std::lock_guard<std::mutex> lg(m_maps_mutex);
		m_socket_id_map.emplace(std::make_pair(id, std::make_shared<c_turbosocket>(std::move(turbosocket))));
	}
}

void c_endpoint_manager::bind_wait_loop() {
	using namespace boost::interprocess;
	const size_t size_of_serialized_data = 22; // TODO magic number
	std::vector<unsigned char> serialized(size_of_serialized_data);
	message_queue bind_queue(open_or_create, "turbosocket_bind_queue", 20, serialized.size()); // sizeof serialized always the same
	while (!m_stop_flag) {
		bind_data data;
		size_t recv_size = 0;
		unsigned int priority = 0;
		bool received_data = bind_queue.try_receive(serialized.data(), serialized.size(), recv_size, priority);
		if (!received_data) {
			std::this_thread::sleep_for(std::chrono::milliseconds(500));
			continue;
		}
		assert(recv_size == size_of_serialized_data);
		data.deserialize(serialized);
		c_endpoint endpoint(e_ipv6_proto_type::eIPv6_UDP, data.port, data.address);
		std::cout << "bind turbosocket with id " << data.turbosocket_id << " to port " << data.port << std::endl;
		std::lock_guard<std::mutex> lg(m_maps_mutex);
		m_udp_socket_map.emplace(std::make_pair(endpoint, m_socket_id_map.at(data.turbosocket_id)));
	}
}



template <typename F>
void c_endpoint_manager::foreach_read(F &&handler) {
	std::lock_guard<std::mutex> lg(m_maps_mutex);
	for (auto & element : m_socket_id_map) {
		std::shared_ptr<c_turbosocket> turbosocket(element.second);
		if (turbosocket->ready_for_read()) {
			void * buf = nullptr;
			size_t buf_size = 0;
			std::tie<void *, size_t>(buf, buf_size) = turbosocket->get_buffer_for_read();
			handler(buf, buf_size);
			turbosocket->received(); // end of receive
		}
	}
}







int main() {
	c_endpoint_manager enpoint_manager;
	while (true) {
		enpoint_manager.foreach_read([](void *buf, size_t buf_size) {
			std::cout << "readed " << buf_size << " bytes\n";
			char *str = static_cast<char *>(buf);
			for (int i = 0; i < 8; i++)
				std::cout << str[i];
			std::cout << std::endl;
		});
		std::this_thread::sleep_for(std::chrono::seconds(3));
	}
}




