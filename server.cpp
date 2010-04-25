/*
 * server.cpp
 *
 *  Created on: Apr 15, 2010
 *      Author: Asad Saeed, Adam Darrah
 */

#include "utility.h"
#include "UDPPlus.h"
#include "UDPPlusConnection.h"

void reciever(UDPPlusConnection *conn);

int main(int argc, char* argv[]) {
  
  UDPPlus *conn = new UDPPlus;
  UDPPlusConnection *open;
  int role = 0;
  const int PORT = 30000;
  char buf[2048];
  int len;
  boost::thread *myThread;

  
  // determine if this will be a server or client application
	cout << endl << "UDP+ Test Driver" << endl
       << "=====================================================" << endl;
	cout << "Assign application role: [1] server [2] client" << endl;
	cin  >> role;
  
  switch (role) {
    // Application is a server
    case 1:
      
      // setup sockadr_in struct
      struct sockaddr_in local;
      memset((char *) &local, 0, sizeof(local));
      local.sin_family = AF_INET;
      local.sin_port = htons(PORT);
      local.sin_addr.s_addr = AI_PASSIVE;
      
      // bind the port
      // this also starts the listener thread
      printf("Binding to port...");
      conn->bind_p(&local, sizeof(local));
      
      // start waiting for an incoming connection
      // if a new host is detected, listener thread will direct it to accept
      printf("Waiting for client connection...");
      open = conn->accept_p();
      myThread = new boost::thread(&reciever, open);
      while (true) {
        fgets(buf, sizeof(buf), stdin);
        if (open->send(buf, strlen(buf)) == -1) {
          myThread->join();
          break;
        }
      }
      
      
      delete open;
      break;
    
    // Application is a client
    case 2:
    default:
      struct hostent *hostAddress = gethostbyname("127.0.0.1");
      // setup sockaddr_in struct
      struct sockaddr_in host;
      memset((char *) &host, 0, sizeof(host));
      host.sin_family = AF_INET;
      host.sin_port = htons(PORT);
      memcpy(&host.sin_addr.s_addr, hostAddress->h_addr, hostAddress->h_length);      // connect to the server @ localhost
      
      // this will also start the listener thread for receiving data
      open = conn->conn(&host, sizeof(host));
      
      myThread = new boost::thread(&reciever, open);
      while (true) {
        fgets(buf, sizeof(buf), stdin);
        if (open->send(buf, strlen(buf)) == -1) {
          myThread->join();
          break;
        }
      }
      
      delete open;
      break;
  }  
	
  return 0;
}

void reciever(UDPPlusConnection *conn) {
  char buf[2048];
  
  while (true) {
    if ( conn->recv(buf, sizeof(buf)) == -1 ) {
      printf("connection closed");
      break;
    }
    printf("%s", buf);
  }
}
