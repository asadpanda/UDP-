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

  this->mainHandler = mainHandler;
  ackWaiting = 0;

  memcpy(&remoteAddress, remote, remoteSize);
  this->remoteAddressLength = remoteSize;
  this->outBufferSize = bufferSize;
  this->inBufferSize = bufferSize;
  inBufferBegin = 0;
  outBufferBegin = 0;
  newAckNum = 0;
  newSeqNum = 0;
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
    srand(time(NULL));
    newSeqNum = rand() % Packet::MAXSIZE;
    //cout << newSeqNum;
    Packet *current = new Packet(Packet::SYN, newSeqNum++, 0);
    current->print();
    outBuffer[(outBufferBegin + outItems) % outBufferSize] = current;
    outItems++;
    send_packet(current);
    currentState = SYN_SENT;
  }
  else {
    incomingConnection->print();
    handlePacket(incomingConnection);
  }

  clock = new boost::thread(boost::bind(&UDPPlusConnection::timer, this));
  timerCondition.notify_one();
  
}

UDPPlusConnection::~UDPPlusConnection() {
  closeConnection();
  clock->join();  //wait for clock to be destroyed.
  //cout << "Destroying Connection";
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
  
  mainHandler->deleteConnection(this);
}

void UDPPlusConnection::closeConnection() {
  boost::mutex::scoped_lock l(sharedMutex);
  if (currentState == FIN_WAIT || currentState == LAST_ACK || currentState == CLOSED || currentState == TIME_WAIT) {
    //cout << "Already Closing" << endl;
    return; //already closing;
  }
  
  cout << "Trying to caputure close lock" << endl;
  cerr << "closing connection" << endl;
  if (currentState == CLOSE_WAIT) {
    currentState = LAST_ACK;
  }
  else {
    currentState = FIN_WAIT;
  }
  
  Packet *temp = new Packet(Packet::FIN | Packet::ACK, newSeqNum++, newAckNum);
  outBuffer[(outBufferBegin + outItems) % outBufferSize] = temp;
  outItems++;
  send_packet(temp);
}

void UDPPlusConnection::timer() {
  bool done = false;
  timeout = milliseconds(1000);
  maximumTimeout = milliseconds(180000);
  time_duration tempTimeout;
  time_duration minTimeout = maximumTimeout; // 3 minutes
  bool connectionTimeout = false;
  int connectionTimeoutCount = 0;
  
  
  boost::mutex::scoped_lock l(sharedMutex);  // change to timerMutex
  while(!done) {
    cerr << "Timer event occurred" << to_simple_string(minTimeout) << endl;
    bool conditionNotified = timerCondition.timed_wait(l, minTimeout);
    ptime currentTime(microsec_clock::universal_time());
    minTimeout = maximumTimeout;

    if (currentState == CLOSED) { break; }
    
    if (connectionTimeout == true && conditionNotified == false) {
      connectionTimeoutCount++;
      if (connectionTimeoutCount >= 2) {
        currentState = CLOSED;
        break;
      }
    }
    
    connectionTimeout = true;
    

    if (currentState == LAST_ACK) {
      closeCondition.notify_all();
      currentState = CLOSED;
      break;
    }
    else {
      if (outBuffer[outBufferBegin] != NULL) {
        if (outBuffer[outBufferBegin]->getTime() + timeout < currentTime) {
          send_packet(outBuffer[outBufferBegin]);
        }
        tempTimeout = outBuffer[outBufferBegin]->getTime() + timeout - currentTime;
        minTimeout = (minTimeout < tempTimeout) ? minTimeout : tempTimeout;
        connectionTimeout = false;
        connectionTimeoutCount = 0;
      }
      if (ackWaiting == 1) {
        if (ackTimestamp + timeout < currentTime) {        
          Packet temp = Packet(Packet::ACK, lowestValidSeq(), newAckNum);
          mainHandler->send_p(&remoteAddress, remoteAddressLength, &temp);
          ackWaiting = 0;
        } else {
          tempTimeout = ackTimestamp + timeout - currentTime;
          minTimeout = (minTimeout < tempTimeout) ? minTimeout : tempTimeout;
          connectionTimeout = false;
          connectionTimeoutCount = 0;
        }
      }
    }
    if (currentState == TIME_WAIT) {
      connectionTimeout = false;
      minTimeout = timeout;
      if (conditionNotified = false) { cerr << "in timewait" << endl; continue; }
      else { currentState = CLOSED; break; }
    }
  }

  cout << "exiting timer" << endl;
  outCondition.notify_all();
  inCondition.notify_all();
  closeCondition.notify_all();
}

void UDPPlusConnection::send_packet(Packet * temp) {
  
  if (temp->sendCount > 10) {
    currentState = CLOSED;
    timerCondition.notify_all();
    inCondition.notify_all();
    outCondition.notify_all();
    closeCondition.notify_all();
    return;
  }
  //cout << "Sending Data Packet" << endl;
  temp->setAckNumber(newAckNum, temp->getField(Packet::ACK));
  temp->updateTime();
  temp->numAck = 0;
  temp->sendCount++;
  if (outItems == 0 && ackWaiting == 0) {
    timerCondition.notify_one();
  }
  mainHandler->send_p(&remoteAddress, remoteAddressLength, temp);
}

const struct sockaddr* UDPPlusConnection::getSockAddr(socklen_t &addrLength) {
  addrLength = this->remoteAddressLength;
  return &remoteAddress;
}

void UDPPlusConnection::handlePacket(Packet *currentPacket) {
  boost::mutex::scoped_lock l(sharedMutex);
  //cout << "Recieved Packet" << endl;
  switch(currentState) {
    case LISTEN:
    {
      //cout << "in LISTEN" << endl;
      if (currentPacket->getField(Packet::SYN)) {
        newAckNum = currentPacket->getSeqNumber();
        //srand(time(NULL));
        //newSeqNum = rand() % Packet::MAXSIZE;
        newSeqNum = 5;
        Packet *current = new Packet(Packet::SYN | Packet::ACK, newSeqNum++, newAckNum++);
        send_packet(current);
        outBuffer[(outBufferBegin + outItems) % outBufferSize] = current;
        outItems++;
        //send packet
        currentState = ESTABLISHED;
        //cout << "connection established" << endl;
        outCondition.notify_all();
        timerCondition.notify_one();
      }
      break;
    }
    case SYN_SENT:
    {
      //cout << "SYN_SENT" << endl;
      if (currentPacket->getField(Packet::SYN | Packet::ACK)) {
        uint16_t ack_num = currentPacket->getAckNumber();
        if (ack_num == newSeqNum) {
          newAckNum = currentPacket->getSeqNumber() + 1;
          
          // sendAck();
          Packet temp = Packet(Packet::ACK, lowestValidSeq(), newAckNum);
          mainHandler->send_p(&remoteAddress, remoteAddressLength, &temp);

          delete outBuffer[outBufferBegin];
          outBuffer[outBufferBegin] = NULL;
          outBufferBegin = (outBufferBegin + 1) % outBufferSize;
          outItems--;
          cout << "connection established";
          currentState = ESTABLISHED;
          outCondition.notify_all();
        }
      }
      delete currentPacket;
      break;
    }
    case ESTABLISHED: cout << "IN ESTABLISHED" << endl;
    case FIN_WAIT: cout << "IN FIN_WAIT" << endl;
    case CLOSE_WAIT: cout << "IN CLOSE_WAIT" << endl;
    {
      if (handleAck(currentPacket)) {
        if ( ! (handleData(currentPacket) || handleFin(currentPacket)) ) {
          delete currentPacket;
        }
      }
      break;
      }
    case LAST_ACK: cout << "IN LAST_ACK" << endl;
    {
      handleAck(currentPacket);
      if (outItems == 0) {
        currentState = CLOSED;
        timerCondition.notify_all();
      }
      delete currentPacket;
      break;
    }
    case TIME_WAIT: cout << "IN TIME_WAIT" << endl;
      handleAck(currentPacket);
      delete currentPacket;
      break;
    case CLOSED: cout << "IN CLOSED" << endl;
      delete currentPacket;
      break;
    default: delete currentPacket;
      break;
  }
 //   if (currentPacket->getHeaderLength != currentPacket->getLength);
}


bool UDPPlusConnection::handleAck(Packet *currentPacket) {
  if (!currentPacket->getField(Packet::ACK)) {
    return false;
  }
  if (outItems == 0)
    return true;
  
  int tempAck = currentPacket->getAckNumber();
  if (tempAck == newSeqNum) {
    lastAckRecv = tempAck;
    releaseBufferTill(newSeqNum);
  }
  else if (tempAck == lastAckRecv) {
    //cerr << outBuffer[outBufferBegin] << endl;
    //cerr << outItems << endl;
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
  int maxLength = 8 * length < (outItems - 1) ? 8 * length : (outItems - 1);

  int counter = 0;
  int index = (outBufferBegin + 1) & outBufferSize;
  for(int i = 0; i < length; i++)
  {
    uint8_t current = 1;
    for(int j = 0; j < 8; j++)
    {
      if (counter >= maxLength) { return true; }
      if (outBuffer[index] == NULL) { return false; }
      if ( (sackRanges[i] & current) != current )
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
  
  int size = (int) ceil((double) (inBufferDelta - 1) / 8);
  uint8_t buf[size];
  memset(&buf, 0, sizeof(buf));
    
  int counter = 1;
  int index = (inBufferBegin + 1) % inBufferSize;
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
  uint16_t bottomAck = newAckNum;
  
  if (currentAckNumber < bottomAck) {
    currentAckNumber += Packet::MAXSIZE;
  }
  if (currentState == CLOSE_WAIT) {
    int tempMaxAckNumber = maxAckNumber;
    if (currentAckNumber > maxAckNumber) { tempMaxAckNumber = maxAckNumber + Packet::MAXSIZE; }
    
    if ((tempMaxAckNumber - currentAckNumber) > inBufferDelta) {
      return false;
    }
  }
      
  int index = currentAckNumber - bottomAck;
  if (index < inBufferSize) {
    index = (index + inBufferBegin) % inBufferSize;
    inBufferDelta = (inBufferDelta > index) ? inBufferDelta : index;
    if (inBuffer[index] != NULL) { delete inBuffer[index]; }
    inBuffer[index] = currentPacket;
    int count = processInBuffer();
    inBufferDelta -=count;
    if (count != 1 || ackWaiting == 1) {
      ackWaiting = 0;
      sendSack();
    } else {
      ackWaiting = 1;
    }
  }
  else {
    Packet temp = Packet(Packet::ACK, lowestValidSeq(), newAckNum);
    mainHandler->send_p(&remoteAddress, remoteAddressLength, &temp);
    return false; }
  return true;
}

bool UDPPlusConnection::handleFin(Packet *currentPacket) {
  if (!currentPacket->getField(Packet::FIN) || currentPacket->getField(Packet::DATA)) {
    return false;
  }
  
  if (!( (currentState == ESTABLISHED) || (currentState == FIN_WAIT)) ) {
    return false;
  }
  
  int currentAckNumber = currentPacket->getSeqNumber();
  if (currentAckNumber < newAckNum) {
    currentAckNumber += Packet::MAXSIZE;
  }
  int index = currentAckNumber - newAckNum;
  if (index < inBufferSize) {
//    if( inBuffer[inBufferBegin + index] != NULL ) {
//      delete inBuffer[inBufferBegin + index];
//    }
    maxAckNumber = (currentAckNumber + 1) % Packet::MAXSIZE;
    if ( currentState == FIN_WAIT ) { currentState = TIME_WAIT; cout << "IN TIME_WAIT" << endl;}
    else { currentState = CLOSE_WAIT; }
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
      inBufferBegin = (inBufferBegin + 1) % inBufferSize;
      if (inBuffer[currentPosition]->getField(Packet::FIN)) {
        if (outItems == 0) {
          if ( currentState == FIN_WAIT ) { currentState = TIME_WAIT; cout << "IN TIME_WAIT" << endl;}
          else { currentState = CLOSE_WAIT; }
          inBuffer[currentPosition] = NULL;
          delete inBuffer[currentPosition];
          inCondition.notify_all();
        }
        done = true;
      }
      else if (inBuffer[currentPosition]->getField(Packet::DATA)) {
        count++;
        inQueue.push_back(inBuffer[currentPosition]);
        inCondition.notify_one();
        inBuffer[currentPosition] = NULL;
      }
      else { // in fin
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

 int UDPPlusConnection::send(const void *buf, size_t len) {
   //cout << "Sending Data" << endl;
   boost::mutex::scoped_lock l(sharedMutex);

   while (currentState == LISTEN || currentState == SYN_SENT || currentState == SYN_RECIEVED) {
     outCondition.wait(l);
   }
   
   switch (currentState) {
     case ESTABLISHED:
     case CLOSE_WAIT:   break;
     case FIN_WAIT:
     case LAST_ACK:
     case TIME_WAIT:
     case CLOSED:       return -1;
     default: break;
   }
         
  if (outItems == outBufferSize)
    outCondition.wait(l);
   
   switch (currentState) {
     case ESTABLISHED:
     case CLOSE_WAIT:   break;
     case FIN_WAIT:
     case LAST_ACK:
     case TIME_WAIT:
     case CLOSED:       return -1;
     default: break;
   }
  
  Packet *currentPacket = new Packet(Packet::DATA | Packet::ACK, newSeqNum++, newAckNum , buf, len);
  send_packet(currentPacket);
  outBuffer[outBufferBegin + outItems % outBufferSize] = currentPacket;
  outItems++;
  return 0;
}

int UDPPlusConnection::recv(void *buf, size_t len) {
  boost::mutex::scoped_lock l(sharedMutex);
  if (currentState == CLOSE_WAIT || currentState == LAST_ACK || currentState == TIME_WAIT || currentState == CLOSED)
    return -1;
 
  if ( inQueue.empty() ) {
    inCondition.wait(l);
  }
  
  if ( !inQueue.empty() ) {
    Packet *currentPacket = inQueue.front();
    inQueue.pop_front();
    currentPacket->getData(buf, len);
    delete currentPacket;
    return 0;
  }
  return -1;
}

void UDPPlusConnection::releaseBufferTill(int newSeqNum) {
  uint16_t init = 0;
  int total = 0;
  if (outBuffer[outBufferBegin] == NULL)
    return;
  
  init = outBuffer[outBufferBegin]->getSeqNumber();
  
  if(init <= newSeqNum) {
    total = newSeqNum - (int)init;
  } else {
    total = ((int) Packet::MAXSIZE + (int) newSeqNum) - ((int)init);
  }
  int bufferLoc = outBufferBegin;
  //cout << "outItems: " << outItems << endl;
  //cout << "total Items: " << total << endl;
  
  for (int i = 0; i < total; i++) {
    //cout << "Releasing Packet " << outBuffer[bufferLoc] << "from output" << endl;
    delete outBuffer[bufferLoc];
    outBufferBegin = (outBufferBegin + 1) % outBufferSize;
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
