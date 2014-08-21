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
  TransportSendListener*  send_listener,
  PublicationInstance*    handle,
  TransportSendElementAllocator* tse_allocator,
  TransportCustomizedElementAllocator* tce_allocator)
  : sample_(0),
    publication_id_(publication_id),
    num_subs_(0),
    send_listener_(send_listener),
    space_available_(false),
    handle_(handle),
    transport_send_element_allocator_(tse_allocator),
    transport_customized_element_allocator_(tce_allocator),
    previous_writer_sample_(0),
    next_writer_sample_(0),
    next_instance_sample_(0),
    next_send_sample_(0),
    previous_send_sample_(0)

{
	std::fill(subscription_ids_,
			  subscription_ids_ + OpenDDS::DCPS::MAX_READERS_PER_ELEM,
			  GUID_UNKNOWN);
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
  , previous_writer_sample_(elem.previous_writer_sample_)
  , next_writer_sample_(elem.next_writer_sample_)
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
  previous_writer_sample_ = rhs.previous_writer_sample_;
  next_writer_sample_ = rhs.next_writer_sample_;
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

ACE_INLINE
DataSampleElement*
DataSampleElement::get_next_send_sample() const
{
  return next_send_sample_;
};

ACE_INLINE
void
DataSampleElement::set_next_send_sample(DataSampleElement* next_send_sample)
{
  this->next_send_sample_ = next_send_sample;
};

ACE_INLINE
const DataSampleHeader&
DataSampleElement::get_header() const
{
  return header_;
}

ACE_INLINE
DataSampleHeader&
DataSampleElement::get_header()
{
  return const_cast<DataSampleHeader&>(static_cast<const DataSampleElement &>(*this).get_header());
}

ACE_INLINE
DataSample*
DataSampleElement::get_sample() const
{
  return sample_;
}

ACE_INLINE
DataSample*
DataSampleElement::get_sample()
{
  return sample_;
}

ACE_INLINE
void
DataSampleElement::set_sample(DataSample* sample)
{
  this->sample_ = sample;
}

ACE_INLINE
PublicationId
DataSampleElement::get_pub_id() const
{
  return publication_id_;
}

ACE_INLINE
CORBA::ULong
DataSampleElement::get_num_subs() const
{
  return num_subs_;
}

ACE_INLINE
void
DataSampleElement::set_num_subs(int num_subs)
{
  this->num_subs_ = num_subs;
}

ACE_INLINE
const OpenDDS::DCPS::RepoId*
DataSampleElement::get_sub_ids() const
{
  return subscription_ids_;
}

ACE_INLINE
OpenDDS::DCPS::RepoId
DataSampleElement::get_sub_id(int index) const
{
  return subscription_ids_[index];
}

ACE_INLINE
void
DataSampleElement::set_sub_id(int index, OpenDDS::DCPS::RepoId id)
{
  this->subscription_ids_[index] = id;
}

ACE_INLINE
TransportSendListener*
DataSampleElement::get_send_listener() const
{
  return send_listener_;
}

ACE_INLINE
TransportSendListener*
DataSampleElement::get_send_listener()
{
  return send_listener_;
}

ACE_INLINE
bool
DataSampleElement::space_available() const
{
  return space_available_;
}

ACE_INLINE
void
DataSampleElement::set_space_available(bool is_space_available)
{
  this->space_available_ = is_space_available;
}

ACE_INLINE
PublicationInstance*
DataSampleElement::get_handle() const
{
  return handle_;
}

ACE_INLINE
TransportSendElementAllocator*
DataSampleElement::get_transport_send_element_allocator() const
{
  return transport_send_element_allocator_;
}

ACE_INLINE
TransportCustomizedElementAllocator*
DataSampleElement::get_transport_customized_element_allocator() const
{
  return transport_customized_element_allocator_;
}

ACE_INLINE
DataSampleElement::DataLinkIdTypeGUIDMap&
DataSampleElement::get_filter_per_link()
{
  return filter_per_link_;
}

ACE_INLINE
void
DataSampleElement::set_filter_out(GUIDSeq *filter_out)
{
  filter_out_ = filter_out;
}

} // namespace DCPS
} // namespace OpenDDS
