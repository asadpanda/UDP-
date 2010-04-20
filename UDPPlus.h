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

class UDPPlus {
public:
  UDPPlus(int max_connection = 10, int bufferSize = 1024);
  virtual ~UDPPlus();

  void conn(const struct sockaddr*, const socklen_t&);
  void close();
	
	void bind_p(const struct sockaddr*, const socklen_t&);
	UDPPlusConnection* accept_p();

  ssize_t recv(int s, void *buf, size_t len, int flags);

private:
  void listen();
	void recieve();
	int findSlot();	
	void send(int conn, Packet*);
	int isHostConnected(struct sockaddr *connection, socklen_t length);

	UDPPlusConnection **connectionList;
	int sockfd;
	int max_connections;
	int bufferSize;
	//boost::thread listener;
	bool bounded;
	
	bool waiting;
	boost::mutex waitingMutex;
	boost::condition_variable waitingCondition;
	UDPPlusConnection *waitingConnection;
	
	friend class UDPPlusConnection;
};

#endif /* UDPPLUS_H_ */
