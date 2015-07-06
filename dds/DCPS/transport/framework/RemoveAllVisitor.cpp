/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "RemoveAllVisitor.h"
#include "TransportQueueElement.h"

#if !defined (__ACE_INLINE__)
#include "RemoveAllVisitor.inl"
#endif /* __ACE_INLINE__ */

OpenDDS::DCPS::RemoveAllVisitor::~RemoveAllVisitor()
{
  DBG_ENTRY_LVL("RemoveAllVisitor","~RemoveAllVisitor",6);
}

int
OpenDDS::DCPS::RemoveAllVisitor::visit_element_remove(TransportQueueElement* element,
                                                      int&                   remove)
{
  DBG_ENTRY_LVL("RemoveAllVisitor","visit_element_remove",6);

  // Always remove the element passed in. Always set the remove flag
  // to true (1).  The BasicQueue<T> will perform the actual removal
  // once we return from this method.
  remove = 1;

  // Add the total_length() of the element->msg() chain to our
  // removed_bytes_ (running) total.
  if (element->msg()) {
    this->removed_bytes_ += element->msg()->total_length();
  }

  // Inform the element that we've made a decision - and it is
  // data_dropped()
  // This visitor is used in TransportSendStrategy::clear and
  // TransportSendBuffer::release. In formal case, the sample
  // is dropped by transport. In the later case, the
  // dropped_by_transport is not used as the sample is cloned
  // and no callback is made to writer.
  element->data_dropped(true);

  // Adjust our status_ to indicate that we actually found (and removed)
  // the sample.
  this->status_ = 1;

  // Continue visitation.
  return 1;
}
