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
#include <netdb.h>
#include <netinet/in.h>
#include <stdint.h>
#include "Packet.h"

class UDPPlus {
public:
  UDPPlus();
  virtual ~UDPPlus();

  ssize_t recv(int s, void *buf, size_t len, int flags);
  
  // encapsulate getaddrinfo method
  // sets up needed structs
  int getaddr();
  
  // encapsulate socket method
  // returns socket file discriptor
  int getsocket();
  
  // encapsulate bind method
  // needs socket file discriptor from getsocket()
  int bind_p(int);

private:
  Packet **packetBuffer; // for array of pointers
  unsigned packetBufferSize;
  int ackedLocation;
  int lastSentLocation;
  
  // structs needed for socket
  struct addrinfo hints, *res;
};

#endif /* UDPPLUS_H_ */
