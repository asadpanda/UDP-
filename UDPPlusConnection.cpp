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
  inBufferDelta = 0;
  outItems = 0;
  numAck = 0;
  lastAckRecv = 0;
  currentState = LISTEN;
  maxAckNumber = -1;

  inBuffer = new Packet*[inBufferSize];
  outBuffer = new Packet*[outBufferSize];
  // build connection information

  // initialize all slots in inBuffer/outBuffer to null
  for (int i = 0; i < inBufferSize; i++) {
    inBuffer[i] = outBuffer[i] = NULL;
  }

  if (incomingConnection == NULL) {
    newSeqNum = rand() % Packet::MAXSIZE;
    Packet *current = new Packet(Packet::SYN, newSeqNum++, 0);
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
  if (currentState > ESTABLISHED)
    return; //already closing;
  
}

void UDPPlusConnection::timer() {
  time_duration minimumTimeout = maximumTimeout; // 3 minutes
  time_duration tempTimeout;
  time_duration minTimeout;
  ptime currentTime(microsec_clock::universal_time());
  boost::mutex::scoped_lock l(sharedMutex);  // change to timerMutex
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
        Packet temp = Packet(Packet::ACK, lowestValidSeq(), newAckNum);
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
  temp->numAck = 0;
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
      if (handleAck(currentPacket)) {
        handleData(currentPacket);
      }
      else {
        handleFin(currentPacket);
        currentState = CLOSE_WAIT;
      }

      break;
    }
    case FIN_WAIT1: {
        // fill holes, no extra packets after in
        // wait for finack
    }
    case FIN_WAIT2: {
      // wait for finack
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


bool UDPPlusConnection::handleAck(Packet *currentPacket) {
  if (!currentPacket->getField(Packet::ACK)) {
    return false;
  }
    int tempAck = currentPacket->getAckNumber();
    if (tempAck == newSeqNum) {
      lastAckRecv = tempAck;
      releaseBufferTill(newSeqNum);
    }
    else if (tempAck == lastAckRecv) {
      outBuffer[outBufferBegin]->numAck++;
      if (outBuffer[outBufferBegin]->numAck >= 3) { // triplicateAck
        outBuffer[outBufferBegin]->numAck++;
        send_packet(outBuffer[outBufferBegin]);
      }
      handleSack(currentPacket);
    }
    else if ( checkIfAckable(tempAck) ) {
      lastAckRecv = tempAck;
      numAck = 0;
      releaseBufferTill(tempAck);
      handleSack(currentPacket);
    }
  return true;
}

bool UDPPlusConnection::handleSack(Packet *currentPacket) {
  if (!currentPacket->getField(Packet::OPT)) {
    return false;
  }
  int length = (currentPacket->getHeaderLength() - Packet::DEFAULTHEADERSIZE);
  uint8_t sackRanges[length];// = new uint16_t[optLength/4][2];
    
  currentPacket->getOptField(&sackRanges, sizeof(sackRanges));
  int maxLength = 8 * length < outItems ? 8 * length : outItems;

  int counter = 0;
  int index = outBufferBegin;
  for(int i = 0; i < length; i++)
  {
    uint8_t current = 1;
    for(int j = 0; j < 8; j++)
    {
      if (counter >= maxLength) { return true; }
      if (outBuffer[index] == NULL) { return false; }
      if ( sackRanges[i] & current != current )
      {
        if (outBuffer[index]->numAck == 3)
          send_packet(outBuffer[index]);
        else
          outBuffer[index]->numAck++;
        current = current << 1;
      }
      else { outBuffer[index]->numAck = -1; }
      index = (index + 1) % outBufferSize;
      counter++;
    }
  }
  return true;  
}

void UDPPlusConnection::sendSack() {
  if (inBufferDelta == 0) {
    Packet temp = Packet(Packet::ACK, lowestValidSeq(), newAckNum);
    mainHandler->send_p(&remoteAddress, remoteAddressLength, &temp);
    return;
  }
  
  int size = ceil((double) inBufferDelta / 8);
  uint8_t buf[size];
  memset(&buf, 0, sizeof(buf));
    
  int counter = 0;
  int index = inBufferBegin;
  for(int i = 0; i < size; i++)
  {
    uint8_t current = 1;
    for(int j = 0; j < 8; j++)
    {
      if (counter >= inBufferDelta) { break; }
      if (inBuffer[index] == NULL) { }
      else
      {
        buf[i] |= current;
      }
      current = current << 1;
      index = (index + 1) % outBufferSize;
      counter++;
    }
  }
  Packet temp = Packet(Packet::ACK | Packet::OPT, lowestValidSeq(), newAckNum, &buf, sizeof(buf));
  mainHandler->send_p(&remoteAddress, remoteAddressLength, &temp);}

bool UDPPlusConnection::handleData(Packet *currentPacket) {
  if (!(currentPacket->getField(Packet::DATA) || currentPacket->getField(Packet::FIN))) {
    return false;
  }
  
  int currentAckNumber = currentPacket->getSeqNumber();
  uint16_t bottomAck = newAckNum - 1;
  
  if (currentAckNumber < bottomAck) {
    currentAckNumber += Packet::MAXSIZE;
  }
  int index = currentAckNumber - bottomAck;
  if (index < inBufferSize) {
    index = (index + inBufferBegin) % inBufferSize;
    inBufferDelta = (inBufferDelta > index) ? inBufferDelta : index;
    if (inBuffer[index] != NULL) { delete inBuffer[index]; }
    else {inItems++;}
    inBuffer[index] = currentPacket;
    int count = processInBuffer();
    inItems -= count;
    inBufferDelta -=count;
    if (count != 1 || ackWaiting == 1) {
      ackWaiting = 0;
      sendSack();
    } else {
      ackWaiting = 1;
    }
  }
  else { return false; }
  return true;
}

bool UDPPlusConnection::handleFin(Packet *currentPacket) {
  if (!currentPacket->getField(Packet::FIN) || currentPacket->getField(Packet::DATA)) {
    return false;
  }
  int currentAckNumber = currentPacket->getSeqNumber();
  if (currentAckNumber < newAckNum) {
    currentAckNumber += Packet::MAXSIZE;
  }
  int index = currentAckNumber - newAckNum;
  if (index < inBufferSize) {
    if( inBuffer[inBufferBegin + index] != NULL ) {
      delete inBuffer[inBufferBegin + index];
    }
    inBuffer[inBufferBegin + index] = currentPacket;
    maxAckNumber = currentAckNumber;
    return true;
  }
  return false;
}
int UDPPlusConnection::processInBuffer() {
  bool done = false;
  int count = 0;
  int currentPosition = inBufferBegin;
  while ( !done ) {
    if (inBuffer[currentPosition] != NULL) {
      newAckNum = inBuffer[currentPosition]->getSeqNumber() + 1;
      if (inBuffer[currentPosition]->getField(Packet::FIN)) {
        if (outItems == 0) {
          Packet *temp = new Packet(Packet::FIN, newSeqNum++, newAckNum);
          outBuffer[outBufferBegin] = temp;
          outItems++;
          send_packet(temp);
          currentState = CLOSE_WAIT;
          inBuffer[currentPosition] = NULL;
          delete inBuffer[currentPosition];
        }
        done = true;
      }
      else if (inBuffer[currentPosition]->getField(Packet::DATA)) {
        count++;
        inQueue.push_back(inBuffer[currentPosition]);
        inBuffer[currentPosition] = NULL;
      }
      else {
        inBuffer[currentPosition] = NULL;
        delete inBuffer[currentPosition];
      }
    }
    else {
      done = true;
    }
    currentPosition = (currentPosition + 1) % inBufferSize;
  }
  return count;
}

void UDPPlusConnection::send(void *buf, size_t len, int flags) {
  boost::mutex::scoped_lock l(sharedMutex);
  while (outItems == outBufferSize)
    outConditionFull.wait(l);
  Packet *currentPacket = new Packet(Packet::DATA | Packet::ACK, newSeqNum++, newAckNum , buf, len);
  outBuffer[outBufferBegin + outItems % outBufferSize] = currentPacket;
  mainHandler->send_p(&remoteAddress, remoteAddressLength, currentPacket);
  if (outItems == 0 && ackWaiting == 0) {
    timerCondition.notify_one();
  }
  outItems++;
}

void UDPPlusConnection::recv(int s, void *buf, size_t len) {
  boost::mutex::scoped_lock l(sharedMutex);
  while (inQueue.empty());
      inConditionEmpty.wait(l);
  Packet *currentPacket = inQueue.front();
  inQueue.pop_front();
  currentPacket->getData(buf, len);
  delete currentPacket;
}

void UDPPlusConnection::releaseBufferTill(int newSeqNum) {
  uint16_t init = 0;
  int total = 0;
  init = outBuffer[outBufferBegin]->getSeqNumber();
  
  if(init < newSeqNum) {
    total = (outBufferSize + (int) newSeqNum) - ((int)init);
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

uint16_t UDPPlusConnection::lowestValidSeq() {
  if (outBuffer[outBufferBegin] != NULL) {
    return outBuffer[outBufferBegin]->getSeqNumber();
  }
  return newSeqNum - 1;
}


bool UDPPlusConnection::checkIfAckable(const uint16_t &ackNumber) {
  int currentAckNumber = ackNumber;
  uint16_t tempAck = outBuffer[outBufferBegin]->getSeqNumber();
  int bottomAck = tempAck;

  if (currentAckNumber < bottomAck)
    currentAckNumber += Packet::MAXSIZE;
  
  if (outItems < (currentAckNumber - bottomAck))
    return true;
  return false;
}
