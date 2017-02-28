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

// LD_PRELOAD="/home/robert/shared_memory_socket/libpreload.so" ./main 1500

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

#define PORT 12345
#define MESSAGE "hi there"
//#define SERVADDR "127.0.0.1"
//#define SERVADDR "fc72:aa65:c5c2:4a2d:54e:7947:b671:e00c"
#define SERVADDR "::1"

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
/*  if (bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
      perror("bind failed");
      exit(3);
  }
*/
 /* now send a datagram */
while (true) {
  if (sendto(sock, buffer, sizeof(buffer), 0,
             (struct sockaddr *)&server_addr,
	     sizeof(server_addr)) < 0) {
      perror("sendto failed");
      exit(4);
  }
}
/*  printf("waiting for a reply...\n");
//  std::this_thread::sleep_for(std::chrono::seconds(5));
  clilen = sizeof(client_addr);
  if (recvfrom(sock, buffer, 1024, 0,
	       (struct sockaddr *)&client_addr,
               &clilen) < 0) {
      perror("recvfrom failed");
      exit(4);
  }

  printf("got '%s' from %s\n", buffer,
	 inet_ntop(AF_INET6, &client_addr.sin6_addr, addrbuf,
		   INET6_ADDRSTRLEN));
*/
  /* close socket */
  close(sock);
  delete [] buffer;
  return 0;
}

