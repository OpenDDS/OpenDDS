/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
//#include "DataSampleList.h"
#include "DataSampleWriterList.h"
#include "DataSampleListElement.h"
#include "Definitions.h"
#include "PublicationInstance.h"

#include "dds/DCPS/transport/framework/TransportSendListener.h"

#if !defined (__ACE_INLINE__)
#include "DataSampleWriterList.inl"
#endif /* __ACE_INLINE__ */

namespace OpenDDS {
namespace DCPS {

bool
DataSampleWriterList::dequeue(const DataSampleListElement* stale)
{
  if (head_ == 0) {
    return false;
  }

  if (stale == head_) {
    DataSampleListElement* head = head_;
    return dequeue_head(head);
  }

  // Search from head_->next_sample_.
  bool found = false;

  for (DataSampleListElement* item = head_->next_sample_ ;
       item != 0 ;
       item = item->next_sample_) {
    if (item == stale) {
      found = true;
      break;
    }
  }

  if (found) {
    // Adjust list size.
    -- size_ ;

    //
    // Remove from the previous element.
    //
    if (stale->previous_sample_ != 0) {
      // Remove from inside of the list.
      stale->previous_sample_->next_sample_ = stale->next_sample_ ;

    } else {
      // Remove from the head of the list.
      head_ = stale->next_sample_ ;

      if (head_ != 0) {
        head_->previous_sample_ = 0;
      }
    }

    //
    // Remove from the next element.
    //
    if (stale->next_sample_ != 0) {
      // Remove the inside of the list.
      stale->next_sample_->previous_sample_ = stale->previous_sample_ ;

    } else {
      // Remove from the tail of the list.
      tail_ = stale->previous_sample_ ;
    }

    stale->next_sample_ = 0;
    stale->previous_sample_ = 0;
  }

  return found;
}


} // namespace DCPS
} // namespace OpenDDS
