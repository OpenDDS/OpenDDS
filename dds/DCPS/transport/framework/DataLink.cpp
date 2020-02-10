/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "DataLink.h"

#include "ReceivedDataSample.h"

#include "TransportImpl.h"
#include "TransportInst.h"
#include "TransportClient.h"

#include "dds/DCPS/DataWriterImpl.h"
#include "dds/DCPS/DataReaderImpl.h"
#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/GuidConverter.h"
#include "dds/DdsDcpsGuidTypeSupportImpl.h"
#include "dds/DCPS/Util.h"
#include "dds/DCPS/Definitions.h"
#include "dds/DCPS/SafetyProfileStreams.h"

#include "EntryExit.h"
#include "tao/debug.h"
#include "ace/Reactor.h"
#include "ace/SOCK.h"


#if !defined (__ACE_INLINE__)
#include "DataLink.inl"
#endif /* __ACE_INLINE__ */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

/// Only called by our TransportImpl object.
DataLink::DataLink(TransportImpl& impl, Priority priority, bool is_loopback,
                   bool is_active)
  : stopped_(false),
    impl_(impl),
    transport_priority_(priority),
    scheduling_release_(false),
    is_loopback_(is_loopback),
    is_active_(is_active),
    started_(false),
    send_response_listener_("DataLink"),
    interceptor_(impl_.reactor(), impl_.reactor_owner())
{
  DBG_ENTRY_LVL("DataLink", "DataLink", 6);

  datalink_release_delay_ = TimeDuration::from_msec(impl.config().datalink_release_delay_);

  id_ = DataLink::get_next_datalink_id();

  if (impl.config().thread_per_connection_) {
    this->thr_per_con_send_task_.reset(new ThreadPerConnectionSendTask(this));

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
  size_t control_chunks = impl.config().datalink_control_chunks_;

  this->mb_allocator_.reset(new MessageBlockAllocator(control_chunks));
  this->db_allocator_.reset(new DataBlockAllocator(control_chunks));
}

DataLink::~DataLink()
{
  DBG_ENTRY_LVL("DataLink", "~DataLink", 6);

  if (!assoc_by_local_.empty()) {
    ACE_DEBUG((LM_WARNING,
               ACE_TEXT("(%P|%t) WARNING: DataLink[%@]::~DataLink() - ")
               ACE_TEXT("link still in use by %d entities when deleted!\n"),
               this, assoc_by_local_.size()));
  }

  if (this->thr_per_con_send_task_ != 0) {
    this->thr_per_con_send_task_->close(1);
  }
}

TransportImpl&
DataLink::impl() const
{
  return impl_;
}

bool
DataLink::add_on_start_callback(const TransportClient_wrch& client, const RepoId& remote)
{
  const DataLink_rch link(this, inc_count());

  GuardType guard(strategy_lock_);

  TransportClient_rch client_lock = client.lock();
  if (client_lock) {
    PendingOnStartsMap::iterator it = pending_on_starts_.find(remote);
    if (it != pending_on_starts_.end()) {
      RepoIdSet::iterator it2 = it->second.find(client_lock->get_repo_id());
      if (it2 != it->second.end()) {
        it->second.erase(it2);
        if (it->second.empty()) {
          pending_on_starts_.erase(it);
        }
        guard.release();
        interceptor_.execute_or_enqueue(new ImmediateStart(link, client, remote));
      } else {
        on_start_callbacks_[remote][client_lock->get_repo_id()] = client;
      }
    } else {
      on_start_callbacks_[remote][client_lock->get_repo_id()] = client;
    }
  }

  if (started_ && !send_strategy_.is_nil()) {
    return false; // link already started
  }
  return true;
}

void
DataLink::remove_startup_callbacks(const RepoId& local, const RepoId& remote)
{
  GuardType guard(strategy_lock_);

  OnStartCallbackMap::iterator oit = on_start_callbacks_.find(remote);
  if (oit != on_start_callbacks_.end()) {
    RepoToClientMap::iterator oit2 = oit->second.find(local);
    if (oit2 != oit->second.end()) {
      oit->second.erase(oit2);
      if (oit->second.empty()) {
        on_start_callbacks_.erase(oit);
      }
    }
  }
  PendingOnStartsMap::iterator pit = pending_on_starts_.find(remote);
  if (pit != pending_on_starts_.end()) {
    RepoIdSet::iterator pit2 = pit->second.find(local);
    if (pit2 != pit->second.end()) {
      pit->second.erase(pit2);
      if (pit->second.empty()) {
        pending_on_starts_.erase(pit);
      }
    }
  }
}

void
DataLink::remove_on_start_callback(const TransportClient_wrch& client, const RepoId& remote)
{
  GuardType guard(strategy_lock_);

  TransportClient_rch client_lock = client.lock();
  if (client_lock) {
    OnStartCallbackMap::iterator it = on_start_callbacks_.find(remote);
    if (it != on_start_callbacks_.end()) {
      RepoToClientMap::iterator it2 = it->second.find(client_lock->get_repo_id());
      if (it2 != it->second.end()) {
        it->second.erase(it2);
        if (it->second.empty()) {
          on_start_callbacks_.erase(it);
        }
      }
    }
  }
}

void
DataLink::invoke_on_start_callbacks(bool success)
{
  const DataLink_rch link(success ? this : 0, inc_count());

  while (true) {
    GuardType guard(strategy_lock_);

    if (on_start_callbacks_.empty()) {
      break;
    }

    RepoId remote;
    TransportClient_wrch client;

    OnStartCallbackMap::iterator it = on_start_callbacks_.begin();
    if (it != on_start_callbacks_.end()) {
      remote = it->first;
      RepoToClientMap::iterator it2 = it->second.begin();
      if (it2 != it->second.end()) {
        client = it2->second;
        it->second.erase(it2);
        if (it->second.empty()) {
          on_start_callbacks_.erase(it);
        }
      }
    }

    guard.release();
    TransportClient_rch client_lock = client.lock();
    if (client_lock) {
      client_lock->use_datalink(remote, link);
    }
  }
}

void
DataLink::invoke_on_start_callbacks(const RepoId& local, const RepoId& remote, bool success)
{
  const DataLink_rch link(success ? this : 0, inc_count());

  TransportClient_wrch client;

  {
    GuardType guard(strategy_lock_);

    OnStartCallbackMap::iterator it = on_start_callbacks_.find(remote);
    if (it != on_start_callbacks_.end()) {
      RepoToClientMap::iterator it2 = it->second.find(local);
      if (it2 != it->second.end()) {
        client = it2->second;
        it->second.erase(it2);
        if (it->second.empty()) {
          on_start_callbacks_.erase(it);
        }
      } else {
        pending_on_starts_[remote].insert(local);
      }
    } else {
      pending_on_starts_[remote].insert(local);
    }
  }

  TransportClient_rch client_lock = client.lock();
  if (client_lock) {
    client_lock->use_datalink(remote, link);
  }
}

//Reactor invokes this after being notified in schedule_stop or cancel_release
int
DataLink::handle_exception(ACE_HANDLE /* fd */)
{
  const MonotonicTimePoint now = MonotonicTimePoint::now();
  if (scheduled_to_stop_at_.is_zero()) {
    if (DCPS_debug_level > 0) {
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) DataLink::handle_exception() - not scheduling or stopping\n")));
    }
    ACE_Reactor_Timer_Interface* reactor = impl_.timer();
    if (reactor->cancel_timer(this) > 0) {
      if (DCPS_debug_level > 0) {
        ACE_DEBUG((LM_DEBUG,
                   ACE_TEXT("(%P|%t) DataLink::handle_exception() - cancelled future release timer\n")));
      }
    }
    return 0;
  } else if (scheduled_to_stop_at_ <= now) {
    if (this->scheduling_release_) {
      if (DCPS_debug_level > 0) {
        ACE_DEBUG((LM_DEBUG,
                   ACE_TEXT("(%P|%t) DataLink::handle_exception() - delay already elapsed so handle_timeout now\n")));
      }
      this->handle_timeout(ACE_Time_Value::zero, 0);
      return 0;
    }
    if (DCPS_debug_level > 0) {
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) DataLink::handle_exception() - stopping now\n")));
    }
    this->stop();
    return 0;
  } else /* SCHEDULE TO STOP IN THE FUTURE*/ {
    if (DCPS_debug_level > 0) {
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) DataLink::handle_exception() - (delay) scheduling timer for future release\n")));
    }
    ACE_Reactor_Timer_Interface* reactor = impl_.timer();
    const TimeDuration future_release_time = scheduled_to_stop_at_ - now;
    reactor->schedule_timer(this, 0, future_release_time.value());
  }
  return 0;
}

//Allows DataLink::stop to be done on the reactor thread so that
//this thread avoids possibly deadlocking trying to access reactor
//to stop strategies or schedule timers
void
DataLink::schedule_stop(const MonotonicTimePoint& schedule_to_stop_at)
{
  if (!stopped_ && scheduled_to_stop_at_.is_zero()) {
    this->scheduled_to_stop_at_ = schedule_to_stop_at;
    notify_reactor();
    // reactor will invoke our DataLink::handle_exception()
  } else {
    if (DCPS_debug_level > 0) {
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) DataLink::schedule_stop() - Already stopped or already scheduled for stop\n")));
    }
  }
}

void
DataLink::notify_reactor()
{
  ReactorTask_rch reactor(impl_.reactor_task());
  reactor->get_reactor()->notify(this);
}

void
DataLink::stop()
{
  this->pre_stop_i();

  TransportSendStrategy_rch send_strategy;
  TransportStrategy_rch recv_strategy;

  {
    GuardType guard(this->strategy_lock_);

    if (this->stopped_) return;

    send_strategy = this->send_strategy_;
    this->send_strategy_.reset();

    recv_strategy = this->receive_strategy_;
    this->receive_strategy_.reset();
  }

  if (!send_strategy.is_nil()) {
    send_strategy->stop();
  }

  if (!recv_strategy.is_nil()) {
    recv_strategy->stop();
  }

  this->stop_i();
  this->stopped_ = true;
  scheduled_to_stop_at_ = MonotonicTimePoint::zero_value;
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
                           const TransportSendListener_wrch& send_listener,
                           bool reliable)
{
  DBG_ENTRY_LVL("DataLink", "make_reservation", 6);

  if (DCPS_debug_level > 9) {
    GuidConverter local(local_publication_id), remote(remote_subscription_id);
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) DataLink::make_reservation() - ")
               ACE_TEXT("creating association local publication  %C ")
               ACE_TEXT("<--> with remote subscription %C.\n"),
               OPENDDS_STRING(local).c_str(),
               OPENDDS_STRING(remote).c_str()));
  }

  {
    GuardType guard(strategy_lock_);

    if (!send_strategy_.is_nil()) {
      send_strategy_->link_released(false);
    }
  }
  {
    GuardType guard(pub_sub_maps_lock_);

    LocalAssociationInfo& info = assoc_by_local_[local_publication_id];
    info.reliable_ = reliable;
    info.associated_.insert(remote_subscription_id);
    ReceiveListenerSet_rch& rls = assoc_by_remote_[remote_subscription_id];

    if (rls.is_nil())
      rls = make_rch<ReceiveListenerSet>();
    rls->insert(local_publication_id, TransportReceiveListener_rch());

    send_listeners_.insert(std::make_pair(local_publication_id, send_listener));
  }
  return 0;
}

int
DataLink::make_reservation(const RepoId& remote_publication_id,
                           const RepoId& local_subscription_id,
                           const TransportReceiveListener_wrch& receive_listener,
                           bool reliable)
{
  DBG_ENTRY_LVL("DataLink", "make_reservation", 6);

  if (DCPS_debug_level > 9) {
    GuidConverter local(local_subscription_id), remote(remote_publication_id);
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) DataLink::make_reservation() - ")
               ACE_TEXT("creating association local subscription %C ")
               ACE_TEXT("<--> with remote publication  %C.\n"),
               OPENDDS_STRING(local).c_str(), OPENDDS_STRING(remote).c_str()));
  }

  {
    GuardType guard(strategy_lock_);

    if (!send_strategy_.is_nil()) {
      send_strategy_->link_released(false);
    }
  }
  {
    GuardType guard(pub_sub_maps_lock_);

    LocalAssociationInfo& info = assoc_by_local_[local_subscription_id];
    info.reliable_ = reliable;
    info.associated_.insert(remote_publication_id);
    ReceiveListenerSet_rch& rls = assoc_by_remote_[remote_publication_id];

    if (rls.is_nil())
      rls = make_rch<ReceiveListenerSet>();
    rls->insert(local_subscription_id, receive_listener);

    recv_listeners_.insert(std::make_pair(local_subscription_id,
                                          receive_listener));
  }
  return 0;
}

template <typename Seq>
void set_to_seq(const RepoIdSet& rids, Seq& seq)
{
  seq.length(static_cast<CORBA::ULong>(rids.size()));
  CORBA::ULong i = 0;
  for (RepoIdSet::const_iterator iter = rids.begin(); iter != rids.end(); ++iter) {
    seq[i++] = *iter;
  }
}

GUIDSeq*
DataLink::peer_ids(const RepoId& local_id) const
{
  GuardType guard(pub_sub_maps_lock_);

  const AssocByLocal::const_iterator iter = assoc_by_local_.find(local_id);

  if (iter == assoc_by_local_.end())
    return 0;

  GUIDSeq_var result = new GUIDSeq;
  set_to_seq(iter->second.associated_, static_cast<GUIDSeq&>(result));
  return result._retn();
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
               OPENDDS_STRING(local).c_str(),
               OPENDDS_STRING(remote).c_str()));
  }

  remove_startup_callbacks(local_id, remote_id);

  //let the specific class release its reservations
  //done this way to prevent deadlock of holding pub_sub_maps_lock_
  //then obtaining a specific class lock in release_reservations_i
  //which reverses lock ordering of the active send logic of needing
  //the specific class lock before obtaining the over arching DataLink
  //pub_sub_maps_lock_
  this->release_reservations_i(remote_id, local_id);

  bool release_remote_required = false;
  {
    GuardType guard(this->pub_sub_maps_lock_);

    if (this->stopped_) return;

    ReceiveListenerSet_rch& rls = assoc_by_remote_[remote_id];
    if (rls->size() == 1) {
      assoc_by_remote_.erase(remote_id);
      release_remote_required = true;
    } else {
      rls->remove(local_id);
    }
    RepoIdSet& ris = assoc_by_local_[local_id].associated_;
    if (ris.size() == 1) {
      DataLinkSet_rch& links = released_locals[local_id];
      if (links.is_nil())
        links = make_rch<DataLinkSet>();
      links->insert_link(rchandle_from(this));
      assoc_by_local_.erase(local_id);
    } else {
      ris.erase(remote_id);
    }

    if (assoc_by_local_.empty()) {
      VDBG_LVL((LM_DEBUG,
                ACE_TEXT("(%P|%t) DataLink::release_reservations: ")
                ACE_TEXT("release_datalink due to no remaining pubs or subs.\n")), 5);

      impl_.release_datalink(this);
    }
  }
  if (release_remote_required)
    release_remote_i(remote_id);
}

void
DataLink::schedule_delayed_release()
{
  DBG_ENTRY_LVL("DataLink", "schedule_delayed_release", 6);

  VDBG((LM_DEBUG, "(%P|%t) DataLink[%@]::schedule_delayed_release\n", this));

  // The samples have to be removed at this point, otherwise the samples
  // can not be delivered when new association is added and still use
  // this connection/datalink.
  if (!this->send_strategy_.is_nil()) {
    this->send_strategy_->clear(TransportSendStrategy::MODE_DIRECT);
  }

  const MonotonicTimePoint future_release_time(MonotonicTimePoint::now() + datalink_release_delay_);
  this->schedule_stop(future_release_time);
}

bool
DataLink::cancel_release()
{
  DBG_ENTRY_LVL("DataLink", "cancel_release", 6);
  if (stopped_) {
    if (DCPS_debug_level > 0) {
      ACE_DEBUG((LM_DEBUG, "(%P|%t) DataLink::cancel_release - link[%@] already stopped_ cannot cancel release\n", this));
    }
    return false;
  }
  if (scheduling_release_) {
    if (DCPS_debug_level > 0) {
      ACE_DEBUG((LM_DEBUG, "(%P|%t) DataLink::cancel_release - link[%@] currently scheduling release, notify reactor of cancel\n", this));
    }
    this->set_scheduling_release(false);
    scheduled_to_stop_at_ = MonotonicTimePoint::zero_value;
    notify_reactor();
  }
  return true;
}

void
DataLink::stop_i()
{
  DBG_ENTRY_LVL("DataLink", "stop_i", 6);
}

ACE_Message_Block*
DataLink::create_control(char submessage_id,
                         DataSampleHeader& header,
                         Message_Block_Ptr data)
{
  DBG_ENTRY_LVL("DataLink", "create_control", 6);

  header.byte_order_ = ACE_CDR_BYTE_ORDER;
  header.message_id_ = TRANSPORT_CONTROL;
  header.submessage_id_ = submessage_id;
  header.message_length_ = static_cast<ACE_UINT32>(data->total_length());

  ACE_Message_Block* message = 0;
  ACE_NEW_MALLOC_RETURN(message,
                        static_cast<ACE_Message_Block*>(
                          this->mb_allocator_->malloc(sizeof(ACE_Message_Block))),
                        ACE_Message_Block(header.max_marshaled_size(),
                                          ACE_Message_Block::MB_DATA,
                                          data.release(),
                                          0,  // data
                                          0,  // allocator_strategy
                                          0,  // locking_strategy
                                          ACE_DEFAULT_MESSAGE_BLOCK_PRIORITY,
                                          ACE_Time_Value::zero,
                                          ACE_Time_Value::max_time,
                                          this->db_allocator_.get(),
                                          this->mb_allocator_.get()),
                        0);

  if (!(*message << header)) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) DataLink::create_control: ")
               ACE_TEXT("cannot put header in message\n")));
    ACE_DES_FREE(message, this->mb_allocator_->free, ACE_Message_Block);
    message = 0;
  }

  return message;
}

SendControlStatus
DataLink::send_control(const DataSampleHeader& header, Message_Block_Ptr message)
{
  DBG_ENTRY_LVL("DataLink", "send_control", 6);

  TransportSendControlElement* const elem = new TransportSendControlElement(1, // initial_count
                                       GUID_UNKNOWN, &send_response_listener_,
                                       header, move(message));

  send_response_listener_.track_message();

  RepoId senderId(header.publication_id_);
  send_start();
  send(elem);
  send_stop(senderId);

  return SEND_CONTROL_OK;
}

/// This method will "deliver" the sample to all TransportReceiveListeners
/// within this DataLink that are interested in the (remote) publisher id
/// that sent the sample.
int
DataLink::data_received(ReceivedDataSample& sample,
                        const RepoId& readerId /* = GUID_UNKNOWN */)
{
  data_received_i(sample, readerId, RepoIdSet(), ReceiveListenerSet::SET_EXCLUDED);
  return 0;
}

void
DataLink::data_received_include(ReceivedDataSample& sample, const RepoIdSet& incl)
{
  data_received_i(sample, GUID_UNKNOWN, incl, ReceiveListenerSet::SET_INCLUDED);
}

void
DataLink::data_received_i(ReceivedDataSample& sample,
                          const RepoId& readerId,
                          const RepoIdSet& incl_excl,
                          ReceiveListenerSet::ConstrainReceiveSet constrain)
{
  DBG_ENTRY_LVL("DataLink", "data_received_i", 6);
  // Which remote publication sent this message?
  const RepoId& publication_id = sample.header_.publication_id_;

  // Locate the set of TransportReceiveListeners associated with this
  // DataLink that are interested in hearing about any samples received
  // from the remote publisher_id.
  if (DCPS_debug_level > 9) {
    const GuidConverter converter(publication_id);
    const GuidConverter reader(readerId);
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) DataLink::data_received_i: ")
               ACE_TEXT("from publication %C received sample: %C to readerId %C (%C).\n"),
               OPENDDS_STRING(converter).c_str(),
               to_string(sample.header_).c_str(),
               OPENDDS_STRING(reader).c_str(),
               constrain == ReceiveListenerSet::SET_EXCLUDED ? "SET_EXCLUDED" : "SET_INCLUDED"));
  }

  if (Transport_debug_level > 9) {
    const GuidConverter converter(publication_id);
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) DataLink::data_received_i: ")
               ACE_TEXT("from publication %C received sample: %C.\n"),
               OPENDDS_STRING(converter).c_str(),
               to_string(sample.header_).c_str()));
  }

  ReceiveListenerSet_rch listener_set;
  TransportReceiveListener_rch listener;
  {
    GuardType guard(this->pub_sub_maps_lock_);
    AssocByRemote::iterator iter = assoc_by_remote_.find(publication_id);
    if (iter != assoc_by_remote_.end()) {
      listener_set = iter->second;
    } else {
      listener = this->default_listener_.lock();
    }
  }

  if (listener_set.is_nil()) {
    if (listener) {
      listener->data_received(sample);
    } else {
      // Nobody has any interest in this message.  Drop it on the floor.
      if (Transport_debug_level > 4) {
        const GuidConverter converter(publication_id);
        ACE_DEBUG((LM_DEBUG,
                   ACE_TEXT("(%P|%t) DataLink::data_received_i: ")
                   ACE_TEXT(" discarding sample from publication %C due to no listeners.\n"),
                   OPENDDS_STRING(converter).c_str()));
      }
    }
    return;
  }

  if (readerId != GUID_UNKNOWN) {
    listener_set->data_received(sample, readerId);
    return;
  }

#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE

  if (sample.header_.content_filter_
      && sample.header_.content_filter_entries_.length()) {
    ReceiveListenerSet subset(*listener_set.in());
    subset.remove_all(sample.header_.content_filter_entries_);
    subset.data_received(sample, incl_excl, constrain);

  } else {
#endif // OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE

    if (DCPS_debug_level > 9) {
      // Just get the set to do our dirty work by having it iterate over its
      // collection of TransportReceiveListeners, and invoke the data_received()
      // method on each one.
      OPENDDS_STRING included_ids;
      bool first = true;
      RepoIdSet::const_iterator iter = incl_excl.begin();
      while(iter != incl_excl.end()) {
        included_ids += (first ? "" : "\n") + OPENDDS_STRING(GuidConverter(*iter));
        first = false;
        ++iter;
      }
      ACE_DEBUG((LM_DEBUG, "(%P|%t) DataLink::data_received_i - normal data received to each subscription in listener_set %C ids:%C\n",
                 constrain == ReceiveListenerSet::SET_EXCLUDED ? "exclude" : "include", included_ids.c_str()));
    }
    listener_set->data_received(sample, incl_excl, constrain);
#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE
  }

#endif // OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE
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

  //this->cancel_release();
  this->set_scheduling_release(false);
  scheduled_to_stop_at_ = MonotonicTimePoint::zero_value;

  ACE_Reactor_Timer_Interface* reactor = impl_.timer();
  reactor->cancel_timer(this);

  this->stop();
  // this->send_listeners_.clear();
  // this->recv_listeners_.clear();
  // Drop our reference to the TransportImpl object
}

void
DataLink::notify(ConnectionNotice notice)
{
  DBG_ENTRY_LVL("DataLink", "notify", 6);

  VDBG((LM_DEBUG,
        ACE_TEXT("(%P|%t) DataLink::notify: this(%X) notify %C\n"),
        this,
        connection_notice_as_str(notice)));

  GuardType guard(this->pub_sub_maps_lock_);

  // Notify the datawriters
  // the lost publications due to a connection problem.
  for (IdToSendListenerMap::iterator itr = send_listeners_.begin();
       itr != send_listeners_.end(); ++itr) {

    TransportSendListener_rch tsl = itr->second.lock();

    if (tsl) {
      if (Transport_debug_level > 0) {
        GuidConverter converter(itr->first);
        ACE_DEBUG((LM_DEBUG,
                   ACE_TEXT("(%P|%t) DataLink::notify: ")
                   ACE_TEXT("notify pub %C %C.\n"),
                   OPENDDS_STRING(converter).c_str(),
                   connection_notice_as_str(notice)));
      }
      AssocByLocal::iterator local_it = assoc_by_local_.find(itr->first);
      if (local_it == assoc_by_local_.end()) {
        if (Transport_debug_level) {
          GuidConverter converter(itr->first);
          ACE_DEBUG((LM_DEBUG,
                     ACE_TEXT("(%P|%t) DataLink::notify: ")
                     ACE_TEXT("try to notify pub %C %C - no associations to notify.\n"),
                     OPENDDS_STRING(converter).c_str(),
                     connection_notice_as_str(notice)));
        }
        break;
      }
      const RepoIdSet& rids = local_it->second.associated_;

      ReaderIdSeq subids;
      set_to_seq(rids, subids);

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
                   OPENDDS_STRING(converter).c_str(),
                   connection_notice_as_str(notice)));
      }
    }
  }

  // Notify the datareaders registered with TransportImpl
  // the lost subscriptions due to a connection problem.
  for (IdToRecvListenerMap::iterator itr = recv_listeners_.begin();
       itr != recv_listeners_.end(); ++itr) {

    TransportReceiveListener_rch trl = itr->second.lock();

    if (trl) {
      if (Transport_debug_level > 0) {
        GuidConverter converter(itr->first);
        ACE_DEBUG((LM_DEBUG,
                   ACE_TEXT("(%P|%t) DataLink::notify: ")
                   ACE_TEXT("notify sub %C %C.\n"),
                   OPENDDS_STRING(converter).c_str(),
                   connection_notice_as_str(notice)));
      }
      AssocByLocal::iterator local_it = assoc_by_local_.find(itr->first);
      if (local_it == assoc_by_local_.end()) {
        if (Transport_debug_level) {
          GuidConverter converter(itr->first);
          ACE_DEBUG((LM_DEBUG,
                     ACE_TEXT("(%P|%t) DataLink::notify: ")
                     ACE_TEXT("try to notify sub %C %C - no associations to notify.\n"),
                     OPENDDS_STRING(converter).c_str(),
                     connection_notice_as_str(notice)));
        }
        break;
      }
      const RepoIdSet& rids = local_it->second.associated_;

      WriterIdSeq pubids;
      set_to_seq(rids, pubids);

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
                   OPENDDS_STRING(converter).c_str()));
      }

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
  return impl_.release_link_resources(this);
}

bool
DataLink::is_target(const RepoId& remote_sub_id)
{
  GuardType guard(this->pub_sub_maps_lock_);
  return assoc_by_remote_.count(remote_sub_id);
}

GUIDSeq*
DataLink::target_intersection(const RepoId& pub_id, const GUIDSeq& in,
                              size_t& n_subs)
{
  GUIDSeq_var res;
  GuardType guard(this->pub_sub_maps_lock_);
  AssocByLocal::const_iterator iter = assoc_by_local_.find(pub_id);

  if (iter != assoc_by_local_.end()) {
    n_subs = iter->second.associated_.size();
    const CORBA::ULong len = in.length();

    for (CORBA::ULong i(0); i < len; ++i) {
      if (iter->second.associated_.count(in[i])) {
        if (res.ptr() == 0) {
          res = new GUIDSeq;
        }

        push_back(res.inout(), in[i]);
      }
    }
  }

  return res._retn();
}

void DataLink::prepare_release()
{
  GuardType guard(this->pub_sub_maps_lock_);

  if (!assoc_releasing_.empty()) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) DataLink::prepare_release: ")
               ACE_TEXT("already prepared for release.\n")));
    return;
  }

  assoc_releasing_ = assoc_by_local_;
}

void DataLink::clear_associations()
{
  for (AssocByLocal::iterator iter = assoc_releasing_.begin();
       iter != assoc_releasing_.end(); ++iter) {
    TransportSendListener_rch tsl = send_listener_for(iter->first);
    if (tsl) {
      ReaderIdSeq sub_ids;
      set_to_seq(iter->second.associated_, sub_ids);
      tsl->remove_associations(sub_ids, false);
      continue;
    }
    TransportReceiveListener_rch trl = recv_listener_for(iter->first);
    if (trl) {
      WriterIdSeq pub_ids;
      set_to_seq(iter->second.associated_, pub_ids);
      trl->remove_associations(pub_ids, false);
    }
  }
  assoc_releasing_.clear();
}

int
DataLink::handle_timeout(const ACE_Time_Value& /*tv*/, const void* /*arg*/)
{
  if (!scheduled_to_stop_at_.is_zero()) {
    VDBG_LVL((LM_DEBUG, "(%P|%t) DataLink::handle_timeout called\n"), 4);
    impl_.unbind_link(this);

    if (assoc_by_remote_.empty() && assoc_by_local_.empty()) {
      this->stop();
    }
  }
  return 0;
}

int
DataLink::handle_close(ACE_HANDLE h, ACE_Reactor_Mask m)
{
  if (h == ACE_INVALID_HANDLE && m == TIMER_MASK) {
    // Reactor is shutting down with this timer still pending.
    // Take the same cleanup actions as if the timeout had expired.
    handle_timeout(ACE_Time_Value::zero, 0);
  }

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

  // Shift the code point up to bits, so that we only use the DS field
  int tos = cp << 2;

  const char* which = "IPV4 TOS";
#if defined (ACE_HAS_IPV6)
  ACE_INET_Addr local_address;

  if (socket.get_local_addr(local_address) == -1) {
    return;

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
               &tos,
               sizeof(tos));

  } else // This is a bit tricky and might be hard to follow...

#endif /* IPV6_TCLASS */
#endif /* ACE_HAS_IPV6 */

#ifdef IP_TOS
  result = socket.set_option(
             IPPROTO_IP,
             IP_TOS,
             &tos,
             sizeof(tos));

  if ((result == -1) && (errno != ENOTSUP)
#ifdef WSAEINVAL
      && (errno != WSAEINVAL)
#endif
     ) {
#endif // IP_TOS
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) DataLink::set_dscp_codepoint() - ")
               ACE_TEXT("failed to set the %C codepoint to %d: %m, ")
               ACE_TEXT("try running as superuser.\n"),
               which,
               cp));
#ifdef IP_TOS
  } else if (DCPS_debug_level > 4) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) DataLink::set_dscp_codepoint() - ")
               ACE_TEXT("set %C codepoint to %d.\n"),
               which,
               cp));
  }
#endif
}

bool
DataLink::handle_send_request_ack(TransportQueueElement* element)
{
  element->data_delivered();
  return true;
}

bool
DataLink::Interceptor::reactor_is_shut_down() const {
  return false;
}

void
DataLink::ImmediateStart::execute() {
  TransportClient_rch client_lock = client_.lock();
  if (client_lock) {
    client_lock->use_datalink(remote_, link_);
  }
}


void
DataLink::network_change() const
{
  for (IdToSendListenerMap::const_iterator itr = send_listeners_.begin();
       itr != send_listeners_.end(); ++itr) {
    TransportSendListener_rch tsl = itr->second.lock();
    if (tsl) {
      tsl->transport_discovery_change();
    }
  }

  for (IdToRecvListenerMap::const_iterator itr = recv_listeners_.begin();
       itr != recv_listeners_.end(); ++itr) {
    TransportReceiveListener_rch trl = itr->second.lock();
    if (trl) {
      trl->transport_discovery_change();
    }
  }
}

#ifndef OPENDDS_SAFETY_PROFILE
std::ostream&
operator<<(std::ostream& str, const DataLink& value)
{
  str << "   There are " << value.assoc_by_local_.size()
      << " local entities currently using this link comprising following associations:"
      << std::endl;

  typedef DataLink::AssocByLocal::const_iterator assoc_iter_t;
  const DataLink::AssocByLocal& abl = value.assoc_by_local_;
  for (assoc_iter_t ait = abl.begin(); ait != abl.end(); ++ait) {
    const RepoIdSet& set = ait->second.associated_;
    for (RepoIdSet::const_iterator rit = set.begin(); rit != set.end(); ++rit) {
      str << GuidConverter(ait->first) << " --> "
          << GuidConverter(*rit) << "   " << std::endl;
    }
  }
  return str;
}
#endif

void
DataLink::terminate_send_if_suspended()
{
  if (send_strategy_)
    send_strategy_->terminate_send_if_suspended();
}

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
