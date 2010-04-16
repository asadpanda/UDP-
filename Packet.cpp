/*
 * Packet.cpp
 *
 *  Created on: Apr 15, 2010
 *      Authors: Asad Saeed, Adam Darrah
 */

#include "Packet.h"
#include <netinet/in.h>

Packet::Packet(char *buffer, size_t length) {
  this->length = length;
  this->buffer = new char[length];
  memcpy(this->buffer, buffer, length);
}

Packet::Packet(const Packet &original) {
  buffer = new char[original.length];
  length = original.length;
  memcpy(buffer, original.buffer, length);
}

Packet::Packet(uint8_t field, uint16_t seqAckNumber, char *firstBuffer,
               size_t firstBufferLength, char *secondBuffer, size_t secondBufferLength) {

  buffer = new char[DEFAULTHEADERSIZE + firstBufferLength + secondBufferLength];
  length = DEFAULTHEADERSIZE + firstBufferLength + secondBufferLength;

  clear();
  setField(field);
  insert_uint16_t(seqAckNumber, buffer + 2);

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

Packet::Packet(uint8_t field, uint16_t seqNumber, uint16_t ackNumber, char *firstBuffer,
    size_t firstBufferLength, char *secondBuffer, size_t secondBufferLength)
{
  buffer = new char[DEFAULTHEADERSIZE + firstBufferLength + secondBufferLength];
  length = DEFAULTHEADERSIZE + 2 + firstBufferLength + secondBufferLength;

  clear();
  setField(field);
  insert_uint16_t(seqNumber, buffer + 2);
  insert_uint16_t(ackNumber, buffer + 4);
  int topHeadLength = 6;
  if ( getField(OPT) ) {
    setHeaderLength(6 + firstBufferLength);
  }

  if ( firstBuffer != NULL ) {
    memcpy(buffer + topHeadLength, firstBuffer, firstBufferLength);
  }
  if (secondBuffer != NULL) {
    memcpy(buffer + topHeadLength + firstBufferLength, secondBuffer, secondBufferLength);
  }

}

void Packet::insert_uint16_t(uint16_t number, char *location) {
  number = htons(number);
  memcpy(location, &number, sizeof(uint16_t));
}

Packet::~Packet() {
  delete buffer;
}

void Packet::clear() {
  memset(buffer, 0, length);
  buffer[1] = 4;
}

bool Packet::getField(uint8_t field) {
  return ((field & buffer[0]) != 0);
}

void Packet::setField(uint8_t field, bool value) {
  if (value) {
    buffer[0] |= field;
  }
  else {
    buffer[0] &= (~field);
  }
}

bool Packet::getSeqNumber(uint16_t &seqNumber) {
  if ( getField(SEQ) ) {
    memcpy(&seqNumber, buffer + 2, sizeof(uint16_t));
    seqNumber = ntohs(seqNumber);
    return true;
  }
  return false;
}

bool Packet::getAckNumber(uint16_t &ackNumber) {
  if (getField(Packet::ACK)) {
    int ackLocation = 2;
    if (getField(Packet::SEQ)) {
      ackLocation += 2;
    }
    memcpy(&ackNumber, buffer + ackLocation, sizeof(uint16_t));
    ackNumber = ntohs(ackNumber);
    return true;
  }
  return false;
}

size_t Packet::getOptField(char *optBuffer, size_t optBufferLength) {
  size_t optStart = sizeof(char[4]);

  if (getField(Packet::SEQ) && getField(Packet::ACK))
    optStart += sizeof(char[2]);
  size_t optLength = getHeaderLength() - optStart;

  if (optLength > optBufferLength)
    optLength = optBufferLength;

  memcpy(optBuffer, buffer + optStart, optLength );

  return optLength;
}

// header length is default 1
uint8_t Packet::getHeaderLength() {
  return buffer[1];
}

void Packet::setHeaderLength(uint8_t headerLength) {
  buffer[1] = headerLength;
}

size_t Packet::getData(char *outBuffer, size_t outBufferLength) {
  size_t headerLength = getHeaderLength();
  size_t dataLength = (length - headerLength);

  // bound check
  if (dataLength > outBufferLength)
    dataLength = outBufferLength;

  memcpy(outBuffer, (buffer + headerLength), dataLength);
  return dataLength;
}
