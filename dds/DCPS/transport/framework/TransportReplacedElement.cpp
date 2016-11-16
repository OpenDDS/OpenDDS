/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "TransportReplacedElement.h"

#if !defined (__ACE_INLINE__)
#include "TransportReplacedElement.inl"
#endif /* __ACE_INLINE__ */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

OpenDDS::DCPS::TransportReplacedElement::~TransportReplacedElement()
{
  DBG_ENTRY_LVL("TransportReplacedElement","~TransportReplacedElement",6);
}

void
OpenDDS::DCPS::TransportReplacedElement::release_element(bool dropped_by_transport)
{
  ACE_UNUSED_ARG(dropped_by_transport);
  DBG_ENTRY_LVL("TransportReplacedElement","release_element",6);

  if (this->msg_ != 0) {
    this->msg_->release();
    this->msg_ = 0;
  }

  if (allocator_) {
    allocator_->free(this);
  }
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
