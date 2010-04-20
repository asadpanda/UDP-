/*
 * UDPPlusConnection.h
 *
 *  Created on: Apr 19, 2010
 *      Author: asaeed
 */

#ifndef UDPPLUSCONNECTION_H_
#define UDPPLUSCONNECTION_H_

#include "utility.h"
#include "UDPPlus.h"
#include "Packet.h"

enum State { LISTEN, SYN_SENT, SYN_RECIEVED, ESTABLISHED, FIN_WAIT1, FIN_WAIT2, CLOSE_WAIT, CLOSING, LAST_ACK };

class UDPPlusConnection {
public:
  UDPPlusConnection(UDPPlus *mainHandler, struct sockaddr remote, int bufferSize, Packet *incomingConnection = NULL);

  virtual ~UDPPlusConnection();

  void handlePacket(Packet *currentPacket);
	
	void send(void *, size_t, int);
	void recv(int s, void *buf, size_t len);
	
private:
  UDPPlus *mainHandler;
  State currentState;

  boost::condition_variable inConditionEmpty;
  boost::condition_variable inConditionFull;
  boost::condition_variable outConditionEmpty;
  boost::condition_variable outConditionFull;
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

	struct sockaddr remote;

};

#endif /* UDPPLUSCONNECTION_H_ */
