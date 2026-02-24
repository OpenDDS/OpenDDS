/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <DCPS/DdsDcps_pch.h> // Only the _pch include should start with DCPS/

#include "ResizeSeqNoInit.h"

#ifdef ACE_HAS_CPP11

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

thread_local OpenDDS_Dcps_Export bool init_in_optional_init_allocator = true;

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // ACE_HAS_CPP11
