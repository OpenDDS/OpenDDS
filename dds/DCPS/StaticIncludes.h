// Only for static builds, include default libraries for both discovery
// and transport.  This file will be included by the code in the executable
// so that the static link includes the initializers for these libraries.
#ifdef ACE_AS_STATIC_LIBS
# ifdef OPENDDS_SAFETY_PROFILE
#  include "dds/DCPS/StaticDiscovery.h"
#  include "dds/DCPS/RTPS/RtpsDiscovery.h"
#  include "dds/DCPS/transport/rtps_udp/RtpsUdp.h"
# else
#  include "dds/DCPS/InfoRepoDiscovery/InfoRepoDiscovery.h"
#  include "dds/DCPS/transport/tcp/Tcp.h"
# endif
#endif
