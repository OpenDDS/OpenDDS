/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "TransportSendControlElement.h"
#include "TransportSendListener.h"
#include "dds/DCPS/transport/framework/EntryExit.h"

#if !defined (__ACE_INLINE__)
#include "TransportSendControlElement.inl"
#endif /* __ACE_INLINE__ */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {


TransportSendControlElement::TransportSendControlElement(int initial_count,
                                                         const RepoId& publisher_id,
                                                         TransportSendListener* listener,
                                                         const DataSampleHeader& header,
                                                         ACE_Message_Block* msg_block,
                                                         TransportSendControlElementAllocator* allocator)
  : TransportQueueElement(initial_count),
    publisher_id_(publisher_id),
    listener_(listener),
    header_(header),
    msg_(msg_block),
    dcps_elem_(0),
    allocator_(allocator)
{
  DBG_ENTRY_LVL("TransportSendControlElement","TransportSendControlElement",6);
}


TransportSendControlElement::TransportSendControlElement(int initial_count,
                                                         const DataSampleElement* dcps_elem,
                                                         TransportSendControlElementAllocator* allocator)
  : TransportQueueElement(initial_count)
  , publisher_id_(dcps_elem->get_pub_id())
  , listener_(dcps_elem->get_send_listener())
  , header_(dcps_elem->get_header())
  , msg_(dcps_elem->get_sample())
  , dcps_elem_(dcps_elem)
  , allocator_(allocator)
{
  DBG_ENTRY_LVL("TransportSendControlElement", "TransportSendControlElement", 6);
}

TransportSendControlElement::~TransportSendControlElement()
{
  DBG_ENTRY_LVL("TransportSendControlElement","~TransportSendControlElement",6);
}

bool
TransportSendControlElement::requires_exclusive_packet() const
{
  DBG_ENTRY_LVL("TransportSendControlElement","requires_exclusive_packet",6);
  return true;
}

namespace
{
  void handle_message(const bool dropped,
                      ACE_Message_Block* const msg,
                      TransportSendListener* const listener,
                      const bool dropped_by_transport)
  {
    if (dropped) {
      listener->control_dropped(msg, dropped_by_transport);
    } else {
      listener->control_delivered(msg);
    }
  }

  void handle_message(const bool dropped,
                      const DataSampleElement* const elem,
                      const bool dropped_by_transport)
  {
    TransportSendListener* const listener = elem->get_send_listener();
    if (dropped) {
      listener->data_dropped(elem, dropped_by_transport);
    } else {
      listener->data_delivered(elem);
    }
  }
}

void
TransportSendControlElement::release_element(bool dropped_by_transport)
{
  ACE_UNUSED_ARG(dropped_by_transport);

  DBG_ENTRY_LVL("TransportSendControlElement","release_element",6);

  // store off local copies to use after "this" pointer deleted
  const bool dropped = this->was_dropped();
  ACE_Message_Block* const msg = this->msg_;
  TransportSendListener* const listener = this->listener_;
  const DataSampleElement* const dcps_elem = dcps_elem_;

  if (allocator_) {
    OPENDDS_DES_FREE_THIS(allocator_->free, TransportSendControlElement);
  }

  // reporting the message w/o using "this" pointer
  if (dcps_elem) {
    handle_message(dropped, dcps_elem, dropped_by_transport);
  } else {
    handle_message(dropped, msg, listener, dropped_by_transport);
  }
}

RepoId
TransportSendControlElement::publication_id() const
{
  DBG_ENTRY_LVL("TransportSendControlElement","publication_id",6);
  return this->publisher_id_;
}

const ACE_Message_Block*
TransportSendControlElement::msg() const
{
  DBG_ENTRY_LVL("TransportSendControlElement","msg",6);
  return this->msg_;
}

const ACE_Message_Block*
TransportSendControlElement::msg_payload() const
{
  DBG_ENTRY_LVL("TransportSendControlElement", "msg_payload", 6);
  return this->msg_->cont();
}

bool
TransportSendControlElement::is_control(RepoId pub_id) const
{
  return (pub_id == this->publisher_id_);
}

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
