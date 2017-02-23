#ifndef C_TURBOSOCKET_HPP
#define C_TURBOSOCKET_HPP

#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/sync/interprocess_condition.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <tuple>

class c_turbosocket final {
	public:
		c_turbosocket() = default;
		c_turbosocket(c_turbosocket && other) noexcept;
		c_turbosocket(const c_turbosocket &) = delete;
		std::tuple<void *, size_t> get_buffer_for_write(); // block until buffer ready
		std::tuple<void *, size_t> get_buffer_for_read(); // block until buffer ready

		void send(); //< send data writed to buffer get by get_buffer
		void send(size_t size, const unsigned char dst_address[16], unsigned short dst_port); ///< send and mark size of sended data
		void received(); //< receive data writed to buffer get by get_buffer
		void connect_as_client();
		void wait_for_connection(); // block function
		bool timed_wait_for_connection(); ///< wait with timeout, return true if connected
		/**
		 * @brief ready_for_read
		 * check if data is ready for read, if return true get_buffer_for_read() returns immediately
		 * not blocks
		 */
		bool ready_for_read();
		uint64_t id() const;
		const std::array<unsigned char, 16> &get_dst_ipv6() const; ///< returns destination address from header
		const std::array<unsigned char, 16> &get_src_ipv6() const; ///< returns source address from header
		unsigned short get_dst_port() const;
		unsigned short get_src_port() const;
	private:
		struct header {
			boost::interprocess::interprocess_mutex mutex;
			// wait when the queue is full
			boost::interprocess::interprocess_condition cond_full;
			// wait when the queue is empty
			boost::interprocess::interprocess_condition cond_empty;
			//Is there any message
			bool message_in = false;
			// socket unique ID
			uint64_t id;
			// size of data
			size_t data_size;

			std::array<unsigned char, 16> destination_ipv6;
			std::array<unsigned char, 16> source_ipv6;
			unsigned short destination_port;
			unsigned short source_port;
		};

		uint64_t get_uid() const;
		header *get_heared_ptr() const;

		const std::string m_queue_name = "tunserver_turbosocket_queue";
		static constexpr size_t m_max_queue_massage_size = 20;
		// header + packet + mutex
		const size_t m_shm_size = 65 * 1024 + sizeof(header);
		boost::interprocess::mapped_region m_shm_region;
		void * m_shm_data_buff; // ptr to shared memory for free use
		boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> m_lock;
		uint64_t m_id = 0;
};

#endif // C_TURBOSOCKET_HPP
