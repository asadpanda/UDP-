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

  srand(time(NULL));
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
  currentState = LISTEN;

  inBuffer = new Packet*[inBufferSize];
  outBuffer = new Packet*[outBufferSize];
  // build connection information


  if (incomingConnection == NULL) {
    newSeqNum = rand() % 65536;
    Packet *current = new Packet(Packet::SYN, newSeqNum++);
    outBuffer[outBufferEnd] = current;
    currentState = SYN_SENT;
   // send(outBufferEnd);
  }
  else {
    handlePacket(incomingConnection);
  }

  clock = new boost::thread(boost::bind(&UDPPlusConnection::timer, this));
}

UDPPlusConnection::~UDPPlusConnection() {
  closeConnection();
  for (int i = 0; i < inBufferSize; i++) {
    if ( inBuffer[i] != NULL) {
      delete i//nBuffer[i];
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

void UDPPlusConnection::close_connection() {
  //close;
}

void UDPPlusConnection::timer() {
  // minimumTimout = maximumTimeout; // 3 minutes
  boost::mutex::scoped_lock l(timerMutex);  // change to timerMutex
  //while(true) {
  timerCondition.timed_wait(l, minTimeout);
  // minimumTimeout = maximumTimeout;
  // if (outBuffer[outBufferBegin] != NULL)
  //     if (tempTimeout = outBuffer[outBufferBegin]->timeStamp + timeout < currentTime) {
  //        resendPacket();
  //     }
  //     tempTimeout = outBuffer[outBufferBegin]->timestamp + timeout - currentTime;
  //     minTimeout = (minTimeout < tempTimeout) ? minTimeout : tempTimeout;
  //     // resend Packet
  // }
  // if (ackWaiting == 1) {
  //   if (ackTimestamp + timeout < currentTime) {
  //    sendAck();
  //    ackWaiting = 0;
  //   }
  //   else {
  //     tempTimeout = outBuffer[outBufferBegin]->timestamp + timeout - currentTime;
  //     minTimeout = (minTimeout < tempTimeout) ? minTimeout : tempTimeout;
  // }
  //
}

const struct sockaddr* UDPPlusConnection::getSockAddr(socklen_t &addrLength) {
  addrLength = this->remoteAddressLength;
  return &remoteAddress;
}

void UDPPlusConnection::handlePacket(Packet *currentPacket) {
  switch(currentState) {
    case LISTEN:
    {
      if (currentPacket->getField(Packet::SYN)) {
        currentPacket->getSeqNumber(newAckNum);
        newSeqNum = rand() % 65536;
        Packet *current = new Packet(Packet::SYN | Packet::ACK, newSeqNum++, newAckNum++);
        boost::mutex::scoped_lock l(outBufferMutex);
        outBuffer[outBufferEnd] = current;
        outBufferEnd = (outBufferEnd + 1) % outBufferSize;
        outItems++;
        //send packet
        currentState = ESTABLISHED;
      }
      break;
    }
    case SYN_SENT:
    {
      if (currentPacket->getField(Packet::SYN | Packet::ACK)) {
        uint16_t ack_num;
        currentPacket->getAckNumber(ack_num);
        if (ack_num == newSeqNum) {
          currentPacket->getSeqNumber(newAckNum);
          newAckNum++;
          boost::mutex::scoped_lock l(outBufferMutex);
            // sendAck();
          delete outBuffer[outBufferBegin];
          outBuffer[outBufferBegin] = NULL;
          outBufferBegin = (outBufferBegin + 1) % outBufferSize;
          outItems--;
          currentState = ESTABLISHED;
        }
      }
      delete currentPacket;
      break;
    }
    case ESTABLISHED:
    {
      handleEstablished(currentPacket);
      break;
    }
    case FIN_WAIT1: {
        // fill holes, no extra packets after in
        // wait for finack
    }
    case FIN_WAIT2: {
        // fill holes, no extra packets out
        // finack recieved
    }
    case CLOSE_WAIT:
        // holes filled, send fin;
    case CLOSING:
    case LAST_ACK:
    case TIME_WAIT:
    case CLOSED:
  }
 //   if (currentPacket->getHeaderLength != currentPacket->getLength);
}

void UDPPlusConnection::handleEstablished(Packet *currentPacket) {
  uint16_t tempAck;
  uint16_t tempSeq;
  if (currentPacket->getAckNumber(tempAck)) {
    uint16_t tempAckUpperBound = tempAck + outBufferSize;
    if (tempAck == newSeqNum) {
      lastAckRecv = tempAck;
      //releaseBufferTill(newSeqNum);
      
    } else if (tempAck == lastAckRecv) {
      numAck++;
      if (numAck >= 3) { // triplicateAck
        numAck = 0;
          // resend Packets
      }
    } else if ( checkIfAckable(tempAck) ) {
        //releaseBufferTill(tempAck);
    }
  }
  if (currentPacket->getField(Packet::FIN)) {
    currentState = CLOSE_WAIT;
    Packet *current = new Packet(Packet::FIN | Packet::ACK, newSeqNum++, newAckNum++);
  } // no items should be sendable now
  if (currentPacket->getField(Packet::DATA)) {
    boost::mutex::scoped_lock l(inBufferMutex);
    if (inItems == inBufferSize) {
      delete currentPacket;
        //discard Packet
    }
    else {
      inBuffer[inBufferEnd] = currentPacket;
      inBufferEnd = (inBufferEnd + 1) % inBufferSize;
    }
  }
  else {
    delete currentPacket;
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

bool UDPPlusConnection::checkIfAckable(const uint16_t &ackNumber) {
  int currentAckNumber = ackNumber;
  uint16_t bottomAck;
  outBuffer[outBufferBegin]->getSeqNumber(bottomAck);
  if ( (newSeqNum - 1) < bottomAck) {
    if ( ((bottomAck <  currentAckNumber) && (currentAckNumber < 65536)) ||
        ((0 <= currentAckNumber) && (currentAckNumber <= (int) newSeqNum)) )
    {
      return true;
    }
  }
  else if ( ((bottomAck < ackNumber) && (ackNumber <= newSeqNum))) {
    return true;
  }
  return false;
}
