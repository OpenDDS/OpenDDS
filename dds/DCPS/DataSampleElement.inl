/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <algorithm>

namespace OpenDDS {
namespace DCPS {

ACE_INLINE
DataSampleElement::DataSampleElement(
  PublicationId           publication_id,
  TransportSendListener*  send_listner,
  PublicationInstance*    handle,
  TransportSendElementAllocator* tse_allocator,
  TransportCustomizedElementAllocator* tce_allocator)
  : sample_(0),
    publication_id_(publication_id),
    num_subs_(0),
    send_listener_(send_listner),
    space_available_(false),
    handle_(handle),
    transport_send_element_allocator_(tse_allocator),
    transport_customized_element_allocator_(tce_allocator),
    previous_sample_(0),
    next_sample_(0),
    next_instance_sample_(0),
    next_send_sample_(0),
    previous_send_sample_(0)
    
{
}

ACE_INLINE
DataSampleElement::DataSampleElement(const DataSampleElement& elem)
  : header_(elem.header_)
  , sample_(elem.sample_->duplicate())
  , publication_id_(elem.publication_id_)
  , num_subs_(elem.num_subs_)
  , send_listener_(elem.send_listener_)
  , space_available_(elem.space_available_)
  , handle_(elem.handle_)
  , transport_send_element_allocator_(
      elem.transport_send_element_allocator_)
  , transport_customized_element_allocator_(
      elem.transport_customized_element_allocator_)
  , filter_out_(elem.filter_out_)
  , filter_per_link_(elem.filter_per_link_)
  , previous_sample_(elem.previous_sample_)
  , next_sample_(elem.next_sample_)
  , next_instance_sample_(elem.next_instance_sample_)
  , next_send_sample_(elem.next_send_sample_)
  , previous_send_sample_(elem.previous_send_sample_)
  
{
  std::copy(elem.subscription_ids_,
            elem.subscription_ids_ + num_subs_,
            subscription_ids_);

}

ACE_INLINE
DataSampleElement::~DataSampleElement()
{
  if (sample_) {
    sample_->release();
  }
}

ACE_INLINE
DataSampleElement&
DataSampleElement::operator=(const DataSampleElement& rhs)
{
  header_ = rhs.header_;
  sample_ = rhs.sample_->duplicate();
  publication_id_ = rhs.publication_id_;
  num_subs_ = rhs.num_subs_;
  std::copy(rhs.subscription_ids_,
            rhs.subscription_ids_ + num_subs_,
            subscription_ids_);
  previous_sample_ = rhs.previous_sample_;
  next_sample_ = rhs.next_sample_;
  next_instance_sample_ = rhs.next_instance_sample_;
  next_send_sample_ = rhs.next_send_sample_;
  previous_send_sample_ = rhs.previous_send_sample_;
  send_listener_ = rhs.send_listener_;
  space_available_ = rhs.space_available_;
  handle_ = rhs.handle_;
  transport_send_element_allocator_ = rhs.transport_send_element_allocator_;
  transport_customized_element_allocator_ =
    rhs.transport_customized_element_allocator_;
  filter_out_ = rhs.filter_out_;
  filter_per_link_ = rhs.filter_per_link_;

  return *this;
}


} // namespace DCPS
} // namespace OpenDDS
