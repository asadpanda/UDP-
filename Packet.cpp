/*
 * Packet.cpp
 *
 *  Created on: Apr 15, 2010
 *      Author: asaeed
 */

#include "Packet.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>


Packet::Packet(int dataLength) {
  buffer = new char[dataLength];
  clear();
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

// header length is default 1
uint8_t Packet::getHeaderLength() {
  return buffer[1];
}

int Packet::getData(char *outBuffer, int outBufferLength) {
  uint8_t headerLength = getHeaderLength();
  int dataLength = (length - headerLength);

  // bound check
  if (dataLength > outBufferLength)
    dataLength = outBufferLength;

  memcpy(outBuffer, (buffer + headerLength), dataLength);
  return dataLength;
}
