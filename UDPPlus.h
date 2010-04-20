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
#include "UDPPlusConnection.h"

class UDPPlus {
public:
  UDPPlus(int, int);
  virtual ~UDPPlus();

  void conn(struct addrinfo);
  void close();
	
	void bind_p(struct addrinfo);
	UDPPlusConnection accept_p();
	void listen();


  ssize_t recv(int s, void *buf, size_t len, int flags);

private:
	void recieve();
	int findSlot();	
	void send(int conn, Packet*);
	int isHostConnected(struct addrinfo *connection, size_t length);

	UDPPlusConnection **connectionList;
	int sockfd;
	int max_connections;
	int bufferSize;
	boost::thread listener;
	bool bounded;
	
	boost::mutex waitingMutex;
	boost::condition_variable waitingCondition;
	UDPPlusConnection *waitingConnection;
	
	friend class UDPPlusConnection;
};

#endif /* UDPPLUS_H_ */
