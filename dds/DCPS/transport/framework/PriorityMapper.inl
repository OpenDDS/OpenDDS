/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

ACE_INLINE
PriorityMapper::PriorityMapper(Priority priority)
  : priority_(priority)
{
}

ACE_INLINE
Priority&
PriorityMapper::priority()
{
  return this->priority_;
}

ACE_INLINE
Priority
PriorityMapper::priority() const
{
  return this->priority_;
}

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
