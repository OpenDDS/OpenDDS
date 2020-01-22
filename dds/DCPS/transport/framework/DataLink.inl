/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "TransportSendStrategy.h"
#include "TransportStrategy.h"
#include "ThreadPerConnectionSendTask.h"
#include "EntryExit.h"
#include "dds/DCPS/GuidConverter.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

ACE_INLINE
Priority&
DataLink::transport_priority()
{
  return this->transport_priority_;
}

ACE_INLINE
Priority
DataLink::transport_priority() const
{
  return this->transport_priority_;
}


ACE_INLINE
bool& DataLink::is_loopback()
{
  return this->is_loopback_;
}


ACE_INLINE
bool  DataLink::is_loopback() const
{
  return this->is_loopback_;
}


ACE_INLINE
bool& DataLink::is_active()
{
  return this->is_active_;
}


ACE_INLINE
bool  DataLink::is_active() const
{
  return this->is_active_;
}

ACE_INLINE const TimeDuration&
DataLink::datalink_release_delay() const
{
  return this->datalink_release_delay_;
}

ACE_INLINE void
DataLink::send_start()
{
  DBG_ENTRY_LVL("DataLink","send_start",6);

  if (this->thr_per_con_send_task_ != 0) {
    this->thr_per_con_send_task_->add_request(SEND_START);

  } else
    this->send_start_i();
}

ACE_INLINE void
DataLink::send_start_i()
{
  DBG_ENTRY_LVL("DataLink","send_start_i",6);
  // This one is easy.  Simply delegate to our TransportSendStrategy
  // data member.

  TransportSendStrategy_rch strategy;
  {
    GuardType guard(this->strategy_lock_);

    strategy = this->send_strategy_;
  }

  if (!strategy.is_nil()) {
    strategy->send_start();
  }
}

ACE_INLINE void
DataLink::send(TransportQueueElement* element)
{
  DBG_ENTRY_LVL("DataLink","send",6);

  if (element->is_request_ack() &&
      this->handle_send_request_ack(element)) {
    return;
  }

  element = this->customize_queue_element(element);
  if (!element) {
    return;
  }

  if (this->thr_per_con_send_task_ != 0) {
    if (this->thr_per_con_send_task_->add_request(SEND, element) == -1) {
      element->data_dropped(true);
    }

  } else {
    this->send_i(element);

  }
}

ACE_INLINE void
DataLink::send_i(TransportQueueElement* element, bool relink)
{
  DBG_ENTRY_LVL("DataLink","send_i",6);
  // This one is easy.  Simply delegate to our TransportSendStrategy
  // data member.

  TransportSendStrategy_rch strategy;
  {
    GuardType guard(this->strategy_lock_);

    strategy = this->send_strategy_;
  }

  if (strategy) {
    strategy->send(element, relink);
  } else {
    element->data_dropped(true);
  }
}

ACE_INLINE void
DataLink::send_stop(RepoId repoId)
{
  DBG_ENTRY_LVL("DataLink","send_stop",6);

  if (this->thr_per_con_send_task_ != 0) {
    this->thr_per_con_send_task_->add_request(SEND_STOP);

  } else
    this->send_stop_i(repoId);
}

ACE_INLINE void
DataLink::send_stop_i(RepoId repoId)
{
  DBG_ENTRY_LVL("DataLink","send_stop_i",6);
  // This one is easy.  Simply delegate to our TransportSendStrategy
  // data member.

  TransportSendStrategy_rch strategy;
  {
    GuardType guard(this->strategy_lock_);

    strategy = this->send_strategy_;
  }

  if (!strategy.is_nil()) {
    strategy->send_stop(repoId);
  }
}

ACE_INLINE
void  DataLink::set_scheduling_release(bool scheduling_release)
{
  this->scheduling_release_ = scheduling_release;
}

ACE_INLINE RemoveResult
DataLink::remove_sample(const DataSampleElement* sample)
{
  DBG_ENTRY_LVL("DataLink", "remove_sample", 6);

  if (this->thr_per_con_send_task_ != 0) {
    const RemoveResult rr = this->thr_per_con_send_task_->remove_sample(sample);
    if (rr == REMOVE_RELEASED || rr == REMOVE_FOUND) {
      VDBG((LM_DEBUG, "(%P|%t) DBG:   "
            "Removed sample from ThreadPerConnection queue.\n"));
      return rr;
    }
  }

  TransportSendStrategy_rch strategy;
  {
    GuardType guard(this->strategy_lock_);

    strategy = this->send_strategy_;
  }

  if (!strategy.is_nil()) {
    return strategy->remove_sample(sample);
  }

  return REMOVE_NOT_FOUND;
}

ACE_INLINE void
DataLink::remove_all_msgs(const RepoId& pub_id)
{
  DBG_ENTRY_LVL("DataLink","remove_all_msgs",6);

  // This one is easy.  Simply delegate to our TransportSendStrategy
  // data member.

  TransportSendStrategy_rch strategy;
  {
    GuardType guard(this->strategy_lock_);

    strategy = this->send_strategy_;
  }

  if (!strategy.is_nil()) {
    strategy->remove_all_msgs(pub_id);
  }
}

ACE_INLINE DataLinkIdType
DataLink::id() const
{
  DBG_ENTRY_LVL("DataLink","id",6);
  return id_;
}

ACE_INLINE int
DataLink::start(const TransportSendStrategy_rch& send_strategy,
                const TransportStrategy_rch& receive_strategy, bool invoke_all)
{
  DBG_ENTRY_LVL("DataLink","start",6);

  // We assume that the send_strategy is not NULL, but the receive_strategy
  // is allowed to be NULL.

  // Attempt to start the strategies, and if there is a start() failure,
  // make sure to stop() any strategy that was already start()'ed.
  if (send_strategy->start() != 0) {
    // Failed to start the TransportSendStrategy.
    invoke_on_start_callbacks(false);
    return -1;
  }

  if ((!receive_strategy.is_nil()) && (receive_strategy->start() != 0)) {
    // Failed to start the TransportReceiveStrategy.

    // Remember to stop() the TransportSendStrategy since we did start it,
    // and now need to "undo" that action.
    send_strategy->stop();
    invoke_on_start_callbacks(false);
    return -1;
  }

  // We started both strategy objects.  Save them to data members since
  // we will now take ownership of them.
  {
    GuardType guard(this->strategy_lock_);

    this->send_strategy_    = send_strategy;
    this->receive_strategy_ = receive_strategy;
  }
  if (invoke_all) {
    invoke_on_start_callbacks(true);
  }
  {
    //catch any associations added during initial invoke_on_start_callbacks
    //only after first use_datalink has resolved does datalink's state truly
    //change to started, thus can't let pending associations proceed normally yet
    GuardType guard(this->strategy_lock_);
    this->started_ = true;
  }
  //Now state transitioned to started so no new on_start_callbacks will be added
  //so resolve any added during transition to started.
  if (invoke_all) {
    invoke_on_start_callbacks(true);
  }
  return 0;
}


ACE_INLINE
const char*
DataLink::connection_notice_as_str(ConnectionNotice notice)
{
  static const char* NoticeStr[] = { "DISCONNECTED",
                                     "RECONNECTED",
                                     "LOST"
                                   };

  return NoticeStr [notice];
}

ACE_INLINE
void
DataLink::terminate_send()
{
  this->send_strategy_->terminate_send(false);
}

ACE_INLINE
void
DataLink::remove_listener(const RepoId& local_id)
{
  GuardType guard(pub_sub_maps_lock_);
  {
    IdToSendListenerMap::iterator pos = send_listeners_.find(local_id);
    if (pos != send_listeners_.end()) {
      send_listeners_.erase(pos);
      if (Transport_debug_level > 5) {
        GuidConverter converter(local_id);
        ACE_DEBUG((LM_DEBUG,
                   ACE_TEXT("(%P|%t) DataLink::remove_listener: ")
                   ACE_TEXT("removed %C from send_listeners\n"),
                   OPENDDS_STRING(converter).c_str()));
      }
      return;
    }
  }
  {
    IdToRecvListenerMap::iterator pos = recv_listeners_.find(local_id);
    if (pos != recv_listeners_.end()) {
      recv_listeners_.erase(pos);
      if (Transport_debug_level > 5) {
        GuidConverter converter(local_id);
        ACE_DEBUG((LM_DEBUG,
                   ACE_TEXT("(%P|%t) DataLink::remove_listener: ")
                   ACE_TEXT("removed %C from recv_listeners\n"),
                   OPENDDS_STRING(converter).c_str()));
      }
      return;
    }
  }
}

ACE_INLINE
TransportSendListener_rch
DataLink::send_listener_for(const RepoId& pub_id) const
{
  // pub_map_ (and send_listeners_) are already locked when entering this
  // private method.
  IdToSendListenerMap::const_iterator found =
    this->send_listeners_.find(pub_id);
  if (found == this->send_listeners_.end()) {
    return TransportSendListener_rch();
  }
  return found->second.lock();
}

ACE_INLINE
TransportReceiveListener_rch
DataLink::recv_listener_for(const RepoId& sub_id) const
{
  // sub_map_ (and recv_listeners_) are already locked when entering this
  // private method.
  IdToRecvListenerMap::const_iterator found =
    this->recv_listeners_.find(sub_id);
  if (found == this->recv_listeners_.end()) {
    return TransportReceiveListener_rch();
  }
  return found->second.lock();
}

ACE_INLINE
void
DataLink::default_listener(const TransportReceiveListener_wrch& trl)
{
  GuardType guard(this->pub_sub_maps_lock_);
  this->default_listener_ = trl;
}

ACE_INLINE
TransportReceiveListener_wrch
DataLink::default_listener() const
{
  GuardType guard(this->pub_sub_maps_lock_);
  return this->default_listener_;
}

ACE_INLINE
void
DataLink::send_final_acks (const RepoId& /*readerid*/)
{
}

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
