// Only for static builds, include default libraries for both discovery
// and transport.  This file will be included by the code in the executable
// so that the static link includes the initializers for these libraries.
#ifdef ACE_AS_STATIC_LIBS
#include "dds/DCPS/InfoRepoDiscovery/InfoRepoDiscovery.h"
#include "dds/DCPS/transport/tcp/Tcp.h"
#endif
