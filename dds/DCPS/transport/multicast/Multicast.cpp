/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "Multicast.h"
#include "MulticastLoader.h"

#include "ace/Service_Config.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

MulticastInitializer::MulticastInitializer()
{
  ACE_Service_Config::process_directive(ace_svc_desc_MulticastLoader);

#if (OPENDDS_MULTICAST_HAS_DLL == 0)
  ACE_Service_Config::process_directive(ACE_TEXT("static OpenDDS_Multicast"));
#endif  /* OPENDDS_MULTICAST_HAS_DLL */
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
