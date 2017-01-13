/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <algorithm>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {


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

ACE_INLINE
void
DataSampleElement::set_transaction_id(ACE_UINT64 transaction_id)
{
  transaction_id_ = transaction_id;
}

ACE_INLINE
ACE_UINT64
DataSampleElement::transaction_id() const
{
  return transaction_id_;
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
