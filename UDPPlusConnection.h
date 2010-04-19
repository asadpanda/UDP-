/*
 * UDPPlusConnection.h
 *
 *  Created on: Apr 19, 2010
 *      Author: asaeed
 */

#ifndef UDPPLUSCONNECTION_H_
#define UDPPLUSCONNECTION_H_

#include "utility.h"

enum State { LISTEN, SYN_SENT, SYN_RECIEVED, ESTABLISHED, FIN_WAIT1, FIN_WAIT2, CLOSE_WAIT, CLOSING, LAST_ACK };

class UDPPlusConnection {
public:
  UDPPlusConnection();
  virtual ~UDPPlusConnection();

  handlePacket(&Packet);
private:
  UDPPlus *mainHandler;
  State currentState;

  boost::condition_variable inConditionEmpty;
  boost::condition_variable inConditionFull;
  boost::condition_variable outConditionEmpty;
  boost::condition_variable outConditionFull;
  boost::mutex inBufferLock;
  boost::mutex outBufferLock;
  Packet **inBuffer; // for array of pointers
  Packet **outBuffer;
  unsigned inBufferSize= 0;
  unsigned outBufferSize = 0;
  uint16_t inBufferBegin = 0;
  uint16_t inBufferEnd = 0;
  uint16_t outBufferBegin = 0;
  uint16_t outBufferEnd = 0;
  uint16_t newAckNum = 0;
  uint16_t newSeqNum = 0;
  uint16_t inItems = 0;
  uint16_t outItems = 0;
  uint8_t numAck = 0;
  uint16_t lastAckRecv = 0;

};

#endif /* UDPPLUSCONNECTION_H_ */
