/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <DCPS/DdsDcps_pch.h> //Only the _pch include should start with DCPS/

#include "TransportClient.h"
#include "TransportConfig.h"
#include "TransportRegistry.h"
#include "TransportExceptions.h"
#include "TransportReceiveListener.h"

#include <dds/DCPS/DataWriterImpl.h>
#include <dds/DCPS/Definitions.h>
#include <dds/DCPS/GuidConverter.h>
#include <dds/DCPS/SendStateDataSampleList.h>
#include <dds/DCPS/Timers.h>

#include <dds/DCPS/RTPS/ICE/Ice.h>

#include <dds/DdsDcpsInfoUtilsC.h>

#include <ace/Reactor_Timer_Interface.h>

#include <algorithm>
#include <iterator>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

TransportClient::TransportClient()
  : pending_assoc_timer_(make_rch<PendingAssocTimer> (TheServiceParticipant->reactor(), TheServiceParticipant->reactor_owner()))
  , expected_transaction_id_(1)
  , max_transaction_id_seen_(0)
  , max_transaction_tail_(0)
  , swap_bytes_(false)
  , cdr_encapsulation_(false)
  , reliable_(false)
  , durable_(false)
  , reverse_lock_(lock_)
  , guid_(GUID_UNKNOWN)
{
}

TransportClient::~TransportClient()
{
  if (Transport_debug_level > 5) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) TransportClient::~TransportClient: %C\n"),
               LogGuid(guid_).c_str()));
  }

  stop_associating();

  ACE_GUARD(ACE_Thread_Mutex, guard, lock_);

  for (PrevPendingMap::iterator it = prev_pending_.begin(); it != prev_pending_.end(); ++it) {
    for (size_t i = 0; i < impls_.size(); ++i) {
      TransportImpl_rch impl = impls_[i].lock();
      if (impl) {
        impl->stop_accepting_or_connecting(it->second->client_, it->second->data_.remote_id_, false, false);
      }
    }
  }
}

void
TransportClient::clean_prev_pending()
{
  for (PrevPendingMap::iterator it = prev_pending_.begin(); it != prev_pending_.end();) {
    if (it->second->safe_to_remove()) {
      prev_pending_.erase(it++);
    } else {
      ++it;
    }
  }
}

void
TransportClient::enable_transport(bool reliable, bool durable, DomainParticipantImpl* dpi)
{
  // Search for a TransportConfig to use:
  TransportConfig_rch tc;

  // 1. If this object is an Entity, check if a TransportConfig has been
  //    bound either directly to this entity or to a parent entity.
  for (RcHandle<EntityImpl> ent = rchandle_from(dynamic_cast<EntityImpl*>(this));
       ent && tc.is_nil(); ent = ent->parent()) {
    tc = ent->transport_config();
  }

  if (tc.is_nil()) {
    TransportRegistry* const reg = TransportRegistry::instance();
    // 2. Check for a TransportConfig that is the default for this Domain.
    tc = reg->domain_default_config(domain_id());

    if (tc.is_nil()) {
      // 3. Use the global_config if one has been set.
      tc = reg->global_config();

      if (!tc.is_nil() && tc->instances_.empty()
          && tc->name() == TransportRegistry::DEFAULT_CONFIG_NAME) {
        // 4. Set the "fallback option" if the global_config is empty.
        //    (only applies if the user hasn't changed the global config)
        tc = reg->fix_empty_default();
      }
    }
  }

  if (tc.is_nil()) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: TransportClient::enable_transport ")
               ACE_TEXT("No TransportConfig found.\n")));
    throw Transport::NotConfigured();
  }

  enable_transport_using_config(reliable, durable, tc, dpi);
}

void
TransportClient::enable_transport_using_config(bool reliable, bool durable,
                                               const TransportConfig_rch& tc,
                                               DomainParticipantImpl* dpi)
{
  config_ = tc;
  swap_bytes_ = tc->swap_bytes_;
  reliable_ = reliable;
  durable_ = durable;
  passive_connect_duration_ = tc->passive_connect_duration_;
  if (passive_connect_duration_ == 0) {
    passive_connect_duration_ = TimeDuration::from_msec(TransportConfig::DEFAULT_PASSIVE_CONNECT_DURATION);
    if (DCPS_debug_level) {
      ACE_DEBUG((LM_WARNING,
        ACE_TEXT("(%P|%t) TransportClient::enable_transport_using_config ")
        ACE_TEXT("passive_connect_duration_ configured as 0, changing to ")
        ACE_TEXT("default value\n")));
    }
  }

  populate_connection_info(dpi);

  const size_t n = tc->instances_.size();

  for (size_t i = 0; i < n; ++i) {
    TransportInst_rch inst = tc->instances_[i];

    if (check_transport_qos(*inst)) {
      TransportImpl_rch impl = inst->get_or_create_impl(domain_id(), dpi);

      if (impl) {
        impls_.push_back(impl);

#if OPENDDS_CONFIG_SECURITY
        impl->local_crypto_handle(get_crypto_handle());
#endif

        cdr_encapsulation_ |= inst->requires_cdr_encapsulation();
      }
    }
  }

  if (impls_.empty()) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: TransportClient::enable_transport ")
               ACE_TEXT("No TransportImpl could be created.\n")));
    throw Transport::NotConfigured();
  }
}

void
TransportClient::populate_connection_info(DomainParticipantImpl* dpi)
{
  conn_info_.length(0);

  const size_t n = config_->instances_.size();
  for (size_t i = 0; i < n; ++i) {
    TransportInst_rch inst = config_->instances_[i];
    if (check_transport_qos(*inst)) {
      TransportImpl_rch impl = inst->get_or_create_impl(domain_id(), dpi);
      if (impl) {
        const CORBA::ULong idx = DCPS::grow(conn_info_) - 1;
        impl->connection_info(conn_info_[idx], CONNINFO_ALL);
      }
    }
  }

  if (conn_info_.length() == 0) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) TransportClient::populate_connection_info: ")
               ACE_TEXT("No connection info\n")));
  }
}

bool
TransportClient::associate(const AssociationData& data, bool active)
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, lock_, false);

  OPENDDS_ASSERT(guid_ != GUID_UNKNOWN);

  if (impls_.empty()) {
    if (DCPS_debug_level) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) TransportClient::associate - ")
                 ACE_TEXT("local %C remote %C no available impls\n"),
                 LogGuid(guid_).c_str(),
                 LogGuid(data.remote_id_).c_str()));
    }
    return false;
  }

  bool all_impls_shut_down = true;
  for (size_t i = 0; i < impls_.size(); ++i) {
    TransportImpl_rch impl = impls_[i].lock();
    if (impl && !impl->is_shut_down()) {
      all_impls_shut_down = false;
      break;
    }
  }

  if (all_impls_shut_down) {
    if (DCPS_debug_level) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) TransportClient::associate - ")
                 ACE_TEXT("local %C remote %C all available impls previously shutdown\n"),
                 LogGuid(guid_).c_str(),
                 LogGuid(data.remote_id_).c_str()));
    }
    return false;
  }

  clean_prev_pending();

  PendingMap::iterator iter = pending_.find(data.remote_id_);

  if (iter == pending_.end()) {
    GUID_t remote_copy(data.remote_id_);
    PendingAssoc_rch pa = make_rch<PendingAssoc>(rchandle_from(this));
    pa->active_ = active;
    pa->impls_.clear();
    pa->blob_index_ = 0;
    pa->data_ = data;
    pa->attribs_.local_id_ = guid_;
    pa->attribs_.priority_ = get_priority_value(data);
    pa->attribs_.local_reliable_ = reliable_;
    pa->attribs_.local_durable_ = durable_;
    pa->attribs_.max_sn_ = get_max_sn();
    iter = pending_.insert(std::make_pair(remote_copy, pa)).first;

    VDBG_LVL((LM_DEBUG, "(%P|%t) TransportClient::associate added PendingAssoc "
              "between %C and remote %C\n",
              LogGuid(guid_).c_str(),
              LogGuid(data.remote_id_).c_str()), 0);
  } else {

    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: TransportClient::associate ")
               ACE_TEXT("already associating with remote.\n")));

    return false;

  }

  PendingAssoc_rch pend = iter->second;

  if (active) {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, pend_guard, pend->mutex_, false);
    pend->impls_.reserve(impls_.size());
    std::reverse_copy(impls_.begin(), impls_.end(),
                      std::back_inserter(pend->impls_));

    return pend->initiate_connect(this, guard);

  } else { // passive

    // call accept_datalink for each impl / blob pair of the same type
    for (size_t i = 0; i < impls_.size(); ++i) {
      // Release the PendingAssoc object's mutex_ since the nested for-loop does not access
      // the PendingAssoc object directly and the functions called by the nested loop can
      // lead to a PendingAssoc object's mutex_ being acquired, which will cause deadlock if
      // it is not released here.
      TransportImpl::ConnectionAttribs attribs;
      TransportImpl_rch impl = impls_[i].lock();
      {
        ACE_GUARD_RETURN(ACE_Thread_Mutex, pend_guard, pend->mutex_, false);
        pend->impls_.push_back(impl);
        attribs = pend->attribs_;
      }
      const OPENDDS_STRING type = impl->transport_type();

      for (CORBA::ULong j = 0; j < data.remote_data_.length(); ++j) {
        if (data.remote_data_[j].transport_type.in() == type) {
          const TransportImpl::RemoteTransport remote = {
            data.remote_id_, data.remote_data_[j].data, data.discovery_locator_.data, data.participant_discovered_at_, data.remote_transport_context_,
            data.publication_transport_priority_,
            data.remote_reliable_, data.remote_durable_};

          TransportImpl::AcceptConnectResult res;
          {
            // This thread acquired lock_ at the beginning of this method.
            // Calling accept_datalink might require getting the lock for the transport's reactor.
            // If the current thread is not an event handler for the transport's reactor, e.g.,
            // the ORB's thread, then the order of acquired locks will be lock_ -> transport reactor lock.
            // Event handlers in the transport reactor may call passive_connection which calls use_datalink
            // which acquires lock_.  The locking order in this case is transport reactor lock -> lock_.
            // To avoid deadlock, we must reverse the lock.
            RcHandle<TransportClient> client = rchandle_from(this);
            ACE_GUARD_RETURN(Reverse_Lock_t, rev_tc_guard, reverse_lock_, false);
            res = impl->accept_datalink(remote, attribs, client);
          }

          //NEED to check that pend is still valid here after you re-acquire the lock_ after accepting the datalink
          iter = pending_.find(data.remote_id_);

          if (iter == pending_.end()) {
            //If Pending Assoc is no longer in pending_ then use_datalink_i has been called from an
            //active side connection and completed, thus pend was removed from pending_.  Can return true.
            return true;
          }
          pend = iter->second;

          if (res.success_) {
            if (res.link_.is_nil()) {
              // In this case, it may be waiting for the TCP connection to be
              // established.  Just wait without trying other transports.
              pending_assoc_timer_->schedule_timer(rchandle_from(this), iter->second);
            } else {
              use_datalink_i(data.remote_id_, res.link_, guard);
              return true;
            }
          }
        }
      }
    }

    pending_assoc_timer_->schedule_timer(rchandle_from(this), iter->second);
  }

  return true;
}

void
TransportClient::PendingAssoc::reset_client()
{
  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  client_.reset();
}

bool
TransportClient::PendingAssoc::safe_to_remove()
{
  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  return !client_ && !scheduled_;
}

void
TransportClient::PendingAssocTimer::ScheduleCommand::execute()
{
  if (timer_->reactor()) {
    const TransportClient_rch client = transport_client_.lock();
    if (client) {
      ACE_Guard<ACE_Thread_Mutex> guard(assoc_->mutex_);
      assoc_->scheduled_ = true;
      const Timers::TimerId id = Timers::schedule(timer_->reactor(), *assoc_, client.in(),
                                                  client->passive_connect_duration_);
      if (id != Timers::InvalidTimerId) {
        timer_->set_id(id);
      }
    }
  }
}

void
TransportClient::PendingAssocTimer::CancelCommand::execute()
{
  if (timer_->reactor() && timer_->get_id() != Timers::InvalidTimerId) {
    ACE_Guard<ACE_Thread_Mutex> guard(assoc_->mutex_);
    Timers::cancel(timer_->reactor(), timer_->get_id());
    timer_->set_id(Timers::InvalidTimerId);
    assoc_->scheduled_ = false;
  }
}

int
TransportClient::PendingAssoc::handle_timeout(const ACE_Time_Value&,
                                              const void* arg)
{
  ThreadStatusManager::Event ev(TheServiceParticipant->get_thread_status_manager());

  RcHandle<TransportClient> client;
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    client = client_.lock();
    scheduled_ = false;
  }

  if (client && client.get() == static_cast<TransportClient*>(const_cast<void*>(arg))) {
    client->use_datalink(data_.remote_id_, DataLink_rch());
  }
  return 0;
}

bool
TransportClient::initiate_connect_i(TransportImpl::AcceptConnectResult& result,
                                    TransportImpl_rch impl,
                                    const TransportImpl::RemoteTransport& remote,
                                    const TransportImpl::ConnectionAttribs& attribs_,
                                    Guard& guard)
{
  if (!guard.locked()) {
    //don't own the lock_ so can't release it...shouldn't happen
    VDBG_LVL((LM_DEBUG,
              ACE_TEXT("(%P|%t) TransportClient::initiate_connect_i ")
              ACE_TEXT("between local %C and remote %C unsuccessful because ")
              ACE_TEXT("guard was not locked\n"),
              LogGuid(guid_).c_str(),
              LogGuid(remote.repo_id_).c_str()), 0);
    return false;
  }

  {
    //can't call connect while holding lock due to possible reactor deadlock
    VDBG_LVL((LM_DEBUG, "(%P|%t) TransportClient::initiate_connect_i - "
              "attempt to connect_datalink between local %C and remote %C\n",
              LogGuid(guid_).c_str(),
              LogGuid(remote.repo_id_).c_str()), 0);
    {
      TransportImpl::ConnectionAttribs attribs = attribs_;
      RcHandle<TransportClient> client = rchandle_from(this);
      ACE_GUARD_RETURN(Reverse_Lock_t, unlock_guard, reverse_lock_, false);
      result = impl->connect_datalink(remote, attribs, client);
    }
    if (!result.success_) {
      if (DCPS_debug_level) {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) TransportClient::initiate_connect_i - ")
                   ACE_TEXT("connect_datalink between local %C remote %C not successful\n"),
                   LogGuid(guid_).c_str(),
                   LogGuid(remote.repo_id_).c_str()));
      }
      return false;
    }
  }

  VDBG_LVL((LM_DEBUG,
            "(%P|%t) TransportClient::initiate_connect_i - "
            "connection between local %C and remote %C initiation successful\n",
            LogGuid(guid_).c_str(),
            LogGuid(remote.repo_id_).c_str()), 0);
  return true;
}

bool
TransportClient::PendingAssoc::initiate_connect(TransportClient* tc,
                                                Guard& guard)
{
  const LogGuid local_log(tc->guid_);
  const LogGuid remote_log(data_.remote_id_);

  VDBG_LVL((LM_DEBUG,
            "(%P|%t) PendingAssoc::initiate_connect - "
            "between %C and remote %C\n",
            local_log.c_str(),
            remote_log.c_str()), 0);
  // find the next impl / blob entry that have matching types
  while (!impls_.empty()) {
    TransportImpl_rch impl = impls_.back().lock();
    if (!impl) {
      impls_.pop_back();
      continue;
    }
    const OPENDDS_STRING type = impl->transport_type();

    for (; blob_index_ < data_.remote_data_.length(); ++blob_index_) {
      if (data_.remote_data_[blob_index_].transport_type.in() == type) {
        const TransportImpl::RemoteTransport remote_transport = {
          data_.remote_id_, data_.remote_data_[blob_index_].data, data_.discovery_locator_.data,
          data_.participant_discovered_at_, data_.remote_transport_context_,
          data_.publication_transport_priority_, data_.remote_reliable_, data_.remote_durable_};

        TransportImpl::AcceptConnectResult res;
        bool ret;
        {
          // Release the PendingAssoc object's mutex_ since initiate_connect_i doesn't need it.
          Reverse_Lock_t rev_mutex(mutex_);
          ACE_GUARD_RETURN(Reverse_Lock_t, rev_pend_guard, rev_mutex, false);
          ret = tc->initiate_connect_i(res, impl, remote_transport, attribs_, guard);
        }
        if (!ret) {
          //tc init connect returned false there is no PendingAssoc left in map because use_datalink_i finished elsewhere
          //so don't do anything further with pend and return success or failure up to tc's associate
          if (res.success_ ) {
            VDBG_LVL((LM_DEBUG, ACE_TEXT("(%P|%t) PendingAssoc::initiate_connect - ")
                                ACE_TEXT("between %C and remote %C success\n"),
                                local_log.c_str(),
                                remote_log.c_str()), 0);
            return true;
          }

          VDBG_LVL((LM_DEBUG, "(%P|%t) PendingAssoc::initiate_connect - "
                              "between %C and remote %C unsuccessful\n",
                              local_log.c_str(),
                              remote_log.c_str()), 0);
        }

        if (res.success_) {

          ++blob_index_;

          if (!res.link_.is_nil()) {

            {
              // use_datalink_i calls PendingAssoc::reset_client which needs the PendingAssoc's mutex_.
              Reverse_Lock_t rev_mutex(mutex_);
              ACE_GUARD_RETURN(Reverse_Lock_t, rev_pend_guard, rev_mutex, false);
              tc->use_datalink_i(data_.remote_id_, res.link_, guard);
            }
          } else {
            VDBG_LVL((LM_DEBUG, "(%P|%t) PendingAssoc::intiate_connect - "
                                "resulting link from initiate_connect_i (local: %C to remote: %C) was nil\n",
                                local_log.c_str(),
                                remote_log.c_str()), 0);
          }

          return true;
        } else {
          VDBG_LVL((LM_DEBUG, "(%P|%t) PendingAssoc::intiate_connect - "
                              "result of initiate_connect_i (local: %C to remote: %C) was not success\n",
                              local_log.c_str(),
                              remote_log.c_str()), 0);
        }
      }
    }

    impls_.pop_back();
    blob_index_ = 0;
  }

  return false;
}

void
TransportClient::use_datalink(const GUID_t& remote_id,
                              const DataLink_rch& link)
{
  ACE_GUARD(ACE_Thread_Mutex, guard, lock_);

  use_datalink_i(remote_id, link, guard);
}

void
TransportClient::use_datalink_i(const GUID_t& remote_id_ref,
                                const DataLink_rch& link,
                                Guard& guard)
{
  // Try to make a local copy of remote_id to use in calls
  // because the reference could be invalidated if the caller
  // reference location is deleted (i.e. in stop_accepting_or_connecting
  // if use_datalink_i was called from passive_connection)
  // Does changing this from a reference to a local affect anything going forward?
  GUID_t remote_id(remote_id_ref);

  LogGuid peerId_log(remote_id);
  VDBG_LVL((LM_DEBUG, "(%P|%t) TransportClient::use_datalink_i "
            "TransportClient(%@) using datalink[%@] from %C\n",
            this,
            link.in(),
            peerId_log.c_str()), 0);

  PendingMap::iterator iter = pending_.find(remote_id);

  if (iter == pending_.end()) {
    VDBG_LVL((LM_DEBUG, "(%P|%t) TransportClient::use_datalink_i "
                        "TransportClient(%@) using datalink[%@] did not find Pending Association to remote %C\n",
                        this,
                        link.in(),
                        peerId_log.c_str()), 0);
    return;
  }

  PendingAssoc_rch pend = iter->second;
  ACE_GUARD(ACE_Thread_Mutex, pend_guard, pend->mutex_);
  const int active_flag = pend->active_ ? ASSOC_ACTIVE : 0;
  bool ok = false;

  if (link.is_nil()) {

    if (pend->active_ && pend->initiate_connect(this, guard)) {
      VDBG_LVL((LM_DEBUG, "(%P|%t) TransportClient::use_datalink_i "
                          "TransportClient(%@) using datalink[%@] link is nil, since this is active side, initiate_connect to remote %C\n",
                          this,
                          link.in(),
                          peerId_log.c_str()), 0);
      return;
    }

    VDBG_LVL((LM_DEBUG, "(%P|%t) TransportClient::use_datalink_i "
              "TransportClient(%@) using datalink[%@] link is nil, since this is passive side, connection to remote %C timed out\n",
              this,
              link.in(),
              peerId_log.c_str()), 0);
  } else { // link is ready to use
    VDBG_LVL((LM_DEBUG, "(%P|%t) TransportClient::use_datalink_i "
              "TransportClient(%@) about to add_link[%@] to remote: %C\n",
              this,
              link.in(),
              peerId_log.c_str()), 0);

    add_link(link, remote_id);
    ok = true;
  }

  // either link is valid or assoc failed, clean up pending object
  for (size_t i = 0; i < pend->impls_.size(); ++i) {
    TransportImpl_rch impl = pend->impls_[i].lock();
    if (impl) {
      impl->stop_accepting_or_connecting(*this, pend->data_.remote_id_, false, !ok);
    }
  }

  pend_guard.release();
  pend->reset_client();
  pending_assoc_timer_->cancel_timer(pend);
  prev_pending_.insert(std::make_pair(iter->first, iter->second));
  pending_.erase(iter);

  // Release TransportClient's lock as we're done updating its data.
  guard.release();

  transport_assoc_done(active_flag | (ok ? ASSOC_OK : 0), remote_id);
}

void
TransportClient::add_link(const DataLink_rch& link, const GUID_t& peer)
{
  links_.insert_link(link);
  data_link_index_[peer] = link;

  TransportReceiveListener_rch trl = get_receive_listener();

  OPENDDS_ASSERT(guid_ != GUID_UNKNOWN);
  if (trl) {
    link->make_reservation(peer, guid_, trl, reliable_);
  } else {
    link->make_reservation(peer, guid_, get_send_listener(), reliable_);
  }
}

void
TransportClient::stop_associating()
{
  ACE_GUARD(ACE_Thread_Mutex, guard, lock_);
  for (PendingMap::iterator it = pending_.begin(); it != pending_.end(); ++it) {
    {
      // The transport impl may have resource for a pending connection.
      ACE_Guard<ACE_Thread_Mutex> guard(it->second->mutex_);
      for (size_t i = 0; i < it->second->impls_.size(); ++i) {
        TransportImpl_rch impl = it->second->impls_[i].lock();
        if (impl) {
          impl->stop_accepting_or_connecting(*this, it->second->data_.remote_id_, true, true);
        }
      }
    }
    it->second->reset_client();
    pending_assoc_timer_->cancel_timer(it->second);
    prev_pending_.insert(std::make_pair(it->first, it->second));
  }
  pending_.clear();
}

void
TransportClient::stop_associating(const GUID_t* repos, CORBA::ULong length)
{
  ACE_GUARD(ACE_Thread_Mutex, guard, lock_);

  if (repos == 0 || length == 0) {
    return;
  } else {
    for (CORBA::ULong i = 0; i < length; ++i) {
      PendingMap::iterator iter = pending_.find(repos[i]);
      if (iter != pending_.end()) {
        {
          // The transport impl may have resource for a pending connection.
          ACE_Guard<ACE_Thread_Mutex> guard(iter->second->mutex_);
          for (size_t i = 0; i < iter->second->impls_.size(); ++i) {
            TransportImpl_rch impl = iter->second->impls_[i].lock();
            if (impl) {
              impl->stop_accepting_or_connecting(*this, iter->second->data_.remote_id_, true, true);
            }
          }
        }
        iter->second->reset_client();
        pending_assoc_timer_->cancel_timer(iter->second);
        prev_pending_.insert(std::make_pair(iter->first, iter->second));
        pending_.erase(iter);
      }
    }
  }
}

void
TransportClient::send_final_acks()
{
  OPENDDS_ASSERT(guid_ != GUID_UNKNOWN);
  links_.send_final_acks(guid_);
}

void
TransportClient::disassociate(const GUID_t& peerId)
{
  OPENDDS_ASSERT(guid_ != GUID_UNKNOWN);

  LogGuid peerId_log(peerId);
  VDBG_LVL((LM_DEBUG, "(%P|%t) TransportClient::disassociate "
            "TransportClient(%@) disassociating from %C\n",
            this,
            peerId_log.c_str()), 5);

  ACE_GUARD(ACE_Thread_Mutex, guard, lock_);

  PendingMap::iterator iter = pending_.find(peerId);
  if (iter != pending_.end()) {
    {
      // The transport impl may have resource for a pending connection.
      ACE_Guard<ACE_Thread_Mutex> guard(iter->second->mutex_);
      for (size_t i = 0; i < iter->second->impls_.size(); ++i) {
        TransportImpl_rch impl = iter->second->impls_[i].lock();
        if (impl) {
          impl->stop_accepting_or_connecting(*this, iter->second->data_.remote_id_, true, true);
        }
      }
    }
    iter->second->reset_client();
    pending_assoc_timer_->cancel_timer(iter->second);
    prev_pending_.insert(std::make_pair(iter->first, iter->second));
    pending_.erase(iter);
    return;
  }

  const DataLinkIndex::iterator found = data_link_index_.find(peerId);

  if (found == data_link_index_.end()) {
    if (DCPS_debug_level > 4) {
      const LogGuid log(peerId);
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) TransportClient::disassociate: ")
                 ACE_TEXT("no link for remote peer %C\n"),
                 log.c_str()));
    }

    return;
  }

  const DataLink_rch link = found->second;

  //now that an _rch is created for the link, remove the iterator from data_link_index_ while still holding lock
  //otherwise it could be removed in transport_detached()
  data_link_index_.erase(found);
  DataLinkSetMap released;

  if (DCPS_debug_level > 4) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) TransportClient::disassociate: ")
               ACE_TEXT("about to release_reservations for link[%@]\n"),
               link.in()));
  }

  OPENDDS_ASSERT(guid_ != GUID_UNKNOWN);
  link->release_reservations(peerId, guid_, &released);

  if (!released.empty()) {

    if (DCPS_debug_level > 4) {
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) TransportClient::disassociate: ")
                 ACE_TEXT("about to remove_link[%@] from links_\n"),
                 link.in()));
    }
    links_.remove_link(link);

    if (DCPS_debug_level > 4) {
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) TransportClient::disassociate: calling remove_listener %C on link[%@]\n"),
                 LogGuid(guid_).c_str(),
                 link.in()));
    }
    // Datalink is no longer used for any remote peer by this TransportClient
    link->remove_listener(guid_);

  }
}

void TransportClient::transport_stop()
{
  ACE_GUARD(ACE_Thread_Mutex, guard, lock_);
  const ImplsType impls = impls_;
  guard.release();

  OPENDDS_ASSERT(guid_ != GUID_UNKNOWN);

  for (size_t i = 0; i < impls.size(); ++i) {
    const TransportImpl_rch impl = impls[i].lock();
    if (impl) {
      impl->client_stop(guid_);
    }
  }
}

void
TransportClient::register_for_reader(const GUID_t& participant,
                                     const GUID_t& writerid,
                                     const GUID_t& readerid,
                                     const TransportLocatorSeq& locators,
                                     OpenDDS::DCPS::DiscoveryListener* listener)
{
  ACE_GUARD(ACE_Thread_Mutex, guard, lock_);
  for (ImplsType::iterator pos = impls_.begin(), limit = impls_.end();
       pos != limit;
       ++pos) {
    TransportImpl_rch impl = pos->lock();
    if (impl) {
      impl->register_for_reader(participant, writerid, readerid, locators, listener);
    }
  }
}

void
TransportClient::unregister_for_reader(const GUID_t& participant,
                                       const GUID_t& writerid,
                                       const GUID_t& readerid)
{
  ACE_GUARD(ACE_Thread_Mutex, guard, lock_);
  for (ImplsType::iterator pos = impls_.begin(), limit = impls_.end();
       pos != limit;
       ++pos) {
    TransportImpl_rch impl = pos->lock();
    if (impl) {
      impl->unregister_for_reader(participant, writerid, readerid);
    }
  }
}

void
TransportClient::register_for_writer(const GUID_t& participant,
                                     const GUID_t& readerid,
                                     const GUID_t& writerid,
                                     const TransportLocatorSeq& locators,
                                     DiscoveryListener* listener)
{
  ACE_GUARD(ACE_Thread_Mutex, guard, lock_);
  for (ImplsType::iterator pos = impls_.begin(), limit = impls_.end();
       pos != limit;
       ++pos) {
    TransportImpl_rch impl = pos->lock();
    if (impl) {
      impl->register_for_writer(participant, readerid, writerid, locators, listener);
    }
  }
}

void
TransportClient::unregister_for_writer(const GUID_t& participant,
                                       const GUID_t& readerid,
                                       const GUID_t& writerid)
{
  ACE_GUARD(ACE_Thread_Mutex, guard, lock_);
  for (ImplsType::iterator pos = impls_.begin(), limit = impls_.end();
       pos != limit;
       ++pos) {
    TransportImpl_rch impl = pos->lock();
    if (impl) {
      impl->unregister_for_writer(participant, readerid, writerid);
    }
  }
}

void
TransportClient::update_locators(const GUID_t& remote,
                                 const TransportLocatorSeq& locators)
{
  ACE_GUARD(ACE_Thread_Mutex, guard, lock_);
  for (ImplsType::iterator pos = impls_.begin(), limit = impls_.end();
       pos != limit;
       ++pos) {
    TransportImpl_rch impl = pos->lock();
    if (impl) {
      impl->update_locators(remote, locators);
    }
  }
}

WeakRcHandle<ICE::Endpoint>
TransportClient::get_ice_endpoint()
{
  // The one-to-many relationship with impls implies that this should
  // return a set of endpoints instead of a single endpoint or null.
  // For now, we will assume a single impl.

  ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, lock_, WeakRcHandle<ICE::Endpoint>());
  for (ImplsType::iterator pos = impls_.begin(), limit = impls_.end();
       pos != limit;
       ++pos) {
    TransportImpl_rch impl = pos->lock();
    if (impl) {
      WeakRcHandle<ICE::Endpoint> endpoint = impl->get_ice_endpoint();
      if (endpoint) { return endpoint; }
    }
  }

  return WeakRcHandle<ICE::Endpoint>();
}

bool
TransportClient::send_response(const GUID_t& peer,
                               const DataSampleHeader& header,
                               Message_Block_Ptr payload)
{
  DataLinkIndex::iterator found = data_link_index_.find(peer);

  if (found == data_link_index_.end()) {
    if (DCPS_debug_level > 4) {
      LogGuid logger(peer);
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) TransportClient::send_response: ")
                 ACE_TEXT("no link for publication %C, ")
                 ACE_TEXT("not sending response.\n"),
                 logger.c_str()));
    }

    return false;
  }

  DataLinkSet singular;
  singular.insert_link(found->second);
  singular.send_response(peer, header, OPENDDS_MOVE_NS::move(payload));
  return true;
}

void
TransportClient::send(SendStateDataSampleList send_list, ACE_UINT64 transaction_id)
{
  if (send_list.head() == 0) {
    return;
  }
  ACE_GUARD(ACE_Thread_Mutex, send_transaction_guard, send_transaction_lock_);
  send_i(send_list, transaction_id);
}

SendControlStatus
TransportClient::send_w_control(SendStateDataSampleList send_list,
                                const DataSampleHeader& header,
                                Message_Block_Ptr msg,
                                const GUID_t& destination)
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, send_transaction_guard,
                   send_transaction_lock_, SEND_CONTROL_ERROR);
  if (send_list.head()) {
    send_i(send_list, 0);
  }
  return send_control_to(header, OPENDDS_MOVE_NS::move(msg), destination);
}

void
TransportClient::send_i(SendStateDataSampleList send_list, ACE_UINT64 transaction_id)
{
  OPENDDS_ASSERT(guid_ != GUID_UNKNOWN);

  if (transaction_id != 0 && transaction_id != expected_transaction_id_) {
    if (transaction_id > max_transaction_id_seen_) {
      max_transaction_id_seen_ = transaction_id;
      max_transaction_tail_ = send_list.tail();
    }
    return;
  } else /* transaction_id == expected_transaction_id */ {

    DataSampleElement* cur = send_list.head();
    if (max_transaction_tail_ == 0) {
      //Means no future transaction beat this transaction into send
      if (transaction_id != 0)
        max_transaction_id_seen_ = expected_transaction_id_;
      // Only send this current transaction
      max_transaction_tail_ = send_list.tail();
    }
    DataLinkSet send_links;

    while (cur != 0) {
      // VERY IMPORTANT NOTE:
      //
      // We have to be very careful in how we deal with the current
      // DataSampleElement.  The issue is that once we have invoked
      // data_delivered() on the send_listener_ object, or we have invoked
      // send() on the pub_links, we can no longer access the current
      // DataSampleElement!Thus, we need to get the next
      // DataSampleElement (pointer) from the current element now,
      // while it is safe.
      DataSampleElement* next_elem;
      if (cur != max_transaction_tail_) {
        next_elem = cur->get_next_send_sample();
      } else {
        next_elem = max_transaction_tail_;
      }
      DataLinkSet_rch pub_links =
        (cur->get_num_subs() > 0)
        ? DataLinkSet_rch(links_.select_links(cur->get_sub_ids(), cur->get_num_subs()))
        : DataLinkSet_rch(&links_, inc_count());

      if (pub_links.is_nil() || pub_links->empty()) {
        // NOTE: This is the "local publisher id is not currently
        //       associated with any remote subscriber ids" case.

        if (DCPS_debug_level > 4) {
          LogGuid logger(cur->get_pub_id());
          ACE_DEBUG((LM_DEBUG,
                     ACE_TEXT("(%P|%t) TransportClient::send_i: ")
                     ACE_TEXT("no links for publication %C, ")
                     ACE_TEXT("not sending element %@ for transaction: %d.\n"),
                     logger.c_str(),
                     cur,
                     cur->transaction_id()));
        }

        // We tell the send_listener_ that all of the remote subscriber ids
        // that wanted the data (all zero of them) have indeed received
        // the data.
        cur->get_send_listener()->data_delivered(cur);

      } else {
        VDBG_LVL((LM_DEBUG,"(%P|%t) DBG: Found DataLinkSet. Sending element %@.\n"
                  , cur), 5);

#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE

        // Content-Filtering adjustment to the pub_links:
        // - If the sample should be filtered out of all subscriptions on a given
        //   DataLink, then exclude that link from the subset that we'll send to.
        // - If the sample should be filtered out of some (or none) of the subs,
        //   then record that information in the DataSampleElement so that the
        //   header's content_filter_entries_ can be marshaled before it's sent.
        if (cur->filter_out_.ptr()) {
          DataLinkSet_rch subset;
          DataLinkSet::GuardType guard(pub_links->lock());
          typedef DataLinkSet::MapType MapType;
          MapType& map = pub_links->map();

          for (MapType::iterator itr = map.begin(); itr != map.end(); ++itr) {
            size_t n_subs;
            GUIDSeq_var ti =
              itr->second->target_intersection(cur->get_pub_id(),
                                               cur->filter_out_.in(), n_subs);

            if (ti.ptr() == 0 || ti->length() != n_subs) {
              if (!subset.in()) {
                subset = make_rch<DataLinkSet>();
              }

              subset->insert_link(itr->second);
              cur->filter_per_link_[itr->first] = ti._retn();

            } else {
              VDBG((LM_DEBUG,
                    "(%P|%t) DBG: DataLink completely filtered-out %@.\n",
                    itr->second.in()));
            }
          }

          if (!subset.in()) {
            guard.release();
            VDBG((LM_DEBUG, "(%P|%t) DBG: filtered-out of all DataLinks.\n"));
            // similar to the "if (pub_links.is_nil())" case above, no links
            cur->get_send_listener()->data_delivered(cur);
            if (cur != max_transaction_tail_) {
              // Move on to the next DataSampleElement to send.
              cur = next_elem;
              continue;
            } else {
              break;
            }
          }

          pub_links = subset;
        }

#endif

        // This will do several things, including adding to the membership
        // of the send_links set.  Any DataLinks added to the send_links
        // set will be also told about the send_start() event.  Those
        // DataLinks (in the pub_links set) that are already in the
        // send_links set will not be told about the send_start() event
        // since they heard about it when they were inserted into the
        // send_links set.
        send_links.send_start(pub_links.in());
        if (cur->get_header().message_id_ != SAMPLE_DATA) {
          pub_links->send_control(cur);
        } else {
          pub_links->send(cur);
        }
      }
      if (cur != max_transaction_tail_) {
        // Move on to the next DataSampleElement to send.
        cur = next_elem;
      } else {
        break;
      }
    }

    // This will inform each DataLink in the set about the stop_send() event.
    // It will then clear the send_links_ set.
    //
    // The reason that the send_links_ set is cleared is because we continually
    // reuse the same send_links_ object over and over for each call to this
    // send method.
    send_links.send_stop(guid_);
    if (transaction_id != 0) {
      expected_transaction_id_ = max_transaction_id_seen_ + 1;
    }
    max_transaction_tail_ = 0;
  }
}

TransportSendListener_rch
TransportClient::get_send_listener()
{
  return rchandle_from(dynamic_cast<TransportSendListener*>(this));
}

TransportReceiveListener_rch
TransportClient::get_receive_listener()
{
  return rchandle_from(dynamic_cast<TransportReceiveListener*>(this));
}

SendControlStatus
TransportClient::send_control(const DataSampleHeader& header,
                              Message_Block_Ptr msg)
{
  OPENDDS_ASSERT(guid_ != GUID_UNKNOWN);
  return links_.send_control(guid_, get_send_listener(), header, OPENDDS_MOVE_NS::move(msg));
}

SendControlStatus
TransportClient::send_control_to(const DataSampleHeader& header,
                                 Message_Block_Ptr msg,
                                 const GUID_t& destination)
{
  OPENDDS_ASSERT(guid_ != GUID_UNKNOWN);

  DataLinkSet singular;
  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, lock_, SEND_CONTROL_ERROR);
    DataLinkIndex::iterator found = data_link_index_.find(destination);

    if (found == data_link_index_.end()) {
      return SEND_CONTROL_ERROR;
    }

    singular.insert_link(found->second);
  }
  return singular.send_control(guid_, get_send_listener(), header, OPENDDS_MOVE_NS::move(msg));
}

bool
TransportClient::remove_sample(const DataSampleElement* sample)
{
  return links_.remove_sample(sample);
}

bool
TransportClient::remove_all_msgs()
{
  OPENDDS_ASSERT(guid_ != GUID_UNKNOWN);
  return links_.remove_all_msgs(guid_);
}

void TransportClient::terminate_send_if_suspended()
{
  links_.terminate_send_if_suspended();
}

bool TransportClient::associated_with(const GUID_t& remote) const
{
  ACE_Guard<ACE_Thread_Mutex> guard(lock_);
  if (!guard.locked()) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: TransportClient::associated_with: "
      "lock failed\n"));
    return false;
  }
  return data_link_index_.count(remote);
}

bool TransportClient::pending_association_with(const GUID_t& remote) const
{
  ACE_Guard<ACE_Thread_Mutex> guard(lock_);
  if (!guard.locked()) {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: TransportClient::pending_association_with: "
      "lock failed\n"));
    return false;
  }
  return pending_.count(remote);
}

void TransportClient::data_acked(const GUID_t& remote)
{
  TransportSendListener_rch send_listener;
  {
    ACE_Guard<ACE_Thread_Mutex> guard(lock_);
    if (!guard.locked()) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: TransportClient::data_acked: "
        "lock failed\n"));
      return;
    }
    send_listener = get_send_listener();
  }
  send_listener->data_acked(remote);
}

bool TransportClient::is_leading(const GUID_t& reader_id) const
{
  OPENDDS_ASSERT(guid_ != GUID_UNKNOWN);
  return links_.is_leading(guid_, reader_id);
}


} // namepsace DCPS
} // namepsace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
