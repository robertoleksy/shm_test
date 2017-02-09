#ifndef MSG_QUEUE_HEADER_HPP
#define MSG_QUEUE_HEADER_HPP

#include <array>
#include <boost/interprocess/ipc/message_queue.hpp>
#include <chrono>
#include <iostream>
#include "c_counter.hpp"

static constexpr size_t size_of_packet = 9000;
static const size_t size_of_queue = 10000;
static char queue_name[] = "message_queue";
std::array<char, size_of_packet> buffer;

#endif // MSG_QUEUE_HEADER_HPP
