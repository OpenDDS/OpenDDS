/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "EntryExit.h"

ACE_INLINE
OpenDDS::DCPS::BuildChainVisitor::BuildChainVisitor()
  : head_(0),
    tail_(0)
{
  DBG_ENTRY_LVL("BuildChainVisitor","BuildChainVisitor",6);
}

ACE_INLINE
ACE_Message_Block*
OpenDDS::DCPS::BuildChainVisitor::chain()
{
  DBG_ENTRY_LVL("BuildChainVisitor","chain",6);

  ACE_Message_Block* head = this->head_;
  this->head_ = this->tail_ = 0;
  return head;
}
