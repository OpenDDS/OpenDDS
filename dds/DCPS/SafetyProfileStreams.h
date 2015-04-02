#ifndef SAFETY_PROFILE_STREAMS_H
#define SAFETY_PROFILE_STREAMS_H

#include "dds/DCPS/PoolAllocator.h"

#ifndef OPENDDS_SAFETY_PROFILE
#include <fstream>
#include <iostream>
#include <iomanip>

#ifdef ACE_LYNXOS_MAJOR
#include <strstream>
#else
#include <sstream>
#endif //ACE_LYNXOS_MAJOR

#endif //OPENDDS_SAFETY_PROFILE

#endif //SAFETY_PROFILE_STREAMS_H
