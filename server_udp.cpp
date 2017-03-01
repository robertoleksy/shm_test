/*
  Example IPv6 UDP client.
  Copyright (C) 2010 Russell Bradford

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License (http://www.gnu.org/copyleft/gpl.html)
  for more details.
*/

// LD_PRELOAD="/home/robert/shared_memory_socket/libpreload.so" ./server 1500

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <chrono>
#include <thread>
#include <iostream>

#define PORT 12345
#define MESSAGE "hi there"
//#define SERVADDR "127.0.0.1"
#define SERVADDR "::1"

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

void c_counter::reset() {
	m_data_in_bytes = 0;
	m_start_point = std::chrono::steady_clock::now();
}

/***************************************************************************************/
int main(int argc, char* argv[]) {
  int sock;
  socklen_t clilen;
  struct sockaddr_in6 server_addr, client_addr;
  size_t packet_size = std::stol(argv[1]);
  char *buffer = new char[packet_size];
  char addrbuf[INET6_ADDRSTRLEN];

  /* create a DGRAM (UDP) socket in the INET6 (IPv6) protocol */
  sock = socket(PF_INET6, SOCK_DGRAM, 0);

  if (sock < 0) {
    perror("creating socket");
    exit(1);
  }

  /* create server address: where we want to send to */

  /* clear it out */
  memset(&server_addr, 0, sizeof(server_addr));

  /* it is an INET address */
  server_addr.sin6_family = AF_INET6;

  /* the server IP address, in network byte order */
  inet_pton(AF_INET6, SERVADDR, &server_addr.sin6_addr);

  /* the port we are going to send to, in network byte order */
  server_addr.sin6_port = htons(PORT);

  /* bind */
  if (bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
      perror("bind failed");
      exit(3);
  }

size_t received_size = 0;

c_counter counter;
auto last_dbg_time = std::chrono::steady_clock::now();
counter.start();

while (true) {
//  printf("waiting for a reply...\n");
  clilen = sizeof(client_addr);
  ssize_t recv_size = recvfrom(sock, buffer, packet_size, 0,
	       (struct sockaddr *)&client_addr,
               &clilen);
   if (recv_size < 0) {
      perror("recvfrom failed");
      exit(4);
  }
//   std::cout << "recv " << recv_size << " bytes\n";
	counter.add_data_size(recv_size);
	auto now = std::chrono::steady_clock::now();
	if (std::chrono::duration_cast<std::chrono::seconds>(now - last_dbg_time).count() > 10) {
			std::cout << counter.get_speed_in_Mbps() << " Mbps for packet size " << packet_size << std::endl;
			last_dbg_time = std::chrono::steady_clock::now();
			counter.reset();
			break;
	}

} // while
  close(sock);
  delete [] buffer;
  return 0;
}

