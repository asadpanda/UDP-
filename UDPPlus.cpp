/*
 * UDPPlus.cpp
 *
 *  Created on: Apr 15, 2010
 *      Author: Asad Saeed, Adam Darrah
 */

#include "UDPPlus.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

using namespace std;

UDPPlus::UDPPlus() {

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

void UDPPlus::init() {
  inBufferSize = 1024;
  outBufferSize = 1024;
  inBufferBegin = 0;
  inBufferEnd = 0;
  outBufferBegin = 0;
  outBufferEnd = 0;

  sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (sock == -1) {
    fprintf(stderr, "Sorry, could not create socket");
    exit(0);
  }
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
