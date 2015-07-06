/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "BuildChainVisitor.h"
#include "TransportQueueElement.h"

#if !defined (__ACE_INLINE__)
#include "BuildChainVisitor.inl"
#endif /* __ACE_INLINE__ */

OpenDDS::DCPS::BuildChainVisitor::~BuildChainVisitor()
{
  DBG_ENTRY_LVL("BuildChainVisitor","~BuildChainVisitor",6);
}

int
OpenDDS::DCPS::BuildChainVisitor::visit_element(TransportQueueElement* element)
{
  DBG_ENTRY_LVL("BuildChainVisitor","visit_element",6);

  if (this->head_ == 0) {
    // This is the first element that we have visited.
    this->head_ = element->msg()->duplicate();
    this->tail_ = this->head_;

    while (this->tail_->cont() != 0) {
      this->tail_ = this->tail_->cont();
    }

  } else {
    // This is not the first element that we have visited.
    this->tail_->cont(element->msg()->duplicate());

    while (this->tail_->cont() != 0) {
      this->tail_ = this->tail_->cont();
    }
  }

  // Visit entire queue.
  return 1;
}

