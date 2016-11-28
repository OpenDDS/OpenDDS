/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "WriterDataSampleList.h"
#include "DataSampleElement.h"

#if !defined (__ACE_INLINE__)
#include "WriterDataSampleList.inl"
#endif /* __ACE_INLINE__ */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

bool
WriterDataSampleList::dequeue(const DataSampleElement* stale)
{
  if (head_ == 0) {
    return false;
  }

  if (stale == head_) {
    DataSampleElement* head = head_;
    return dequeue_head(head);
  }

  // Search from head_->next_writer_sample_.
  bool found = false;

  for (DataSampleElement* item = head_->next_writer_sample_ ;
       item != 0 ;
       item = item->next_writer_sample_) {
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
    if (stale->previous_writer_sample_ != 0) {
      // Remove from inside of the list.
      stale->previous_writer_sample_->next_writer_sample_ = stale->next_writer_sample_ ;

    } else {
      // Remove from the head of the list.
      head_ = stale->next_writer_sample_ ;

      if (head_ != 0) {
        head_->previous_writer_sample_ = 0;
      }
    }

    //
    // Remove from the next element.
    //
    if (stale->next_writer_sample_ != 0) {
      // Remove the inside of the list.
      stale->next_writer_sample_->previous_writer_sample_ = stale->previous_writer_sample_ ;

    } else {
      // Remove from the tail of the list.
      tail_ = stale->previous_writer_sample_ ;
    }

    stale->next_writer_sample_ = 0;
    stale->previous_writer_sample_ = 0;
  }

  return found;
}


} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
