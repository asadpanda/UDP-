/*
 * UDPPlus.cpp
 *
 *  Created on: Apr 15, 2010
 *      Author: Asad Saeed, Adam Darrah
 *
 *      currently only supports a single connection.
 */

#include "UDPPlus.h"
#include "utility.h"
#include "UDPPlusConnection.h"

using namespace std;

UDPPlus::UDPPlus(int max_conn, int buf) {
	max_connections = max_conn;
	bufferSize = buf;
	connectionList = new UDPPlusConnection[max_conn];
	bounded = false;
	
	// make all slots initially null
	for(int i=0; i < max_conn; i++)
		connectionList[i] = NULL;
	
	if(sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP) == -1) {
		// throw error
		printf("error creating socket");
		exit(0);
	}
}

UDPPlus::~UDPPlus() {
	delete connectionList;
}

void UDPPlus::bind_p(struct addrinfo) {
	if(!bounded)
	{
		bounded = true;
	} else {
		printf("already bounded");
		exit(0);
	}
	bind(sockfd, addrinfo.ai_addr, sizeof(addrinfo.ai_addr));
	boost::thread listener(boost::bind(&UDPPLus::listen));
}

&UDPPlusConnection UDPPlus::accept_p() {

	boost::mutex::scoped_lock l(waitingMutex);
	waiting = true;
	if (waitingConnection == NULL)
		waitingCondition.wait(l);
	waiting = false;
	UDPPlusConnection *tempConnection = waitingConnection;
	waitingConnection = NULL;	
	return tempConnection;
}

void UDPPlus::listen() {
	char buffer[5000];
	int location;
	struct addrinfo connection;
	while(true) {

		memset(&connection, 0, sizeof(connection));
		int length = recvfrom(sockfd, buffer, sizeof(buffer), 0, &connection, sizeof(connection));
		location = isHostConnected(&connection, sizeof(connection));
		if (location >= 0) {
			connectionList[location]->handlePacket(Packet(buffer, length));
		}
		else {
			boost::mutex::scoped_lock l(waitingMutex);
			if (waiting == true) {
				Packet *tempPacket = Packet(buffer, length);
				if (tempPacket->getField(Packet::SYN)) {
					waitingConnection = new UDPPlusConnection(this, connection, sizeof(connection), tempPacket);
					waitingCondition.notify_one();
				} else {
					delete tempPacket;
				}
			}
		}		
	}
}
		
int UDPPlus::isHostConnected(struct addrinfo *connection, size_t length) {
	for (int i=0; i < max_connections; i++) {
		if (connectionList[i] != NULL) {
			if (memcmp(&connection, connectionList[i]->getAddrInfo(), length) == 0) {
				return i;
			}
		}
	}
	return -1;
}
																 

void UDPPlus::conn(struct addrinfo) {
	if(!bounded)
	{
		bounded = true;
	} else {
		printf("already bounded");
		exit(0);
	}
	connect(sockfd, addrinfo.ai_addr, sizeof(addrinfo.ai_addr));
	int location = findSlot();
	if (location == -1) {
		printf("gone over connections")
	}
  // build connection information
  UDPPlusConnection *active = new UDPPlusConnection(this, addrinfo.sockaddr, bufferSize );
	connectionList[location] = *active;
}

int UDPPlus::findSlot() {
	for(int i = 0; i < max_connections, i++) {
		if(connectionList[i] == NULL)
			return i;
	}
	return -1;
}	

void UDPPlus::close() {
  // close connection
}
