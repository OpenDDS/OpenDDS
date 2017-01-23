/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "DataSampleElement.h"
#include "PublicationInstance.h"

#if !defined (__ACE_INLINE__)
#include "DataSampleElement.inl"
#endif /* __ACE_INLINE__ */


OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

DataSampleElement::DataSampleElement(
  PublicationId           publication_id,
  TransportSendListener*  send_listener,
  PublicationInstance_rch handle,
  TransportSendElementAllocator* tse_allocator,
  TransportCustomizedElementAllocator* tce_allocator)
  : transaction_id_(0),
    sample_(0),
    publication_id_(publication_id),
    num_subs_(0),
    send_listener_(send_listener),
    handle_(handle),
    transport_send_element_allocator_(tse_allocator),
    transport_customized_element_allocator_(tce_allocator),
    previous_writer_sample_(0),
    next_writer_sample_(0),
    next_instance_sample_(0),
    previous_instance_sample_(0),
    next_send_sample_(0),
    previous_send_sample_(0)

{
  std::fill(subscription_ids_,
    subscription_ids_ + OpenDDS::DCPS::MAX_READERS_PER_ELEM,
    GUID_UNKNOWN);
}

DataSampleElement::DataSampleElement(const DataSampleElement& elem)
  : transaction_id_(elem.transaction_id_)
  , header_(elem.header_)
  , sample_(elem.sample_ ? elem.sample_->duplicate() : 0)
  , publication_id_(elem.publication_id_)
  , num_subs_(elem.num_subs_)
  , send_listener_(elem.send_listener_)
  , handle_(elem.handle_)
  , transport_send_element_allocator_(
      elem.transport_send_element_allocator_)
  , transport_customized_element_allocator_(
      elem.transport_customized_element_allocator_)
  , filter_out_(elem.filter_out_)
  , filter_per_link_(elem.filter_per_link_)
  , previous_writer_sample_(elem.previous_writer_sample_)
  , next_writer_sample_(elem.next_writer_sample_)
  , next_instance_sample_(elem.next_instance_sample_)
  , previous_instance_sample_(elem.previous_instance_sample_)
  , next_send_sample_(elem.next_send_sample_)
  , previous_send_sample_(elem.previous_send_sample_)

{
  std::copy(elem.subscription_ids_,
            elem.subscription_ids_ + num_subs_,
            subscription_ids_);
}

DataSampleElement::~DataSampleElement()
{
  if (sample_) {
    sample_->release();
  }
}

DataSampleElement&
DataSampleElement::operator=(const DataSampleElement& rhs)
{
  transaction_id_ = rhs.transaction_id_;
  header_ = rhs.header_;
  sample_ = rhs.sample_->duplicate();
  publication_id_ = rhs.publication_id_;
  num_subs_ = rhs.num_subs_;

  std::copy(rhs.subscription_ids_,
            rhs.subscription_ids_ + num_subs_,
            subscription_ids_);

  previous_writer_sample_ = rhs.previous_writer_sample_;
  next_writer_sample_ = rhs.next_writer_sample_;
  next_instance_sample_ = rhs.next_instance_sample_;
  previous_instance_sample_ = rhs.previous_instance_sample_;
  next_send_sample_ = rhs.next_send_sample_;
  previous_send_sample_ = rhs.previous_send_sample_;
  send_listener_ = rhs.send_listener_;
  handle_ = rhs.handle_;
  transport_send_element_allocator_ = rhs.transport_send_element_allocator_;
  transport_customized_element_allocator_ =
    rhs.transport_customized_element_allocator_;
  filter_out_ = rhs.filter_out_;
  filter_per_link_ = rhs.filter_per_link_;

  return *this;
}

PublicationInstance_rch
DataSampleElement::get_handle() const
{
  return handle_;
}


} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

