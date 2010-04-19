/*
 * UDPPlus.h
 *
 *  Created on: Apr 15, 2010
 *      Author: Asad Saeed, Adam Darrah
 */

#ifndef UDPPLUS_H_
#define UDPPLUS_H_

#include "utility.h"
#include "Packet.h"

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


  
  // structs needed for socket
  struct addrinfo hints, *res;
};

#endif /* UDPPLUS_H_ */
