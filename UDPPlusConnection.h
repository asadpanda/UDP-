/*
 * UDPPlusConnection.h
 *
 *  Created on: Apr 19, 2010
 *      Author: asaeed
 */

//          Copyright Joe Coder 2004 - 2006.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)


#ifndef UDPPLUSCONNECTION_H_
#define UDPPLUSCONNECTION_H_

#include "utility.h"
#include "Packet.h"

using namespace boost::posix_time;

// State enumeration
// state model is based on TCP
enum State { LISTEN, SYN_SENT, SYN_RECIEVED, ESTABLISHED, FIN_WAIT, CLOSE_WAIT, LAST_ACK, TIME_WAIT, CLOSED };

class UDPPlus;

class UDPPlusConnection {
public:

  // initializes all data members
  // initializes buffer slots to NULL
  // creates a timer boost thread
  UDPPlusConnection(UDPPlus *mainHandler,
      const struct sockaddr *remote,
      const socklen_t &remoteSize,
      int &bufferSize,
      Packet *incomingConnection = 0);

  // closes connection
  // deletes data structure and thread
  virtual ~UDPPlusConnection();

	// returns sockaddr struct for this connection
  // required by UDPPlus to determine if connection is new or not
  const struct sockaddr* getSockAddr(socklen_t &);
  
  // send function for client applications
  // wraps send_packet method and connection state
	int send(const void *, size_t);
  
  // pops a data packet off the front of the inqueue
  // sets given buffer and length values to that of the packet
	// when done, the packet is deleted
  int recv(void *buf, size_t len);
  
  // changes state to either FIN_WAIT or LAST_ACK
  // sends out a fin packet to close connection
	void closeConnection();
	
private:
  // this method is threaded
  // will detect timeouts
  // resends packet not yet acked, or sends an ack
  void timer();

  // given a packet, redirects the data to
  // an appropriate destination based on the conneciton state
  // also establishes a connection
  void handlePacket(Packet *currentPacket);
  //void handleEstablished(Packet *currentPacket);
  
  // releases the packets from the inbuffer appropriately
  // detects triple acks
  bool handleAck(Packet *currentPacket);
  // handles a sack packet
  // detects the length and data in optional field
  bool handleSack(Packet *currentPacket);
  // sends an ack packet assuming there are
  // enough packets waiting to be acked
  bool handleData(Packet *currentPacket);
  // handles a fin packet
  // prepares the connection to be closed
  bool handleFin(Packet *currentPacket);
  // sets the OPT flag to true
  void sendSack();
  
  // wrapper for UDPPlus send method
  // prepares a packet and sends it through
  void send_packet(Packet*);
  
  // loop through inBuffer and check the sequence numbers
  // if sequence number is less than newSeqNum, delete from buffer
  void releaseBufferTill(int newSeqNum);
  
  // returns next lowerst valid sequence number to be used
  uint16_t lowestValidSeq();
  
  // determines if a packet has been acked
  // if the packets sequence number is less than the given arg
  // return true, else false
  bool checkIfAckable(const uint16_t &);
  
  // counts how many packets are awaiting to be acked
  // also deletes the packets out of the inbuffer
  int processInBuffer();

  UDPPlus *mainHandler; // belongs to a UDPPlus object
                        // stores ptr to object to call UDPPlus methods
  State currentState; // current connection state
  
  boost::thread *clock;
	time_duration timeout;
  time_duration maximumTimeout;
  ptime ackTimestamp;
  uint8_t ackWaiting; // 
  uint8_t numAck;

  boost::condition_variable timerCondition;
  boost::condition_variable inCondition;
  boost::condition_variable outCondition;
  boost::condition_variable closeCondition;
  boost::mutex sharedMutex;

  deque< Packet* > inQueue;
  Packet **inBuffer; // for array of pointers
  Packet **outBuffer;
  int inBufferSize; // changed from unsigned
  int outBufferSize;
  uint16_t inBufferBegin; // Current position 
  uint16_t outBufferBegin;
  //uint16_t inItems;       // Number of items
  uint16_t outItems;      // Number of Items in the outgoing buffer
  int8_t inBufferDelta;   // This is used to determine the difference between
                          // the last acked segment recieved and the highest segment
                          // recieved that hasn't been acked yet (out of order packets)
  
  uint16_t newAckNum;     // The Remote Sequence number that can be confirmed recieved.
  uint16_t newSeqNum;     // New Sequence number that will be sent out
  uint16_t lastAckRecv;
  int maxAckNumber;

	struct sockaddr remoteAddress;
	socklen_t remoteAddressLength;

	friend class UDPPlus;
};

#endif /* UDPPLUSCONNECTION_H_ */
