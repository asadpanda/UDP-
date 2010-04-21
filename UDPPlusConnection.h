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

enum State { LISTEN, SYN_SENT, SYN_RECIEVED, ESTABLISHED, FIN_WAIT1, FIN_WAIT2, CLOSE_WAIT, CLOSING, LAST_ACK, TIME_WAIT, CLOSED };

class UDPPlus;

class UDPPlusConnection {
public:
  UDPPlusConnection(UDPPlus *mainHandler,
      const struct sockaddr *remote,
      const socklen_t &remoteSize,
      int &bufferSize,
      Packet *incomingConnection = 0);

  virtual ~UDPPlusConnection();

  void handlePacket(Packet *currentPacket);
	
  const struct sockaddr* getSockAddr(socklen_t &);
	void send(void *, size_t, int);
	void recv(int s, void *buf, size_t len);
	void closeConnection();
	
private:
	time_duration timeout;
	void timer();
	bool checkIfAckable(const uint16_t &);
  UDPPlus *mainHandler;
  State currentState;
  void handleEstablished(Packet *currentPacket);

  boost::condition_variable timerCondition;
  boost::condition_variable inConditionEmpty;
  boost::condition_variable inConditionFull;
  boost::condition_variable outConditionEmpty;
  boost::condition_variable outConditionFull;
  boost::mutex ackMutex;
  boost::mutex inBufferMutex;
  boost::mutex outBufferMutex;
  Packet **inBuffer; // for array of pointers
  Packet **outBuffer;

  int inBufferSize; // changed from unsigned
  int outBufferSize;
  uint16_t inBufferBegin;
  uint16_t inBufferEnd;
  uint16_t outBufferBegin;
  uint16_t outBufferEnd;
  uint16_t newAckNum;
  uint16_t newSeqNum;
  uint16_t inItems;
  uint16_t outItems;
  uint8_t numAck;
  uint16_t lastAckRecv;

	struct sockaddr remoteAddress;
	socklen_t remoteAddressLength;

	boost::thread *clock;

	friend class UDPPlus;
};

#endif /* UDPPLUSCONNECTION_H_ */
