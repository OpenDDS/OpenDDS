/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "DataLink.h"
#include "RepoIdSet.h"

#include "DataLinkSetMap.h"
#include "ReceivedDataSample.h"

#include "TransportImpl.h"
#include "TransportInst.h"
#include "SendResponseListener.h"

#include "dds/DCPS/DataWriterImpl.h"
#include "dds/DCPS/DataReaderImpl.h"
#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/GuidConverter.h"
#include "dds/DdsDcpsGuidTypeSupportImpl.h"
#include "dds/DCPS/Util.h"

#include "EntryExit.h"
#include "tao/ORB_Core.h"
#include "tao/debug.h"
#include "ace/Reactor.h"
#include "ace/SOCK.h"

#include <iostream>
#include <sstream>

#if !defined (__ACE_INLINE__)
#include "DataLink.inl"
#endif /* __ACE_INLINE__ */

namespace OpenDDS {
namespace DCPS {

/// Only called by our TransportImpl object.
DataLink::DataLink(TransportImpl* impl, CORBA::Long priority, bool is_loopback,
                   bool is_active)
  : stopped_(false),
    thr_per_con_send_task_(0),
    transport_priority_(priority),
    strategy_condition_(strategy_lock_),
    send_control_allocator_(0),
    mb_allocator_(0),
    db_allocator_(0),
    is_loopback_(is_loopback),
    is_active_(is_active),
    start_failed_(false)
{
  DBG_ENTRY_LVL("DataLink", "DataLink", 6);

  impl->_add_ref();
  this->impl_ = impl;

  datalink_release_delay_.sec(this->impl_->config_->datalink_release_delay_ / 1000);
  datalink_release_delay_.usec(this->impl_->config_->datalink_release_delay_ % 1000 * 1000);

  id_ = DataLink::get_next_datalink_id();

  if (this->impl_->config_->thread_per_connection_) {
    this->thr_per_con_send_task_ = new ThreadPerConnectionSendTask(this);

    if (this->thr_per_con_send_task_->open() == -1) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) DataLink::DataLink: ")
                 ACE_TEXT("failed to open ThreadPerConnectionSendTask\n")));

    } else if (DCPS_debug_level > 4) {
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) DataLink::DataLink - ")
                 ACE_TEXT("started new thread to send data with.\n")));
    }
  }

  // Initialize transport control sample allocators:
  size_t control_chunks = this->impl_->config_->datalink_control_chunks_;

  this->send_control_allocator_ =
    new TransportSendControlElementAllocator(control_chunks);

  this->mb_allocator_ = new MessageBlockAllocator(control_chunks);
  this->db_allocator_ = new DataBlockAllocator(control_chunks);
}

DataLink::~DataLink()
{
  DBG_ENTRY_LVL("DataLink", "~DataLink", 6);

  if ((this->pub_map_.size() > 0) || (this->sub_map_.size() > 0)) {
    ACE_DEBUG((LM_WARNING,
               ACE_TEXT("(%P|%t) WARNING: DataLink::~DataLink() - ")
               ACE_TEXT("link still in use by %d publications ")
               ACE_TEXT("and %d subscriptions when deleted!\n"),
               this->pub_map_.size(),
               this->sub_map_.size()));
  }

  delete this->db_allocator_;
  delete this->mb_allocator_;

  delete this->send_control_allocator_;

  if (this->thr_per_con_send_task_ != 0) {
    this->thr_per_con_send_task_->close(1);
    delete this->thr_per_con_send_task_;
  }
}

TransportImpl_rch
DataLink::impl() const
{
  return impl_;
}

void
DataLink::wait_for_start()
{
  GuardType guard(this->strategy_lock_);
  while ((this->send_strategy_.is_nil() || this->receive_strategy_.is_nil())
         && !this->start_failed_) {
    this->strategy_condition_.wait();
  }
}

void
DataLink::stop()
{
  GuardType guard(this->strategy_lock_);

  if (this->stopped_) return; // already stopped

  // Stop the TransportSendStrategy and the TransportReceiveStrategy.
  if (!this->send_strategy_.is_nil()) {
    this->send_strategy_->stop();
    this->send_strategy_ = 0;
  }

  if (!this->receive_strategy_.is_nil()) {
    this->receive_strategy_->stop();
    this->receive_strategy_ = 0;
  }

  // Tell our subclass about the "stop" event.
  this->stop_i();

  this->stopped_ = true;
}

void
DataLink::resume_send()
{
  if (!this->send_strategy_->isDirectMode())
    this->send_strategy_->resume_send();
}

int
DataLink::make_reservation(const RepoId& remote_subscription_id,
                           const RepoId& local_publication_id,
                           TransportSendListener* send_listener)
{
  DBG_ENTRY_LVL("DataLink", "make_reservation", 6);
  int pub_result      = 0;
  int sub_result      = 0;
  int pub_undo_result = 0;
  int sub_undo_result = 0;

  bool first_pub = false;

  if (DCPS_debug_level > 9) {
    GuidConverter local(local_publication_id), remote(remote_subscription_id);
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) DataLink::make_reservation() - ")
               ACE_TEXT("creating association local publication  %C ")
               ACE_TEXT("<--> with remote subscription %C.\n"),
               std::string(local).c_str(),
               std::string(remote).c_str()));
  }

  {
    GuardType guard(this->strategy_lock_);

    if (!this->send_strategy_.is_nil()) {
      this->send_strategy_->link_released(false);
    }
  }

  {
    GuardType guard(this->pub_map_lock_);

    first_pub = this->pub_map_.size() == 0; // empty

    // Update our pub_map_.  The last argument is a 0 because remote
    // subscribers don't have a TransportReceiveListener object.
    pub_result = this->pub_map_.insert(local_publication_id,
                                       remote_subscription_id, 0);

    // Take advantage of the lock and store the send listener as well.
    this->send_listeners_[local_publication_id] = send_listener;
  }

  if (pub_result == 0) {
    {
      GuardType guard(this->sub_map_lock_);
      sub_result = this->sub_map_.insert(remote_subscription_id,
                                         local_publication_id);
    }

    if (sub_result == 0) {
      // If this is our first reservation, we should notify the
      // subclass that it should start:
      if (!first_pub || start_i() == 0) {
        // Success!
        return 0;

      } else {
        GuardType guard(this->sub_map_lock_);
        sub_undo_result = this->sub_map_.remove(remote_subscription_id,
                                                local_publication_id);
      }
    }

    GuardType guard(this->pub_map_lock_);
    pub_undo_result = this->pub_map_.remove(local_publication_id,
                                            remote_subscription_id);
  }

  // We only get to here when an error occurred somewhere along the way.
  // None of this needs the lock_ to be acquired.

  if (pub_result == 0) {
    if (sub_result != 0) {
      GuidConverter local(local_publication_id), remote(remote_subscription_id);
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: DataLink::make_reservation: ")
                 ACE_TEXT("failed to insert remote subscription %C ")
                 ACE_TEXT("to local publication %C reservation into sub_map_.\n"),
                 std::string(remote).c_str(), std::string(local).c_str()));
    }

    if (pub_undo_result != 0) {
      GuidConverter local(local_publication_id), remote(remote_subscription_id);
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: DataLink::make_reservation: ")
                 ACE_TEXT("failed to remove (undo) local publication %C ")
                 ACE_TEXT("to remote subscription %C reservation from pub_map_.\n"),
                 std::string(local).c_str(), std::string(remote).c_str()));
    }

    if (sub_undo_result != 0) {
      GuidConverter local(local_publication_id), remote(remote_subscription_id);
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: DataLink::make_reservation: ")
                 ACE_TEXT("failed to remove (undo) remote subscription %C ")
                 ACE_TEXT("to local publication %C reservation from sub_map_.\n"),
                 std::string(remote).c_str(), std::string(local).c_str()));
    }

  } else {
    GuidConverter local(local_publication_id), remote(remote_subscription_id);
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: DataLink::make_reservation: ")
               ACE_TEXT("failed to insert local publication %C to remote ")
               ACE_TEXT("subscription %C reservation into pub_map_.\n"),
               std::string(local).c_str(), std::string(remote).c_str()));
  }

  return -1;
}

int
DataLink::make_reservation(const RepoId& remote_publication_id,
                           const RepoId& local_subcription_id,
                           TransportReceiveListener* receive_listener)
{
  DBG_ENTRY_LVL("DataLink", "make_reservation", 6);
  int sub_result      = 0;
  int pub_result      = 0;
  int sub_undo_result = 0;
  int pub_undo_result = 0;

  bool first_sub = false;

  if (DCPS_debug_level > 9) {
    GuidConverter local(local_subcription_id), remote(remote_publication_id);
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) DataLink::make_reservation() - ")
               ACE_TEXT("creating association local subscription %C ")
               ACE_TEXT("<--> with remote publication  %C.\n"),
               std::string(local).c_str(), std::string(remote).c_str()));
  }

  {
    GuardType guard(this->strategy_lock_);

    if (!this->send_strategy_.is_nil()) {
      this->send_strategy_->link_released(false);
    }
  }

  {
    GuardType guard(this->sub_map_lock_);

    first_sub = this->sub_map_.size() == 0; // empty

    // Update our sub_map_.
    sub_result = this->sub_map_.insert(local_subcription_id, remote_publication_id);

    this->recv_listeners_[local_subcription_id] = receive_listener;
  }

  if (sub_result == 0) {
    {
      GuardType guard(this->pub_map_lock_);
      pub_result = this->pub_map_.insert(remote_publication_id,
                                         local_subcription_id,
                                         receive_listener);
    }

    if (pub_result == 0) {
      // If this is our first reservation, we should notify the
      // subclass that it should start:
      if (!first_sub || start_i() == 0) {
        // Success!
        return 0;

      } else {
        GuardType guard(this->pub_map_lock_);
        pub_undo_result = this->pub_map_.remove(remote_publication_id,
                                                local_subcription_id);
      }
    }

    GuardType guard(this->sub_map_lock_);
    // Since we failed to insert into into the pub_map_, and have
    // already inserted it in the sub_map_, we better attempt to
    // undo the insert that we did to the sub_map_.  Otherwise,
    // the sub_map_ and pub_map_ will become inconsistent.
    sub_undo_result = this->sub_map_.remove(local_subcription_id,
                                            remote_publication_id);
  }

  //this->send_strategy_->link_released (false);

  // We only get to here when an error occurred somewhere along the way.
  // None of this needs the lock_ to be acquired.

  if (sub_result == 0) {
    if (pub_result != 0) {
      GuidConverter local(local_subcription_id), remote(remote_publication_id);
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: DataLink::make_reservation: ")
                 ACE_TEXT("Failed to insert remote publication %C to local ")
                 ACE_TEXT("subcription %C reservation into pub_map_.\n"),
                 std::string(remote).c_str(), std::string(local).c_str()));
    }

    if (sub_undo_result != 0) {
      GuidConverter local(local_subcription_id), remote(remote_publication_id);
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: DataLink::make_reservations: ")
                 ACE_TEXT("failed to remove (undo) local subcription %C to ")
                 ACE_TEXT("remote publication %C reservation from sub_map_.\n"),
                 std::string(local).c_str(), std::string(remote).c_str()));
    }

    if (pub_undo_result != 0) {
      GuidConverter local(local_subcription_id), remote(remote_publication_id);
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: DataLink::make_reservations: ")
                 ACE_TEXT("failed to remove (undo) remote publication %C to ")
                 ACE_TEXT("local subcription %C reservation from pub_map_.\n"),
                 std::string(remote).c_str(), std::string(local).c_str()));
    }

  } else {
    GuidConverter local(local_subcription_id), remote(remote_publication_id);
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: DataLink::make_reservations: ")
               ACE_TEXT("failed to insert local subcription %C to remote ")
               ACE_TEXT("publication %C reservation into sub_map_.\n"),
               std::string(local).c_str(), std::string(remote).c_str()));
  }

  return -1;
}

GUIDSeq*
DataLink::peer_ids(const RepoId& local_id) const
{
  // Is 'local_id' a local publication?
  GuardType guard(this->pub_map_lock_);
  if (this->send_listeners_.count(local_id)) {
    ReceiveListenerSet_rch rls = this->pub_map_.find(local_id);
    if (rls.is_nil()) {
      return 0;
    }
    GUIDSeq_var result = new GUIDSeq;
    result->length(static_cast<CORBA::ULong>(rls->size()));
    CORBA::ULong i = 0;
    for (ReceiveListenerSet::MapType::iterator iter = rls->map().begin();
         iter != rls->map().end(); ++iter) {
      result[i++] = iter->first;
    }
    return result._retn();
  }
  guard.release();

  // Is 'local_id' a local subscription?
  GuardType sub_guard(this->sub_map_lock_);
  if (this->recv_listeners_.count(local_id)) {
    RepoIdSet_rch ris = this->sub_map_.find(local_id);
    if (ris.is_nil()) {
      return 0;
    }
    GUIDSeq_var result = new GUIDSeq;
    result->length(static_cast<CORBA::ULong>(ris->size()));
    CORBA::ULong i = 0;
    for (RepoIdSet::MapType::iterator iter = ris->map().begin();
         iter != ris->map().end(); ++iter) {
      result[i++] = iter->first;
    }
    return result._retn();
  }
  return 0;
}

/// This gets invoked when a TransportClient::remove_associations()
/// call has been made.  Because this DataLink can be shared amongst
/// different TransportClient objects, and different threads could
/// be "managing" the different TransportClient objects, we need
/// to make sure that this release_reservations() works in conjunction
/// with a simultaneous call (in another thread) to one of this
/// DataLink's make_reservation() methods.
void
DataLink::release_reservations(RepoId remote_id, RepoId local_id,
                               DataLinkSetMap& released_locals)
{
  DBG_ENTRY_LVL("DataLink", "release_reservations", 6);

  if (DCPS_debug_level > 9) {
    GuidConverter local(local_id);
    GuidConverter remote(remote_id);
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) DataLink::release_reservations() - ")
               ACE_TEXT("releasing association local: %C ")
               ACE_TEXT("<--> with remote %C.\n"),
               std::string(local).c_str(),
               std::string(remote).c_str()));
  }

  // See if the remote_id is a publisher_id.
  ReceiveListenerSet_rch listener_set;

  {
    GuardType guard(this->pub_map_lock_);
    listener_set = this->pub_map_.find(remote_id);
  }

  if (listener_set.is_nil()) {
    // The remote_id is not a publisher_id.
    // See if it is a subscriber_id by looking in our sub_map_.
    RepoIdSet_rch id_set;

    {
      GuardType guard(this->sub_map_lock_);
      id_set = this->sub_map_.find(remote_id);
    }

    if (id_set.is_nil()) {
      // We don't know about the remote_id.
      GuidConverter converter(remote_id);
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: DataLink::release_reservations: ")
                 ACE_TEXT("unable to locate remote %C in pub_map_ or sub_map_.\n"),
                 std::string(converter).c_str()));

    } else {
      VDBG_LVL((LM_DEBUG, "(%P|%t) DataLink::release_reservations: the remote_id is a sub id.\n"), 5);
      //guard.release ();
      // The remote_id is a subscriber_id.
      this->release_reservations_i(remote_id, local_id);
      this->release_remote_subscriber(remote_id,
                                      local_id,
                                      id_set,
                                      released_locals);

      if (id_set->size() == 0) {
        // Remove the remote_id(sub) after the remote/local ids is released
        // and there are no local pubs associated with this sub.
        GuardType guard(this->sub_map_lock_);
        id_set = this->sub_map_.remove_set(remote_id);
        this->release_remote_i(remote_id);
      }

      //guard.acquire ();
    }

  } else {
    VDBG_LVL((LM_DEBUG,
              ACE_TEXT("(%P|%t) DataLink::release_reservations: ")
              ACE_TEXT("the remote_id is a pub id.\n")), 5);
    //guard.release ();
    // The remote_id is a publisher_id.
    this->release_reservations_i(remote_id, local_id);
    this->release_remote_publisher(remote_id,
                                   local_id,
                                   listener_set,
                                   released_locals);

    if (listener_set->size() == 0) {
      GuardType guard(this->pub_map_lock_);
      // Remove the remote_id(pub) after the remote/local ids is released
      // and there are no local subs associated with this pub.
      listener_set = this->pub_map_.remove_set(remote_id);
      this->release_remote_i(remote_id);
    }

    //guard.acquire ();
  }

  VDBG_LVL((LM_DEBUG,
            ACE_TEXT("(%P|%t) DataLink::release_reservations: ")
            ACE_TEXT("maps tot size: %d.\n"),
            this->pub_map_.size() + this->sub_map_.size()),
           5);

  if ((this->pub_map_.size() + this->sub_map_.size()) == 0) {
    // Add reference before schedule timer with reactor and remove reference after
    // handle_timeout is called. This would avoid DataLink deletion while handling
    // timeout.
    this->_add_ref();
    VDBG((LM_DEBUG, "(%P|%t) DataLink[%@]::release_reservations last "
                    "reservation released\n", this));

    if (this->datalink_release_delay_ > ACE_Time_Value::zero) {
      // The samples has to be removed at this point, otherwise the sample
      // can not be delivered when new association is added and still use
      // this connection/datalink.
      if (!this->send_strategy_.is_nil()) {
        this->send_strategy_->clear();
      }

      CORBA::ORB_var orb = TheServiceParticipant->get_ORB();
      ACE_Reactor* reactor = orb->orb_core()->reactor();

      this->impl_->release_datalink(this, true);
      reactor->schedule_timer(this, 0, this->datalink_release_delay_);

    } else {
      this->impl_->release_datalink(this, false);
      this->handle_timeout(ACE_OS::gettimeofday(), (const void *)0);
    }
  }
}

bool
DataLink::cancel_release()
{
  CORBA::ORB_var orb = TheServiceParticipant->get_ORB();
  ACE_Reactor* reactor = orb->orb_core()->reactor();
  return reactor->cancel_timer(this) > 0;
}

int
DataLink::start_i()
{
  DBG_ENTRY_LVL("DataLink", "start_i", 6);

  return 0;
}

void
DataLink::stop_i()
{
  DBG_ENTRY_LVL("DataLink", "stop_i", 6);
}

ACE_Message_Block*
DataLink::create_control(char submessage_id,
                         DataSampleHeader& header,
                         ACE_Message_Block* data)
{
  DBG_ENTRY_LVL("DataLink", "create_control", 6);

  header.byte_order_ = ACE_CDR_BYTE_ORDER;
  header.message_id_ = TRANSPORT_CONTROL;
  header.submessage_id_ = submessage_id;
  header.message_length_ = static_cast<ACE_UINT32>(data->total_length());

  ACE_Message_Block* message;
  ACE_NEW_MALLOC_RETURN(message,
                        static_cast<ACE_Message_Block*>(
                            this->mb_allocator_->malloc(sizeof (ACE_Message_Block))),
                        ACE_Message_Block(header.max_marshaled_size(),
                                          ACE_Message_Block::MB_DATA,
                                          data,
                                          0,  // data
                                          0,  // allocator_strategy
                                          0,  // locking_strategy
                                          ACE_DEFAULT_MESSAGE_BLOCK_PRIORITY,
                                          ACE_Time_Value::zero,
                                          ACE_Time_Value::max_time,
                                          this->db_allocator_,
                                          this->mb_allocator_),
                        0);

  *message << header;

  return message;
}

SendControlStatus
DataLink::send_control(const DataSampleHeader& header, ACE_Message_Block* message)
{
  DBG_ENTRY_LVL("DataLink", "send_control", 6);
  SendResponseListener listener;

  TransportSendControlElement* elem;

  ACE_NEW_MALLOC_RETURN(elem,
                        static_cast<TransportSendControlElement*>(
                          this->send_control_allocator_->malloc()),
                        TransportSendControlElement(1,  // initial_count
                                                    GUID_UNKNOWN,
                                                    &listener,
                                                    header,
                                                    message,
                                                    this->send_control_allocator_),
                        SEND_CONTROL_ERROR);

  send_start();
  send(elem);
  send_stop();

  return SEND_CONTROL_OK;
}

/// This method will "deliver" the sample to all TransportReceiveListeners
/// within this DataLink that are interested in the (remote) publisher id
/// that sent the sample.
int
DataLink::data_received(ReceivedDataSample& sample)
{
  DBG_ENTRY_LVL("DataLink", "data_received", 6);

  // Which remote publisher sent this message?
  RepoId publisher_id = sample.header_.publication_id_;

  // Locate the set of TransportReceiveListeners associated with this
  // DataLink that are interested in hearing about any samples received
  // from the remote publisher_id.
  ReceiveListenerSet_rch listener_set;

  if (Transport_debug_level > 9) {
    std::stringstream buffer;
    buffer << sample.header_;
    GuidConverter converter(publisher_id);
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) DataLink::data_received: ")
               ACE_TEXT(" publisher %C received sample: %C.\n"),
               std::string(converter).c_str(),
               buffer.str().c_str()));
  }

  {
    GuardType guard(this->pub_map_lock_);
    listener_set = this->pub_map_.find(publisher_id);
  }

  if (listener_set.is_nil()) {
    // Nobody has any interest in this message.  Drop it on the floor.
    if (Transport_debug_level > 4) {
      GuidConverter converter(publisher_id);
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) DataLink::data_received: ")
                 ACE_TEXT(" discarding sample from publisher %C due to no listeners.\n"),
                 std::string(converter).c_str()));
    }

    return 0;
  }

#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE
  if (sample.header_.content_filter_
      && sample.header_.content_filter_entries_.length()) {
    ReceiveListenerSet subset(*listener_set.in());
    subset.remove_all(sample.header_.content_filter_entries_);
    subset.data_received(sample);
  } else {
#endif // OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE

  // Just get the set to do our dirty work by having it iterate over its
  // collection of TransportReceiveListeners, and invoke the data_received()
  // method on each one.
  listener_set->data_received(sample);
#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE
  }
#endif // OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE

  return 0;
}

void
DataLink::ack_received(ReceivedDataSample& sample)
{
  RepoId publication = GUID_UNKNOWN;
  Serializer serializer(
    sample.sample_,
    sample.header_.byte_order_ != ACE_CDR_BYTE_ORDER);
  serializer >> publication;

  TransportSendListener* listener;
  {
    GuardType guard(this->pub_map_lock_);
    IdToSendListenerMap::const_iterator where
    = this->send_listeners_.find(publication);

    if (where == this->send_listeners_.end()) {
      GuidConverter converter(publication);

      // Ack could be for a different publisher.
      if (this->pub_map_.find(publication) == 0) {
        if (DCPS_debug_level > 0) {
          ACE_ERROR((LM_WARNING,
                     ACE_TEXT("(%P|%t) DataLink::ack_received: ")
                     ACE_TEXT("publication %C not found.\n"),
                     std::string(converter).c_str()));
        }
      } else {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("(%P|%t) DataLink::ack_received: ")
                   ACE_TEXT("listener for publication %C not found.\n"),
                   std::string(converter).c_str()));
      }
      return;
    }

    listener = where->second;
  }

  listener->deliver_ack(sample.header_, sample.sample_);
}

/// No locking needed because the caller (release_reservations()) should
/// have already acquired our lock_.
// Ciju: Don't believe a guard is necessary here
void
DataLink::release_remote_subscriber(RepoId subscriber_id, RepoId publisher_id,
                                    RepoIdSet_rch& pubid_set,
                                    DataLinkSetMap& released_publishers)
{
  DBG_ENTRY_LVL("DataLink", "release_remote_subscriber", 6);

  RepoIdSet::MapType& pubid_map = pubid_set->map();

  for (RepoIdSet::MapType::iterator itr = pubid_map.begin();
       itr != pubid_map.end();
       ++itr) {
    if (publisher_id == itr->first) {
      // Remove the publisher_id => subscriber_id association.
      int ret = 0;
      {
        GuardType guard(this->pub_map_lock_);

        ret = this->pub_map_.release_subscriber(publisher_id,
                                                subscriber_id);
      }

      if ( ret == 1) {
        // This means that this release() operation has caused the
        // publisher_id to no longer be associated with *any* subscribers.
        released_publishers.insert_link(publisher_id,this);
        {
          GuardType guard(this->released_local_lock_);
          released_local_pubs_.insert_id(publisher_id, subscriber_id);
        }
      }
    }
  }

  // remove the publisher_id from the pubset that associate with the remote sub.
  if (pubid_set->remove_id(publisher_id) == -1) {
    GuidConverter converter(publisher_id);
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: DataLink::release_remote_subscriber: ")
               ACE_TEXT(" failed to remove pub %C from PubSet.\n"),
               std::string(converter).c_str()));
  }
}

/// No locking needed because the caller (release_reservations()) should
/// have already acquired our lock_.
// Ciju: Don't believe a guard is necessary here
void
DataLink::release_remote_publisher(RepoId publisher_id, RepoId subscriber_id,
                                   ReceiveListenerSet_rch& listener_set,
                                   DataLinkSetMap& released_subscribers)
{
  DBG_ENTRY_LVL("DataLink", "release_remote_publisher", 6);

  if (listener_set->exist(subscriber_id)) {
    // Remove the publisher_id => subscriber_id association.

    int result = 0;
    {
      GuardType guard(this->sub_map_lock_);
      result = this->sub_map_.release_publisher(subscriber_id,publisher_id);
    }

    if (result == 1) {
      // This means that this release() operation has caused the
      // subscriber_id to no longer be associated with *any* publishers.
      released_subscribers.insert_link(subscriber_id,this);
      {
        GuardType guard(this->released_local_lock_);
        released_local_subs_.insert_id(subscriber_id, publisher_id);
      }
    }
  }

  if (listener_set->remove(subscriber_id) == -1) {
    GuidConverter converter(subscriber_id);
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: DataLink::release_remote_publisher: ")
               ACE_TEXT(" failed to remove sub %C from ListenerSet.\n"),
               std::string(converter).c_str()));
  }
}

// static
ACE_UINT64
DataLink::get_next_datalink_id()
{
  static ACE_UINT64 next_id = 0;
  static LockType lock;

  ACE_UINT64 id;
  {
    GuardType guard(lock);
    id = next_id++;

    if (0 == next_id) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("ERROR: DataLink::get_next_datalink_id: ")
                 ACE_TEXT("has rolled over and is reusing ids!\n")));
    }
  }

  return id;
}

void
DataLink::transport_shutdown()
{
  DBG_ENTRY_LVL("DataLink", "transport_shutdown", 6);

  if (! this->send_strategy_.is_nil ()) {
    this->send_strategy_->transport_shutdown ();
  }

  if (this->cancel_release()) {
    this->handle_timeout(ACE_OS::gettimeofday(), (const void *)0);
  }

  stop();

  // Drop our reference to the TransportImpl object
  this->impl_ = 0;
}

void
DataLink::notify(ConnectionNotice notice)
{
  DBG_ENTRY_LVL("DataLink", "notify", 6);

  VDBG((LM_DEBUG,
        ACE_TEXT("(%P|%t) DataLink::notify: this(%X) notify %C\n"),
        this,
        connection_notice_as_str(notice)));

  {
    GuardType guard(this->pub_map_lock_);

    ReceiveListenerSetMap::MapType & map = this->pub_map_.map();

    // Notify the datawriters
    // the lost publications due to a connection problem.
    for (ReceiveListenerSetMap::MapType::iterator itr = map.begin();
         itr != map.end();
         ++itr) {

      TransportSendListener* tsl = send_listener_for(itr->first);
      if (tsl != 0) {
        if (Transport_debug_level > 0) {
          GuidConverter converter(itr->first);
          ACE_DEBUG((LM_DEBUG,
                     ACE_TEXT("(%P|%t) DataLink::notify: ")
                     ACE_TEXT("notify pub %C %C.\n"),
                     std::string(converter).c_str(),
                     connection_notice_as_str(notice)));
        }

        ReceiveListenerSet_rch subset = itr->second;

        ReaderIdSeq subids;
        subset->get_keys(subids);

        switch (notice) {
        case DISCONNECTED:
          tsl->notify_publication_disconnected(subids);
          break;
        case RECONNECTED:
          tsl->notify_publication_reconnected(subids);
          break;
        case LOST:
          tsl->notify_publication_lost(subids);
          break;
        default:
          ACE_ERROR((LM_ERROR,
                     ACE_TEXT("(%P|%t) ERROR: DataLink::notify: ")
                     ACE_TEXT("unknown notice to TransportSendListener\n")));
          break;
        }

      } else {
        if (Transport_debug_level > 0) {
          GuidConverter converter(itr->first);
          ACE_DEBUG((LM_DEBUG,
                     ACE_TEXT("(%P|%t) DataLink::notify: ")
                     ACE_TEXT("not notify pub %C %C \n"),
                     std::string(converter).c_str(),
                     connection_notice_as_str(notice)));
        }
      }

    }
  }

  {
    GuardType guard(this->sub_map_lock_);

    // Notify the datareaders registered with TransportImpl
    // the lost subscriptions due to a connection problem.
    RepoIdSetMap::MapType & map = this->sub_map_.map();

    for (RepoIdSetMap::MapType::iterator itr = map.begin();
         itr != map.end();
         ++itr) {

      TransportReceiveListener* trl = recv_listener_for(itr->first);
      if (trl != 0) {
        if (Transport_debug_level > 0) {
          GuidConverter converter(itr->first);
          ACE_DEBUG((LM_DEBUG,
                     ACE_TEXT("(%P|%t) DataLink::notify: ")
                     ACE_TEXT("notify sub %C %C.\n"),
                     std::string(converter).c_str(),
                     connection_notice_as_str(notice)));
        }

        RepoIdSet_rch pubset = itr->second;
        RepoIdSet::MapType & map = pubset->map();

        WriterIdSeq pubids;
        pubids.length(static_cast<CORBA::ULong>(pubset->size()));
        CORBA::ULong i = 0;

        for (RepoIdSet::MapType::iterator iitr = map.begin();
             iitr != map.end();
             ++iitr) {
          pubids[i++] = iitr->first;
        }

        switch (notice) {
        case DISCONNECTED:
          trl->notify_subscription_disconnected(pubids);
          break;
        case RECONNECTED:
          trl->notify_subscription_reconnected(pubids);
          break;
        case LOST:
          trl->notify_subscription_lost(pubids);
          break;
        default:
          ACE_ERROR((LM_ERROR,
                     ACE_TEXT("(%P|%t) ERROR: DataLink::notify: ")
                     ACE_TEXT("unknown notice to datareader.\n")));
          break;
        }

      } else {
        if (Transport_debug_level > 0) {
          GuidConverter converter(itr->first);
          ACE_DEBUG((LM_DEBUG,
                     ACE_TEXT("(%P|%t) DataLink::notify: ")
                     ACE_TEXT("not notify sub %C subscription lost.\n"),
                     std::string(converter).c_str()));
        }

      }
    }
  }
}

void
DataLink::notify_connection_deleted()
{
  GuardType guard(this->released_local_lock_);

  if (Transport_debug_level > 5) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) DataLink::notify_connection_deleted: ")
               ACE_TEXT("pmap %d smap %d\n"),
               released_local_pubs_.map().size(),
               released_local_subs_.map().size()));
  }

  RepoIdSet::MapType& pmap = released_local_pubs_.map();

  for (RepoIdSet::MapType::iterator itr = pmap.begin();
       itr != pmap.end();
       ++itr) {

    TransportSendListener* tsl = send_listener_for(itr->first);
    if (tsl != 0) {
      if (Transport_debug_level > 0) {
        GuidConverter converter(itr->first);
        ACE_DEBUG((LM_DEBUG,
                   ACE_TEXT("(%P|%t) DataLink:: notify_connection_deleted: ")
                   ACE_TEXT("notify pub %C connection deleted.\n"),
                   std::string(converter).c_str()));
      }

      tsl->notify_connection_deleted();
    }
  }

  RepoIdSet::MapType& smap = released_local_subs_.map();

  for (RepoIdSet::MapType::iterator itr2 = smap.begin();
       itr2 != smap.end();
       ++itr2) {

    TransportReceiveListener* trl = recv_listener_for(itr2->first);

    if (trl != 0) {
      if (Transport_debug_level > 0) {
        GuidConverter converter(itr2->first);
        ACE_DEBUG((LM_DEBUG,
                   ACE_TEXT("(%P|%t) DataLink::notify_connection_deleted: ")
                   ACE_TEXT("notify sub %C connection deleted.\n"),
                   std::string(converter).c_str()));
      }

      trl->notify_connection_deleted();
    }
  }
}

void
DataLink::pre_stop_i()
{
  if (this->thr_per_con_send_task_ != 0) {
    this->thr_per_con_send_task_->close(1);
  }
}

bool
DataLink::release_resources()
{
  DBG_ENTRY_LVL("DataLink", "release_resources", 6);

  this->prepare_release();

  return impl_->release_link_resources(this);
}

bool
DataLink::is_target(const RepoId& sub_id)
{
  GuardType guard(this->sub_map_lock_);
  RepoIdSet_rch pubs = this->sub_map_.find(sub_id);

  return !pubs.is_nil();
}

GUIDSeq*
DataLink::target_intersection(const GUIDSeq& in)
{
  GUIDSeq_var res;
  GuardType guard(this->sub_map_lock_);
  const CORBA::ULong len = in.length();
  for (CORBA::ULong i(0); i < len; ++i) {
    RepoIdSet_rch target = this->sub_map_.find(in[i]);
    if (!target.is_nil()) {
      if (res.ptr() == 0) {
        res = new GUIDSeq;
      }
      push_back(res.inout(), in[i]);
    }
  }
  return res._retn();
}

CORBA::ULong
DataLink::num_targets() const
{
  GuardType guard(this->sub_map_lock_);
  return static_cast<CORBA::ULong>(this->sub_map_.size());
}

RepoIdSet_rch
DataLink::get_targets() const
{
  GuardType guard(this->sub_map_lock_);
  RepoIdSet_rch ret(new RepoIdSet);
  this->sub_map_.get_keys(*ret.in());
  return ret;
}

bool
DataLink::exist(const RepoId& remote_id, const RepoId& local_id,
                const bool& pub_side, bool& last)
{
  if (pub_side) {
    RepoIdSet_rch pubs;
    {
      GuardType guard(this->sub_map_lock_);
      pubs = this->sub_map_.find(remote_id);
    }

    GuardType guard(this->pub_map_lock_);

    if (!pubs.is_nil())
      return pubs->exist(local_id, last);

  } else {
    ReceiveListenerSet_rch subs;
    {
      GuardType guard(this->pub_map_lock_);
      subs = this->pub_map_.find(remote_id);
    }

    GuardType guard(this->sub_map_lock_);

    if (!subs.is_nil())
      return subs->exist(local_id, last);
  }

  return false;
}

void DataLink::prepare_release()
{
  {
    GuardType guard(this->sub_map_lock_);

    if (this->sub_map_releasing_.size() > 0) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) DataLink::prepare_release: ")
                 ACE_TEXT("sub_map is already released.\n")));
      return;
    }

    this->sub_map_releasing_ = this->sub_map_;
  }
  {
    GuardType guard(this->pub_map_lock_);

    if (this->pub_map_releasing_.size() > 0) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) DataLink::prepare_release: ")
                 ACE_TEXT("pub_map is already released.\n")));
      return;
    }

    this->pub_map_releasing_ = this->pub_map_;
  }
}

void DataLink::clear_associations()
{
  // The pub_map_ has an entry for each pub_id
  // Create iterator to traverse Publisher map.
  ReceiveListenerSetMap::MapType& pub_map = pub_map_releasing_.map();

  for (ReceiveListenerSetMap::MapType::iterator pub_map_iter = pub_map.begin();
       pub_map_iter != pub_map.end();) {
    // Extract the pub id
    RepoId pub_id = pub_map_iter->first;

    // Each pub_id (may)has an associated DataWriter
    // Dependends upon whether we are an actual pub or sub.
    TransportSendListener* tsl = send_listener_for(pub_id);

    ReceiveListenerSet_rch sub_id_set = pub_map_iter->second;
    // The iterator seems to get corrupted if the element currently
    // being pointed at gets unbound. Hence advance it.
    ++pub_map_iter;

    // Check is DataWriter exists (could have been deleted before we got here.
    if (tsl != NULL) {
      // Each pub-id is mapped to a bunch of sub-id's
      //ReceiveListenerSet_rch sub_id_set = pub_entry->int_id_;
      ReaderIdSeq sub_ids;
      sub_id_set->get_keys(sub_ids);

      // after creating remote id sequence, remove from DataWriter
      // I believe the 'notify_lost' should be set to false, since
      // it doesn't look like we meet any of the conditions for setting
      // it true. Check interface documentations.
      tsl->remove_associations(sub_ids, false);
    }
  }

  // sub -> pub
  // Create iterator to traverse Subscriber map.
  RepoIdSetMap::MapType& sub_map = sub_map_releasing_.map();

  for (RepoIdSetMap::MapType::iterator sub_map_iter = sub_map.begin();
       sub_map_iter != sub_map.end();) {
    // Extract the sub id
    RepoId sub_id = sub_map_iter->first;
    // Each sub_id (may)has an associated DataReader
    // Dependends upon whether we are an actual pub or sub.
    TransportReceiveListener* trl = recv_listener_for(sub_id);

    RepoIdSet_rch pub_id_set = sub_map_iter->second;
    // The iterator seems to get corrupted if the element currently
    // being pointed at gets unbound. Hence advance it.
    ++sub_map_iter;

    // Check id DataReader exists (could have been deleted before we got here.)
    if (trl != NULL) {
      // Each sub-id is mapped to a bunch of pub-id's
      CORBA::ULong pub_ids_count =
        static_cast<CORBA::ULong>(pub_id_set->size());
      WriterIdSeq pub_ids(pub_ids_count);
      pub_ids.length(pub_ids_count);

      int count = 0;

      // create a sequence of associated pub-id's
      for (RepoIdSet::MapType::iterator pub_ids_iter = pub_id_set->map().begin();
           pub_ids_iter != pub_id_set->map().end(); ++pub_ids_iter) {
        pub_ids[count++] = pub_ids_iter->first;
      }

      // after creating remote id sequence, remove from DataReader
      // I believe the 'notify_lost' should be set to false, since
      // it doesn't look like we meet any of the conditions for setting
      // it true. Check interface documentations.
      trl->remove_associations(pub_ids, false);
    }
  }

  sub_map_releasing_.clear();
  pub_map_releasing_.clear();
}

int
OpenDDS::DCPS::DataLink::handle_timeout(const ACE_Time_Value& /*tv*/,
                                        const void* /*arg*/)
{
  if ((this->pub_map_.size() + this->sub_map_.size()) == 0) {
    this->pre_stop_i();

    // The TransportImpl ptr should be cleaned in the dstr.
    // This link will be used as a callback after the actual
    // connection is closed.
    //this->impl_ = 0;

    TransportSendStrategy_rch send_strategy = 0;
    TransportStrategy_rch recv_strategy = 0;
    {
      GuardType guard2(this->strategy_lock_);

      if (!this->send_strategy_.is_nil()) {
        send_strategy =  this->send_strategy_; // save copy
        this->send_strategy_ = 0;
      }

      if (!this->receive_strategy_.is_nil()) {
        recv_strategy = this->receive_strategy_; // save copy
        this->receive_strategy_ = 0;
      }
    }

    if (!send_strategy.is_nil()) {
      send_strategy->stop();
    }

    if (!recv_strategy.is_nil()) {
      recv_strategy->stop();
    }

    // Tell our subclass to handle a "stop" event.
    this->stop_i();
  }

  VDBG((LM_DEBUG, "(%P|%t) DataLink[%@]::handle_timeout\n", this));
  this->_remove_ref();

  return 0;
}

void
DataLink::set_dscp_codepoint(int cp, ACE_SOCK& socket)
{
  /**
   * The following IPV6 code was lifted in spirit from the RTCORBA
   * implementation of setting the DiffServ codepoint.
   */
  int result = 0;
  const char* which = "IPV4 TOS";
#if defined (ACE_HAS_IPV6)
  ACE_INET_Addr local_address;

  if (socket.get_local_addr(local_address) == -1) {
    return ;

  } else if (local_address.get_type() == AF_INET6)
#if !defined (IPV6_TCLASS)
  {
    if (DCPS_debug_level > 0) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: DataLink::set_dscp_codepoint() - ")
                 ACE_TEXT("IPV6 TCLASS not supported yet, not setting codepoint %d.\n"),
                 cp));
    }

    return;
  }

#else /* IPV6_TCLASS */
  {
    which = "IPV6 TCLASS";
    result = socket.set_option(
               IPPROTO_IPV6,
               IPV6_TCLASS,
               &cp,
               sizeof(cp));

  } else // This is a bit tricky and might be hard to follow...

#endif /* IPV6_TCLASS */
#endif /* ACE_HAS_IPV6 */
  result = socket.set_option(
             IPPROTO_IP,
             IP_TOS,
             &cp,
             sizeof(cp));

  if ((result == -1) && (errno != ENOTSUP)
#ifdef WSAEINVAL
      && (errno != WSAEINVAL)
#endif
     ) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) DataLink::set_dscp_codepoint() - ")
               ACE_TEXT("failed to set the %C codepoint to %d: %m, ")
               ACE_TEXT("try running as superuser.\n"),
               which,
               cp));

  } else if (DCPS_debug_level > 4) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) DataLink::set_dscp_codepoint() - ")
               ACE_TEXT("set %C codepoint to %d.\n"),
               which,
               cp));
  }
}

std::ostream&
operator<<(std::ostream& str, const DataLink& value)
{
  str << "   There are " << value.pub_map_.map().size()
  << " publications currently associated with this link:"
  << std::endl;

  for (ReceiveListenerSetMap::MapType::const_iterator
       pubLocation = value.pub_map_.map().begin();
       pubLocation != value.pub_map_.map().end();
       ++pubLocation) {
    for (ReceiveListenerSet::MapType::const_iterator
         subLocation = pubLocation->second->map().begin();
         subLocation != pubLocation->second->map().end();
         ++subLocation) {
      str << GuidConverter(pubLocation->first) << " --> "
      << GuidConverter(subLocation->first) << "   " << std::endl;
    }
  }

  return str;
}

}
}
