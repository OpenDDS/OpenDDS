#ifndef SAFETY_PROFILE_STREAMS_H
#define SAFETY_PROFILE_STREAMS_H

#include "dds/DCPS/PoolAllocator.h"

#ifndef OPENDDS_SAFETY_PROFILE
#include <fstream>
#include <iostream>
#include <iomanip>

#ifdef ACE_LYNXOS_MAJOR
#include <strstream>
#define STRINGSTREAM std::strstream
#define STRINGSTREAM_CSTR
#else
#include <sstream>
#define STRINGSTREAM std::stringstream
#define STRINGSTREAM_CSTR .c_str()
#endif


#endif //OPENDDS_SAFETY_PROFILE

#endif //SAFETY_PROFILE_STREAMS_H
