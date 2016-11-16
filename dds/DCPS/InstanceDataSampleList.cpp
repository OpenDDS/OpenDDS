/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "InstanceDataSampleList.h"
#include "DataSampleElement.h"
#include "Definitions.h"
#include "PublicationInstance.h"

#include "dds/DCPS/transport/framework/TransportSendListener.h"

#if !defined (__ACE_INLINE__)
#include "InstanceDataSampleList.inl"
#endif /* __ACE_INLINE__ */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

bool
InstanceDataSampleList::on_some_list(const DataSampleElement* iter)
{
  return iter->next_instance_sample_ || iter->previous_instance_sample_
    || (iter->handle_ && iter->handle_->samples_.head_ == iter);
}

bool
InstanceDataSampleList::dequeue(const DataSampleElement* stale)
{
  if (head_ == 0) {
    return false;
  }

  if (stale == head_) {
    DataSampleElement* tmp;
    return dequeue_head(tmp);
  }

  if (stale == tail_) {
    tail_ = tail_->previous_instance_sample_;
    tail_->next_instance_sample_ = 0;

  } else {
    stale->previous_instance_sample_->next_instance_sample_ =
      stale->next_instance_sample_;
    stale->next_instance_sample_->previous_instance_sample_ =
      stale->previous_instance_sample_;
  }

  stale->next_instance_sample_ = stale->previous_instance_sample_ = 0;
  --size_;
  return true;
}


} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
