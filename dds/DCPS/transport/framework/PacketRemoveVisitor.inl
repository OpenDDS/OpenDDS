/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "EntryExit.h"

ACE_INLINE OpenDDS::DCPS::RemoveResult
OpenDDS::DCPS::PacketRemoveVisitor::status() const
{
  DBG_ENTRY_LVL("PacketRemoveVisitor", "status", 6);
  return this->status_;
}
