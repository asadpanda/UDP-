/*
 * Packet.h
 *
 *  Created on: Apr 15, 2010
 *      Author: asaeed
 */

#ifndef PACKET_H_
#define PACKET_H_

#include <stdint.h>

class Packet {

public:
  const static uint8_t ACK = 0x80;
  const static uint8_t SYN = 0x40;
  const static uint8_t FIN = 0x20;
  const static uint8_t OPT = 0x10;

  Packet(int);
  ~Packet();

  bool getField(uint8_t field);
  void setField(uint8_t field, bool value=true);
  void clear();
  uint8_t getHeaderLength();
  int getData(char *outBuffer, int outBufferLength);


private:
  char *buffer;
  int length;
};

#endif /* PACKET_H_ */
