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
  outBufferBegin = 0;
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
    newSeqNum = rand() % Packet::MAXSIZE;
    Packet *current = new Packet(Packet::SYN, newSeqNum++);
    outBuffer[(outBufferBegin + outItems) % outBufferSize] = current;
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
        send_packet(outBuffer[outBufferBegin]);
      }
      tempTimeout = outBuffer[outBufferBegin]->getTime() + timeout - currentTime;
      minTimeout = (minimumTimeout < tempTimeout) ? minimumTimeout : tempTimeout;
    }
    if (ackWaiting == 1) {
      if (ackTimestamp + timeout < currentTime) {
        uint16_t lowestValidSeq = outBuffer[outBufferBegin]->getSeqNumber();
        
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

void UDPPlusConnection::send_packet(Packet * temp) {
  temp->setAckNumber(newAckNum);
  temp->updateTime();
  mainHandler->send_p(&remoteAddress, remoteAddressLength, temp);
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
        newAckNum = currentPacket->getSeqNumber();
        newSeqNum = rand() % Packet::MAXSIZE;
        Packet *current = new Packet(Packet::SYN | Packet::ACK, newSeqNum++, newAckNum++);
        boost::mutex::scoped_lock l(outBufferMutex);
        outBuffer[(outBufferBegin + outItems) % outBufferSize] = current;
        outItems++;
        //send packet
        send_packet(current);
        currentState = ESTABLISHED;
      }
      break;
    }
    case SYN_SENT:
    {
      if (currentPacket->getField(Packet::SYN | Packet::ACK)) {
        uint16_t ack_num = currentPacket->getAckNumber();
        if (ack_num == newSeqNum) {
          newAckNum = currentPacket->getSeqNumber() + 1;
          boost::mutex::scoped_lock l(outBufferMutex);
          
          // sendAck();
          uint16_t lowestValidSeq = outBuffer[outBufferBegin]->getSeqNumber();
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
  if (currentPacket->getField(Packet::ACK)) {
    int tempAck = currentPacket->getAckNumber();
    if (tempAck == newSeqNum) {
      lastAckRecv = tempAck;
      releaseBufferTill(newSeqNum);
      
    }
    else if (tempAck == lastAckRecv) {
      numAck++;
      if (numAck >= 3) { // triplicateAck
        numAck = 0;
        send_packet(outBuffer[outBufferBegin]);
      }
    }
    else if ( checkIfAckable(tempAck) ) {
        lastAckRecv = tempAck;
        numAck = 0;
        releaseBufferTill(tempAck);
    }
  }

  if (currentPacket->getField(Packet::DATA)) {
    boost::mutex::scoped_lock l(inBufferMutex);
    
    int currentAckNumber = currentPacket->getSeqNumber();
    uint16_t bottomAck = newAckNum - 1;
    
    if (currentAckNumber < bottomAck) {
      currentAckNumber + Packet::MAXSIZE;
    }
    int index = currentAckNumber - bottomAck;
    if (index < inBufferSize) {
      index = (index + inBufferBegin) % inBufferSize;
      if (inBuffer[index] != NULL) { delete currentPacket; }
      inBuffer[index] = currentPacket;
      processInBuffer();
    } else { delete currentPacket; }  
  }
  if (currentPacket->getField(Packet::FIN)) {
    currentState = CLOSE_WAIT;
    Packet *current = new Packet(Packet::FIN | Packet::ACK, newSeqNum++, newAckNum++);
  } // no items should be sendable now
  else {
    delete currentPacket;
  }
}

void UDPPlusConnection::processInBuffer() {
  bool done = false;
  int currentPosition = inBufferBegin;
  while ( !done ) {
    if (inBuffer[currentPosition] != NULL) {
      inQueue.push_back(inBuffer[currentPosition]);
      inBuffer[currentPosition] = NULL;
    }
    else {
      done = true;
    }
    currentPosition = (currentPosition + 1) % inBufferSize;
  }
}

void UDPPlusConnection::send(void *buf, size_t len, int flags) {
  boost::mutex::scoped_lock l(outBufferMutex);
  while (outItems == outBufferSize)
    outConditionFull.wait(l);
  Packet *currentPacket = new Packet(Packet::DATA | Packet::ACK, newSeqNum++, newAckNum , buf, len);
  outBuffer[outBufferBegin + outItems % outBufferSize] = currentPacket;
  mainHandler->send_p(&remoteAddress, remoteAddressLength, currentPacket);
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
  init = outBuffer[outBufferBegin]->getSeqNumber();
  
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
  uint16_t tempAck = outBuffer[outBufferBegin]->getSeqNumber();
  int bottomAck = tempAck;

  if (currentAckNumber < bottomAck)
    currentAckNumber + Packet::MAXSIZE;
  
  if (outItems < (currentAckNumber - bottomAck))
    return true;
  return false;
}
