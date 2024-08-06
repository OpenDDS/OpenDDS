#ifndef OPENDDS_DCPS_STATICINCLUDES_H
#define OPENDDS_DCPS_STATICINCLUDES_H

#include "Definitions.h"

#if OPENDDS_CONFIG_AUTO_STATIC_INCLUDES
// Explict checks depend on *_HAS_DLL macros being set to 0 to initialize static
// plugins.

#  if defined OPENDDS_INFOREPODISCOVERY_HAS_DLL && !OPENDDS_INFOREPODISCOVERY_HAS_DLL
#    include "InfoRepoDiscovery/InfoRepoDiscovery.h"
#  endif

#  if defined OPENDDS_RTPS_HAS_DLL && !OPENDDS_RTPS_HAS_DLL
#    include "RTPS/RtpsDiscovery.h"
#  endif

#  if defined OPENDDS_TCP_HAS_DLL && !OPENDDS_TCP_HAS_DLL
#    include "transport/tcp/Tcp.h"
#  endif

#  if defined OPENDDS_RTPS_UDP_HAS_DLL && !OPENDDS_RTPS_UDP_HAS_DLL
#    include "transport/rtps_udp/RtpsUdp.h"
#  endif

#  if defined OPENDDS_UDP_HAS_DLL && !OPENDDS_UDP_HAS_DLL
#    include "transport/udp/Udp.h"
#  endif

#  if defined OPENDDS_SHMEM_HAS_DLL && !OPENDDS_SHMEM_HAS_DLL
#    include "transport/shmem/Shmem.h"
#  endif

#  if defined OPENDDS_MULTICAST_HAS_DLL && !OPENDDS_MULTICAST_HAS_DLL
#    include "transport/multicast/Multicast.h"
#  endif

#  if defined OPENDDS_SECURITY_HAS_DLL && !OPENDDS_SECURITY_HAS_DLL
#    include "security/BuiltInPlugins.h"
#  endif

#elif OPENDDS_DO_MANUAL_STATIC_INCLUDES
// Only for static builds, include default libraries for both discovery
// and transport.  This file will be included by the code in the executable
// so that the static link includes the initializers for these libraries.
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
