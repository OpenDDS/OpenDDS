/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
//#include "DataSampleList.h"
#include "InstanceDataSampleList.h"
#include "DataSampleElement.h"
#include "Definitions.h"
#include "PublicationInstance.h"

#include "dds/DCPS/transport/framework/TransportSendListener.h"

#if !defined (__ACE_INLINE__)
#include "InstanceDataSampleList.inl"
#endif /* __ACE_INLINE__ */

namespace OpenDDS {
namespace DCPS {


bool
InstanceDataSampleList::dequeue(const DataSampleElement* stale)
{
  if (head_ == 0) {
    return false;
  }

  // Same as dequeue from head.
  if (stale == head_) {
    DataSampleElement* tmp = head_;
    return dequeue_head(tmp);
  }

  // Search from head_->next_instance_sample_.
  DataSampleElement* previous = head_;
  DataSampleElement* item;
  for (item = head_->next_instance_sample_;
       item != 0;
       item = item->next_instance_sample_) {
    if (item == stale) {
      previous->next_instance_sample_ = item->next_instance_sample_;
      if (previous->next_instance_sample_ == 0) {
        tail_ = previous;
      }
      --size_ ;
      item->next_instance_sample_ = 0;
      break;
    }

    previous = item;
  }

  return item;
}


} // namespace DCPS
} // namespace OpenDDS
