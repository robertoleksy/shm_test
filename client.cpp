#include <chrono>
#include <iostream>
#include <thread>

#include "c_turbosocket.hpp"
#include "c_counter.hpp"


int main() {
	c_turbosocket turbosocket;
	turbosocket.connect_as_client();
	void * buf = nullptr;
	size_t buf_size = 0;
	int i = 0;
	c_counter counter;
	auto last_dbg_time = std::chrono::steady_clock::now();
	counter.start();
	while (1) {
		std::tie<void *, size_t>(buf, buf_size) = turbosocket.get_buffer_for_write();
		{ // critical section
			std::memset(buf, i, buf_size);
			unsigned char *mark1 = static_cast<unsigned char *>(buf) + 7;
			unsigned char *mark2 = static_cast<unsigned char *>(buf) + 99;
			*mark1 = static_cast<unsigned char>(i % 256);
			*mark2 = static_cast<unsigned char>((i + 1) % 256);
		}
		counter.add_data_size(buf_size);
		turbosocket.send();
		i++;
		auto now = std::chrono::steady_clock::now();
		if (std::chrono::duration_cast<std::chrono::seconds>(now - last_dbg_time).count() > 3) {
			std:: cout << "\n\n\n";
			std::cout << "speed in GBps " << counter.get_speed_in_GBps() << std::endl;
			std::cout << "speed in MBps " << counter.get_speed_in_MBps() << std::endl;
			std::cout << "speed in Gbps " << counter.get_speed_in_Gbps() << std::endl;
			std::cout << "speed in Mbps " << counter.get_speed_in_Mbps() << std::endl;
			last_dbg_time = std::chrono::steady_clock::now();
			counter.reset();
		}
	}
}
