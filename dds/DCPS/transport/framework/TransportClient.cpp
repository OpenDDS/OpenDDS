/*
 * $Id$
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
#include "dds/DCPS/DataSampleList.h"
#include "dds/DCPS/GuidConverter.h"
#include "dds/DCPS/Definitions.h"
#include "dds/DCPS/Service_Participant.h"

#include "ace/Reactor_Timer_Interface.h"

#include <algorithm>
#include <iterator>

namespace OpenDDS {
namespace DCPS {

TransportClient::TransportClient()
: swap_bytes_(false)
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
            std::string(converter).c_str()));
   }

   //### Debug statements to track where connection is failing
   ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###TransportClient::~TransportClient --> TRYING TO lock_\n"));

   ACE_GUARD(ACE_Thread_Mutex, guard, lock_);

   //### Debug statements to track where connection is failing
   ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###TransportClient::~TransportClient --> GOT lock_\n"));

   for (DataLinkSet::MapType::iterator iter = links_.map().begin();
         iter != links_.map().end(); ++iter) {
      //### Debug statements to track where connection is failing
      GuidConverter repo_converted(repo_id_);
      ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###TransportClient::~TransportClient --> remove_listener for: %C\n", std::string(repo_converted).c_str()));
      iter->second->remove_listener(repo_id_);
   }

   ACE_Reactor_Timer_Interface* timer = TheServiceParticipant->timer();
   for (PendingMap::iterator it = pending_.begin(); it != pending_.end(); ++it) {
      for (size_t i = 0; i < impls_.size(); ++i) {
         //### Debug statements to track where connection is failing
         GuidConverter this_tc_converted(repo_id_);
         GuidConverter remote_repo_converted(it->second.data_.remote_id_);
         ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###TransportClient::~TransportClient --> stop_accepting_or_connecting for tc: %C for remote: %C \n", std::string(this_tc_converted).c_str(), std::string(remote_repo_converted).c_str()));
         impls_[i]->stop_accepting_or_connecting(this, it->second.data_.remote_id_);
      }
      //### Debug statements to track where connection is failing
      ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###TransportClient::~TransportClient --> cancel_timer for PENDING ASSOC \n"));
      //### shouldn't really do this, timer should never be 0, right?
      if (timer != 0)
        timer->cancel_timer(&it->second);
   }

   for (std::vector<TransportImpl_rch>::iterator it = impls_.begin();
         it != impls_.end(); ++it) {
      //### Debug statements to track where connection is failing
      ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###TransportClient::~TransportClient --> detach_client \n"));

      (*it)->detach_client(this);
   }
   //### Debug statements to track where connection is failing
   ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###TransportClient::~TransportClient --> RELEASING lock_\n"));
}

void
TransportClient::enable_transport(bool reliable, bool durable)
{
  //### Debug statements to track where connection is failing
  ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###TransportClient::enable_transport --> enter (TransportClient: %@)\n", this));
   // Search for a TransportConfig to use:
   TransportConfig_rch tc;

   // 1. If this object is an Entity, check if a TransportConfig has been
   //    bound either directly to this entity or to a parent entity.
   for (const EntityImpl* ent = dynamic_cast<const EntityImpl*>(this);
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
   //### Debug statements to track where connection is failing
   ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###TransportClient::enable_transport --> exit\n"));
}

void
TransportClient::enable_transport_using_config(bool reliable, bool durable,
      const TransportConfig_rch& tc)
{
  //### Debug statements to track where connection is failing
  ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###TransportClient::enable_transport_using_config --> enter(TransportClient: %@)\n", this));

   swap_bytes_ = tc->swap_bytes_;
   cdr_encapsulation_ = false;
   reliable_ = reliable;
   durable_ = durable;
   passive_connect_duration_.set(tc->passive_connect_duration_ / 1000,
         (tc->passive_connect_duration_ % 1000) * 1000);

   const size_t n = tc->instances_.size();
   for (size_t i = 0; i < n; ++i) {
      TransportInst_rch inst = tc->instances_[i];
      if (check_transport_qos(*inst.in())) {
         TransportImpl_rch impl = inst->impl();
         if (!impl.is_nil()) {
            impl->attach_client(this);
            impls_.push_back(impl);
            const CORBA::ULong len = conn_info_.length();
            conn_info_.length(len + 1);
            impl->connection_info(conn_info_[len]);
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
   //### Debug statements to track where connection is failing
   ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###TransportClient::enable_transport_using_config --> exit\n"));
}

void
TransportClient::transport_detached(TransportImpl* which)
{

   //### Debug statements to track where connection is failing
   ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###TransportClient::transport_detached --> TRYING TO lock_\n"));

   ACE_GUARD(ACE_Thread_Mutex, guard, lock_);

   //### Debug statements to track where connection is failing
   ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###TransportClient::transport_detached --> GOT lock_\n"));

   // Remove any DataLinks created by the 'which' TransportImpl from our local
   // data structures (both links_ and data_link_index_).
   for (DataLinkSet::MapType::iterator iter = links_.map().begin();
         iter != links_.map().end();) {
      TransportImpl_rch impl = iter->second->impl();
      if (impl.in() == which) {
         for (DataLinkIndex::iterator it2 = data_link_index_.begin();
               it2 != data_link_index_.end();) {
            if (it2->second.in() == iter->second.in()) {
               data_link_index_.erase(it2++);
            } else {
               ++it2;
            }
         }
         iter->second->remove_listener(repo_id_);
         links_.map().erase(iter++);
      } else {
         ++iter;
      }
   }

   // Remove the 'which' TransportImpl from the impls_ list
   for (std::vector<TransportImpl_rch>::iterator it = impls_.begin();
         it != impls_.end(); ++it) {
      if (it->in() == which) {
         impls_.erase(it);
         for (PendingMap::iterator it2 = pending_.begin();
               it2 != pending_.end(); ++it2) {
            which->stop_accepting_or_connecting(this, it2->first);
         }
         //### Debug statements to track where connection is failing
         ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###TransportClient::transport_detached --> RELEASE lock_ \n"));
         return;
      }
   }

   //### Debug statements to track where connection is failing
   ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###TransportClient::transport_detached --> RELEASE lock_ \n"));
}

bool
TransportClient::associate(const AssociationData& data, bool active)
{

   //### Debug statements to track where associate is failing
   ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###TransportClient::associate begun\n"));

   //### Debug statements to track where connection is failing
   ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###TransportClient::associate --> TRYING TO lock_\n"));

   ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, lock_, false);

   //### Debug statements to track where connection is failing
   ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###TransportClient::associate --> GOT lock_  \n"));

   repo_id_ = get_repo_id();

   if (impls_.empty()) {
      //### Debug statements to track where connection is failing
      ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###TransportClient::associate --> RELEASE lock_ \n"));
      return false;
   }

   PendingMap::iterator iter = pending_.find(data.remote_id_);
   if (iter == pending_.end()) {
      iter = pending_.insert(std::make_pair(data.remote_id_, PendingAssoc())).first;
      //### Debug statements to track where connection is failing
      GuidConverter tc_assoc(this->repo_id_);
      GuidConverter remote_new(data.remote_id_);
      ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###TransportClient::associate --> INSERTED NEW PENDING ASSOC TransportClient: %C remote_id: %C\n", std::string(tc_assoc).c_str(), std::string(remote_new).c_str()));
      ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###TransportClient::associate --> after insertion there are %d pending assocs\n", pending_.size()));

      PendingMap::iterator iter_print = pending_.begin();
      //### Debug statements to track where associate is failing
         ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###TransportClient::associate --> PRINT OUT ALL PENDING ASSOC IN PENDINGMAP\n"));
         int i = 0;
      while(iter_print != pending_.end()) {
         //### Debug statements to track where associate is failing
         GuidConverter remote_print(iter_print->first);
            ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###TransportClient::associate --> PendingAssoc #%d is for RepoId: %C\n", i, std::string(remote_print).c_str()));
            iter_print++;
            i++;
      }


   } else {
      if (iter->second.removed_) {
         iter->second.removed_ = false;
      } else {
         //TODO: ACE_ERROR blah blah... already associated with remote
         //### Debug statements to track where associate is failing
         ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###TransportClient::associate already associated, exiting false\n"));
         //### Debug statements to track where connection is failing
         ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###TransportClient::associate --> RELEASE lock_ \n"));
         return false;
      }
   }

   PendingAssoc& pend = iter->second;
   pend.active_ = active;
   pend.impls_.clear();
   pend.blob_index_ = 0;
   pend.data_ = data;
   pend.attribs_.local_id_ = repo_id_;
   pend.attribs_.priority_ = get_priority_value(data);
   pend.attribs_.local_reliable_ = reliable_;
   pend.attribs_.local_durable_ = durable_;

   //### Debug statements to track where associate is failing
   ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###TransportClient::associate PendingAssoc created\n"));

   if (active) {
      //### Debug statements to track where associate is failing
      ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###TransportClient::associate active side about to call initiate_connect\n"));
      pend.impls_.reserve(impls_.size());
      std::reverse_copy(impls_.begin(), impls_.end(),
            std::back_inserter(pend.impls_));


      pend.initiate_connect(this, guard);
      //### Debug statements to track where connection is failing
      ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###TransportClient::associate --> RELEASE lock_ \n"));
      //### Debug statements to track where associate is failing
      ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###TransportClient::associate active side assoc complete about to return initiate_connect\n"));
      //return pend.initiate_connect(this, guard);
      return true;

   } else { // passive
      //### Debug statements to track where associate is failing
      ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###TransportClient::associate passive side about to try to accept_datalink and use_datalink\n"));

      // call accept_datalink for each impl / blob pair of the same type
      for (size_t i = 0; i < impls_.size(); ++i) {
         const std::string type = impls_[i]->transport_type();
         for (CORBA::ULong j = 0; j < data.remote_data_.length(); ++j) {
            if (data.remote_data_[j].transport_type.in() == type) {
               const TransportImpl::RemoteTransport remote = {
                     data.remote_id_, data.remote_data_[j].data,
                     data.publication_transport_priority_,
                     data.remote_reliable_, data.remote_durable_};


               TransportImpl::AcceptConnectResult res;
               //### Debug statements to track where associate is failing
              ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###TransportClient::associate passive side pre-accept_datalink res Success? : %s \n", res.success_ ? "TRUE": "FALSE"));
               {
                  //can't call accept_datalink while holding lock due to possible reactor deadlock with passive_connection
                  ACE_GUARD_RETURN(Reverse_Lock_t, unlock_guard, reverse_lock_, false);
                  //### Debug statements to track where connection is failing
                  ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###TransportClient::associate --> RELEASE lock_ using LOCK reverse_lock_\n"));
               //const TransportImpl::AcceptConnectResult res =
                  res = impls_[i]->accept_datalink(remote, pend.attribs_, this);
               }
               //### Debug statements to track where connection is failing
               ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###TransportClient::associate --> LOCKED lock_ after RELEASE reverse_lock \n"));

               //NEED to check that pend is still valid here after you re-acquire the lock_ after accepting the datalink

               //### Debug statements to track where associate is failing
               ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###TransportClient::associate passive side post-accept_datalink res Success? : %s with res.link: %s\n", res.success_ ? "TRUE": "FALSE", res.link_.is_nil() ? "NIL" :"NOT NIL"));

               if (res.success_ && !res.link_.is_nil()) {
                  //### Debug statements to track where associate is failing
                  ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###TransportClient::associate passive side About to use_datalink_i \n"));
                  use_datalink_i(data.remote_id_, res.link_, guard);
                  //### Debug statements to track where connection is failing
                  ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###TransportClient::associate --> RELEASE lock_ \n"));
                  //### Debug statements to track where associate is failing
                  ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###TransportClient::associate passive side use_datalink_i returned: associate to return true\n"));
                  return true;
               }
            }
         }
         pend.impls_.push_back(impls_[i]);
      }

      ACE_Reactor_Timer_Interface* timer = TheServiceParticipant->timer();
      timer->schedule_timer(&pend, this, passive_connect_duration_);
   }
   //### Debug statements to track where associate is failing
   ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###TransportClient::associate associate about to return from associate\n"));
   //### Debug statements to track where connection is failing
   ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###TransportClient::associate --> RELEASE lock_ \n"));
   return true;
}

int
TransportClient::PendingAssoc::handle_timeout(const ACE_Time_Value&,
      const void* arg)
{
   TransportClient* tc = static_cast<TransportClient*>(const_cast<void*>(arg));
   tc->use_datalink(data_.remote_id_, 0);
   return 0;
}

bool
TransportClient::initiate_connect_i(TransportImpl::AcceptConnectResult& result,
                                    const TransportImpl_rch impl,
                                    const TransportImpl::RemoteTransport& remote,
                                    const TransportImpl::ConnectionAttribs& attribs_,
                                    Guard& guard)
{
   if(!guard.locked()) {
      //don't own the lock_ so can't release it...shouldn't happen
      return false;
   }

   {
      //can't call connect while holding lock due to possible reactor deadlock
      ACE_GUARD_RETURN(Reverse_Lock_t, unlock_guard, reverse_lock_, false);
      result = impl->connect_datalink(remote, attribs_, this);
   }
   //Check to make sure the pending assoc still exists in the map and hasn't been slated for removal
   //figure out how to respond to these possible results that occurred while lock was released to connect
   PendingMap::iterator iter = pending_.find(remote.repo_id_);
   if (iter == pending_.end()) {
      return false;
      //log some sort of error message...
      //PendingAssoc's are only erased from pending_ in use_datalink_i after
   } else {
      if (iter->second.removed_) {
         //this occurs if the transport client was told to disassociate while connecting
         //disassociate cleans up everything except this local AcceptConnectResult whose destructor
         //should take care of it because link has not been shifted into links_ by use_datalink_i
         return false;

      }

   }
   return true;
}

bool
TransportClient::PendingAssoc::initiate_connect(TransportClient* tc,
      Guard& guard)
{
   //### Debug statements to track where associate is failing
   ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###TransportClient::PendingAssoc::initiate_connect\n"));


   // find the next impl / blob entry that have matching types
   while (!impls_.empty()) {
      const TransportImpl_rch& impl = impls_.back();
      const std::string type = impl->transport_type();
      for (; blob_index_ < data_.remote_data_.length(); ++blob_index_) {
         if (data_.remote_data_[blob_index_].transport_type.in() == type) {
            const TransportImpl::RemoteTransport remote = {
                  data_.remote_id_, data_.remote_data_[blob_index_].data,
                  data_.publication_transport_priority_,
                  data_.remote_reliable_, data_.remote_durable_};

            //### Debug statements to track where associate is failing
            ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###TransportClient::PendingAssoc::initiate_connect --> FOUND MATCHING TYPE\n"));

            //### Debug statements to track where associate is failing
            ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###TransportClient::PendingAssoc::initiate_connect --> connect_datalink with match\n"));

            //###some transport impl's rely on reactor call to connect which can't be done while holding lock_
            //###this AcceptConnectResult used to be const...should it still be?
            TransportImpl::AcceptConnectResult res;
            if(!tc->initiate_connect_i(res, impl, remote, attribs_, guard)){
               //tc init connect returned false there is no PendingAssoc left in map because use_datalink_i finished elsewhere
               //so don't do anything further with pend and return success or failure up to tc's associate
               if(res.success_ && !this->removed_) {
                  return true;
               }
               return false;
            }

            if (res.success_) {

               //### Debug statements to track where associate is failing
               ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###TransportClient::PendingAssoc::initiate_connect result of connect_datalink was success_\n"));

               ++blob_index_;
               if (!res.link_.is_nil()) {

                  //### Debug statements to track where associate is failing
                  ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###TransportClient::PendingAssoc::initiate_connect AcceptConnectResult link not nil so use_datalink_i\n"));

                  tc->use_datalink_i(data_.remote_id_, res.link_, guard);
               }
               //### Debug statements to track where associate is failing
               ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###TransportClient::PendingAssoc::initiate_connect SUCCEEDED\n"));
               return true;
            }
         }
      }
      impls_.pop_back();
      blob_index_ = 0;
   }
   //### Debug statements to track where associate is failing
   ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###TransportClient::PendingAssoc::initiate_connect DID NOT CONNECT\n"));
   return false;
}

void
TransportClient::use_datalink(const RepoId& remote_id,
      const DataLink_rch& link)
{

   //### Debug statements to track where connection is failing
   ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###TransportClient::use_datalink --> begin \n"));
   GuidConverter tc_repo(this->repo_id_);
   GuidConverter remote(remote_id);
   ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###TransportClient::use_datalink with this TransportClient: %C connecting to remote_id = %C\n", std::string(tc_repo).c_str(), std::string(remote).c_str()));

   //### Debug statements to track where connection is failing
   ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###TransportClient::use_datalink --> TRYING TO lock_\n"));

   ACE_GUARD(ACE_Thread_Mutex, guard, lock_);

   //### Debug statements to track where connection is failing
   ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###TransportClient::use_datalink --> GOT lock_\n"));
   GuidConverter tc_repo_post_lock(this->repo_id_);
   GuidConverter remote_post_lock(remote_id);
   ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###TransportClient::use_datalink AFTER LOCKING with this TransportClient: %C connecting to remote_id = %C\n", std::string(tc_repo_post_lock).c_str(), std::string(remote_post_lock).c_str()));

   //### Debug statements to track where connection is failing
   ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###TransportClient::use_datalink --> got the lock_ so can call use_datalink_i\n"));

   use_datalink_i(remote_id, link, guard);

   //### Debug statements to track where connection is failing
   ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###TransportClient::use_datalink --> RELEASE lock_ \n"));

   //### Debug statements to track where connection is failing
   ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###TransportClient::use_datalink --> end\n"));
}

void
TransportClient::use_datalink_i(const RepoId& remote_id_ref,
      const DataLink_rch& link,
      Guard& guard)
{
   //### Debug statements to track where associate is failing
   ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###TransportClient::use_datalink_i search pending assoc's for remote id\n"));

   //### Debugging where associate is failing
   //### try to make a local copy of remote_id to use in calls because the reference could be invalidated if the caller
   //### reference location is deleted (i.e. in stop_accepting_or_connecting if user_datalink_i was called from passive_connection)
   //### Does changing this from a reference to a local affect anything going forward?  Anything need to specifically access that memory, or
   //### do all further uses simply look up by value and modify references that way?
   RepoId remote_id(remote_id_ref);

   PendingMap::iterator iter_print = pending_.begin();
   //### Debug statements to track where associate is failing
      ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###TransportClient::use_datalink_i PRINT OUT ALL PENDING ASSOC IN PENDINGMAP\n"));
      int i = 0;
   while(iter_print != pending_.end()) {
      //### Debug statements to track where associate is failing
      GuidConverter remote_print(iter_print->first);
         ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###TransportClient::use_datalink_i PendingAssoc #%d is for RepoId: %C\n", i, std::string(remote_print).c_str()));
         iter_print++;
         i++;
   }


   PendingMap::iterator iter = pending_.find(remote_id);
   if (iter == pending_.end()) {
      //### Debug statements to track where associate is failing
      ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###TransportClient::use_datalink_i didn't find a PendingAssoc for remote_id\n"));
      return;
   }

   //### Debug statements to track where associate is failing
   ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###TransportClient::use_datalink_i found pending assoc for remote id\n"));


   PendingAssoc& pend = iter->second;
   const int active_flag = pend.active_ ? ASSOC_ACTIVE : 0;
   bool ok = false;

   if (pend.removed_) { // no-op
      //### Debug statements to track where associate is failing
      ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###TransportClient::use_datalink_i pend was removed previously\n"));
   } else if (link.is_nil()) {
      //### Debug statements to track where associate is failing
      ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###TransportClient::use_datalink_i pend's link is nil try to initiate_connect if active(%b)\n", pend.active_));
      if (pend.active_ && pend.initiate_connect(this, guard)) {
         return;
      }

   } else { // link is ready to use
      //### Debug statements to track where associate is failing
      ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###TransportClient::use_datalink_i pend's link should be ready to use, add_link\n"));
      add_link(link, remote_id);
      ok = true;
   }

   // either link is valid or assoc failed, clean up pending object
   // for passive side processing
   if (!pend.active_) {
      //### Debug statements to track where associate is failing
      ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###TransportClient::use_datalink_i clean up pend calling stop_accepting_or_connecting on each impls_\n"));
      for (size_t i = 0; i < pend.impls_.size(); ++i) {
         pend.impls_[i]->stop_accepting_or_connecting(this, pend.data_.remote_id_);
      }
   }

   //### Debug statements to track where associate is failing
   ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###TransportClient::use_datalink_i get ServiceParticipant's timer (reactor)\n"));

   //### Get a pointer to singleton to check it exists below
   Service_Participant * this_serv_part = TheServiceParticipant->instance();

   //### this if/else is all debugging, can be deleted
   if (this_serv_part == 0) {
      //### Debug statements to track where associate is failing
      ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###TransportClient::use_datalink_i Service Participant == 0\n"));
   } else {
      //### Debug statements to track where associate is failing
      ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###TransportClient::use_datalink_i Service Participant != 0\n"));
   }

   ACE_Reactor_Timer_Interface* timer = TheServiceParticipant->timer();


   //### this if is debugging only, can be deleted
   if (timer == 0) {
      //### Debug statements to track where associate is failing
      ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###TransportClient::use_datalink_i timer == 0\n"));
   }

   //### Debug statements to track where associate is failing
   ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###TransportClient::use_datalink_i cancel time for pend\n"));
   //### shouldn't really do this, timer should never be 0, right?
   if (timer != 0)
     timer->cancel_timer(&pend);

   //### Debug statements to track where associate is failing
   ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###TransportClient::use_datalink_i canceled timer, now delete iter which is pend\n"));

   pending_.erase(iter);

   //### Debug statements to track where associate is failing
   ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###TransportClient::use_datalink_i search iter(which is pend) successfully erased\n"));

   guard.release();
   //### Debug statements to track where connection is failing
   ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###TransportClient::use_datalink_i --> RELEASE lock_ guard.release() ??? \n"));


   //### Debug statements to track where associate is failing
   ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###TransportClient::use_datalink_i guard released about to call transport_assoc_done  \n"));
   ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###TransportClient::use_datalink_i guard calling transport_assoc_done with active_flag = %s \n", pend.active_ ? "ASSOC_ACTIVE" : "0"));
   ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###TransportClient::use_datalink_i guard calling transport_assoc_done with ok ? = %s \n", ok ? "ASSOC_OK" : "0"));
   GuidConverter remote(remote_id);
   ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###TransportClient::use_datalink_i guard calling transport_assoc_done with remote_id = %C\n", std::string(remote).c_str()));

   transport_assoc_done(active_flag | (ok ? ASSOC_OK : 0), remote_id);

   //### Debug statements to track where associate is failing
   ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###TransportClient::use_datalink_i exiting function call\n"));
}

void
TransportClient::add_link(const DataLink_rch& link, const RepoId& peer)
{
   links_.insert_link(link.in());
   data_link_index_[peer] = link;

   TransportReceiveListener* trl = get_receive_listener();
   if (trl) {
      link->make_reservation(peer, repo_id_, trl);
   } else {
      link->make_reservation(peer, repo_id_, get_send_listener());
   }
}

void
TransportClient::disassociate(const RepoId& peerId)
{
   //### Debug statements to track where connection is failing
   ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###TransportClient::disassociate --> TRYING TO lock_\n"));

   ACE_GUARD(ACE_Thread_Mutex, guard, lock_);

   //### Debug statements to track where connection is failing
   ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###TransportClient::disassociate --> GOT lock_\n"));

   const PendingMap::iterator iter = pending_.find(peerId);
   if (iter != pending_.end()) {
      iter->second.removed_ = true;
      return;
   }

   const DataLinkIndex::iterator found = data_link_index_.find(peerId);
   if (found == data_link_index_.end()) {
      if (DCPS_debug_level > 4) {
         const GuidConverter converter(peerId);
         ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) TransportClient::disassociate: ")
               ACE_TEXT("no link for remote peer %C\n"),
               std::string(converter).c_str()));
      }

      //### Debug statements to track where connection is failing
      ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###TransportClient::disassociate --> RELEASE lock_ \n"));
      return;
   }

   const DataLink_rch link = found->second;
   DataLinkSetMap released;
   link->release_reservations(peerId, repo_id_, released);
   data_link_index_.erase(found);

   if (!released.empty()) {
      // Datalink is no longer used for any remote peer
      link->remove_listener(repo_id_);
      links_.remove_link(link);
   }

   //### Debug statements to track where connection is failing
   ACE_DEBUG((LM_DEBUG, "(%P|%t|%T) ###TransportClient::disassociate --> RELEASE lock_ \n"));
}

bool
TransportClient::send_response(const RepoId& peer,
      const DataSampleHeader& header,
      ACE_Message_Block* payload)
{
   DataLinkIndex::iterator found = data_link_index_.find(peer);
   if (found == data_link_index_.end()) {
      payload->release();
      if (DCPS_debug_level > 4) {
         GuidConverter converter(peer);
         ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) TransportClient::send_response: ")
               ACE_TEXT("no link for publication %C, ")
               ACE_TEXT("not sending response.\n"),
               std::string(converter).c_str()));
      }
      return false;
   }

   DataLinkSet singular;
   singular.insert_link(found->second.in());
   singular.send_response(peer, header, payload);
   return true;
}

void
TransportClient::send(const DataSampleList& samples)
{
   DataSampleListElement* cur = samples.head_;

   while (cur) {
      // VERY IMPORTANT NOTE:
      //
      // We have to be very careful in how we deal with the current
      // DataSampleListElement.  The issue is that once we have invoked
      // data_delivered() on the send_listener_ object, or we have invoked
      // send() on the pub_links, we can no longer access the current
      // DataSampleListElement!  Thus, we need to get the next
      // DataSampleListElement (pointer) from the current element now,
      // while it is safe.
      DataSampleListElement* next_elem = cur->next_send_sample_;
      DataLinkSet_rch pub_links =
            (cur->num_subs_ > 0)
            ? links_.select_links(cur->subscription_ids_, cur->num_subs_)
                  : DataLinkSet_rch(&links_, false);

      if (pub_links.is_nil() || pub_links->empty()) {
         // NOTE: This is the "local publisher id is not currently
         //       associated with any remote subscriber ids" case.

         if (DCPS_debug_level > 4) {
            GuidConverter converter(cur->publication_id_);
            ACE_DEBUG((LM_DEBUG,
                  ACE_TEXT("(%P|%t) TransportClient::send: ")
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

#ifndef OPENDDS_NO_CONTENT_SUBSCRIPTION_PROFILE
         // Content-Filtering adjustment to the pub_links:
         // - If the sample should be filtered out of all subscriptions on a given
         //   DataLink, then exclude that link from the subset that we'll send to.
         // - If the sample should be filtered out of some (or none) of the subs,
         //   then record that information in the DataSampleListElement so that the
         //   header's content_filter_entries_ can be marshaled before it's sent.
         if (cur->filter_out_.ptr()) {
            DataLinkSet_rch subset;
            DataLinkSet::GuardType guard(pub_links->lock());
            typedef DataLinkSet::MapType MapType;
            MapType& map = pub_links->map();
            for (MapType::iterator itr = map.begin(); itr != map.end(); ++itr) {
               size_t n_subs;
               GUIDSeq_var ti =
                     itr->second->target_intersection(cur->publication_id_,
                           cur->filter_out_, n_subs);
               if (ti.ptr() == 0 || ti->length() != n_subs) {
                  if (!subset.in()) {
                     subset = new DataLinkSet;
                  }
                  subset->insert_link(itr->second.in());
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
               cur->send_listener_->data_delivered(cur);
               cur = next_elem;
               continue;
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
         send_links_.send_start(pub_links.in());
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

   send_links_.send_stop();
}

TransportSendListener*
TransportClient::get_send_listener()
{
   return dynamic_cast<TransportSendListener*>(this);
}

TransportReceiveListener*
TransportClient::get_receive_listener()
{
   return dynamic_cast<TransportReceiveListener*>(this);
}

SendControlStatus
TransportClient::send_control(const DataSampleHeader& header,
      ACE_Message_Block* msg,
      void* extra /* = 0*/)
{
   TransportSendListener* listener = get_send_listener();

   if (extra) {
      DataLinkSet_rch pub_links(&links_, false);
      return listener->send_control_customized(pub_links, header, msg, extra);

   } else {
      return links_.send_control(repo_id_, listener, header, msg);
   }
}

bool
TransportClient::remove_sample(const DataSampleListElement* sample)
{
   return links_.remove_sample(sample);
}

bool
TransportClient::remove_all_msgs()
{
   return links_.remove_all_msgs(repo_id_);
}

}
}
