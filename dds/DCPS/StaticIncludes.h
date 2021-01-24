#ifndef OPENDDS_DCPS_STATICINCLUDES_H
#define OPENDDS_DCPS_STATICINCLUDES_H

// Only for static builds, include default libraries for both discovery
// and transport.  This file will be included by the code in the executable
// so that the static link includes the initializers for these libraries.
#ifdef ACE_AS_STATIC_LIBS
#  ifdef OPENDDS_SAFETY_PROFILE
#    include "StaticDiscovery.h"
#    include "RTPS/RtpsDiscovery.h"
#    include "transport/rtps_udp/RtpsUdp.h"
#  else
#    include "InfoRepoDiscovery/InfoRepoDiscovery.h"
#    include "transport/tcp/Tcp.h"
#  endif
#endif

#endif // OPENDDS_DCPS_STATICINCLUDES_H
