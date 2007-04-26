// -*- C++ -*-
//
// $Id$

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "ThreadPerConRemoveVisitor.h"
#include "TransportQueueElement.h"
#include "dds/DCPS/DataSampleList.h"


#if !defined (__ACE_INLINE__)
#include "ThreadPerConRemoveVisitor.inl"
#endif /* __ACE_INLINE__ */


TAO::DCPS::ThreadPerConRemoveVisitor::~ThreadPerConRemoveVisitor()
{
  DBG_ENTRY("ThreadPerConRemoveVisitor","~ThreadPerConRemoveVisitor");
}


int
TAO::DCPS::ThreadPerConRemoveVisitor::visit_element(SendRequest* element,
                                                    int&         remove)
{
  DBG_ENTRY("ThreadPerConRemoveVisitor","visit_element");

  if ((this->sample_ != 0) && (element->op_ == SEND) && (*(element->element_) == this->sample_))
    {
      // We are visiting the element that we want to remove, since the
      // element "matches" our sample_.

      // In order to have the BasicQueue<T> remove the element that we
      // are currently visiting, set the remove flag to true (1).  The
      // BasicQueue<T> will perform the actual removal once we return
      // from this method.
      remove = 1;

      // Inform the element that we've made a decision - and it is
      // data_dropped()
      element->element_->data_dropped();

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


