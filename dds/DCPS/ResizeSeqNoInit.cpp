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

thread_local bool optional_init_allocator_must_init = true;

OpenDDS_Dcps_Export bool get_optional_init_allocator_must_init()
{
  return optional_init_allocator_must_init;
}

OptionalInitAllocatorScopedSetter::OptionalInitAllocatorScopedSetter(bool value)
: prev_value(optional_init_allocator_must_init)
{
  optional_init_allocator_must_init = value;
}

OptionalInitAllocatorScopedSetter::~OptionalInitAllocatorScopedSetter()
{
  optional_init_allocator_must_init = prev_value;
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // ACE_HAS_CPP11
