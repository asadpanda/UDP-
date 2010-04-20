/*
 * UDPPlusConnection.cpp
 *
 *  Created on: Apr 19, 2010
 *      Author: asaeed
 */

#include "UDPPlusConnection.h"

UDPPlusConnection::UDPPlusConnection(UDPPlus *mainHandler,
    const struct sockaddr *remote,
    const socklen_t &remoteSize,
    int &bufferSize,
    Packet *incomingConnection) {

  this->mainHandler = mainHandler;
  timeout = milliseconds(500);

  memcpy(&remoteAddress, remote, remoteSize);
  this->remoteAddressLength = remoteSize;
  this->outBufferSize = bufferSize;
  this->inBufferSize = bufferSize;
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
   // send(outBufferEnd);
  }
  else {
    currentState = SYN_RECIEVED;
    handlePacket(incomingConnection);
  }

  clock = new boost::thread(boost::bind(&UDPPlusConnection::timer, this));
}

UDPPlusConnection::~UDPPlusConnection() {
  close();
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

void UDPPlusConnection::close() {
  //close;
}

void UDPPlusConnection::timer() {
  boost::mutex::scoped_lock l(ackMutex);
  timerCondition.timed_wait(l, timeout);
  // timer
}

const struct sockaddr* UDPPlusConnection::getSockAddr(socklen_t &addrLength) {
  addrLength = this->remoteAddressLength;
  return &remoteAddress;
}
void UDPPlusConnection::handlePacket(Packet *currentPacket) {
  if (currentState == SYN_SENT) {
    if (currentPacket->getField(Packet::SYN | Packet::ACK)) {
			uint16_t ack_num;
			currentPacket->getAckNumber(ack_num);
      if (ack_num == newSeqNum) {
				uint16_t newAckNumber;
        currentPacket->getSeqNumber(newAckNumber);
        currentState == ESTABLISHED;
      }
    }
    delete currentPacket;
  }
  else if (currentState == ESTABLISHED) {
    uint16_t tempAck;
    uint16_t tempSeq;
    if (currentPacket->getAckNumber(tempAck)) {
      if (tempAck == newSeqNum) {
        lastAckRecv = tempAck;
        numAck = 0;
      } else if (tempAck == lastAckRecv) {
        numAck++;
        if (numAck >= 3) { // triplicateAck
          numAck = 0;
          // resend Packets
        }
      } else if ( tempAck > lastAckRecv  ) {
      }
    }
 //   if (currentPacket->getHeaderLength != currentPacket->getLength);
  }
}

void UDPPlusConnection::send(void *buf, size_t len, int flags) {
  boost::mutex::scoped_lock l(outBufferMutex);
  while (outItems == outBufferSize)
    outConditionFull.wait(l);
  Packet *currentPacket = new Packet(Packet::DATA, newSeqNum++, buf, len);
  outBuffer[outBufferBegin + outItems % outBufferSize] = currentPacket;
  outItems++;
  outConditionEmpty.notify_one();
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
