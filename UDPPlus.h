/*
 * UDPPlus.h
 *
 *  Created on: Apr 15, 2010
 *      Author: Asad Saeed, Adam Darrah
 */

#ifndef UDPPLUS_H_
#define UDPPLUS_H_

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdint.h>
#include "Packet.h"

class UDPPlus {
public:
  UDPPlus();
  virtual ~UDPPlus();

  ssize_t recv(int s, void *buf, size_t len, int flags);
  int bind();

private:
  Packet **packetBuffer; // for array of pointers
  unsigned packetBufferSize;
  int ackedLocation;
  int lastSentLocation;
};

#endif /* UDPPLUS_H_ */
