/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "Udp.h"
#include "UdpLoader.h"

#include "ace/Service_Config.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

UdpInitializer::UdpInitializer()
{
  ACE_Service_Config::process_directive(ace_svc_desc_UdpLoader);

#if (OPENDDS_UDP_HAS_DLL == 0)
  ACE_Service_Config::process_directive(ACE_TEXT("static OpenDDS_Udp"));
#endif  /* OPENDDS_UDP_HAS_DLL */
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
