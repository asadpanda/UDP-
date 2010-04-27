/*
 * server.cpp
 *
 *  Created on: Apr 15, 2010
 *      Author: Asad Saeed, Adam Darrah
 */

#include "utility.h"
#include "UDPPlus.h"
#include "UDPPlusConnection.h"

void sender(UDPPlusConnection *open);
void reciever(UDPPlusConnection *conn);

int main(int argc, char* argv[]) {
  
  UDPPlus *conn = new UDPPlus;
  UDPPlusConnection *open;
  int role = 0;
  const int PORT = 9555;
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
      local.sin_addr.s_addr = INADDR_ANY;
      
      // bind the port
      // this also starts the listener thread
      printf("Binding to port...");
      conn->bind_p( (struct sockaddr *) &local, sizeof(local));
      
      // start waiting for an incoming connection
      // if a new host is detected, listener thread will direct it to accept
      printf("Waiting for client connection...");
      open = conn->accept_p();
      //myThread = new boost::thread(&sender, open);
      reciever(open);
      //myThread->join();
      open->closeConnection();
      conn->close_all();
      delete open;
      break;
    
    // Application is a client
    case 2:
    default:
      // setup sockaddr_in struct
      struct sockaddr_in host;
      memset((char *) &host, 0, sizeof(host));
      host.sin_family = AF_INET;
      host.sin_port = htons(PORT);
      inet_pton(AF_INET, "127.0.0.1", &host.sin_addr);
      
      // this will also start the listener thread for receiving data
      open = conn->conn( (struct sockaddr *) &host, sizeof(host));
      
      //  myThread = new boost::thread(&sender, open);
      sender(open);
      
      cout << "Sending Finished: Joining Recieving thread" << endl;
      //myThread->join();
      open->closeConnection();
      delete open;
      break;
  }  
	
  return 0;
}

void sender(UDPPlusConnection *open) {
  cerr << "sending loop starting" << endl;
  for (int i = 0; i < 9; i++) {
    stringstream tempStream;
    // while (true) {
    tempStream << i;
    string temp = "((DATAGRAM:" + tempStream.str() + "))";
    //std::getline(std::cin, temp);
    //if (temp == "-1") { break; }
    if (open->send(temp.c_str(), temp.size()) == -1) {
      break;
    }
  }
}

void reciever(UDPPlusConnection *conn) {
  char buf[2048];
  
  cout << "Reciever Thread Started" << endl;
  while (true) {
    int value = conn->recv(buf, sizeof(buf));
    cerr << "Reciever Return Value:" << value << endl;
    if ( value == -1 ) {
      printf("connection closed");
      return;
    }
    cout << buf;
  }
}
