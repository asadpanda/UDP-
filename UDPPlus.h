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

class UDPPlusConnection;

// Create a UDPPlus object in the application layer to use
// the UDP+ transport layer protocol. The UDP+ class encapsulates
// core UDP c++ methods and adds in additional features.

class UDPPlus {
public:
  
  // initializes all data members and connectionList
  // also creates socket to be used in all connections
  UDPPlus(int max_connection = 10, int bufferSize = 1024);
  
  // closes all open UDPPlusConnection objects in connectionList
  // deallocates memory
  virtual ~UDPPlus();

  // binds a socket to be used for connection
  // builds a UDPPlusConnection object and stores in connectionList
  // returns pointer to UDPPlusconnection object
  UDPPlusConnection* conn(const struct sockaddr*, const socklen_t&);
	
  // binds a port to be listened to
  // starts a listener thread to listen for incoming packets
	void bind_p(const struct sockaddr*, const socklen_t&);
  
  // accepts a new incoming connection
	UDPPlusConnection* accept_p();

	// methods to close UDP+ connections
	// close will close a single connection object
	// close_all will close all open connections
	// close_all will also close the socket (sockfd)
  void close_one(UDPPlusConnection&);
	void close_all();

private:
  
  // listens to binded port for incoming data
  // when incoming data is received, check if the host is known (isHostConnected)
  // if host is already connected, process the incoming packet
  // if host is not connected, check SYN bits and process connection
  void listen();
	
  void recieve();
  void send(int conn, Packet*);

  // searches connectionList for an open indicie
  // returns the indicies location if found
  // returns -1 if no open slot is available
	int findSlot();
    
  // checks if an incoming packet is sent from a known host
  // if returns -1, host was not found and is new
	int isHostConnected(struct sockaddr *connection, socklen_t length);

  // array of all UDPPlusConnections
	UDPPlusConnection **connectionList;
	int sockfd;
	int max_connections;
	int bufferSize;
	boost::thread *listener;
	bool bounded;
	bool waiting;
	boost::mutex waitingMutex;
	boost::condition_variable waitingCondition;
	UDPPlusConnection *waitingConnection;
	
	// UDPPlusConnecion object are friended so that
	// these objects can call private UDPPlus methods
	friend class UDPPlusConnection;
};

#endif /* UDPPLUS_H_ */
