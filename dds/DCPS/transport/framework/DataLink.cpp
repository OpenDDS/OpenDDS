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
DataLink::DataLink(const TransportImpl_rch& impl, Priority priority, bool is_loopback,
                   bool is_active)
  : stopped_(false),
    scheduled_to_stop_at_(ACE_Time_Value::zero),
    default_listener_(),
    impl_(impl),
    thr_per_con_send_task_(0),
    transport_priority_(priority),
    scheduling_release_(false),
    send_control_allocator_(0),
    mb_allocator_(0),
    db_allocator_(0),
    is_loopback_(is_loopback),
    is_active_(is_active),
    started_(false),
    send_response_listener_("DataLink")
{
  DBG_ENTRY_LVL("DataLink", "DataLink", 6);

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

  if (!assoc_by_local_.empty()) {
    ACE_DEBUG((LM_WARNING,
               ACE_TEXT("(%P|%t) WARNING: DataLink[%@]::~DataLink() - ")
               ACE_TEXT("link still in use by %d entities when deleted!\n"),
               this, assoc_by_local_.size()));
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


bool
DataLink::add_on_start_callback(const TransportClient_rch& client, const RepoId& remote)
{
  GuardType guard(strategy_lock_);

  if (started_ && !send_strategy_.is_nil()) {
    return false; // link already started
  }
  on_start_callbacks_.push_back(std::make_pair(client, remote));
  return true;
}

void
DataLink::remove_on_start_callback(const TransportClient_rch& client, const RepoId& remote)
{
  GuardType guard(strategy_lock_);
  on_start_callbacks_.erase(
    std::remove(on_start_callbacks_.begin(),
                on_start_callbacks_.end(),
                std::make_pair(client, remote)),
    on_start_callbacks_.end());
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

    OnStartCallback last_callback = on_start_callbacks_.back();
    on_start_callbacks_.pop_back();

    guard.release();
    last_callback.first->use_datalink(last_callback.second, link);
  }
}

//Reactor invokes this after being notified in schedule_stop or cancel_release
int
DataLink::handle_exception(ACE_HANDLE /* fd */)
{
  TransportImpl_rch impl = this->impl_;
  if(this->scheduled_to_stop_at_ == ACE_Time_Value::zero) {
    if (DCPS_debug_level > 0) {
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) DataLink::handle_exception() - not scheduling or stopping\n")));
    }
    if (impl) {
      ACE_Reactor_Timer_Interface* reactor = impl->timer();
      if (reactor->cancel_timer(this) > 0) {
        if (DCPS_debug_level > 0) {
          ACE_DEBUG((LM_DEBUG,
                     ACE_TEXT("(%P|%t) DataLink::handle_exception() - cancelled future release timer\n")));
        }
      }
    } else {
      if (DCPS_debug_level > 0) {
        ACE_DEBUG((LM_DEBUG,
                   ACE_TEXT("(%P|%t) DataLink::handle_exception() - impl_ == 0\n")));
      }
    }
    return 0;
  } else if (this->scheduled_to_stop_at_ <= ACE_OS::gettimeofday()) {
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
    if (impl) {
      ACE_Reactor_Timer_Interface* reactor = impl->timer();
      ACE_Time_Value future_release_time = this->scheduled_to_stop_at_ - ACE_OS::gettimeofday();
      reactor->schedule_timer(this, 0, future_release_time);
    }
  }
  return 0;
}

//Allows DataLink::stop to be done on the reactor thread so that
//this thread avoids possibly deadlocking trying to access reactor
//to stop strategies or schedule timers
void
DataLink::schedule_stop(const ACE_Time_Value& schedule_to_stop_at)
{
  if (!this->stopped_ && this->scheduled_to_stop_at_ == ACE_Time_Value::zero) {
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
  TransportReactorTask_rch reactor(impl_->reactor_task());
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
  this->scheduled_to_stop_at_ = ACE_Time_Value::zero;
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
                           const TransportSendListener_rch& send_listener)
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

    assoc_by_local_[local_publication_id].insert(remote_subscription_id);
    ReceiveListenerSet_rch& rls = assoc_by_remote_[remote_subscription_id];

    if (rls.is_nil())
      rls = make_rch<ReceiveListenerSet>();
    rls->insert(local_publication_id, TransportReceiveListener_rch());

    send_listeners_.insert(std::make_pair(local_publication_id,send_listener));
  }
  return 0;
}

int
DataLink::make_reservation(const RepoId& remote_publication_id,
                           const RepoId& local_subscription_id,
                           const TransportReceiveListener_rch& receive_listener)
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

    assoc_by_local_[local_subscription_id].insert(remote_publication_id);
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
  set_to_seq(iter->second, static_cast<GUIDSeq&>(result));
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

  //let the specific class release its reservations
  //done this way to prevent deadlock of holding pub_sub_maps_lock_
  //then obtaining a specific class lock in release_reservations_i
  //which reverses lock ordering of the active send logic of needing
  //the specific class lock before obtaining the over arching DataLink
  //pub_sub_maps_lock_
  this->release_reservations_i(remote_id, local_id);

  GuardType guard(this->pub_sub_maps_lock_);

  ReceiveListenerSet_rch& rls = assoc_by_remote_[remote_id];
  if (rls->size() == 1) {
    assoc_by_remote_.erase(remote_id);
    release_remote_i(remote_id);
  } else {
    rls->remove(local_id);
  }
  RepoIdSet& ris = assoc_by_local_[local_id];
  if (ris.size() == 1) {
    DataLinkSet_rch& links = released_locals[local_id];
    if (links.is_nil())
      links = make_rch<DataLinkSet>();
    links->insert_link(rchandle_from(this));
    {
      GuardType guard(this->released_assoc_by_local_lock_);
      released_assoc_by_local_[local_id].insert(remote_id);
    }
    assoc_by_local_.erase(local_id);
  } else {
    ris.erase(remote_id);
  }

  if (assoc_by_local_.empty()) {
    VDBG_LVL((LM_DEBUG,
              ACE_TEXT("(%P|%t) DataLink::release_reservations: ")
              ACE_TEXT("release_datalink due to no remaining pubs or subs.\n")), 5);
    this->impl_->release_datalink(this);
  }
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
    this->send_strategy_->clear();
  }

  ACE_Time_Value future_release_time = ACE_OS::gettimeofday() + this->datalink_release_delay_;
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
    this->scheduled_to_stop_at_ = ACE_Time_Value::zero;
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
                          this->mb_allocator_->malloc(sizeof(ACE_Message_Block))),
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

  TransportSendControlElement* const elem =
    TransportSendControlElement::alloc(1, // initial_count
                                       GUID_UNKNOWN, &send_response_listener_,
                                       header, message, send_control_allocator_);
  if (!elem) return SEND_CONTROL_ERROR;

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
               ACE_TEXT("from publication %C received sample: %C to readerId %C (%s).\n"),
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
  {
    GuardType guard(this->pub_sub_maps_lock_);
    AssocByRemote::iterator iter = assoc_by_remote_.find(publication_id);
    if (iter != assoc_by_remote_.end())
      listener_set = iter->second;

    if (listener_set.is_nil() && this->default_listener_) {
      this->default_listener_->data_received(sample);
      return;
    }
  }

  if (listener_set.is_nil()) {
    // Nobody has any interest in this message.  Drop it on the floor.
    if (Transport_debug_level > 4) {
      const GuidConverter converter(publication_id);
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) DataLink::data_received_i: ")
                 ACE_TEXT(" discarding sample from publication %C due to no listeners.\n"),
                 OPENDDS_STRING(converter).c_str()));
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
      ACE_DEBUG((LM_DEBUG, "(%P|%t) DataLink::data_received_i - normal data received to each subscription in listener_set %s ids:%C\n",
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

  if (!this->send_strategy_.is_nil()) {
    this->send_strategy_->transport_shutdown();
  }

  //this->cancel_release();
  this->set_scheduling_release(false);
  this->scheduled_to_stop_at_ = ACE_Time_Value::zero;
  ACE_Reactor_Timer_Interface* reactor = this->impl_->timer();
  reactor->cancel_timer(this);
  this->stop();

  // Drop our reference to the TransportImpl object
  this->impl_.reset();
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

    const TransportSendListener_rch& tsl = itr->second;

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
      const RepoIdSet& rids = local_it->second;

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

    const TransportReceiveListener_rch& trl = itr->second;

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
      const RepoIdSet& rids = local_it->second;

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

void DataLink::notify_connection_deleted()
{
  // Locking:
  // - if released_assoc_by_local_lock_ and pub_sub_maps_lock_ are both held at
  //   the same time, pub_sub_maps_lock_ must be locked first
  // - pub_sub_maps_lock_ must not be held during the callbacks to
  //   notify_connection_deleted() which may re-enter the DataLink
  AssocByLocal released;
  {
    GuardType guard(released_assoc_by_local_lock_);
    released = released_assoc_by_local_;
  }

  RepoIdSet found;
  for (AssocByLocal::iterator iter = released.begin();
       iter != released.end(); ++iter) {

    TransportSendListener_rch tsl;
    {
      GuardType guard(pub_sub_maps_lock_);
      tsl = send_listener_for(iter->first);
    }

    if (tsl) {
      for (RepoIdSet::iterator ris_it = iter->second.begin();
           ris_it != iter->second.end(); ++ris_it) {
        tsl->notify_connection_deleted(*ris_it);
      }
      found.insert(iter->first);
      continue;
    }

    TransportReceiveListener_rch trl;
    {
      GuardType guard(pub_sub_maps_lock_);
      trl = recv_listener_for(iter->first);
    }

    if (trl) {
      for (RepoIdSet::iterator ris_it = iter->second.begin();
           ris_it != iter->second.end(); ++ris_it) {
        trl->notify_connection_deleted(*ris_it);
      }
      found.insert(iter->first);
    }
  }

  GuardType guard(released_assoc_by_local_lock_);
  for (RepoIdSet::const_iterator it = found.begin(); it != found.end(); ++it) {
    const AssocByLocal::iterator iter = released_assoc_by_local_.find(*it);
    if (iter != released_assoc_by_local_.end()) {
      iter->second.clear();
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
    n_subs = iter->second.size();
    const CORBA::ULong len = in.length();

    for (CORBA::ULong i(0); i < len; ++i) {
      if (iter->second.count(in[i])) {
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
      set_to_seq(iter->second, sub_ids);
      tsl->remove_associations(sub_ids, false);
      continue;
    }
    TransportReceiveListener_rch trl = recv_listener_for(iter->first);
    if (trl) {
      WriterIdSeq pub_ids;
      set_to_seq(iter->second, pub_ids);
      trl->remove_associations(pub_ids, false);
    }
  }
  assoc_releasing_.clear();
}

int
DataLink::handle_timeout(const ACE_Time_Value& /*tv*/, const void* /*arg*/)
{
  if (this->scheduled_to_stop_at_ != ACE_Time_Value::zero) {
    VDBG_LVL((LM_DEBUG, "(%P|%t) DataLink::handle_timeout called\n"), 4);
    this->impl_->unbind_link(this);

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

#ifndef OPENDDS_SAFETY_PROFILE
std::ostream&
operator<<(std::ostream& str, const DataLink& value)
{
  str << "   There are " << value.assoc_by_local_.size()
      << " local entities currently using this link comprising following associations:"
      << std::endl;

  for (DataLink::AssocByLocal::const_iterator
       localId = value.assoc_by_local_.begin();
       localId != value.assoc_by_local_.end();
       ++localId) {
    for (RepoIdSet::const_iterator
         remoteId = localId->second.begin();
         remoteId != localId->second.end();
         ++remoteId) {
      str << GuidConverter(localId->first) << " --> "
          << GuidConverter(*remoteId) << "   " << std::endl;
    }
  }
  return str;
}
#endif
}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
