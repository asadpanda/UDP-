/*
 * UDPPlusConnection.cpp
 *
 *  Created on: Apr 19, 2010
 *      Author: asaeed
 */

#include "UDPPlusConnection.h"

UDPPlusConnection::UDPPlusConnection(UDPPlus *mainHandler, struct sockaddr remote, int bufferSize, Packet *incomingConnection) {
  this->mainHandler = mainHandler;
  this->remote = remote;
  this->outbufferSize = bufferSize;
  this->inbufferSize = bufferSize;
  inBufferBegin = 0;
  inBufferEnd = 0;
  outBufferBegin = 0;
  outBufferEnd = 0;
  newAckNum = 0;
  newSeqNum = 0;
  inItems = 0;
  outItems = 0;
  numAck = 0;
  lastAckRecv = 0;

  inBuffer = new Packet*[inBufferSize];
  outBuffer = new Packet*[outBufferSize];
  // build connection information
  uint16_t randomValue = rand() % 65536;

  if (incomingConnection == NULL) {
    Packet *current = new Packet(Packet::SYN, randomValue);
    outBuffer[outBufferEnd] = current;
    currentState = SYN_SENT;
    send(outBufferEnd);
  }
  else {
    currentState = SYN_RECIEVED;
    handlePacket(incomingConnection);
  }
}

UDPPlusConnection::~UDPPlusConnection() {
  for (int i = 0; i < inBufferSize; i++) {
    if ( inBuffer[i] != NULL) {
      delete inBuffer[i];
    }
  }
  for (int i = 0; i < outBufferSize; i++) {
    if ( outBuffer[i] != NULL) {
      delete outBuffer[i];
    }
  }
  delete inBuffer;
  delete outBuffer;
}

void UDPPlusConnection::handlePacket(Packet *currentPacket) {
  if (currentState == SYN_SENT)
  if (currentState == SYN_SENT) {
    if (currentPacket->getField(Packet::SYN | Packet::ACK)) {
      if (currentPacket->getAckNumber() == newSeqNum) {
        newAckNumber = currentPacket->getSeqNumber();
        currentState == ESTABLISHED;
      }
    }
    delete Packet;
  }
  else if (currentState == ESTABLISHED) {
    uint16_t tempAck;
    uint16_t tempSeq;
    if (currentPacket->getAckNumber(tempAck)) {
      if (tempAck == newSeqNum) {
        lastAckRecv = tempAck;
        numAck = 0;
      } else if (tempAck == lastRecvAck) {
        numAck++;
        if (numAck >= 3) { // triplicateAck
          numAck = 0;
          // resend Packets
        }
      } else if ( tempAck > lastRecvAck  ) {
      }
    }
 //   if (currentPacket->getHeaderLength != currentPacket->getLength);
  }
}

void UDPPlusConnection::send(void *buf, size_t len, int flags) {
  boost::mutex::scoped_lock l(outBufferMutex);
  while (outItems == outBufferSize)
    outConditionFull.wait(l);
  Packet *currentPacket = new Packet(Packet::SEQ, ++lastSeqNumber, buf, len);
  outBuffer[outBufferBegin + outItems % outBufferSize] = currentPacket;
  outItems++;
  outConditionEmpty.notify_one();
  }
}

void UDPPlusConnection::recv(int s, void *buf, size_t len) {
  boost::mutex::scoped_lock l(inBufferMutex);
  while (inItems == 0);
      inConditionEmpty.wait(l);
  Packet *currentPacket = inBuffer[inBufferBegin];
  inBuffer[inBufferBegin] = 0;
  inBufferBegin = (inBufferBegin + 1) % inBufferSize;
  inItems--;
  currentPacket->getData(buf, len);
  delete currentPacket;
  inConditionFull.notify_one();
}
