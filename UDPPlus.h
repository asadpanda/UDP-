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
#include <boost/thread.hpp> // boost install instructions can be found here: http://www.technoboria.com/2009/07/simple-guide-to-installing-boost-on-mac-os-x/
                            // make sure to move /boost directory to /usr/include directory
#include "Packet.h"

enum State { LISTEN, SYN_SENT, SYN_RECIEVED, ESTABLISHED, FIN_WAIT1, FIN_WAIT2, CLOSE_WAIT, CLOSING, LAST_ACK };

class UDPPlus {
public:
  UDPPlus();
  virtual ~UDPPlus();

  void connect();
  void close();
  void recieve();


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

  
  // structs needed for socket
  struct addrinfo hints, *res;
};

#endif /* UDPPLUS_H_ */
