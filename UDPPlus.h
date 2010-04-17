/*
 * UDPPlus.h
 *
 *  Created on: Apr 15, 2010
 *      Author: Asad Saeed, Adam Darrah
 */

#ifndef UDPPLUS_H_
#define UDPPLUS_H_

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdint.h>
#include "Packet.h"

enum State { LISTEN, SYN_SENT, SYN_RECIEVED, ESTABLISHED, FIN_WAIT1, FIN_WAIT2, CLOSE_WAIT, CLOSING, LAST_ACK };

class UDPPlus {
public:
  UDPPlus();
  virtual ~UDPPlus();

  void init();
  void connect();
  void close();
  void recieve();


  ssize_t recv(int s, void *buf, size_t len, int flags);
  int bind();

private:

  int sock;
  State currentState;
  Packet **inBuffer; // for array of pointers
  Packet **outBuffer;
  unsigned inBufferSize;
  unsigned outBufferSize;
  uint16_t inBufferBegin;
  uint16_t inBufferEnd;
  uint16_t outBufferBegin;
  uint16_t outBufferEnd;
  uint16_t lastAckNum;
  uint16_t lastSeqNum;

};

#endif /* UDPPLUS_H_ */
