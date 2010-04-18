/*
 * UDPPlus.cpp
 *
 *  Created on: Apr 15, 2010
 *      Author: Asad Saeed, Adam Darrah
 *
 *      currently only supports a single connection.
 */

#include "UDPPlus.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

using namespace std;

UDPPlus::UDPPlus() {
  inBufferSize = 1024;
  outBufferSize = 1024;
  inBufferBegin = 0;
  inBufferEnd = 0;
  outBufferBegin = 0;
  outBufferEnd = 0;
}

UDPPlus::~UDPPlus() {
  for (unsigned i=0; i<inBufferSize; i++) {
    if (inBuffer[i] != NULL) {
      delete inBuffer[i];
    }
  }
  for (unsigned i=0; i<outBufferSize; i++) {
    if (outBuffer[i] != NULL) {
      delete outBuffer[i];
    }
  }
  delete inBuffer;
  delete outBuffer;
}

void UDPPlus::connect() {
  inBuffer = new Packet*[inBufferSize];
  outBuffer = new Packet*[outBufferSize];
  // build connection information
  uint16_t randomValue = rand() % 65536;
  Packet *current = new Packet(Packet::SEQ | Packet::SYN, randomValue);
  outBuffer[outBufferEnd++] = current;
}

void UDPPlus::close() {
  Packet *current = new Packet(Packet::SEQ | Packet::FIN, lastSeqNum++);
  outBuffer[outBufferEnd++] = current;
  // close connection
}

void UDPPlus::recieve() {
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
