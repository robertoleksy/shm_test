#include "c_turbosocket.hpp"
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <random>
#include <thread>

using namespace boost::interprocess;

std::tuple<void *, size_t> c_turbosocket::get_buffer_for_write_to_server() {
	m_lock_client_to_server.lock();
	if (m_header_client_to_server->message_in)
		m_header_client_to_server->cond_full.wait(m_lock_client_to_server);
	return std::make_tuple(static_cast<char *>(m_shm_region_client_to_server.get_address()) + sizeof(header), m_size_of_single_buffer);
}

std::tuple<void *, size_t> c_turbosocket::get_buffer_for_write_to_client() {
	m_lock_server_to_client.lock();
	if (m_header_server_to_client->message_in)
		m_header_server_to_client->cond_full.wait(m_lock_server_to_client);
	return std::make_tuple(static_cast<char *>(m_shm_region_server_to_client.get_address()) + sizeof(header), m_size_of_single_buffer);
}

std::tuple<void *, size_t> c_turbosocket::get_buffer_for_read_from_server() {
	m_lock_server_to_client.lock();
	if (!m_header_server_to_client->message_in)
		m_header_server_to_client->cond_empty.wait(m_lock_server_to_client);
	void *data_ptr = static_cast<char *>(m_shm_region_server_to_client.get_address()) + sizeof(header);
	if (m_header_server_to_client->data_size == 0)
		return std::make_tuple(data_ptr, m_size_of_single_buffer);
	else
		return std::make_tuple(data_ptr, m_header_server_to_client->data_size);
}

std::tuple<void *, size_t> c_turbosocket::get_buffer_for_read_from_client() {
	m_lock_server_to_client.lock();
	if (!m_header_client_to_server->message_in)
		m_header_client_to_server->cond_empty.wait(m_lock_client_to_server);

	void *data_ptr = static_cast<char *>(m_shm_region_client_to_server.get_address()) + sizeof(header);
	if (m_header_client_to_server->data_size == 0)
		return std::make_tuple(data_ptr, m_size_of_single_buffer);
	else
		return std::make_tuple(data_ptr, m_header_client_to_server->data_size);

}

void c_turbosocket::send_to_server(size_t size, const unsigned char dst_address[16], unsigned short dst_port) {
	std::cout << "send to server\n";
	m_header_client_to_server->data_size = size;
	std::copy(dst_address, dst_address + 16, m_header_client_to_server->ipv6.begin());
	m_header_client_to_server->port = dst_port;
	m_header_client_to_server->message_in = true;
	m_header_client_to_server->cond_empty.notify_one();
	m_lock_client_to_server.unlock();
}

void c_turbosocket::send_to_client(size_t size, const unsigned char dst_address[16], unsigned short dst_port) {
	std::cout << "send to client\n";
	m_header_server_to_client->data_size = size;
	std::copy(dst_address, dst_address + 16, m_header_server_to_client->ipv6.begin());
	m_header_server_to_client->port = dst_port;
	m_header_server_to_client->message_in = true;
	m_header_server_to_client->cond_empty.notify_one();
	m_lock_server_to_client.unlock();
}

void c_turbosocket::received_from_server() {
	std::cout << "received from server\n";
	m_header_server_to_client->message_in = false;
	m_header_server_to_client->cond_full.notify_one();
	m_lock_client_to_server.unlock();
}

void c_turbosocket::received_from_client() {
	std::cout << "received from client\n";
	m_header_client_to_server->message_in = false;
	m_header_client_to_server->cond_full.notify_one();
	m_lock_server_to_client.unlock();
}

std::tuple<void *, size_t> c_turbosocket::get_buffer_for_write() {
	assert(false);
//	std::cout << "full lock" << std::endl;
	header * const header_ptr = static_cast<header *>(m_shm_region.get_address());
	m_lock.lock();
	if (header_ptr->message_in)
		header_ptr->cond_full.wait(m_lock);

	return std::make_tuple(m_shm_data_buff, m_shm_size - sizeof(header));
}

std::tuple<void *, size_t> c_turbosocket::get_buffer_for_read() {
	assert(false);
//	std::cout << "empty lock" << std::endl;
	header * const header_ptr = static_cast<header *>(m_shm_region.get_address());
	m_lock.lock();
	if (!header_ptr->message_in)
		header_ptr->cond_empty.wait(m_lock);
	if (header_ptr->data_size == 0)
		return std::make_tuple(m_shm_data_buff, m_shm_size - sizeof(header));
	else
		return std::make_tuple(m_shm_data_buff, header_ptr->data_size);
}

void c_turbosocket::send() {
	assert(false);
//	std::cout << "empty unlock" << std::endl;
	header * const header_ptr = static_cast<header *>(m_shm_region.get_address());
	header_ptr->message_in = true;
	header_ptr->cond_empty.notify_one();
	m_lock.unlock();
}

void c_turbosocket::send(size_t size, const unsigned char dst_address[16], unsigned short dst_port) {
	assert(false);
	header * const header_ptr = static_cast<header *>(m_shm_region.get_address());
	header_ptr->data_size = size;
	std::copy(dst_address, dst_address + 16, header_ptr->destination_ipv6.begin());
	header_ptr->destination_port = dst_port;
	send();
}

void c_turbosocket::received() {
	assert(false);
//	std::cout << "full unlock" << std::endl;
	header * const header_ptr = static_cast<header *>(m_shm_region.get_address());
	header_ptr->message_in = false;
	header_ptr->cond_full.notify_one();
	m_lock.unlock();
}

void c_turbosocket::connect_as_client() {
	message_queue msg_queue(open_or_create, m_queue_name.c_str(), 20, m_max_queue_massage_size);
	const std::string shm_name = [this]() {
		std::string ret = "turbosocket" + std::to_string(std::hash<std::thread::id>()(std::this_thread::get_id()));
		ret.resize(m_max_queue_massage_size, '\0');
		ret.back() = '\0';
		return ret;
	}(); // lambda

	// open shm
	std::cout << "open shm" << std::endl;
	std::cout << "shm name: " << shm_name << std::endl;
	open_or_create_shm(shm_name.c_str());
	msg_queue.send(shm_name.data(), shm_name.size(), 0); // send string without '\0'
}

void c_turbosocket::wait_for_connection() {
	message_queue msg_queue(open_or_create, m_queue_name.c_str(), 20, m_max_queue_massage_size);
	std::array<char, m_max_queue_massage_size> shm_name;
	size_t recv_size = 0;
	unsigned int priority = 0;
	msg_queue.receive(shm_name.data(), shm_name.size(), recv_size, priority); // receive shm name
	std::cout << "receive shm name with size " << recv_size << std::endl;
	for(auto &c : shm_name)
		std::cout << c;
	std::cout << std::endl;
	open_or_create_shm(shm_name.data());
}

bool c_turbosocket::timed_wait_for_connection() { // TODO code duplication
//	std::cout << "timed wait for connection\n";
	message_queue msg_queue(open_or_create, m_queue_name.c_str(), 20, m_max_queue_massage_size);
	std::array<char, m_max_queue_massage_size> shm_name;
	size_t recv_size = 0;
	unsigned int priority = 0;
	//msg_queue.receive(shm_name.data(), shm_name.size(), recv_size, priority); // receive shm name
	boost::posix_time::ptime timeout_point = boost::posix_time::second_clock::universal_time() + boost::posix_time::seconds(1); // TODO configure this
	bool ret = msg_queue.timed_receive(shm_name.data(), shm_name.size(), recv_size, priority, timeout_point);
	if (!ret) return false;
	std::cout << "receive shm name with size " << recv_size << std::endl;
	for(auto &c : shm_name)
		std::cout << c;
	std::cout << std::endl;
	open_or_create_shm(shm_name.data());
	return true;
}

bool c_turbosocket::ready_for_read() {
	assert(false);
	bool lock = m_lock.try_lock();
	if (!lock) return false;
	void * addr = m_shm_region.get_address();
	header * const header_ptr = static_cast<header *>(addr);
	bool ret = header_ptr->message_in;
	m_lock.unlock();
	return ret;
}

bool c_turbosocket::server_data_ready_for_read() {
	bool lock = m_lock_client_to_server.try_lock();
	if (!lock) return false;
	bool ret = m_header_client_to_server->message_in;
	m_lock_client_to_server.unlock();
	return ret;
}

uint64_t c_turbosocket::id() const {
	return m_id;
}

const std::array<unsigned char, 16> &c_turbosocket::get_srv_ipv6() const {
	return m_header_client_to_server->ipv6;
}

const std::array<unsigned char, 16> &c_turbosocket::get_cli_ipv6() const {
	return m_header_server_to_client->ipv6;
}

unsigned short c_turbosocket::get_srv_port() const {
	return m_header_client_to_server->destination_port;
}

unsigned short c_turbosocket::get_cli_port() const {
	return m_header_server_to_client->source_port;
}

uint64_t c_turbosocket::get_uid() const {
	std::random_device rd;
	std::mt19937_64 gen(rd());
	std::uniform_int_distribution<uint64_t> dis(1); // minimal value == 1
	return dis(gen);
}

c_turbosocket::header *c_turbosocket::get_heared_ptr() const {
	void *addr = m_shm_region.get_address();
	header * const header_ptr = static_cast<header *>(addr);
	return header_ptr;
}

void c_turbosocket::open_or_create_shm(const char *name) {
	shared_memory_object shm(open_or_create, name, read_write);
	shm.truncate(static_cast<offset_t>(m_shm_size));
	m_shm_region_client_to_server = mapped_region(shm, read_write, 0, m_shm_size / 2); // first half
	m_shm_region_server_to_client = mapped_region(shm, read_write, m_shm_size / 2, m_shm_size / 2); // second half

	std::cout << "alocate headers" << std::endl;
	void *addr = m_shm_region_client_to_server.get_address();
	m_header_client_to_server = new(addr) header;
	m_lock_client_to_server = scoped_lock<interprocess_mutex>(m_header_client_to_server->mutex, defer_lock_type());
	// server to client
	addr = m_shm_region_server_to_client.get_address();
	m_header_server_to_client = new(addr) header;
	m_lock_server_to_client = scoped_lock<interprocess_mutex>(m_header_server_to_client->mutex, defer_lock_type());

	if (m_header_client_to_server->id == 0) {
		std::cout << "create new id\n";
		m_id = get_uid();
		m_header_client_to_server->id = m_id;
		m_header_server_to_client->id = m_id;
	} else {
		m_id = m_header_client_to_server->id;
	}
}
