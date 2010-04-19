/*
 * UDPPlusConnection.cpp
 *
 *  Created on: Apr 19, 2010
 *      Author: asaeed
 */

#include "UDPPlusConnection.h"

UDPPlusConnection::UDPPlusConnection(UDPPlus *mainHandler) {
  this->mainHandler = mainHandler;
}

UDPPlusConnection::~UDPPlusConnection() {
  // TODO Auto-generated destructor stub
}

void UDPPlusConnection::send(void *buf, size_t len, int flags) {
  boost::mutex::scoped_lock l(outBufferMutex);
  while (outItems == outBufferSize)
    outConditionFull.wait(outBufferMutex);
  Packet *currentPacket = new Packet(Packet::SEQ, ++lastSeqNumber, buf, len);
  outBuffer[outBufferBegin + outItems % outBufferSize] = currentPacket;
  outItems++;
  outConditionEmpty.notify_one();

  }
}

void UDPPlusConnection::recv(int s, void *buf, size_t len) {
  boost::mutex::scoped_lock l(inBufferMutex);
  while (inItems == 0);
      inConditionEmpty.wait(inBufferMutex);
  Packet *currentPacket = inBuffer[inBufferBegin];
  inBuffer[inBufferBegin] = 0;
  inBufferBegin = (inBufferBegin + 1) % inBufferSize;
  inItems--;
  currentPacket->getData(buf, len);
  delete currentPacket;
  inConditionFull.notify_one();
}
