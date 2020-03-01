/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include "TransportClient.h"
#include "TransportConfig.h"
#include "TransportRegistry.h"
#include "TransportExceptions.h"
#include "TransportReceiveListener.h"

#include "dds/DdsDcpsInfoUtilsC.h"

#include "dds/DCPS/DataWriterImpl.h"
#include "dds/DCPS/SendStateDataSampleList.h"
#include "dds/DCPS/GuidConverter.h"
#include "dds/DCPS/Definitions.h"

#include "ace/Reactor_Timer_Interface.h"

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
  , repo_id_(GUID_UNKNOWN)
{
}

TransportClient::~TransportClient()
{
  if (Transport_debug_level > 5) {
    GuidConverter converter(repo_id_);
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) TransportClient::~TransportClient: %C\n"),
               OPENDDS_STRING(converter).c_str()));
  }

  stop_associating();

  ACE_GUARD(ACE_Thread_Mutex, guard, lock_);

  for (PrevPendingMap::iterator it = prev_pending_.begin(); it != prev_pending_.end(); ++it) {
    for (size_t i = 0; i < impls_.size(); ++i) {
      RcHandle<TransportImpl> impl = impls_[i].lock();
      if (impl) {
        impl->stop_accepting_or_connecting(*this, it->second->data_.remote_id_);
      }
    }
  }
}

void
TransportClient::clean_prev_pending()
{
  for (PendingMap::iterator it = prev_pending_.begin(); it != prev_pending_.end();) {
    if (it->second->safe_to_remove()) {
      prev_pending_.erase(it++);
    } else {
      ++it;
    }
  }
}

void
TransportClient::enable_transport(bool reliable, bool durable)
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

  enable_transport_using_config(reliable, durable, tc);
}

void
TransportClient::enable_transport_using_config(bool reliable, bool durable,
                                               const TransportConfig_rch& tc)
{
  config_ = tc;
  swap_bytes_ = tc->swap_bytes_;
  cdr_encapsulation_ = false;
  reliable_ = reliable;
  durable_ = durable;
  unsigned long duration = tc->passive_connect_duration_;
  if (duration == 0) {
    duration = TransportConfig::DEFAULT_PASSIVE_CONNECT_DURATION;
    if (DCPS_debug_level) {
      ACE_DEBUG((LM_WARNING,
        ACE_TEXT("(%P|%t) TransportClient::enable_transport_using_config ")
        ACE_TEXT("passive_connect_duration_ configured as 0, changing to ")
        ACE_TEXT("default value\n")));
    }
  }
  passive_connect_duration_ = TimeDuration::from_msec(duration);

  populate_connection_info();

  const size_t n = tc->instances_.size();

  for (size_t i = 0; i < n; ++i) {
    TransportInst_rch inst = tc->instances_[i];

    if (check_transport_qos(*inst)) {
      TransportImpl_rch impl = inst->impl();

      if (impl) {
        impls_.push_back(impl);

#if defined(OPENDDS_SECURITY)
        impl->local_crypto_handle(get_crypto_handle());
#endif

        cdr_encapsulation_ |= inst->requires_cdr();
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
TransportClient::populate_connection_info()
{
  conn_info_.length(0);

  const size_t n = config_->instances_.size();
  for (size_t i = 0; i < n; ++i) {
    TransportInst_rch inst = config_->instances_[i];
    if (check_transport_qos(*inst)) {
      TransportImpl_rch impl = inst->impl();
      if (impl) {
        const CORBA::ULong len = conn_info_.length();
        conn_info_.length(len + 1);
        impl->connection_info(conn_info_[len], CONNINFO_ALL);
      }
    }
  }
}

bool
TransportClient::associate(const AssociationData& data, bool active)
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, lock_, false);

  repo_id_ = get_repo_id();

  if (impls_.empty()) {
    if (DCPS_debug_level) {
      GuidConverter writer_converter(repo_id_);
      GuidConverter reader_converter(data.remote_id_);
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) TransportClient::associate - ")
                 ACE_TEXT("local %C remote %C no available impls\n"),
                 OPENDDS_STRING(writer_converter).c_str(),
                 OPENDDS_STRING(reader_converter).c_str()));
    }
    return false;
  }

  bool all_impls_shut_down = true;
  for (size_t i = 0; i < impls_.size(); ++i) {
    RcHandle<TransportImpl> impl = impls_[i].lock();
    if (impl && !impl->is_shut_down()) {
      all_impls_shut_down = false;
      break;
    }
  }

  if (all_impls_shut_down) {
    if (DCPS_debug_level) {
      GuidConverter writer_converter(repo_id_);
      GuidConverter reader_converter(data.remote_id_);
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) TransportClient::associate - ")
                 ACE_TEXT("local %C remote %C all available impls previously shutdown\n"),
                 OPENDDS_STRING(writer_converter).c_str(),
                 OPENDDS_STRING(reader_converter).c_str()));
    }
    return false;
  }

  clean_prev_pending();

  PendingMap::iterator iter = pending_.find(data.remote_id_);

  if (iter == pending_.end()) {
    RepoId remote_copy(data.remote_id_);
    iter = pending_.insert(std::make_pair(remote_copy, make_rch<PendingAssoc>(this))).first;

    GuidConverter tc_assoc(repo_id_);
    GuidConverter remote_new(data.remote_id_);
    VDBG_LVL((LM_DEBUG, "(%P|%t) TransportClient::associate added PendingAssoc "
              "between %C and remote %C\n",
              OPENDDS_STRING(tc_assoc).c_str(),
              OPENDDS_STRING(remote_new).c_str()), 0);
  } else {

    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: TransportClient::associate ")
               ACE_TEXT("already associating with remote.\n")));

    return false;

  }

  PendingAssoc_rch pend = iter->second;
  pend->active_ = active;
  pend->impls_.clear();
  pend->blob_index_ = 0;
  pend->data_ = data;
  pend->attribs_.local_id_ = repo_id_;
  pend->attribs_.priority_ = get_priority_value(data);
  pend->attribs_.local_reliable_ = reliable_;
  pend->attribs_.local_durable_ = durable_;

  if (active) {
    pend->impls_.reserve(impls_.size());
    std::reverse_copy(impls_.begin(), impls_.end(),
                      std::back_inserter(pend->impls_));

    return pend->initiate_connect(this, guard);

  } else { // passive

    // call accept_datalink for each impl / blob pair of the same type
    for (size_t i = 0; i < impls_.size(); ++i) {
      RcHandle<TransportImpl> impl = impls_[i].lock();
      pend->impls_.push_back(impl);
      const OPENDDS_STRING type = impl->transport_type();

      for (CORBA::ULong j = 0; j < data.remote_data_.length(); ++j) {
        if (data.remote_data_[j].transport_type.in() == type) {
          const TransportImpl::RemoteTransport remote = {
            data.remote_id_, data.remote_data_[j].data,
            data.publication_transport_priority_,
            data.remote_reliable_, data.remote_durable_};

          TransportImpl::AcceptConnectResult res;
          {
            // This thread acquired lock_ at the beginning of this method.  Calling accept_datalink might require getting the lock for the transport's reactor.
            // If the current thread is not an event handler for the transport's reactor, e.g., the ORB's thread, then the order of acquired locks will be lock_ -> transport reactor lock.
            // Event handlers in the transport reactor may call passive_connection which calls use_datalink which acquires lock_.  The locking order in this case is transport reactor lock -> lock_.
            // To avoid deadlock, we must reverse the lock.
            TransportImpl::ConnectionAttribs attribs = pend->attribs_;
            RcHandle<TransportClient> client = rchandle_from(this);
            ACE_GUARD_RETURN(Reverse_Lock_t, unlock_guard, reverse_lock_, false);
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
                // In this case, it may be waiting for the TCP connection to be established.  Just wait without trying other transports.
                pending_assoc_timer_->schedule_timer(this, iter->second);
            } else {
              use_datalink_i(data.remote_id_, res.link_, guard);
              return true;
            }
          }
        }
      }
    }

    pending_assoc_timer_->schedule_timer(this, iter->second);
  }

  return true;
}

void
TransportClient::PendingAssoc::reset_client() {
  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  client_.reset();
}

bool
TransportClient::PendingAssoc::safe_to_remove() {
  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  return !client_ && !scheduled_;
}

int
TransportClient::PendingAssoc::handle_timeout(const ACE_Time_Value&,
                                              const void* arg)
{
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
    GuidConverter local(repo_id_);
    GuidConverter remote_conv(remote.repo_id_);
    VDBG_LVL((LM_DEBUG, ACE_TEXT("(%P|%t) TransportClient::initiate_connect_i ")
                        ACE_TEXT("between local %C and remote %C unsuccessful because ")
                        ACE_TEXT("guard was not locked\n"),
                        OPENDDS_STRING(local).c_str(),
                        OPENDDS_STRING(remote_conv).c_str()), 0);
    return false;
  }

  {
    //can't call connect while holding lock due to possible reactor deadlock
    guard.release();
    GuidConverter local(repo_id_);
    GuidConverter remote_conv(remote.repo_id_);
    VDBG_LVL((LM_DEBUG, "(%P|%t) TransportClient::initiate_connect_i - "
                        "attempt to connect_datalink between local %C and remote %C\n",
                        OPENDDS_STRING(local).c_str(),
                        OPENDDS_STRING(remote_conv).c_str()), 0);
    result = impl->connect_datalink(remote, attribs_, rchandle_from(this));
    guard.acquire();
    if (!result.success_) {
      if (DCPS_debug_level) {
        GuidConverter writer_converter(repo_id_);
        GuidConverter reader_converter(remote.repo_id_);
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) TransportClient::associate - ")
                   ACE_TEXT("connect_datalink between local %C remote %C not successful\n"),
                   OPENDDS_STRING(writer_converter).c_str(),
                   OPENDDS_STRING(reader_converter).c_str()));
      }
      return false;
    }
  }

  GuidConverter local(repo_id_);
  GuidConverter remote_conv(remote.repo_id_);
  VDBG_LVL((LM_DEBUG, "(%P|%t) TransportClient::initiate_connect_i - "
                      "connection between local %C and remote %C initiation successful\n",
                      OPENDDS_STRING(local).c_str(),
                      OPENDDS_STRING(remote_conv).c_str()), 0);
  return true;
}

bool
TransportClient::PendingAssoc::initiate_connect(TransportClient* tc,
                                                Guard& guard)
{
  GuidConverter local(tc->repo_id_);
  GuidConverter remote(data_.remote_id_);
  VDBG_LVL((LM_DEBUG, "(%P|%t) PendingAssoc::initiate_connect - "
                      "between %C and remote %C\n",
                      OPENDDS_STRING(local).c_str(),
                      OPENDDS_STRING(remote).c_str()), 0);
  // find the next impl / blob entry that have matching types
  while (!impls_.empty()) {
    RcHandle<TransportImpl> impl = impls_.back().lock();
    if (!impl) {
      continue;
    }
    const OPENDDS_STRING type = impl->transport_type();

    for (; blob_index_ < data_.remote_data_.length(); ++blob_index_) {
      if (data_.remote_data_[blob_index_].transport_type.in() == type) {
        const TransportImpl::RemoteTransport remote = {
          data_.remote_id_, data_.remote_data_[blob_index_].data,
          data_.publication_transport_priority_,
          data_.remote_reliable_, data_.remote_durable_};

        TransportImpl::AcceptConnectResult res;
        GuidConverter tmp_local(tc->repo_id_);
        GuidConverter tmp_remote(data_.remote_id_);
        if (!tc->initiate_connect_i(res, impl, remote, attribs_, guard)) {
          //tc init connect returned false there is no PendingAssoc left in map because use_datalink_i finished elsewhere
          //so don't do anything further with pend and return success or failure up to tc's associate
          if (res.success_ ) {
            GuidConverter local(tc->repo_id_);
            GuidConverter remote(data_.remote_id_);
            VDBG_LVL((LM_DEBUG, ACE_TEXT("(%P|%t) PendingAssoc::initiate_connect - ")
                                ACE_TEXT("between %C and remote %C success\n"),
                                OPENDDS_STRING(local).c_str(),
                                OPENDDS_STRING(remote).c_str()), 0);
            return true;
          }

          VDBG_LVL((LM_DEBUG, "(%P|%t) PendingAssoc::initiate_connect - "
                              "between %C and remote %C unsuccessful\n",
                              OPENDDS_STRING(tmp_local).c_str(),
                              OPENDDS_STRING(tmp_remote).c_str()), 0);
        }

        if (res.success_) {

          ++blob_index_;

          if (!res.link_.is_nil()) {

            tc->use_datalink_i(data_.remote_id_, res.link_, guard);
          } else {
            GuidConverter local(tc->repo_id_);
            GuidConverter remote(data_.remote_id_);
            VDBG_LVL((LM_DEBUG, "(%P|%t) PendingAssoc::intiate_connect - "
                                "resulting link from initiate_connect_i (local: %C to remote: %C) was nil\n",
                                OPENDDS_STRING(local).c_str(),
                                OPENDDS_STRING(remote).c_str()), 0);
          }

          return true;
        } else {
          GuidConverter local(tc->repo_id_);
          GuidConverter remote(data_.remote_id_);
          VDBG_LVL((LM_DEBUG, "(%P|%t) PendingAssoc::intiate_connect - "
                              "result of initiate_connect_i (local: %C to remote: %C) was not success \n",
                              OPENDDS_STRING(local).c_str(),
                              OPENDDS_STRING(remote).c_str()), 0);
        }
      }
    }

    impls_.pop_back();
    blob_index_ = 0;
  }

  return false;
}

void
TransportClient::use_datalink(const RepoId& remote_id,
                              const DataLink_rch& link)
{
  ACE_GUARD(ACE_Thread_Mutex, guard, lock_);

  use_datalink_i(remote_id, link, guard);
}

void
TransportClient::use_datalink_i(const RepoId& remote_id_ref,
                                const DataLink_rch& link,
                                Guard& guard)
{
  // Try to make a local copy of remote_id to use in calls
  // because the reference could be invalidated if the caller
  // reference location is deleted (i.e. in stop_accepting_or_connecting
  // if use_datalink_i was called from passive_connection)
  // Does changing this from a reference to a local affect anything going forward?
  RepoId remote_id(remote_id_ref);

  GuidConverter peerId_conv(remote_id);
  VDBG_LVL((LM_DEBUG, "(%P|%t) TransportClient::use_datalink_i "
            "TransportClient(%@) using datalink[%@] from %C\n",
            this,
            link.in(),
            OPENDDS_STRING(peerId_conv).c_str()), 0);

  PendingMap::iterator iter = pending_.find(remote_id);

  if (iter == pending_.end()) {
    VDBG_LVL((LM_DEBUG, "(%P|%t) TransportClient::use_datalink_i "
                        "TransportClient(%@) using datalink[%@] did not find Pending Association to remote %C\n",
                        this,
                        link.in(),
                        OPENDDS_STRING(peerId_conv).c_str()), 0);
    return;
  }

  PendingAssoc_rch pend = iter->second;
  const int active_flag = pend->active_ ? ASSOC_ACTIVE : 0;
  bool ok = false;

  if (link.is_nil()) {

    if (pend->active_ && pend->initiate_connect(this, guard)) {
      VDBG_LVL((LM_DEBUG, "(%P|%t) TransportClient::use_datalink_i "
                          "TransportClient(%@) using datalink[%@] link is nil, since this is active side, initiate_connect\n",
                          this,
                          link.in(),
                          OPENDDS_STRING(peerId_conv).c_str()), 0);
      return;
    }

  } else { // link is ready to use
    VDBG_LVL((LM_DEBUG, "(%P|%t) TransportClient::use_datalink_i "
              "TransportClient(%@) about to add_link[%@] to remote: %C\n",
              this,
              link.in(),
              OPENDDS_STRING(peerId_conv).c_str()), 0);

    add_link(link, remote_id);
    ok = true;
  }

  // either link is valid or assoc failed, clean up pending object
  // for passive side processing
  if (!pend->active_) {

    for (size_t i = 0; i < pend->impls_.size(); ++i) {
      RcHandle<TransportImpl> impl = pend->impls_[i].lock();
      if (impl) {
        impl->stop_accepting_or_connecting(*this, pend->data_.remote_id_);
      }
    }
  }

  iter->second->reset_client();
  pending_assoc_timer_->cancel_timer(this, pend);
  prev_pending_.insert(std::make_pair(iter->first, iter->second));
  pending_.erase(iter);

  guard.release();

  transport_assoc_done(active_flag | (ok ? ASSOC_OK : 0), remote_id);
}

void
TransportClient::add_link(const DataLink_rch& link, const RepoId& peer)
{
  links_.insert_link(link);
  data_link_index_[peer] = link;

  TransportReceiveListener_rch trl = get_receive_listener();

  if (trl) {
    link->make_reservation(peer, repo_id_, trl, reliable_);

  } else {
    link->make_reservation(peer, repo_id_, get_send_listener(), reliable_);
  }
}

void
TransportClient::stop_associating()
{
  ACE_GUARD(ACE_Thread_Mutex, guard, lock_);
  for (PendingMap::iterator it = pending_.begin(); it != pending_.end(); ++it) {
    it->second->reset_client();
    pending_assoc_timer_->cancel_timer(this, it->second);
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
        iter->second->reset_client();
        pending_assoc_timer_->cancel_timer(this, iter->second);
        prev_pending_.insert(std::make_pair(iter->first, iter->second));
        pending_.erase(iter);
      }
    }
  }
}

void
TransportClient::send_final_acks()
{
  links_.send_final_acks(get_repo_id());
}

void
TransportClient::disassociate(const RepoId& peerId)
{
  GuidConverter peerId_conv(peerId);
  VDBG_LVL((LM_DEBUG, "(%P|%t) TransportClient::disassociate "
            "TransportClient(%@) disassociating from %C\n",
            this,
            OPENDDS_STRING(peerId_conv).c_str()), 5);

  ACE_GUARD(ACE_Thread_Mutex, guard, lock_);

  PendingMap::iterator iter = pending_.find(peerId);
  if (iter != pending_.end()) {
    iter->second->reset_client();
    pending_assoc_timer_->cancel_timer(this, iter->second);
    prev_pending_.insert(std::make_pair(iter->first, iter->second));
    pending_.erase(iter);
    return;
  }

  const DataLinkIndex::iterator found = data_link_index_.find(peerId);

  if (found == data_link_index_.end()) {
    if (DCPS_debug_level > 4) {
      const GuidConverter converter(peerId);
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) TransportClient::disassociate: ")
                 ACE_TEXT("no link for remote peer %C\n"),
                 OPENDDS_STRING(converter).c_str()));
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
                 ACE_TEXT("about to release_reservations for link[%@] \n"),
                 link.in()));
    }

    link->release_reservations(peerId, repo_id_, released);

  if (!released.empty()) {

    if (DCPS_debug_level > 4) {
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) TransportClient::disassociate: ")
                 ACE_TEXT("about to remove_link[%@] from links_\n"),
                 link.in()));
    }
    links_.remove_link(link);

    if (DCPS_debug_level > 4) {
      GuidConverter converter(repo_id_);
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) TransportClient::disassociate: calling remove_listener %C on link[%@]\n"),
                 OPENDDS_STRING(converter).c_str(),
                 link.in()));
    }
    // Datalink is no longer used for any remote peer by this TransportClient
    link->remove_listener(repo_id_);

  }
}

void
TransportClient::register_for_reader(const RepoId& participant,
                                     const RepoId& writerid,
                                     const RepoId& readerid,
                                     const TransportLocatorSeq& locators,
                                     OpenDDS::DCPS::DiscoveryListener* listener)
{
  ACE_GUARD(ACE_Thread_Mutex, guard, lock_);
  for (ImplsType::iterator pos = impls_.begin(), limit = impls_.end();
       pos != limit;
       ++pos) {
    RcHandle<TransportImpl> impl = pos->lock();
    if (impl) {
      impl->register_for_reader(participant, writerid, readerid, locators, listener);
    }
  }
}

void
TransportClient::unregister_for_reader(const RepoId& participant,
                                       const RepoId& writerid,
                                       const RepoId& readerid)
{
  ACE_GUARD(ACE_Thread_Mutex, guard, lock_);
  for (ImplsType::iterator pos = impls_.begin(), limit = impls_.end();
       pos != limit;
       ++pos) {
    RcHandle<TransportImpl> impl = pos->lock();
    if (impl) {
      impl->unregister_for_reader(participant, writerid, readerid);
    }
  }
}

void
TransportClient::register_for_writer(const RepoId& participant,
                                     const RepoId& readerid,
                                     const RepoId& writerid,
                                     const TransportLocatorSeq& locators,
                                     DiscoveryListener* listener)
{
  ACE_GUARD(ACE_Thread_Mutex, guard, lock_);
  for (ImplsType::iterator pos = impls_.begin(), limit = impls_.end();
       pos != limit;
       ++pos) {
    RcHandle<TransportImpl> impl = pos->lock();
    if (impl) {
      impl->register_for_writer(participant, readerid, writerid, locators, listener);
    }
  }
}

void
TransportClient::unregister_for_writer(const RepoId& participant,
                                       const RepoId& readerid,
                                       const RepoId& writerid)
{
  ACE_GUARD(ACE_Thread_Mutex, guard, lock_);
  for (ImplsType::iterator pos = impls_.begin(), limit = impls_.end();
       pos != limit;
       ++pos) {
    RcHandle<TransportImpl> impl = pos->lock();
    if (impl) {
      impl->unregister_for_writer(participant, readerid, writerid);
    }
  }
}

void
TransportClient::update_locators(const RepoId& remote,
                                 const TransportLocatorSeq& locators)
{
  ACE_GUARD(ACE_Thread_Mutex, guard, lock_);
  for (ImplsType::iterator pos = impls_.begin(), limit = impls_.end();
       pos != limit;
       ++pos) {
    RcHandle<TransportImpl> impl = pos->lock();
    if (impl) {
      impl->update_locators(remote, locators);
    }
  }
}

ICE::Endpoint*
TransportClient::get_ice_endpoint()
{
  // The one-to-many relationship with impls implies that this should
  // return a set of endpoints instead of a single endpoint or null.
  // For now, we will assume a single impl.

  ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, lock_, 0);
  for (ImplsType::iterator pos = impls_.begin(), limit = impls_.end();
       pos != limit;
       ++pos) {
    RcHandle<TransportImpl> impl = pos->lock();
    if (impl) {
      ICE::Endpoint* endpoint = impl->get_ice_endpoint();
      if (endpoint) { return endpoint; }
    }
  }

  return 0;
}

bool
TransportClient::send_response(const RepoId& peer,
                               const DataSampleHeader& header,
                               Message_Block_Ptr payload)
{
  DataLinkIndex::iterator found = data_link_index_.find(peer);

  if (found == data_link_index_.end()) {
    if (DCPS_debug_level > 4) {
      GuidConverter converter(peer);
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) TransportClient::send_response: ")
                 ACE_TEXT("no link for publication %C, ")
                 ACE_TEXT("not sending response.\n"),
                 OPENDDS_STRING(converter).c_str()));
    }

    return false;
  }

  DataLinkSet singular;
  singular.insert_link(found->second);
  singular.send_response(peer, header, move(payload));
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
                                const RepoId& destination)
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, send_transaction_guard,
                   send_transaction_lock_, SEND_CONTROL_ERROR);
  if (send_list.head()) {
    send_i(send_list, 0);
  }
  return send_control_to(header, move(msg), destination);
}

void
TransportClient::send_i(SendStateDataSampleList send_list, ACE_UINT64 transaction_id)
{
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

    while (true) {
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
          GuidConverter converter(cur->get_pub_id());
          ACE_DEBUG((LM_DEBUG,
                     ACE_TEXT("(%P|%t) TransportClient::send_i: ")
                     ACE_TEXT("no links for publication %C, ")
                     ACE_TEXT("not sending element %@ for transaction: %d.\n"),
                     OPENDDS_STRING(converter).c_str(),
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

  #endif // OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE

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
    RepoId pub_id(repo_id_);
    send_links.send_stop(pub_id);
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
  return links_.send_control(repo_id_, get_send_listener(), header, move(msg));
}

SendControlStatus
TransportClient::send_control_to(const DataSampleHeader& header,
                                 Message_Block_Ptr msg,
                                 const RepoId& destination)
{
  DataLinkSet singular;
  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, lock_, SEND_CONTROL_ERROR);
    DataLinkIndex::iterator found = data_link_index_.find(destination);

    if (found == data_link_index_.end()) {
      return SEND_CONTROL_ERROR;
    }

    singular.insert_link(found->second);
  }
  return singular.send_control(repo_id_, get_send_listener(), header, move(msg));
}

bool
TransportClient::remove_sample(const DataSampleElement* sample)
{
  return links_.remove_sample(sample);
}

bool
TransportClient::remove_all_msgs()
{
  return links_.remove_all_msgs(repo_id_);
}

void
TransportClient::terminate_send_if_suspended() {
  links_.terminate_send_if_suspended();
}

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
