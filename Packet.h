/*
 * Packet.h
 *
 *  Created on: Apr 15, 2010
 *      Authors: Asad Saeed, Adam Darrah
 */

#ifndef PACKET_H_
#define PACKET_H_

#include <stdio.h>
#include <stdint.h>
#include <string.h>

class Packet {

public:
  const static unsigned DEFAULTHEADERSIZE = 6;
  const static uint8_t SEQ = 0x80;
  const static uint8_t ACK = 0x40;
  const static uint8_t SYN = 0x20;
  const static uint8_t FIN = 0x10;
  const static uint8_t OPT = 0x08;

  const static int SEQLOCATION = 2;
  const static int ACKLOCATION = 4;

  Packet(const Packet&);
  Packet(char *buffer, size_t length);
  Packet(uint8_t field, uint16_t seqAckNumber, char *firstBuffer = 0,
      size_t firstBufferLength = 0, char *secondBuffer = 0, size_t secondBufferLength = 0);
  Packet(uint8_t field, uint16_t seqNumber, uint16_t ackNumber, char *firstBuffer = 0,
      size_t firstBufferLength = 0, char *secondBuffer = 0, size_t secondBufferLength = 0);
  ~Packet();

  void insert_uint16_t(uint16_t number, char *location);
  bool getField(uint8_t field);
  void setField(uint8_t field, bool value=true);
  bool getSeqNumber(uint16_t &seqNumber);
  bool getAckNumber(uint16_t &ackNumber);
  size_t getOptField(char *optBuffer, size_t optBufferLength);

  void clear();

  uint8_t getHeaderLength();
  void setHeaderLength(uint8_t headerLength);

  size_t getData(char *outBuffer, size_t outBufferLength);


private:
  char *buffer;
  size_t length;
};

#endif /* PACKET_H_ */
