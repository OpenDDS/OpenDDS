/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "EntryExit.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

ACE_INLINE
QueueRemoveVisitor::QueueRemoveVisitor(
  const TransportQueueElement::MatchCriteria& mc)
  : mc_(mc)
  , status_(REMOVE_NOT_FOUND)
  , removed_bytes_(0)
{
  DBG_ENTRY_LVL("QueueRemoveVisitor", "QueueRemoveVisitor", 6);
}

ACE_INLINE RemoveResult
QueueRemoveVisitor::status() const
{
  DBG_ENTRY_LVL("QueueRemoveVisitor", "status", 6);
  return this->status_;
}

ACE_INLINE int
QueueRemoveVisitor::removed_bytes() const
{
  DBG_ENTRY_LVL("QueueRemoveVisitor", "removed_bytes", 6);
  return static_cast<int>(this->removed_bytes_);
}

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
