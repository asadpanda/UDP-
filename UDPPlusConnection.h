/*
 * UDPPlusConnection.h
 *
 *  Created on: Apr 19, 2010
 *      Author: asaeed
 */

#ifndef UDPPLUSCONNECTION_H_
#define UDPPLUSCONNECTION_H_

#include "utility.h"
#include "Packet.h"

using namespace boost::posix_time;

enum State { LISTEN, SYN_SENT, SYN_RECIEVED, ESTABLISHED, FIN_WAIT, CLOSE_WAIT, LAST_ACK, TIME_WAIT, CLOSED };

class UDPPlus;

class UDPPlusConnection {
public:
  UDPPlusConnection(UDPPlus *mainHandler,
      const struct sockaddr *remote,
      const socklen_t &remoteSize,
      int &bufferSize,
      Packet *incomingConnection = 0);

  virtual ~UDPPlusConnection();

	
  const struct sockaddr* getSockAddr(socklen_t &);
  
  // send function for client applications
	int send(const void *, size_t);
  
	int recv(void *buf, size_t len);
	void closeConnection();
	
private:
  void timer();

  void handlePacket(Packet *currentPacket);
  void handleEstablished(Packet *currentPacket);
  bool handleAck(Packet *currentPacket);
  bool handleSack(Packet *currentPacket);
  bool handleData(Packet *currentPacket);
  bool handleFin(Packet *currentPacket);
  void sendSack();
  void send_packet(Packet*);
  // loop through inBuffer and check the sequence numbers
  // if sequence number is less than newSeqNum, delete from buffer
  void releaseBufferTill(int newSeqNum);
  uint16_t lowestValidSeq();
  bool checkIfAckable(const uint16_t &);
  int processInBuffer();

  UDPPlus *mainHandler;
  State currentState;
  
  boost::thread *clock;
	time_duration timeout;
  time_duration maximumTimeout;
  ptime ackTimestamp;
  uint8_t ackWaiting; // 
  uint8_t numAck;

  boost::condition_variable timerCondition;
  boost::condition_variable inCondition;
  boost::condition_variable outCondition;
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
