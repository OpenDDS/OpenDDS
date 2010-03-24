/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "TransportSendListener.h"
#include "TransportImpl.h"
#include "DataLinkSet.h"
#include "DataLinkSetMap.h"
#include "DataLink.h"
#include "dds/DCPS/AssociationData.h"
#include "dds/DCPS/DataSampleList.h"
#include "dds/DCPS/RepoIdConverter.h"
#include "ace/Message_Block.h"
#include "EntryExit.h"

ACE_INLINE
OpenDDS::DCPS::TransportInterface::TransportInterface()
  : swap_bytes_(0)
{
  DBG_ENTRY_LVL("TransportInterface","TransportInterface",6);
  this->send_links_ = new DataLinkSet();
}

/// Return the connection_info_ that we retrieved from the TransportImpl
/// in our attach_transport() method.  Once we retrieve it from the
/// TransportImpl (in the attach_transport() method), we "cache" it in
/// a data member (this->connection_info_) so that we don't have to ask
/// the TransportImpl for it each time.
ACE_INLINE const OpenDDS::DCPS::TransportInterfaceInfo&
OpenDDS::DCPS::TransportInterface::connection_info() const
{
  DBG_ENTRY_LVL("TransportInterface","connection_info",6);
  return this->connection_info_;
}

ACE_INLINE int
OpenDDS::DCPS::TransportInterface::swap_bytes() const
{
  DBG_ENTRY_LVL("TransportInterface","swap_bytes",6);
  return this->swap_bytes_;
}

ACE_INLINE OpenDDS::DCPS::SendControlStatus
OpenDDS::DCPS::TransportInterface::send_control(RepoId                 pub_id,
                                                TransportSendListener* listener,
                                                ACE_Message_Block*     msg)
{
  DBG_ENTRY_LVL("TransportInterface","send_control",6);

  DataLinkSet_rch pub_links = this->local_map_.find_set(pub_id);

  if (pub_links.is_nil()) {
    // We get here if there aren't any remote subscribers that (currently)
    // have an interest in this publisher id.  Just like in the case of
    // the send() method, this is not an error.  A better way to understand
    // would be if this send_control() method were renamed to:
    //   send_control_to_all_interested_remote_subscribers(pub_id,...).
    //
    // And when we get to this spot in the logic, we have still fulfilled
    // our duties - we sent it to all interested remote subscribers - all
    // zero of them.
    listener->control_delivered(msg);
    return SEND_CONTROL_OK;

  } else {
    // Just have the DataLinkSet do the send_control for us, on each
    // DataLink in the set.
    return pub_links->send_control(pub_id, listener, msg);
  }
}

ACE_INLINE bool
OpenDDS::DCPS::TransportInterface::send_response(
  RepoId             pub_id,
  ACE_Message_Block* msg)
{
  DBG_ENTRY_LVL("TransportInterface","send_response",6);

  DataLinkSet_rch links = this->remote_map_.find_set(pub_id);

  if (links.is_nil()) {
    // No link to publication.
    msg->release();
    RepoIdConverter converter(pub_id);
    ACE_DEBUG((LM_WARNING,
               ACE_TEXT("(%P|%t) WARNING: TransportInterface::send_response() - ")
               ACE_TEXT("unable to find link to send SAMPLE_ACK message on to ")
               ACE_TEXT("reach publication %C.\n"),
               std::string(converter).c_str()));
    this->remote_map_.dump();
    return false;

  } else {
    links->send_response(pub_id, msg);
    return true;
  }

}

ACE_INLINE int
OpenDDS::DCPS::TransportInterface::remove_sample
(const DataSampleListElement* sample,
 bool  dropped_by_transport)
{
  DBG_ENTRY_LVL("TransportInterface","remove_sample",6);

  // ciju: After discussions with Tim B., we feel strongly feel that
  // this section should be protected with some sort of locking mechanism.
  // The pub_links could become invalid anytime after the find_set().
  // I believe it best to use the TransportInterface lock to protect
  // this area.

  // ciju: The lock isn't necessary. The DataLinkSet is ref counted and therefore
  // isn't going anywhere untill this method exits.

  DataLinkSet_rch pub_links =
    this->local_map_.find_set(sample->publication_id_);

  // We only need to do something here if the publication_id_ is associated
  // with at least one DataLink.  If it is not associated with any DataLinks
  // (ie, pub_links.is_nil() is true), then we don't have anything to do
  // here, and we don't consider this an error condition - just return 0
  // in the "no DataLinks associated with the sample->publication_id_" case.
  if (!pub_links.is_nil()) {
    // Tell the DataLinkSet to tell each DataLink in the set to attempt
    // the remove_sample() operation.
    return pub_links->remove_sample(sample, dropped_by_transport);
  }

  // The sample->publication_id_ isn't associated with any DataLinks, so
  // there are no samples to even attempt to remove.  This isn't considered
  // an error condition (which would require a -1 to be returned) - it just
  // means we do nothing except return 0.
  return -1;
}

ACE_INLINE int
OpenDDS::DCPS::TransportInterface::remove_all_msgs(RepoId pub_id)
{
  DBG_ENTRY_LVL("TransportInterface","remove_all_msgs",6);

  DataLinkSet_rch pub_links = this->local_map_.find_set(pub_id);

  if (!pub_links.is_nil()) {
    return pub_links->remove_all_msgs(pub_id);
  }

  return 0;
}

ACE_INLINE int
OpenDDS::DCPS::TransportInterface::add_subscriptions(
  RepoId                  local_id,
  const AssociationInfo&  info,
  CORBA::Long             priority,
  TransportSendListener*  send_listener)
{
  DBG_ENTRY_LVL("TransportInterface","add_subscriptions",6);
  // Delegate to generic add_associations operation
  return this->add_associations(local_id,
                                info,
                                priority,
                                0,
                                send_listener);
}

ACE_INLINE int
OpenDDS::DCPS::TransportInterface::add_publications(
  RepoId                    local_id,
  const AssociationInfo&    info,
  CORBA::Long               priority,
  TransportReceiveListener* receive_listener)
{
  DBG_ENTRY_LVL("TransportInterface","add_publications",6);
  // Delegate to generic add_associations operation
  return this->add_associations(local_id,
                                info,
                                priority,
                                receive_listener,
                                0);
}

ACE_INLINE void
OpenDDS::DCPS::TransportInterface::send(const DataSampleList& samples)
{
  DBG_ENTRY_LVL("TransportInterface","send",6);

  DataSampleListElement* cur = samples.head_;

  while (cur) {
    // VERY IMPORTANT NOTE:
    //
    // We have to be very careful in how we deal with the current
    // DataSampleListElement.  The issue is that once we have invoked
    // data_delivered() on the send_listener_ object, or we have invoked
    // send() on the pub_links, we can no longer access the current
    // DataSampleListElement!Thus, we need to get the next
    // DataSampleListElement (pointer) from the current element now,
    // while it is safe.
    DataSampleListElement* next_elem = cur->next_send_sample_;
    DataLinkSet_rch pub_links;

    if (cur->num_subs_ > 0) {
      pub_links = this->local_map_.find_set(cur->publication_id_,
                                            cur->subscription_ids_,
                                            cur->num_subs_);

    } else {
      pub_links = this->local_map_.find_set(cur->publication_id_);
    }

    if (pub_links.is_nil()) {
      // NOTE: This is the "local publisher id is not currently
      //       associated with any remote subscriber ids" case.

      if (DCPS_debug_level > 4) {
        OpenDDS::DCPS::RepoIdConverter converter(cur->publication_id_);
        ACE_DEBUG((LM_DEBUG,
                   ACE_TEXT("(%P|%t) TransportInterface::send: ")
                   ACE_TEXT("no links for publication %C, ")
                   ACE_TEXT("not sending %d samples.\n"),
                   std::string(converter).c_str(),
                   samples.size_));
      }

      // We tell the send_listener_ that all of the remote subscriber ids
      // that wanted the data (all zero of them) have indeed received
      // the data.
      cur->send_listener_->data_delivered(cur);

    } else {
      VDBG_LVL((LM_DEBUG,"(%P|%t) DBG: Found DataLinkSet. Sending element %@.\n"
                , cur), 5);

      // This will do several things, including adding to the membership
      // of the send_links_ set.  Any DataLinks added to the send_links_
      // set will be also told about the send_start() event.  Those
      // DataLinks (in the pub_links set) that are already in the
      // send_links_ set will not be told about the send_start() event
      // since they heard about it when they were inserted into the
      // send_links_ set.
      this->send_links_->send_start(pub_links.in());
      pub_links->send(cur);
    }

    // Move on to the next DataSampleListElement to send.
    cur = next_elem;
  }

  // This will inform each DataLink in the set about the stop_send() event.
  // It will then clear the send_links_ set.
  //
  // The reason that the send_links_ set is cleared is because we continually
  // reuse the same send_links_ object over and over for each call to this
  // send method.

  this->send_links_->send_stop();
}

ACE_INLINE OpenDDS::DCPS::TransportImpl_rch
OpenDDS::DCPS::TransportInterface::get_transport_impl()
{
  return this->impl_;
}
