/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "EntryExit.h"

ACE_INLINE
OpenDDS::DCPS::RemoveAllVisitor::RemoveAllVisitor()
  : status_(0),
    removed_bytes_(0)
{
  DBG_ENTRY_LVL("RemoveAllVisitor","RemoveAllVisitor",6);
}

ACE_INLINE int
OpenDDS::DCPS::RemoveAllVisitor::status() const
{
  DBG_ENTRY_LVL("RemoveAllVisitor","status",6);
  return this->status_;
}

ACE_INLINE int
OpenDDS::DCPS::RemoveAllVisitor::removed_bytes() const
{
  DBG_ENTRY_LVL("RemoveAllVisitor","removed_bytes",6);
  return static_cast<int>(this->removed_bytes_);
}
