#include "msg_queue_header.hpp"

using namespace boost::interprocess;

int main() {

	//Create a message_queue.
	message_queue msg_queue(open_or_create, queue_name, size_of_queue, size_of_packet);
	char i = 0;
	c_counter counter;
	auto last_dbg_time = std::chrono::steady_clock::now();
	counter.start();
	while (true) {
		buffer.fill(i);
		buffer[4] = (i % 7);
		buffer[50] = (i + 1) % 100;
		msg_queue.send(buffer.data(), buffer.size(), 0);
		counter.add_data_size(buffer.size());
		i++;

		auto now = std::chrono::steady_clock::now();
		if (std::chrono::duration_cast<std::chrono::seconds>(now - last_dbg_time).count() > 3) {
			std::cout << "speed in GBps " << counter.get_speed_in_GBps() << std::endl;
			std::cout << "speed in MBps " << counter.get_speed_in_MBps() << std::endl;
			std::cout << "speed in Gbps " << counter.get_speed_in_Gbps() << std::endl;
			std::cout << "speed in Mbps " << counter.get_speed_in_Mbps() << std::endl;
			last_dbg_time = std::chrono::steady_clock::now();
		}
	}
}
