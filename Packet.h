/*
 * Packet.h
 *
 *  Created on: Apr 15, 2010
 *      Authors: Asad Saeed, Adam Darrah
 *
 *  This Packet class encapsulates data being sent
 *  through UDP+.  It builds a buffered data set
 *  that will be forwarded to the reciever.
 *
 *
 */

//          Copyright Joe Coder 2004 - 2006.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)


#ifndef PACKET_H_
#define PACKET_H_

#include "utility.h"

using namespace boost::posix_time;

class Packet {

public:
  const static int MAXSIZE = 65536;
  const static unsigned DEFAULTHEADERSIZE = 6;
  const static uint8_t DATA = 0x80;
  const static uint8_t ACK = 0x40;
  const static uint8_t SYN = 0x20;
  const static uint8_t FIN = 0x10;
  const static uint8_t OPT = 0x08;

  const static int SEQLOCATION = 2;
  const static int ACKLOCATION = 4;

  int numAck;
  int sendCount;

  Packet(const Packet&);
  Packet(const void *buffer, size_t length);
  Packet(uint8_t field, uint16_t seqNumber, uint16_t ackNumber, const void *firstBuffer = 0,
      size_t firstBufferLength = 0, const void *secondBuffer = 0, size_t secondBufferLength = 0);
  ~Packet();

  void print();
  void insert_uint16_t(uint16_t number, void *location);
  bool getField(uint8_t field);
  void setField(uint8_t field, bool value=true);
  uint16_t getSeqNumber();
  uint16_t getAckNumber();
  void setSeqNumber(uint16_t seqNumber, bool shouldSet=true);
  void setAckNumber(uint16_t ackNumber, bool shouldSet=true);


  size_t getOptField(void *optBuffer, size_t optBufferLength);

  void clear();

  uint8_t getHeaderLength();
  void setHeaderLength(uint8_t headerLength);

  size_t getData(void *outBuffer, size_t outBufferLength);
  size_t getLength();
  void updateTime();
  ptime getTime();

  char* getBuffer();
  

private:
  char *buffer;
  size_t length;
  ptime sendingTime;
};

#endif /* PACKET_H_ */
