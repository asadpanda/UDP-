/*
 * UDPPlusConnection.cpp
 *
 *  Created on: Apr 19, 2010
 *      Author: asaeed
 */

#include "UDPPlusConnection.h"
#include "UDPPlus.h"

UDPPlusConnection::UDPPlusConnection(UDPPlus *mainHandler,
    const struct sockaddr *remote,
    const socklen_t &remoteSize,
    int &bufferSize,
    Packet *incomingConnection) {

  srand(time(NULL));
  this->mainHandler = mainHandler;
  timeout = milliseconds(500);
  ackWaiting = 0;
  maximumTimeout = milliseconds(180000);

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

  // initialize all slots in inBuffer/outBuffer to null
  for (int i = 0; i < inBufferSize; i++) {
    inBuffer[i] = outBuffer[i] = NULL;
  }

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

void UDPPlusConnection::closeConnection() {
  //close;
}

void UDPPlusConnection::timer() {
  time_duration minimumTimeout = maximumTimeout; // 3 minutes
  time_duration tempTimeout;
  time_duration minTimeout;
  ptime currentTime(microsec_clock::universal_time());
  boost::mutex::scoped_lock l(timerMutex);  // change to timerMutex
  while(true) {
    timerCondition.timed_wait(l, minimumTimeout);
    minimumTimeout = maximumTimeout;
    if (outBuffer[outBufferBegin] != NULL) {
      if (outBuffer[outBufferBegin]->getTime() + timeout < currentTime) {
        outBuffer[outBufferBegin]->setAckNumber(newAckNum);
        mainHandler->send_p(&remoteAddress, remoteAddressLength, outBuffer[outBufferEnd]);
      }
      tempTimeout = outBuffer[outBufferBegin]->getTime() + timeout - currentTime;
      minTimeout = (minimumTimeout < tempTimeout) ? minimumTimeout : tempTimeout;
      outBuffer[outBufferBegin]->setAckNumber(newAckNum);
      mainHandler->send_p(&remoteAddress, remoteAddressLength, outBuffer[outBufferEnd]);
    }
    if (ackWaiting == 1) {
      if (ackTimestamp + timeout < currentTime) {
        uint16_t lowestValidSeq;
        outBuffer[outBufferBegin]->getSeqNumber(lowestValidSeq);
        
        Packet temp = Packet(Packet::ACK, lowestValidSeq, newAckNum);
        mainHandler->send_p(&remoteAddress, remoteAddressLength, &temp);
        
        ackWaiting = 0;
      } else {
        tempTimeout = outBuffer[outBufferBegin]->getTime() + timeout - currentTime;
        minTimeout = (minimumTimeout < tempTimeout) ? minimumTimeout : tempTimeout;
      }
    }
  }
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
        mainHandler->send_p(&remoteAddress, remoteAddressLength, current);
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
          Packet temp = Packet(Packet::ACK, lowestValidSeq, newAckNum);
          mainHandler->send_p(&remoteAddress, remoteAddressLength, &temp);

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
      break;
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
        sendAck()
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
  Packet *currentPacket = new Packet(Packet::DATA | Packet::ACK, newSeqNum++, newAckNum , buf, len);
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

void UDPPlusConnection::releaseBufferTill(int newSeqNum) {
  uint16_t init = 0;
  int total = 0;
  outBuffer[outBufferBegin]->getSeqNumber(init);
  
  if(init < newSeqNum) {
    total = ((int)init + outBufferSize) - newSeqNum;
  } else {
    total = newSeqNum - (int)init;
  }
  total--;
  int bufferLoc = outBufferBegin;
  
  for (int i = 0; i < total; i++) {
    delete outBuffer[bufferLoc];
    outItems--;
    outBuffer[bufferLoc] = NULL;
    bufferLoc = (bufferLoc + 1) % outBufferSize;
  }
  
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
