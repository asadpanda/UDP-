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
  listenerDone = false;
	
	// make all slots initially null
	for(int i=0; i < max_conn; i++)
		connectionList[i] = NULL;
	
  // create UDP socket to work with IPv4 and IPv6
	sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if(sockfd < 0) {
		// throw error
		printf("error creating socket");
		exit(0);
	}
  int arg = fcntl(sockfd, F_GETFL, NULL); 
  arg |= O_NONBLOCK; 
  fcntl(sockfd, F_SETFL, arg);
}

UDPPlus::~UDPPlus() {
  // close all open connections
  close_all();

  waitingCondition.notify_all();
  close(sockfd);
  listener->join();  
  for (int i = 0; i < max_connections; i++) {
    if (connectionList[i] != NULL) {
      delete connectionList[i];
      connectionList[i] = NULL;
    }
  }
  // stop listener thread
	delete connectionList;
}

void UDPPlus::bind_p(const struct sockaddr *info, const socklen_t &infoLength) {
	if(!bounded)
	{
		bounded = true;
	} else {
		printf("already bounded");
		exit(1);
	}

	if (bind(sockfd, info, infoLength) < 0) {
    exit(2);
  }
  listener = new boost::thread(boost::bind(&UDPPlus::listen, this));
  cerr << "Main Thread";
}

void UDPPlus::send_p(struct sockaddr *connection, socklen_t len, Packet* p) {
  sendto(sockfd, p->getBuffer(), p->getLength(), 0, connection, len);
}


UDPPlusConnection * UDPPlus::accept_p() {
	boost::mutex::scoped_lock l(waitingMutex);
	waiting = true;
	if (waitingConnection == NULL)
		waitingCondition.wait(l);
	waiting = false;
  if (waitingConnection == NULL) {
    return NULL;
  }
	UDPPlusConnection *tempConnection = waitingConnection;
  int location = findSlot();
  connectionList[location] = tempConnection;
	waitingConnection = NULL;
	return tempConnection;
}

void UDPPlus::listen() {
	char buffer[5000];
	int location;
	struct sockaddr connection;
  cerr << "new thread";
	socklen_t connectionLength;
	while(true) {
		memset(&connection, 0, sizeof(connection));
		connectionLength = sizeof(connection);
    cerr << "listening for new connection";
		int length = recvfrom(sockfd, buffer, sizeof(buffer), 0, &connection, &connectionLength);
    if (length == -1) {
      waitingCondition.notify_all();
      cerr << "listener thread: socket closed";
      break;
    }
    cerr << "waiting for mutex";
    boost::mutex::scoped_lock l(waitingMutex);
    cerr << "test";
		location = isHostConnected(&connection, connectionLength);
		if (location >= 0) {
      Packet *temp = new Packet(buffer, length);
      temp->print();
			connectionList[location]->handlePacket(temp);
		}
		else {
			//
			if (waiting == true) {
				Packet *tempPacket = new Packet(buffer, length);
        tempPacket->print();
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
  boost::mutex::scoped_lock l(waitingMutex);
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
	if (connect(sockfd, info, infoLength) < 0) {
    exit(1);
  }
  listener = new boost::thread(boost::bind(&UDPPlus::listen, this));
  boost::mutex::scoped_lock l(waitingMutex);
  cerr << "waiting mutex grabbed";
	int location = findSlot();
//	if (location == -1) {
//    cerr << "no location found";
//		return NULL;
//	}
  // build connection information
  cerr << "creating new connection";
  UDPPlusConnection *active = new UDPPlusConnection(this, info, infoLength, bufferSize);
	connectionList[location] = active;
	return active;
}

int UDPPlus::findSlot() {
  boost::mutex::scoped_lock l(waitingMutex);
	for(int i = 0; i < max_connections; i++) {
		if(connectionList[i] == NULL)
			return i;
	}
	return -1;
}	

void UDPPlus::close_one(UDPPlusConnection *conn) {
  boost::mutex::scoped_lock l(waitingMutex);
  // close single connection
	conn->closeConnection();
}

void UDPPlus::close_all() {
  boost::mutex::scoped_lock l(waitingMutex);
  // close all connections
	for(int i=0; i < max_connections; i++) {
		if(connectionList[i] != NULL) {
			close_one(connectionList[i]);
		}
	}
	// close the socket
	close(sockfd);
}

void UDPPlus::deleteConnection(UDPPlusConnection *connection) {
  boost::mutex::scoped_lock l(waitingMutex);
  for( int i = 0; i < max_connections; i++) {
    if (connectionList[i] == connection) { connectionList[i] = NULL; }
  }
}
