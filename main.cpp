#include <chrono>
#include <iostream>
#include <thread>

#include "c_turbosocket.hpp"


int main() {
	c_turbosocket turbosocket;
	turbosocket.wait_for_connection();
	void * buf = nullptr;
	size_t buf_size = 0;
	int i = 0;
	while (1) {
		unsigned char mark1_expected_value = static_cast<unsigned char>(i % 256);
		unsigned char mark2_expected_value = static_cast<unsigned char>((i + 1) % 256);

		std::tie<void *, size_t>(buf, buf_size) = turbosocket.get_buffer_for_read();
		{ // critical section
			unsigned char *mark1 = static_cast<unsigned char *>(buf) + 7;
			unsigned char *mark2 = static_cast<unsigned char *>(buf) + 99;
			if (*mark1 != mark1_expected_value) {
				std::cout << "mark 1 error" << std::endl;
				return 1;
			}
			if (*mark2 != mark2_expected_value) {
				std::cout << "mark 2 error" << std::endl;
				return 1;
			}
		}
		turbosocket.received();

		i++;
	}
}
