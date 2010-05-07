/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "QueueRemoveVisitor.h"
#include "TransportQueueElement.h"
#include "dds/DCPS/DataSampleList.h"

#if !defined (__ACE_INLINE__)
#include "QueueRemoveVisitor.inl"
#endif /* __ACE_INLINE__ */

OpenDDS::DCPS::QueueRemoveVisitor::~QueueRemoveVisitor()
{
  DBG_ENTRY_LVL("QueueRemoveVisitor","~QueueRemoveVisitor",6);
}

int
OpenDDS::DCPS::QueueRemoveVisitor::visit_element_remove(TransportQueueElement* element,
                                                        int&                   remove)
{
  DBG_ENTRY_LVL("QueueRemoveVisitor","visit_element_remove",6);

  if( this->sample_ == *element) {
    // We are visiting the element that we want to remove, since the
    // element "matches" our sample_.

    // In order to have the BasicQueue<T> remove the element that we
    // are currently visiting, set the remove flag to true (1).  The
    // BasicQueue<T> will perform the actual removal once we return
    // from this method.
    remove = 1;

    // Add the total_length() of the element->msg() chain to our
    // removed_bytes_ (running) total.
    this->removed_bytes_ += element->msg()->total_length();

    // Inform the element that we've made a decision - and it is
    // data_dropped() by transport.
    // This visitor is used in TransportSendStrategy::do_remove_sample
    // The dropped_by_transport flag should be false(default) as the
    // data_dropped is resulted from writer's remove_sample call.
    this->sample_.released (element->data_dropped());

    // Adjust our status_ to indicate that we actually found (and removed)
    // the sample.
    this->status_ = 1;

    if (this->sample_.msg() != 0) {
      // Stop visitation since we've handled the element that matched
      // our sample_.
      // N.B. This test means that if we are comparing by sample, we
      //      remove just the single element matching the sample, but if
      //      we are comparing by publication Id value, we visit the
      //      entire chain and remove all samples originating from that
      //      publication Id.
      return 0;
    }
  }

  // Continue visitation.
  return 1;
}
