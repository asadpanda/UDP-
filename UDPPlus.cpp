/*
 * UDPPlus.cpp
 *
 *  Created on: Apr 15, 2010
 *      Author: Asad Saeed, Adam Darrah
 */

#include "UDPPlus.h"

UDPPlus::UDPPlus() {
  ackedLocation = 0;
  lastSentLocation = 0;
  packetBufferSize = 1024;
  packetBuffer = new Packet*[packetBufferSize];
}

UDPPlus::~UDPPlus() {
  for (unsigned i=0; i<packetBufferSize; i++) {
    if (packetBuffer[i] != NULL) {
      delete packetBuffer[i];
    }
  }
  delete packetBuffer;
}

int UDPPlus::getaddr() {
  // make sure hints struct is empty
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AI_PASSIVE;   // compatible with IPv4 and IPv6
  hints.as_socktype = SOCK_DGRAM; // UDP stream sockets
  hints.ai_flags = AI_PASSIVE;    // use localhost
  
  return getaddrinfo(NULL, "30000", &hints, &res);
}

int UDPPlus::getsocket() {
  return socket(res->ai_family, res->ai_socktype, res->ai_protocol);
}

int UDPPlus::bind_p (int sockfd) {
  return bind(sockfd, res->ai_addr, res->ai_addrlen);
}
