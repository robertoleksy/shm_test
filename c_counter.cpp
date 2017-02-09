#include "c_counter.hpp"

void c_counter::start() {
	m_start_point = std::chrono::steady_clock::now();
}

void c_counter::add_data_size(size_t size_in_bytes) {
	  m_data_in_bytes += size_in_bytes;
}

size_t c_counter::get_speed_in_MBps() {
	auto now = std::chrono::steady_clock::now();
	size_t seconds = std::chrono::duration_cast<std::chrono::seconds>(now - m_start_point).count();
	return m_data_in_bytes / 1024 / 1024 / seconds;
}

size_t c_counter::get_speed_in_GBps() {
	auto now = std::chrono::steady_clock::now();
	size_t seconds = std::chrono::duration_cast<std::chrono::seconds>(now - m_start_point).count();
	return m_data_in_bytes / 1024 / 1024 / 1024 / seconds;
}

size_t c_counter::get_speed_in_Mbps() {
	auto now = std::chrono::steady_clock::now();
	size_t seconds = std::chrono::duration_cast<std::chrono::seconds>(now - m_start_point).count();
	return m_data_in_bytes * 8 / 1024 / 1024 / seconds;
}

size_t c_counter::get_speed_in_Gbps() {
	auto now = std::chrono::steady_clock::now();
	size_t seconds = std::chrono::duration_cast<std::chrono::seconds>(now - m_start_point).count();
	return m_data_in_bytes * 8 / 1024 / 1024 / 1024 / seconds;
}
