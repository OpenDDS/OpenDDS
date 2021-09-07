/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "EntryExit.h"
#include "TransportSendElement.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

ACE_INLINE
TransportCustomizedElement::TransportCustomizedElement(
  TransportQueueElement* orig)
  : TransportQueueElement(1),
    orig_(orig),
    original_send_element_(find_original_send_element(orig)),
    publication_id_(orig ? orig->publication_id() : GUID_UNKNOWN),
    subscription_id_(GUID_UNKNOWN),
    sequence_(SequenceNumber::SEQUENCENUMBER_UNKNOWN()),
    fragment_(false),
    exclusive_(false)
{
  DBG_ENTRY_LVL("TransportCustomizedElement", "TransportCustomizedElement", 6);
}

ACE_INLINE
RepoId TransportCustomizedElement::publication_id() const
{
  DBG_ENTRY_LVL("TransportCustomizedElement", "publication_id", 6);
  return publication_id_;
}

ACE_INLINE
void TransportCustomizedElement::set_publication_id(const RepoId& id)
{
  publication_id_ = id;
}

ACE_INLINE
RepoId TransportCustomizedElement::subscription_id() const
{
  if (subscription_id_ != GUID_UNKNOWN) {
    return subscription_id_;
  }
  DBG_ENTRY_LVL("TransportCustomizedElement", "subscription_id", 6);
  const TransportSendElement* ose = original_send_element();
  return ose ? ose->subscription_id() : GUID_UNKNOWN;
}

ACE_INLINE
void TransportCustomizedElement::set_subscription_id(const RepoId& id)
{
  subscription_id_ = id;
}

ACE_INLINE
SequenceNumber
TransportCustomizedElement::sequence() const
{
  if (sequence_ != SequenceNumber::SEQUENCENUMBER_UNKNOWN()) {
    return sequence_;
  }
  return orig_ ? orig_->sequence()
    : SequenceNumber::SEQUENCENUMBER_UNKNOWN();
}

ACE_INLINE
void TransportCustomizedElement::set_sequence(const SequenceNumber& value)
{
  sequence_ = value;
}

ACE_INLINE
void TransportCustomizedElement::set_fragment(TransportQueueElement* orig)
{
  fragment_ = true;
  set_publication_id(orig->publication_id());
  set_subscription_id(orig->subscription_id());
  set_sequence(orig->sequence());
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
