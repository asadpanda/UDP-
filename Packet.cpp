/*
 * Packet.cpp
 *
 *  Created on: Apr 15, 2010
 *      Authors: Asad Saeed, Adam Darrah
 */

#include "Packet.h"
#include "utility.h"

Packet::Packet(const void *buffer, size_t length) {
  this->length = length;
  this->buffer = new char[length];
  memcpy(this->buffer, buffer, length);
}

Packet::Packet(const Packet &original) {
  numAck = original.numAck;
  sendCount = original.sendCount;
  buffer = new char[original.length];
  length = original.length;
  memcpy(buffer, original.buffer, length);
}


Packet::Packet(uint8_t field, uint16_t seqNumber, uint16_t ackNumber, const void *firstBuffer,
    size_t firstBufferLength, const void *secondBuffer, size_t secondBufferLength)
{
  sendCount = 0;
  numAck = 0;
  buffer = new char[DEFAULTHEADERSIZE + firstBufferLength + secondBufferLength];
  length = DEFAULTHEADERSIZE + firstBufferLength + secondBufferLength;

  clear();
  setField(field);
  setSeqNumber(seqNumber);
  setAckNumber(ackNumber, getField(ACK));
  if ( getField(OPT) ) {
    setHeaderLength(DEFAULTHEADERSIZE + firstBufferLength);
  }

  if ( firstBuffer != NULL ) {
    memcpy(buffer + DEFAULTHEADERSIZE, firstBuffer, firstBufferLength);
  }
  if (secondBuffer != NULL) {
    memcpy(buffer + DEFAULTHEADERSIZE + firstBufferLength, secondBuffer, secondBufferLength);
  }

}

void Packet::insert_uint16_t(uint16_t number, void *location) {
  number = htons(number);
  memcpy(location, &number, sizeof(uint16_t));
}

Packet::~Packet() {
  delete buffer;
}

void Packet::clear() {
  memset(buffer, 0, length);
  buffer[1] = DEFAULTHEADERSIZE;
}

bool Packet::getField(uint8_t field) {
  return ((field & buffer[0]) == field);
}

void Packet::setField(uint8_t field, bool value) {
  if (value) {
    buffer[0] |= field;
  }
  else {
    buffer[0] &= (~field);
  }
}

uint16_t Packet::getSeqNumber() {
  uint16_t seqNumber;
  memcpy(&seqNumber, buffer + SEQLOCATION, sizeof(uint16_t));
  return( ntohs(seqNumber) );
}

uint16_t Packet::getAckNumber() {
  uint16_t ackNumber;
  memcpy(&ackNumber, buffer + ACKLOCATION, sizeof(uint16_t));
  return( ntohs(ackNumber) );
}

void Packet::setSeqNumber(uint16_t seqNumber, bool shouldSet) {
  if (shouldSet) {
    insert_uint16_t(seqNumber, buffer + SEQLOCATION);
  }
}

void Packet::setAckNumber(uint16_t ackNumber, bool shouldSet) {
  if (shouldSet) {
    setField(ACK);
    insert_uint16_t(ackNumber, buffer + ACKLOCATION);
  }
}

size_t Packet::getOptField(void *optBuffer, size_t optBufferLength) {
  size_t optLength = getHeaderLength() - DEFAULTHEADERSIZE;

  if (optLength > optBufferLength)
    optLength = optBufferLength;

  memcpy(optBuffer, buffer + DEFAULTHEADERSIZE, optLength );

  return optLength;
}

// header length is default 1
uint8_t Packet::getHeaderLength() {
  return buffer[1];
}

void Packet::setHeaderLength(uint8_t headerLength) {
  buffer[1] = headerLength;
}

size_t Packet::getData(void *outBuffer, size_t outBufferLength) {
  size_t headerLength = getHeaderLength();
  size_t dataLength = (length - headerLength);

  // bound check
  if (dataLength > outBufferLength)
    dataLength = outBufferLength;

  memcpy(outBuffer, (buffer + headerLength), dataLength);
  return dataLength;
}

size_t Packet::getLength() {
  return length;
}

void Packet::updateTime() {
  sendingTime = boost::posix_time::microsec_clock::universal_time();
}

ptime Packet::getTime() {
  return sendingTime;
}

char* Packet::getBuffer() {
  return buffer;
}

void Packet::print() {
  char buf[2048];
  cout << "-----Sequence#:" << getSeqNumber() << " Acknowledgment#:" << getAckNumber() << "------" << endl;
  cout << "\t\tDATA:" << getField(DATA) << " ACK:" << getField(ACK) << " SYN:" << getField(SYN) << " FIN:" << getField(FIN) << " OPT:" << getField(OPT) << endl;
  if (getField(DATA)) { getData(buf, sizeof(buf)); cout << buf; }
}
