/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "Udp.h"
#include "UdpLoader.h"

#include "ace/Service_Config.h"

namespace OpenDDS {
namespace DCPS {

UdpInitializer::UdpInitializer()
{
  ACE_Service_Config::process_directive(ace_svc_desc_UdpLoader);

#if (OPENDDS_UDP_HAS_DLL == 0)
  ACE_Service_Config::process_directive(ACE_TEXT("static UdpLoader"));
#endif  /* OPENDDS_UDP_HAS_DLL */
}

} // namespace DCPS
} // namespace OpenDDS
