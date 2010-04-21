/*
 * UDPPlus.cpp
 *
 *  Created on: Apr 15, 2010
 *      Author: Asad Saeed, Adam Darrah
 *
 */

#include "UDPPlus.h"
#include "utility.h"
#include "UDPPlusConnection.h"
#include "Packet.h"

using namespace std;

UDPPlus::UDPPlus(int max_conn, int buf) {
	max_connections = max_conn;
	bufferSize = buf;
	connectionList = new UDPPlusConnection*[max_conn];
	bounded = false;
	waiting = false;
	
	// make all slots initially null
	for(int i=0; i < max_conn; i++)
		connectionList[i] = NULL;
	
  // create UDP socket to work with IPv4 and IPv6
	sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(sockfd == -1) {
		// throw error
		printf("error creating socket");
		exit(0);
	}
}

UDPPlus::~UDPPlus() {
  // close all open connections
  close_all();
  for (int i = 0; i < max_connections; i++) {
    if (connectionList[i] != NULL) {
      delete connectionList[i];
      connectionList[i] = NULL;
    }
  }
  delete listener;
	delete connectionList;
}

void UDPPlus::bind_p(const struct sockaddr *info, const socklen_t &infoLength) {
	if(!bounded)
	{
		bounded = true;
	} else {
		printf("already bounded");
		exit(0);
	}

	bind(sockfd, info, infoLength);
  listener = new boost::thread(boost::bind(&UDPPlus::listen, this));
}

UDPPlusConnection * UDPPlus::accept_p() {
	boost::mutex::scoped_lock l(waitingMutex);
	waiting = true;
	if (waitingConnection == NULL)
		waitingCondition.wait(l);
	waiting = false;
	UDPPlusConnection *tempConnection = waitingConnection;
  int location = findSlot();
  connectionList
	waitingConnection = NULL;	
	return tempConnection;
}

void UDPPlus::listen() {
	char buffer[5000];
	int location;
	struct sockaddr connection;
	socklen_t connectionLength;
	while(true) {

		memset(&connection, 0, sizeof(connection));
		connectionLength = sizeof(connection);
		int length = recvfrom(sockfd, buffer, sizeof(buffer), 0, &connection, &connectionLength);
		location = isHostConnected(&connection, connectionLength);
		if (location >= 0) {
			connectionList[location]->handlePacket(new Packet(buffer, length));
		}
		else {
			boost::mutex::scoped_lock l(waitingMutex);
			if (waiting == true) {
				Packet *tempPacket = new Packet(buffer, length);
				if (tempPacket->getField(Packet::SYN)) {
					waitingConnection = new UDPPlusConnection(this, &connection, connectionLength, bufferSize, tempPacket);
          waiting = false;
					waitingCondition.notify_one();
				} else {
					delete tempPacket;
				}
			}
		}		
	}
}
		
int UDPPlus::isHostConnected(struct sockaddr *connection, socklen_t length) {
	for (int i=0; i < max_connections; i++) {
		if (connectionList[i] != NULL) {
		  socklen_t tempAddressLength;
		  const struct sockaddr *tempAddress = connectionList[i]->getSockAddr(tempAddressLength);
			if (tempAddressLength == length && memcmp(connection, tempAddress, length) == 0) {
				return i;
			}
		}
	}
	return -1;
}
																 

UDPPlusConnection* UDPPlus::conn(const struct sockaddr *info, const socklen_t &infoLength) {
	if(!bounded)
	{
		bounded = true;
	} else {
		printf("already bounded");
		exit(0);
	}
  // connect will bind a socket
	connect(sockfd, info, infoLength);
  listener = new boost::thread(boost::bind(&UDPPlus::listen, this));

	int location = findSlot();
	if (location == -1) {
		printf("gone over connections");
		return NULL;
	}
  // build connection information
  UDPPlusConnection *active = new UDPPlusConnection(this, info, infoLength, bufferSize);
	connectionList[location] = active;
	return active;
}

int UDPPlus::findSlot() {
	for(int i = 0; i < max_connections; i++) {
		if(connectionList[i] == NULL)
			return i;
	}
	return -1;
}	

void UDPPlus::close_one(UDPPlusConnection &conn) {
  // close single connection
	conn.close_connection();
}

void UDPPlus::close_all() {
  // close all connections
	for(int i=0; i < max_connections; i++) {
		if(connectionList[i] != NULL) {
			close_one(*connectionList[i]);
		}
	}
	// close the socket
	close(sockfd);
}
