// -*- C++ -*-
//
// $Id$

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "RemoveAllVisitor.h"
#include "TransportQueueElement.h"
#include "dds/DCPS/DataSampleList.h"


#if !defined (__ACE_INLINE__)
#include "RemoveAllVisitor.inl"
#endif /* __ACE_INLINE__ */


TAO::DCPS::RemoveAllVisitor::~RemoveAllVisitor()
{
  DBG_ENTRY_LVL("RemoveAllVisitor","~RemoveAllVisitor",5);
}


int
TAO::DCPS::RemoveAllVisitor::visit_element(TransportQueueElement* element,
                                           int&                   remove)
{
  DBG_ENTRY_LVL("RemoveAllVisitor","visit_element",5);

  // Always remove the element passed in. Always set the remove flag
  // to true (1).  The BasicQueue<T> will perform the actual removal
  // once we return from this method.
  remove = 1;

  // Add the total_length() of the element->msg() chain to our
  // removed_bytes_ (running) total.
  this->removed_bytes_ += element->msg()->total_length();

  // Inform the element that we've made a decision - and it is
  // data_dropped()
  element->data_dropped();

  // Adjust our status_ to indicate that we actually found (and removed)
  // the sample.
  this->status_ = 1;

  // Continue visitation.
  return 1;
}


