// -*- C++ -*-
//
// $Id$

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "QueueRemoveVisitor.h"
#include "TransportQueueElement.h"
#include "dds/DCPS/DataSampleList.h"


#if !defined (__ACE_INLINE__)
#include "QueueRemoveVisitor.inl"
#endif /* __ACE_INLINE__ */


OpenDDS::DCPS::QueueRemoveVisitor::~QueueRemoveVisitor()
{
  DBG_ENTRY_LVL("QueueRemoveVisitor","~QueueRemoveVisitor",5);
}


int
OpenDDS::DCPS::QueueRemoveVisitor::visit_element(TransportQueueElement* element,
                                             int&                   remove)
{
  DBG_ENTRY_LVL("QueueRemoveVisitor","visit_element",5);

  if (((this->sample_ != 0) && (*element == this->sample_)) ||
      ((this->sample_ == 0) && (element->is_control(this->pub_id_))))
    {
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
      // data_dropped()
      element->data_dropped();

      // Adjust our status_ to indicate that we actually found (and removed)
      // the sample.
      this->status_ = 1;

      if (this->sample_ != 0)
        {
          // Stop visitation since we've handled the element that matched
          // our sample_.
          return 0;
        }

      // When this->sample_ == 0, we visit every element.
    }

  // Continue visitation.
  return 1;
}


