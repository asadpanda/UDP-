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
