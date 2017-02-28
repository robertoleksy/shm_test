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
		bool operator == (const c_endpoint &rhs) const;
		boost::asio::ip::address_v6::bytes_type get_ip_as_bytes();
		unsigned short get_port();
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

bool c_endpoint::operator ==(const c_endpoint &rhs) const {
	if (m_address != rhs.m_address) return false;
	if (m_port != rhs.m_port) return false;
	if (m_proto != rhs.m_proto) return false;
	return true;
}

boost::asio::ip::address_v6::bytes_type c_endpoint::get_ip_as_bytes() {
	return m_address.to_bytes();
}

unsigned short c_endpoint::get_port() {
	return m_port;
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
//		bool new_connection = turbosocket.timed_wait_for_connection();
//		if (!new_connection) continue;
		turbosocket.wait_for_connection();
		std::cout << "new connsetion\n";
		uint64_t id = turbosocket.id();
		assert(id != 0);
		std::lock_guard<std::mutex> lg(m_maps_mutex);
		std::cout << "emplace turbosocket with id " << id << " into m_socket_id_map\n";
		m_socket_id_map.emplace(std::make_pair(id, std::make_shared<c_turbosocket>(std::move(turbosocket))));
	}
}

void c_endpoint_manager::bind_wait_loop() {
	using namespace boost::interprocess;
	message_queue bind_queue(open_or_create, "turbosocket_bind_queue", 20, 16);
	while (!m_stop_flag) {
		bind_data data;
		size_t recv_size = 0;
		unsigned int priority = 0;
		std::array<unsigned int, 16> input_buffer;
		boost::asio::ip::address_v6::bytes_type ip_as_bytes;
		std::cout << "receive ip address\n";
		bind_queue.receive(ip_as_bytes.data(), ip_as_bytes.size(), recv_size, priority);
		std::cout << "received " << recv_size << std::endl;
		std::cout << "receive port\n";
		bind_queue.receive(input_buffer.data(), input_buffer.size(), recv_size, priority);
		std::memcpy(&data.port, input_buffer.data(), sizeof(data.port));
		std::cout << "received " << recv_size << "\n";
		std::cout << "receive turbosocket id\n";
		bind_queue.receive(input_buffer.data(), input_buffer.size(), recv_size, priority);
		std::memcpy(&data.turbosocket_id, input_buffer.data(), sizeof(data.turbosocket_id));
		std::cout << "received " << recv_size << std::endl;
		std::cout << "end of bind receive" << std::endl;
		std::cout << "bind ip " << data.address << "\n";
		std::cout << "port " << data.port << "\n";
		std::cout << "turbosocket id " << data.turbosocket_id << "\n";
		data.address = boost::asio::ip::address_v6(ip_as_bytes);
		c_endpoint endpoint(e_ipv6_proto_type::eIPv6_UDP, data.port, data.address);
		std::cout << "bind turbosocket with id " << data.turbosocket_id << " to port " << data.port << std::endl;
		std::cout << "address " << data.address << std::endl;
		std::unique_lock<std::mutex> lg(m_maps_mutex);
		while (m_socket_id_map.find(data.turbosocket_id) == m_socket_id_map.end()) {
			lg.unlock();
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			lg.lock();
		}
		std::cout << "add to m_socket_id_map\n";
		m_udp_socket_map.emplace(std::make_pair(endpoint, m_socket_id_map.at(data.turbosocket_id)));
	}
}



template <typename F>
void c_endpoint_manager::foreach_read(F &&handler) {
	std::lock_guard<std::mutex> lg(m_maps_mutex);
	for (auto & element : m_socket_id_map) {
		std::shared_ptr<c_turbosocket> turbosocket(element.second);
		if (turbosocket->server_data_ready_for_read()) {
			void * buf_from_client = nullptr;
			size_t buf_from_client_size = 0;
			std::tie<void *, size_t>(buf_from_client, buf_from_client_size) = turbosocket->get_buffer_for_read_from_client();
			boost::asio::ip::address_v6::bytes_type ipv6_bytes;
			std::copy(turbosocket->get_srv_ipv6().begin(), turbosocket->get_srv_ipv6().end(), ipv6_bytes.begin());
			boost::asio::ip::address_v6 ipv6(ipv6_bytes);
			unsigned short port = ntohs(turbosocket->get_srv_port());
			std::cout << "readed data, destination:\n";
			std::cout << "ip " << ipv6 << "\n";
			std::cout << "port " << port << std::endl;
			handler(buf_from_client, buf_from_client_size);
			turbosocket->received_from_client(); // end of receive

			c_endpoint packet_endpoint(e_ipv6_proto_type::eIPv6_UDP, port, ipv6);
			std::cout << "find binded socket to address " << ipv6 << " on port " << port << "\n";
			auto it = m_udp_socket_map.find(packet_endpoint);
			if (it != m_udp_socket_map.end()) {
				std::cout << "found destination in m_udp_socket_map map\n";
				void *buffer_to_client = nullptr;
				size_t buffer_to_client_size = 0;
				std::tie<void *, size_t>(buffer_to_client, buffer_to_client_size) = it->second->get_buffer_for_write_to_client();
				std::memcpy(buffer_to_client, buf_from_client, buf_from_client_size);
				it->second->send_to_client(buf_from_client_size, packet_endpoint.get_ip_as_bytes().data(), packet_endpoint.get_port());
			} else {
				std::cout << "not found destination in m_udp_socket_map map, ignore\n";
			}

/*			// send response
			std::cout << "send response\n";
			std::string response = "response";
			std::tie<void *, size_t>(buf_from_client, buf_from_client_size) = turbosocket->get_buffer_for_write_to_client();
			response.copy(static_cast<char *>(buf_from_client), response.size());
			std::array<unsigned char, 16> ip;
			ip.fill(0xAB);
			turbosocket->send_to_client(response.size() + 1, ip.data(), 4321); // size + 1 for \0*/
		}
	}
}







int main() {
	c_endpoint_manager enpoint_manager;
	while (true) {
		enpoint_manager.foreach_read([](void *buf, size_t buf_size) {
			std::cout << "readed " << buf_size << " bytes\n";
			char *str = static_cast<char *>(buf);
			for (size_t i = 0; i < buf_size; i++)
				std::cout << str[i];
			std::cout << "end of foreach lambda loop" << std::endl;
		});
		std::this_thread::sleep_for(std::chrono::seconds(0));
	}
}




