#include "c_turbosocket.hpp"
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <random>
#include <thread>

using namespace boost::interprocess;

c_turbosocket::c_turbosocket(c_turbosocket &&other) noexcept {
	if (this == &other) return;
	this->m_shm_region = std::move(other.m_shm_region);
	this->m_shm_data_buff = other.m_shm_data_buff;
	other.m_shm_data_buff = nullptr;
	this->m_lock = std::move(other.m_lock);
}

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
	shm.truncate(static_cast<offset_t>(m_shm_size));
	m_shm_region = mapped_region(shm, read_write);

	std::cout << "alocate header" << std::endl;
	void * addr = m_shm_region.get_address();
	header * const header_ptr = new(addr) header;
	header_ptr->id = get_uid();
	m_id = header_ptr->id;
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
	m_id = header_ptr->id;
	std::cout << "created shm address " << addr << std::endl;
	std::cout << "shm data addr " << m_shm_data_buff << std::endl;
	std::cout << "shm size " << m_shm_region.get_size() << std::endl;
}

bool c_turbosocket::timed_wait_for_connection() { // TODO code duplication
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
	shared_memory_object shm(open_only, shm_name.data(), read_write);
	m_shm_region = mapped_region(shm, read_write);
	void * addr = m_shm_region.get_address();
	header * const header_ptr = static_cast<header *>(addr);
	m_shm_data_buff = static_cast<char *>(addr) + sizeof(header);
	m_lock = scoped_lock<interprocess_mutex>(header_ptr->mutex, defer_lock_type());
	m_id = header_ptr->id;
	std::cout << "created shm address " << addr << std::endl;
	std::cout << "shm data addr " << m_shm_data_buff << std::endl;
	std::cout << "shm size " << m_shm_region.get_size() << std::endl;
	std::cout << "ID " << m_id << std::endl;
	return true;
}

bool c_turbosocket::ready_for_read() {
	bool lock = m_lock.try_lock();
	if (!lock) return false;
	void * addr = m_shm_region.get_address();
	header * const header_ptr = static_cast<header *>(addr);
	bool ret = header_ptr->message_in;
	m_lock.unlock();
	return ret;
}

uint64_t c_turbosocket::id() const {
	return m_id;
}

uint64_t c_turbosocket::get_uid() const {
	std::random_device rd;
	std::mt19937_64 gen(rd());
	std::uniform_int_distribution<uint64_t> dis;
	return dis(gen);
}
