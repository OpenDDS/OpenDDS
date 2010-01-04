/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
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
#include "TransportConfiguration.h"
#include "dds/DCPS/DataWriterImpl.h"
#include "dds/DCPS/DataReaderImpl.h"
#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/RepoIdConverter.h"

#include "EntryExit.h"
#include "tao/ORB_Core.h"
#include "tao/debug.h"
#include "ace/Reactor.h"
#include "ace/SOCK.h"

#include <iostream>     // For operator<<() diagnostic formatting.
#include <sstream>      // For Guid value conversion formatting.

#if !defined (__ACE_INLINE__)
#include "DataLink.inl"
#endif /* __ACE_INLINE__ */

/// Only called by our TransportImpl object.
OpenDDS::DCPS::DataLink::DataLink(TransportImpl* impl, CORBA::Long priority)
  : thr_per_con_send_task_(0),
    transport_priority_(priority),
    send_control_allocator_(0),
    mb_allocator_(0),
    db_allocator_(0)
{
  DBG_ENTRY_LVL("DataLink","DataLink",6);

  impl->_add_ref();
  this->impl_ = impl;

  datalink_release_delay_.sec(this->impl_->config_->datalink_release_delay_/1000);
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

OpenDDS::DCPS::DataLink::~DataLink()
{
  DBG_ENTRY_LVL("DataLink","~DataLink",6);

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

void
OpenDDS::DCPS::DataLink::resume_send()
{
  if (!this->send_strategy_->isDirectMode())
    this->send_strategy_->resume_send();
}

/// Only called by our TransportImpl object.
///
/// Return Codes: 0 means successful reservation made.
///              -1 means failure.
int
OpenDDS::DCPS::DataLink::make_reservation(
  RepoId subscriber_id,  /* remote */
  RepoId publisher_id,   /* local */
  TransportSendListener* send_listener)
{
  DBG_ENTRY_LVL("DataLink","make_reservation",6);
  int pub_result      = 0;
  int sub_result      = 0;
  int pub_undo_result = 0;
  int sub_undo_result = 0;

  bool first_pub = false;

  if (DCPS_debug_level > 9) {
    RepoIdConverter local(publisher_id);
    RepoIdConverter remote(subscriber_id);
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) DataLink::make_reservation() - ")
               ACE_TEXT("creating association local: publisher %C ")
               ACE_TEXT("<--> with remote subscriber %C.\n"),
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
    pub_result = this->pub_map_.insert(publisher_id, subscriber_id, 0);

    // Take advantage of the lock and store the send listener as well.
    this->send_listeners_[publisher_id] = send_listener;
  }

  if (pub_result == 0) {
    {
      GuardType guard(this->sub_map_lock_);
      sub_result = this->sub_map_.insert(subscriber_id,publisher_id);
    }

    if (sub_result == 0) {
      // If this is our first reservation, we should notify the
      // subclass that it should start:
      if (!first_pub || start_i() == 0) {
        // Success!
        return 0;

      } else {
        GuardType guard(this->sub_map_lock_);
        sub_undo_result = this->sub_map_.remove(subscriber_id,
                                                publisher_id);
      }
    }

    GuardType guard(this->pub_map_lock_);
    pub_undo_result = this->pub_map_.remove(publisher_id,
                                            subscriber_id);
  }

  // We only get to here when an error occurred somewhere along the way.
  // None of this needs the lock_ to be acquired.

  if (pub_result == 0) {
    if (sub_result != 0) {
      RepoIdConverter sub_converter(subscriber_id);
      RepoIdConverter pub_converter(publisher_id);
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: DataLink::make_reservation: ")
                 ACE_TEXT("failed to insert remote subscriber %C ")
                 ACE_TEXT("to local publisher %C reservation into sub_map_.\n"),
                 std::string(sub_converter).c_str(),
                 std::string(pub_converter).c_str()));
    }

    if (pub_undo_result != 0) {
      RepoIdConverter pub_converter(publisher_id);
      RepoIdConverter sub_converter(subscriber_id);
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: DataLink::make_reservation: ")
                 ACE_TEXT("failed to remove (undo) local publisher %C ")
                 ACE_TEXT("to remote subscriber %C reservation from pub_map_.\n"),
                 std::string(pub_converter).c_str(),
                 std::string(sub_converter).c_str()));
    }

    if (sub_undo_result != 0) {
      RepoIdConverter sub_converter(subscriber_id);
      RepoIdConverter pub_converter(publisher_id);
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: DataLink::make_reservation: ")
                 ACE_TEXT("failed to remove (undo) local subscriber %C ")
                 ACE_TEXT("to remote publisher %C reservation from sub_map_.\n"),
                 std::string(sub_converter).c_str(),
                 std::string(pub_converter).c_str()));
    }

  } else {
    RepoIdConverter pub_converter(publisher_id);
    RepoIdConverter sub_converter(subscriber_id);
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: DataLink::make_reservation: ")
               ACE_TEXT("failed to insert local publisher %C to remote ")
               ACE_TEXT("subscriber %C reservation into pub_map_.\n"),
               std::string(pub_converter).c_str(),
               std::string(sub_converter).c_str()));
  }

  return -1;
}

/// Only called by our TransportImpl object.
int
OpenDDS::DCPS::DataLink::make_reservation
(RepoId                    publisher_id,     /* remote */
 RepoId                    subscriber_id,    /* local */
 TransportReceiveListener* receive_listener)
{
  DBG_ENTRY_LVL("DataLink","make_reservation",6);
  int sub_result      = 0;
  int pub_result      = 0;
  int sub_undo_result = 0;
  int pub_undo_result = 0;

  bool first_sub = false;

  if (DCPS_debug_level > 9) {
    RepoIdConverter local(subscriber_id);
    RepoIdConverter remote(publisher_id);
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) DataLink::make_reservation() - ")
               ACE_TEXT("creating association local subscriber: %C ")
               ACE_TEXT("<--> with remote publisher %C.\n"),
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
    GuardType guard(this->sub_map_lock_);

    first_sub = this->sub_map_.size() == 0; // empty

    // Update our sub_map_.
    sub_result = this->sub_map_.insert(subscriber_id,publisher_id);
  }

  if (sub_result == 0) {
    {
      GuardType guard(this->pub_map_lock_);
      pub_result = this->pub_map_.insert(publisher_id,
                                         subscriber_id,
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
        pub_undo_result = this->pub_map_.remove(publisher_id,
                                                subscriber_id);
      }
    }

    GuardType guard(this->sub_map_lock_);
    // Since we failed to insert into into the pub_map_, and have
    // already inserted it in the sub_map_, we better attempt to
    // undo the insert that we did to the sub_map_.  Otherwise,
    // the sub_map_ and pub_map_ will become inconsistent.
    sub_undo_result = this->sub_map_.remove(subscriber_id,
                                            publisher_id);
  }

  //this->send_strategy_->link_released (false);

  // We only get to here when an error occurred somewhere along the way.
  // None of this needs the lock_ to be acquired.

  if (sub_result == 0) {
    if (pub_result != 0) {
      RepoIdConverter pub_converter(publisher_id);
      RepoIdConverter sub_converter(subscriber_id);
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: DataLink::make_reservation: ")
                 ACE_TEXT("Failed to insert remote publisher %C to local ")
                 ACE_TEXT("subscriber %C reservation into pub_map_.\n"),
                 std::string(pub_converter).c_str(),
                 std::string(sub_converter).c_str()));
    }

    if (sub_undo_result != 0) {
      RepoIdConverter sub_converter(subscriber_id);
      RepoIdConverter pub_converter(publisher_id);
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: DataLink::make_reservations: ")
                 ACE_TEXT("failed to remove (undo) local subscriber %C to remote ")
                 ACE_TEXT("publisher %C reservation from sub_map_.\n"),
                 std::string(sub_converter).c_str(),
                 std::string(pub_converter).c_str()));
    }

    if (pub_undo_result != 0) {
      RepoIdConverter pub_converter(publisher_id);
      RepoIdConverter sub_converter(subscriber_id);
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: DataLink::make_reservations: ")
                 ACE_TEXT("failed to remove (undo) local publisher %C to remote ")
                 ACE_TEXT("subscriber %C reservation from pub_map_.\n"),
                 std::string(pub_converter).c_str(),
                 std::string(sub_converter).c_str()));
    }

  } else {
    RepoIdConverter sub_converter(subscriber_id);
    RepoIdConverter pub_converter(publisher_id);
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: DataLink::make_reservations: ")
               ACE_TEXT("failed to insert local subscriber %C to remote ")
               ACE_TEXT("publisher %C reservation into sub_map_.\n"),
               std::string(sub_converter).c_str(),
               std::string(pub_converter).c_str()));
  }

  return -1;
}

/// This gets invoked when a TransportInterface::remove_associations()
/// call has been made.  Because this DataLink can be shared amongst
/// different TransportInterface objects, and different threads could
/// be "managing" the different TransportInterface objects, we need
/// to make sure that this release_reservations() works in conjunction
/// with a simultaneous call (in another thread) to one of this
/// DataLink's make_reservation() methods.
void
OpenDDS::DCPS::DataLink::release_reservations(RepoId          remote_id,
                                              RepoId          local_id,
                                              DataLinkSetMap& released_locals)
{
  DBG_ENTRY_LVL("DataLink","release_reservations",6);

  if (DCPS_debug_level > 9) {
    RepoIdConverter local(local_id);
    RepoIdConverter remote(remote_id);
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
      RepoIdConverter converter(remote_id);
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: DataLink::release_reservations: ")
                 ACE_TEXT("unable to locate remote %C in pub_map_ or sub_map_.\n"),
                 std::string(converter).c_str()));

    } else {
      VDBG_LVL((LM_DEBUG, "(%P|%t) DataLink::release_reservations: the remote_id is a sub id.\n"), 5);
      //guard.release ();
      // The remote_id is a subscriber_id.
      this->release_remote_subscriber(remote_id,
                                      local_id,
                                      id_set,
                                      released_locals);

      if (id_set->size() == 0) {
        // Remove the remote_id(sub) after the remote/local ids is released
        // and there are no local pubs associated with this sub.
        id_set = this->sub_map_.remove_set(remote_id);
      }

      //guard.acquire ();
    }

  } else {
    VDBG_LVL((LM_DEBUG,
              ACE_TEXT("(%P|%t) DataLink::release_reservations: ")
              ACE_TEXT("the remote_id is a pub id.\n")), 5);
    //guard.release ();
    // The remote_id is a publisher_id.
    this->release_remote_publisher(remote_id,
                                   local_id,
                                   listener_set,
                                   released_locals);

    if (listener_set->size() == 0) {
      GuardType guard(this->pub_map_lock_);
      // Remove the remote_id(pub) after the remote/local ids is released
      // and there are no local subs associated with this pub.
      listener_set = this->pub_map_.remove_set(remote_id);
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

    if (this->datalink_release_delay_ > ACE_Time_Value::zero) {
      // The samples has to be removed at this point, otherwise the sample
      // can not be delivered when new association is added and still use
      // this connection/datalink.
      this->send_strategy_->clear();

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
OpenDDS::DCPS::DataLink::cancel_release()
{
  CORBA::ORB_var orb = TheServiceParticipant->get_ORB();
  ACE_Reactor* reactor = orb->orb_core()->reactor();
  return reactor->cancel_timer(this) > 0;
}

int
OpenDDS::DCPS::DataLink::start_i()
{
  DBG_ENTRY_LVL("DataLink","start_i",6);

  return 0;
}

void
OpenDDS::DCPS::DataLink::stop_i()
{
  DBG_ENTRY_LVL("DataLink","stop_i",6);
}

void
OpenDDS::DCPS::DataLink::control_delivered(ACE_Message_Block* message)
{
  DBG_ENTRY_LVL("DataLink","control_delivered",6);

  message->release();
}

void
OpenDDS::DCPS::DataLink::control_dropped(ACE_Message_Block* message,
                                         bool /*dropped_by_transport*/)
{
  DBG_ENTRY_LVL("DataLink","control_dropped",6);

  message->release();
}

ACE_Message_Block*
OpenDDS::DCPS::DataLink::create_control(char submessage_id,
                                        ACE_Message_Block* data)
{
  DBG_ENTRY_LVL("DataLink","create_control",6);

  DataSampleHeader header;

  header.byte_order_ = this->impl_->swap_bytes() ? !TAO_ENCAP_BYTE_ORDER
                                                 : TAO_ENCAP_BYTE_ORDER;
  header.message_id_ = TRANSPORT_CONTROL;
  header.submessage_id_ = submessage_id;
  header.message_length_ = data->total_length();

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

  message << header;

  return message;
}

OpenDDS::DCPS::SendControlStatus
OpenDDS::DCPS::DataLink::send_control(ACE_Message_Block* message)
{
  DBG_ENTRY_LVL("DataLink","send_control",6);

  TransportSendControlElement* elem;

  ACE_NEW_MALLOC_RETURN(elem,
                        static_cast<TransportSendControlElement*>(
                          this->send_control_allocator_->malloc()),
                        TransportSendControlElement(1,  // initial_count
                                                    GUID_UNKNOWN,
                                                    this,
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
OpenDDS::DCPS::DataLink::data_received(ReceivedDataSample& sample)
{
  DBG_ENTRY_LVL("DataLink","data_received",6);

  // Which remote publisher sent this message?
  RepoId publisher_id = sample.header_.publication_id_;

  // Locate the set of TransportReceiveListeners associated with this
  // DataLink that are interested in hearing about any samples received
  // from the remote publisher_id.
  ReceiveListenerSet_rch listener_set;

  if (OpenDDS::DCPS::Transport_debug_level > 9) {
    std::stringstream buffer;
    buffer << sample.header_;
    RepoIdConverter converter(publisher_id);
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
    if (OpenDDS::DCPS::Transport_debug_level > 4) {
      RepoIdConverter converter(publisher_id);
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) DataLink::data_received: ")
                 ACE_TEXT(" discarding sample from publisher %C due to no listeners.\n"),
                 std::string(converter).c_str()));
    }

    return 0;
  }

  // Just get the set to do our dirty work by having it iterate over its
  // collection of TransportReceiveListeners, and invoke the data_received()
  // method on each one.
  listener_set->data_received(sample);

  return 0;
}

void
OpenDDS::DCPS::DataLink::ack_received(ReceivedDataSample& sample)
{
  RepoId publication = GUID_UNKNOWN;
  TAO::DCPS::Serializer serializer(
    sample.sample_,
    sample.header_.byte_order_ != TAO_ENCAP_BYTE_ORDER);
  serializer >> publication;

  TransportSendListener* listener;
  {
    GuardType guard(this->pub_map_lock_);
    IdToSendListenerMap::const_iterator where
    = this->send_listeners_.find(publication);

    if (where == this->send_listeners_.end()) {
      RepoIdConverter converter(publication);
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) DataLink::ack_received: ")
                 ACE_TEXT("listener for publication %C not found.\n"),
                 std::string(converter).c_str()));
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
OpenDDS::DCPS::DataLink::release_remote_subscriber
(RepoId          subscriber_id,
 RepoId          publisher_id,
 RepoIdSet_rch&      pubid_set,
 DataLinkSetMap& released_publishers)
{
  DBG_ENTRY_LVL("DataLink","release_remote_subscriber",6);

  RepoIdSet::MapType& pubid_map = pubid_set->map();

  for (RepoIdSet::MapType::iterator itr = pubid_map.begin();
       itr != pubid_map.end();
       ++itr) {
    if (publisher_id == itr->first) {
      // Remove the publisher_id => subscriber_id association.
      if (this->pub_map_.release_subscriber(publisher_id,
                                            subscriber_id) == 1) {
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
    RepoIdConverter converter(publisher_id);
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
OpenDDS::DCPS::DataLink::release_remote_publisher
(RepoId              publisher_id,
 RepoId              subscriber_id,
 ReceiveListenerSet_rch& listener_set,
 DataLinkSetMap&     released_subscribers)
{
  DBG_ENTRY_LVL("DataLink","release_remote_publisher",6);

  if (listener_set->exist(subscriber_id)) {
    // Remove the publisher_id => subscriber_id association.
    if (this->sub_map_.release_publisher(subscriber_id,publisher_id) == 1) {
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
    RepoIdConverter converter(subscriber_id);
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: DataLink::release_remote_publisher: ")
               ACE_TEXT(" failed to remove sub %C from ListenerSet.\n"),
               std::string(converter).c_str()));
  }
}

// static
ACE_UINT64
OpenDDS::DCPS::DataLink::get_next_datalink_id()
{
  static ACE_UINT64 next_id = 0;
  static LockType lock;

  DBG_ENTRY_LVL("DataLink","get_next_datalink_id",6);

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
OpenDDS::DCPS::DataLink::transport_shutdown()
{
  DBG_ENTRY_LVL("DataLink","transport_shutdown",6);

  if (this->cancel_release()) {
    this->handle_timeout(ACE_OS::gettimeofday(), (const void *)0);
  }

  {
    GuardType guard(this->strategy_lock_);

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
  }

  // Drop our reference to the TransportImpl object
  this->impl_ = 0;
}

void
OpenDDS::DCPS::DataLink::notify(enum ConnectionNotice notice)
{
  DBG_ENTRY_LVL("DataLink","notify",6);

  VDBG((LM_DEBUG,
        ACE_TEXT("(%P|%t) DataLink::notify: this(%X) notify %C\n"),
        this,
        connection_notice_as_str(notice)));

  {
    GuardType guard(this->pub_map_lock_);

    ReceiveListenerSetMap::MapType & map = this->pub_map_.map();

    // Notify the datawriters registered with TransportImpl
    // the lost publications due to a connection problem.
    for (ReceiveListenerSetMap::MapType::iterator itr = map.begin();
         itr != map.end();
         ++itr) {
      DataWriterImpl* dw = this->impl_->find_publication(itr->first);

      if (dw != 0) {
        if (OpenDDS::DCPS::Transport_debug_level > 0) {
          RepoIdConverter converter(itr->first);
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
          dw->notify_publication_disconnected(subids);
          break;
        case RECONNECTED:
          dw->notify_publication_reconnected(subids);
          break;
        case LOST:
          dw->notify_publication_lost(subids);
          break;
        default:
          ACE_ERROR((LM_ERROR,
                     ACE_TEXT("(%P|%t)ERROR: DataLink::notify: ")
                     ACE_TEXT("unknown notice to datawriter\n")));
          break;
        }

      } else {
        if (OpenDDS::DCPS::Transport_debug_level > 0) {
          RepoIdConverter converter(itr->first);
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
      // subscription_handles
      DataReaderImpl* dr = this->impl_->find_subscription(itr->first);

      if (dr != 0) {
        if (OpenDDS::DCPS::Transport_debug_level > 0) {
          RepoIdConverter converter(itr->first);
          ACE_DEBUG((LM_DEBUG,
                     ACE_TEXT("(%P|%t) DataLink::notify: ")
                     ACE_TEXT("notify sub %C %C.\n"),
                     std::string(converter).c_str(),
                     connection_notice_as_str(notice)));
        }

        RepoIdSet_rch pubset = itr->second;
        RepoIdSet::MapType & map = pubset->map();

        WriterIdSeq pubids;
        pubids.length(pubset->size());
        CORBA::ULong i = 0;

        for (RepoIdSet::MapType::iterator iitr = map.begin();
             iitr != map.end();
             ++iitr) {
          pubids[i++] = iitr->first;
        }

        switch (notice) {
        case DISCONNECTED:
          dr->notify_subscription_disconnected(pubids);
          break;
        case RECONNECTED:
          dr->notify_subscription_reconnected(pubids);
          break;
        case LOST:
          dr->notify_subscription_lost(pubids);
          break;
        default:
          ACE_ERROR((LM_ERROR,
                     ACE_TEXT("(%P|%t) ERROR: DataLink::notify: ")
                     ACE_TEXT("unknown notice to datareader.\n")));
          break;
        }

      } else {
        if (OpenDDS::DCPS::Transport_debug_level > 0) {
          RepoIdConverter converter(itr->first);
          ACE_DEBUG((LM_DEBUG,
                     ACE_TEXT("(%P|%t)DataLink::notify: ")
                     ACE_TEXT("not notify sub %C subscription lost.\n"),
                     std::string(converter).c_str()));
        }

      }
    }
  }
}

void
OpenDDS::DCPS::DataLink::notify_connection_deleted()
{
  GuardType guard(this->released_local_lock_);

  RepoIdSet::MapType& pmap = released_local_pubs_.map();

  for (RepoIdSet::MapType::iterator itr = pmap.begin();
       itr != pmap.end();
       ++itr) {
    DataWriterImpl* dw = this->impl_->find_publication(itr->first);

    if (dw != 0) {
      if (OpenDDS::DCPS::Transport_debug_level > 0) {
        RepoIdConverter converter(itr->first);
        ACE_DEBUG((LM_DEBUG,
                   ACE_TEXT("(%P|%t)DataLink:: notify_connection_deleted: ")
                   ACE_TEXT("notify pub %C connection deleted.\n"),
                   std::string(converter).c_str()));
      }

      dw->notify_connection_deleted();
    }
  }

  RepoIdSet::MapType& smap = released_local_subs_.map();

  for (RepoIdSet::MapType::iterator itr2 = smap.begin();
       itr2 != smap.end();
       ++itr2) {
    DataReaderImpl* dr = this->impl_->find_subscription(itr2->first);

    if (dr != 0) {
      if (OpenDDS::DCPS::Transport_debug_level > 0) {
        RepoIdConverter converter(itr2->first);
        ACE_DEBUG((LM_DEBUG,
                   ACE_TEXT("(%P|%t) DataLink::notify_connection_deleted: ")
                   ACE_TEXT("notify sub %C connection deleted.\n"),
                   std::string(converter).c_str()));
      }

      dr->notify_connection_deleted();
    }
  }
}

void
OpenDDS::DCPS::DataLink::pre_stop_i()
{
  if (this->thr_per_con_send_task_ != 0) {
    this->thr_per_con_send_task_->close(1);
  }
}

ACE_Message_Block*
OpenDDS::DCPS::DataLink::marshal_acks(bool byte_order)
{
  DBG_ENTRY_LVL("DataLink","marshal_acks",6);
  return this->sub_map_.marshal(byte_order);
}

bool
OpenDDS::DCPS::DataLink::release_resources()
{
  DBG_ENTRY_LVL("DataLink", "release_resources",6);

  this->prepare_release();

  return impl_->release_link_resources(this);
}

bool
OpenDDS::DCPS::DataLink::is_target(const RepoId& sub_id)
{
  GuardType guard(this->sub_map_lock_);
  RepoIdSet_rch pubs = this->sub_map_.find(sub_id);

  return !pubs.is_nil();
}

bool
OpenDDS::DCPS::DataLink::exist(const RepoId& remote_id,
                               const RepoId& local_id,
                               const bool&   pub_side,
                               bool& last)
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

void OpenDDS::DCPS::DataLink::prepare_release()
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

void OpenDDS::DCPS::DataLink::clear_associations()
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
    DataWriterImpl *dw = this->impl_->find_publication(pub_id, true);

    ReceiveListenerSet_rch sub_id_set = pub_map_iter->second;
    // The iterator seems to get corrupted if the element currently
    // being pointed at gets unbound. Hence advance it.
    ++pub_map_iter;

    // Check is DataWriter exists (could have been deleted before we got here.
    if (dw != NULL) {
      // Each pub-id is mapped to a bunch of sub-id's
      //ReceiveListenerSet_rch sub_id_set = pub_entry->int_id_;
      ReaderIdSeq sub_ids;
      sub_id_set->get_keys(sub_ids);

      // after creating remote id sequence, remove from DataWriter
      // I believe the 'notify_lost' should be set to false, since
      // it doesn't look like we meet any of the conditions for setting
      // it true. Check interface documentations.
      dw->remove_associations(sub_ids, false);

      // Since we requested a safe copy, we now need to remove the local reference.
      dw->_remove_ref();
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
    DataReaderImpl *dr = this->impl_->find_subscription(sub_id, true);

    RepoIdSet_rch pub_id_set = sub_map_iter->second;
    // The iterator seems to get corrupted if the element currently
    // being pointed at gets unbound. Hence advance it.
    ++sub_map_iter;

    // Check id DataReader exists (could have been deleted before we got here.)
    if (dr != NULL) {
      // Each sub-id is mapped to a bunch of pub-id's
      ssize_t pub_ids_count = pub_id_set->size();
      WriterIdSeq pub_ids(pub_ids_count);
      pub_ids.length(pub_ids_count);

      int count = 0;

      // create a sequence of associated pub-id's
      for (RepoIdSet::MapType::iterator pub_ids_iter = pub_id_set->map().begin();
           pub_ids_iter != pub_id_set->map().end(); ++pub_ids_iter) {
        pub_ids [count++] = pub_ids_iter->first;
      }

      // after creating remote id sequence, remove from DataReader
      // I believe the 'notify_lost' should be set to false, since
      // it doesn't look like we meet any of the conditions for setting
      // it true. Check interface documentations.
      dr->remove_associations(pub_ids, false);

      // Since we requested a safe copy, we now need to remove the local reference.
      dr->_remove_ref();
    }
  }

  sub_map_releasing_.clear();
  pub_map_releasing_.clear();
}

int
OpenDDS::DCPS::DataLink::handle_timeout(const ACE_Time_Value &/*tv*/,
                                        const void * arg)
{
  ACE_UNUSED_ARG(arg);

  if ((this->pub_map_.size() + this->sub_map_.size()) == 0) {
    this->pre_stop_i();

    // The TransportImpl ptr should be cleaned in the dstr.
    // This link will be used as a callback after the actual
    // connection is closed.
    //this->impl_ = 0;

    TransportSendStrategy_rch send_strategy = 0;
    TransportReceiveStrategy_rch recv_strategy = 0;
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

  this->_remove_ref();

  return 0;
}

void
OpenDDS::DCPS::DataLink::set_dscp_codepoint(int cp, ACE_SOCK& socket)
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
operator<<(std::ostream& str, const OpenDDS::DCPS::DataLink& value)
{
  using namespace OpenDDS::DCPS;
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
      str << RepoIdConverter(pubLocation->first) << " --> "
      << RepoIdConverter(subLocation->first) << "   " << std::endl;
    }
  }

  return str;
}
