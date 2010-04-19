#ifndef UTILITY_H
#define UTILITY_H

//Gives ANSI version of standard includes
//Also defines enumerated type for error
//messages

#include <iostream>
#include <limits>
#include <cmath>
#include <cstdlib>
#include <cstddef>
#include <fstream>
#include <cctype>
#include <ctime>
#include <string>
#include <vector>

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <boost/thread.hpp> // boost install instructions can be found here: http://www.technoboria.com/2009/07/simple-guide-to-installing-boost-on-mac-os-x/
                            // make sure to move /boost directory to /usr/include directory
#include <boost/bind.hpp>

using namespace std;

enum Error_code { success, fail, exceeds_range,
not_present, duplicate_error, underflow, overflow };

#endif //UTILITY_H
