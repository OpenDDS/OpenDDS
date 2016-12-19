/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "Shmem.h"
#include "ShmemLoader.h"

#include "ace/Service_Config.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

ShmemInitializer::ShmemInitializer()
{
  ACE_Service_Config::process_directive(ace_svc_desc_ShmemLoader);

#if (OPENDDS_SHMEM_HAS_DLL == 0)
  ACE_Service_Config::process_directive(ACE_TEXT("static OpenDDS_Shmem"));
#endif  /* OPENDDS_SHMEM_HAS_DLL */
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
