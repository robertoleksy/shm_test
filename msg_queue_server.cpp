#include "msg_queue_header.hpp"

using namespace boost::interprocess;

int main() {
	//Create a message_queue.
	message_queue msg_queue(open_or_create, queue_name, size_of_queue, size_of_packet);
	char i = 0;
	size_t recv_data_size = 0;
	unsigned int priority = 0;
	while (true) {
		msg_queue.receive(buffer.data(), buffer.size(), recv_data_size, priority);
		if (buffer[4] != (i % 7)) {
			std::cout << "marker 1 error" << std::endl;
			std::cout << buffer[4] << " vs " << (i % 7);
			std::cout << "recv size " << recv_data_size << std::endl;
			return 1;
		}
		if (buffer[50] != (i + 1) % 100) {
			std::cout << "marker 2 error" << std::endl;
			return 1;
		}
		i++;
	}
}
