#include "c_turbosocket.hpp"
#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <thread>

using namespace boost::interprocess;

std::tuple<void *, size_t> c_turbosocket::get_buffer_for_write() {
//	std::cout << "full lock" << std::endl;
	header * const header_ptr = static_cast<header *>(m_shm_region.get_address());
	m_lock.lock();
	if (header_ptr->message_in)
		header_ptr->cond_full.wait(m_lock);

	return std::make_tuple(m_shm_data_buff, m_shm_size - sizeof(header));
}

std::tuple<void *, size_t> c_turbosocket::get_buffer_for_read() {
//	std::cout << "empty lock" << std::endl;
	header * const header_ptr = static_cast<header *>(m_shm_region.get_address());
	m_lock.lock();
	if (!header_ptr->message_in)
		header_ptr->cond_empty.wait(m_lock);

	return std::make_tuple(m_shm_data_buff, m_shm_size - sizeof(header));
}

void c_turbosocket::send() {
//	std::cout << "empty unlock" << std::endl;
	header * const header_ptr = static_cast<header *>(m_shm_region.get_address());
	header_ptr->message_in = true;
	header_ptr->cond_empty.notify_one();
	m_lock.unlock();
}

void c_turbosocket::received() {
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
	shared_memory_object shm(create_only, shm_name.data(), read_write);
	//shared_memory_object shm(create_only, "shm_name", read_write);
	shm.truncate(static_cast<offset_t>(m_shm_size));
	m_shm_region = mapped_region(shm, read_write);

	std::cout << "alocate header" << std::endl;
	void * addr = m_shm_region.get_address();
	header * const header_ptr = new(addr) header;
	m_shm_data_buff = static_cast<char *>(addr) + sizeof(header);
	m_lock = scoped_lock<interprocess_mutex>(header_ptr->mutex, defer_lock_type());
	std::cout << "created shm address " << addr << std::endl;
	std::cout << "shm data addr " << m_shm_data_buff << std::endl;
	std::cout << "header assress " << static_cast<void *>(header_ptr) << std::endl;
	std::cout << "shm size " << m_shm_region.get_size() << std::endl;

	std::cout << "send shm name " << shm_name << std::endl;
	std::cout << "size " << shm_name.size() << std::endl;
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
	shared_memory_object shm(open_only, shm_name.data(), read_write);
	m_shm_region = mapped_region(shm, read_write);
	void * addr = m_shm_region.get_address();
	header * const header_ptr = static_cast<header *>(addr);
	m_shm_data_buff = static_cast<char *>(addr) + sizeof(header);
	m_lock = scoped_lock<interprocess_mutex>(header_ptr->mutex, defer_lock_type());
	std::cout << "created shm address " << addr << std::endl;
	std::cout << "shm data addr " << m_shm_data_buff << std::endl;
	std::cout << "shm size " << m_shm_region.get_size() << std::endl;
}
