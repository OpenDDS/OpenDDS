/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "TransportImpl.h"
#include "DataLink.h"
#include "TransportExceptions.h"
#include "dds/DCPS/BuiltInTopicUtils.h"
#include "dds/DCPS/DataWriterImpl.h"
#include "dds/DCPS/DataReaderImpl.h"
#include "dds/DCPS/PublisherImpl.h"
#include "dds/DCPS/SubscriberImpl.h"
#include "dds/DCPS/RepoIdConverter.h"
#include "dds/DCPS/Util.h"
#include "dds/DCPS/MonitorFactory.h"
#include "dds/DCPS/Service_Participant.h"
#include "tao/debug.h"
#include <sstream>

#if !defined (__ACE_INLINE__)
#include "TransportImpl.inl"
#endif /* __ACE_INLINE__ */

namespace OpenDDS {
namespace DCPS {

TransportImpl::TransportImpl()
  : monitor_(0)
{
  DBG_ENTRY_LVL("TransportImpl", "TransportImpl", 6);
  if (TheServiceParticipant->monitor_factory_) {
    monitor_ = TheServiceParticipant->monitor_factory_->create_transport_monitor(this);
  }
}

TransportImpl::~TransportImpl()
{
  DBG_ENTRY_LVL("TransportImpl", "~TransportImpl", 6);
}

void
TransportImpl::shutdown()
{
  DBG_ENTRY_LVL("TransportImpl", "shutdown", 6);

  // Stop datalink clean task.
  this->dl_clean_task_.close(1);

  if (!this->reactor_task_.is_nil()) {
    this->reactor_task_->stop();
  }

  this->pre_shutdown_i();

  std::set<TransportClient*> local_clients;

  {
    GuardType guard(this->lock_);

    if (this->config_.is_nil()) {
      // This TransportImpl is already shutdown.
//MJM: So, I read here that config_i() actually "starts" us?
      return;
    }

    local_clients.swap(this->clients_);

    // We can release our lock_ now.
  }

  for (std::set<TransportClient*>::iterator it = local_clients.begin();
       it != local_clients.end(); ++it) {
    (*it)->transport_detached(this);
  }

  // Tell our subclass about the "shutdown event".
  this->shutdown_i();

  {
    GuardType guard(this->lock_);
    this->reactor_task_ = 0;
    // The shutdown_i() path may access the configuration so remove configuration
    // reference after shutdown is performed.

    // Drop our references to the config_.
    this->config_ = 0;
  }
}

bool
TransportImpl::configure(TransportInst* config)
{
  DBG_ENTRY_LVL("TransportImpl","configure",6);

  GuardType guard(this->lock_);

  if (config == 0) {
    ACE_ERROR_RETURN((LM_ERROR,
                      "(%P|%t) ERROR: invalid configuration.\n"),
                     false);
  }

  if (!this->config_.is_nil()) {
    // We are rejecting this configuration attempt since this
    // TransportImpl object has already been configured.
    ACE_ERROR_RETURN((LM_ERROR,
                      "(%P|%t) ERROR: TransportImpl already configured.\n"),
                     false);
  }

  config->_add_ref();
  this->config_ = config;

  // Let our subclass take a shot at the configuration object.
  if (this->configure_i(config) == false) {
    if (Transport_debug_level > 0) {
      dump();
    }

    this->config_ = 0;

    // The subclass rejected the configuration attempt.
    ACE_ERROR_RETURN((LM_ERROR,
                      "(%P|%t) ERROR: TransportImpl configuration failed.\n"),
                     false);
  }

  // Open the DL Cleanup task
  // We depend upon the existing config logic to ensure the
  // DL Cleanup task is opened only once
  if (this->dl_clean_task_.open()) {
    ACE_ERROR_RETURN((LM_ERROR,
                      "(%P|%t) ERROR: DL Cleanup task failed to open : %p\n",
                      ACE_TEXT("open")), false);
  }

  // Success.
  if (this->monitor_) {
    this->monitor_->report();
  }

  if (Transport_debug_level > 0) {
    std::stringstream os;
    dump(os);

    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("\n(%P|%t) TransportImpl::configure()\n%C"),
               os.str().c_str()));
  }

  return true;
}

void
TransportImpl::create_reactor_task()
{
  if (this->reactor_task_.in()) {
    return;
  }

  this->reactor_task_ = new TransportReactorTask;
  if (0 != this->reactor_task_->open(0)) {
    throw Transport::MiscProblem(); // error already logged by TRT::open()
  }
}

void
TransportImpl::attach_client(TransportClient* client)
{
  DBG_ENTRY_LVL("TransportImpl", "attach_client", 6);

  GuardType guard(this->lock_);
  clients_.insert(client);
}

void
TransportImpl::detach_client(TransportClient* client)
{
  DBG_ENTRY_LVL("TransportImpl", "attach_client", 6);

  GuardType guard(this->lock_);
  clients_.erase(client);
}

bool
TransportImpl::add_pending_association(const RepoId& local_id,
                                       const RepoId& remote_id,
                                       TransportSendListener* tsl)
{
  DBG_ENTRY_LVL("TransportImpl", "add_pending_association", 6);

  GuardType guard(this->lock_);

  // Cache the Association data so it can be used for the callback
  // to notify datawriter on_publication_matched.

  PendingAssociationsMap::iterator iter =
    pending_association_sub_map_.find(local_id);

  if (iter != pending_association_sub_map_.end()) {
    iter->second.push_back(remote_id);

  } else {
    association_listeners_[local_id] = tsl;
    pending_association_sub_map_[local_id].push_back(remote_id);
  }

  // Acks for this new pending association may arrive at this time.
  // If check for individual association, it needs remove the association
  // from pending_association_sub_map_ so the fully_associated won't be
  // called multiple times. To simplify, check by pub id since the
  // check_fully_association overloaded function clean the pending list
  // after calling fully_associated.
  check_fully_association(local_id);

  return true;
}

bool
TransportImpl::demarshal_acks(ACE_Message_Block* acks, bool swap_bytes)
{
  DBG_ENTRY_LVL("TransportImpl","demarshal_acks",6);

  {
    GuardType guard(this->lock_);

    int status = this->acked_sub_map_.demarshal(acks, swap_bytes);

    if (status == -1) {
      ACE_ERROR_RETURN((LM_ERROR,
                        "(%P|%t) ERROR: TransportImpl::demarshal_acks failed\n"),
                       false);
    }
  }

  check_fully_association();
  return true;
}

void
TransportImpl::check_fully_association()
{
  DBG_ENTRY_LVL("TransportImpl", "check_fully_association", 6);

  GuardType guard(this->lock_);

  if (Transport_debug_level > 8) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) ack dump: \n")));

    acked_sub_map_.dump();
  }

  PendingAssociationsMap::iterator penditer =
    pending_association_sub_map_.begin();

  while (penditer != pending_association_sub_map_.end()) {
    PendingAssociationsMap::iterator cur = penditer;
    ++penditer;

    check_fully_association(cur->first);
  }
}

void
TransportImpl::check_fully_association(const RepoId& pub_id)
{
  DBG_ENTRY_LVL("TransportImpl", "check_fully_association", 6);

  PendingAssociationsMap::iterator penditer =
    pending_association_sub_map_.find(pub_id);

  typedef std::vector<SubscriptionId> SubscriptionIdList;
  if (penditer != pending_association_sub_map_.end()) {
    SubscriptionIdList& associations = penditer->second;

    SubscriptionIdList::iterator iter = associations.begin();

    while (iter != associations.end()) {
      if (check_fully_association(penditer->first, *iter)) {
        iter = associations.erase(iter);
        association_listeners_.erase(pub_id);

      } else {
        ++iter;
      }
    }

    if (associations.size() == 0) {
      pending_association_sub_map_.erase(penditer);
    }
  }
}

bool
TransportImpl::check_fully_association(const RepoId& pub_id,
                                       const RepoId& sub_id)
{
  DBG_ENTRY_LVL("TransportImpl", "check_fully_association", 6);


  TransportSendListener* tsl = association_listeners_[pub_id];

  bool acked = false;
  if (this->acked(pub_id, sub_id) && tsl) {
    acked = true;
  }

  if (acked && tsl) {
    this->remove_ack(pub_id, sub_id);

    tsl->fully_associated(sub_id);

    return true;

  } else if (acked && Transport_debug_level > 8) {
    std::stringstream buffer;
    buffer << " pub " << pub_id << " - sub " << sub_id;
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) acked but DW is not registered: %C \n"),
               buffer.str().c_str()));
  }

  return false;
}

bool
TransportImpl::acked(RepoId pub_id, RepoId sub_id)
{
  int ret = false;
  RepoIdSet_rch set = this->acked_sub_map_.find(pub_id);

  if (!set.is_nil()) {
    bool last = false;
    ret = set->exist(sub_id, last);
  }

  if (Transport_debug_level > 8) {
    std::stringstream buffer;
    buffer << " pub " << pub_id << " - sub " << sub_id;
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) %C %C \n"),
               ret ? "acked" : "pending", buffer.str().c_str()));
  }

  return ret;
}

bool
TransportImpl::release_link_resources(DataLink* link)
{
  DBG_ENTRY_LVL("TransportImpl", "release_link_resources",6);

  // Create a smart pointer without ownership (bumps up ref count)
  DataLink_rch dl(link, false);

  dl_clean_task_.add(dl);

  return true;
}

void
TransportImpl::remove_ack(RepoId pub_id, RepoId sub_id)
{
  this->acked_sub_map_.remove(pub_id, sub_id);
}

void
TransportImpl::report()
{
  if (this->monitor_) {
    this->monitor_->report();
  }
}

void
TransportImpl::dump()
{
  std::stringstream os;
  dump(os);

  ACE_DEBUG((LM_DEBUG,
             ACE_TEXT("\n(%P|%t) TransportImpl::dump() -\n%C"),
             os.str().c_str()));
}

void
TransportImpl::dump(ostream& os)
{
  os << TransportInst::formatNameForDump("name")
     << config()->name();

  if (this->config_.is_nil()) {
    os << " (not configured)" << std::endl;
  } else {
    os << std::endl;
    this->config_->dump(os);
  }
}

}
}
