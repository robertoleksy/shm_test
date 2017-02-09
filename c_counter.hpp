#ifndef C_COUNTER_HPP
#define C_COUNTER_HPP

#include <chrono>

class c_counter
{
	public:
		void start();
		void add_data_size(size_t size_in_bytes);
		size_t get_speed_in_MBps();
		size_t get_speed_in_GBps();
		size_t get_speed_in_Mbps();
		size_t get_speed_in_Gbps();
		void reset();
	private:
		size_t m_data_in_bytes = 0;
		std::chrono::steady_clock::time_point m_start_point;
};

#endif // C_COUNTER_HPP
