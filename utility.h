#ifndef UTILITY_H
#define UTILITY_H

//Gives ANSI version of standard includes
//Also defines enumerated type for error
//messages

#include <iostream>
#include <sstream>
#include <limits>
#include <cmath>
#include <cstdlib>
#include <cstddef>
#include <fstream>
#include <cctype>
#include <ctime>
#include <string>
#include <vector>
#include <deque>

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

// we used the free library "boost" for cross platform threading

//          Copyright Joe Coder 2004 - 2006.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <boost/thread.hpp>
#include <boost/thread/condition.hpp>
#include <boost/bind.hpp>
#include <boost/date_time.hpp>
//#include <pthread.h>

#define CONNECTION_CLOSED -1

using namespace std;

enum Error_code { success, error, closed, exceeds_range,
not_present, duplicate_error, underflow, overflow };

template<class T, void(T::*mem_fn)()>
void* functionProxy(void* p)
{
  (static_cast<T*>(p)->*mem_fn)();
  return 0;
}

#endif //UTILITY_H
