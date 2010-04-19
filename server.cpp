/*
 * server.cpp
 *
 *  Created on: Apr 15, 2010
 *      Author: Asad Saeed, Adam Darrah
 */

#include "utility.h"
#include "UDPPlus.h"

UDPPlus *conn;

int main(int argc, char* argv[]) {	
  
	conn = new UDPPlus(5, 512);
	
  return 0;
}
