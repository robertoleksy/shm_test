#ifndef C_TURBOSOCKET_HPP
#define C_TURBOSOCKET_HPP

#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/sync/interprocess_condition.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <tuple>

class c_turbosocket final {
	public:
		std::tuple<void *, size_t> get_buffer_for_write(); // block until buffer ready
		std::tuple<void *, size_t> get_buffer_for_read(); // block until buffer ready

		void send(); //< send data writed to buffer get by get_buffer
		void received(); //< receive data writed to buffer get by get_buffer
		void connect_as_client();
		void wait_for_connection(); // block function
	private:
		struct header {
			boost::interprocess::interprocess_mutex mutex;
			// wait when the queue is full
			boost::interprocess::interprocess_condition cond_full;
			// wait when the queue is empty
			boost::interprocess::interprocess_condition cond_empty;
			//Is there any message
			bool message_in = false;
		};

		const std::string m_queue_name = "tunserver_turbosocket_queue";
		static constexpr size_t m_max_queue_massage_size = 20;
		// header + packet + mutex
		const size_t m_shm_size = 65 * 1024 + sizeof(header);
		boost::interprocess::mapped_region m_shm_region;
		void * m_shm_data_buff; // ptr to shared memory for free use
		boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> m_lock;

};

#endif // C_TURBOSOCKET_HPP
