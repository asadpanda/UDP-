/*
 * Packet.h
 *
 *  Created on: Apr 15, 2010
 *      Author: asaeed
 */

#ifndef PACKET_H_
#define PACKET_H_


class Packet {

public:
  const static uint8_t ACK = 0x80;
  const static uint8_t SYN = 0x40;
  const static uint8_t FIN = 0x20;
  const static uint8_t OPT = 0x10;

  Packet();
  virtual ~Packet();

private:
  char *buffer;
  int length;
};

#endif /* PACKET_H_ */
