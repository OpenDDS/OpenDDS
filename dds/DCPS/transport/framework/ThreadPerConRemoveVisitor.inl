/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "EntryExit.h"

ACE_INLINE
OpenDDS::DCPS::ThreadPerConRemoveVisitor::ThreadPerConRemoveVisitor(
  const ACE_Message_Block* sample)
  : sample_(sample)
  , status_(REMOVE_NOT_FOUND)
{
  DBG_ENTRY("ThreadPerConRemoveVisitor", "ThreadPerConRemoveVisitor");
}

ACE_INLINE OpenDDS::DCPS::RemoveResult
OpenDDS::DCPS::ThreadPerConRemoveVisitor::status() const
{
  DBG_ENTRY("ThreadPerConRemoveVisitor", "status");
  return this->status_;
}
