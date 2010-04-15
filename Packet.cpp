/*
 * Packet.cpp
 *
 *  Created on: Apr 15, 2010
 *      Author: asaeed
 */

#include "Packet.h"


Packet::Packet(uint16_t seqnum, int length) {
  buffer = new char[length];
  this->length = length;

  clear();
}

Packet::~Packet() {
  delete buffer;
}

bool Packet::getField(uint8_t field) {
  return ((field & buffer[0]) != 0);
}

void Packet::setField(uint8_t field, bool value = true) {
  if (value) {
    buffer[0] |= field;
  }
  else {
    buffer[0] &= (~field);
  }
}

void Packet::clear() {
  for (int i=0; i < length; i++)
    buffer[i] = 0;
}

// header length is
uint8_t Packet::getHeaderLength() {
  return buffer[1];
}

int Packet::getData(char *outBuffer, int outBufferLength) {
  return 1;
}
