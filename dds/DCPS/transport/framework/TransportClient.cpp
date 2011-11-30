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
#include "DataLinkSetMap.h"

#include "dds/DdsDcpsDataReaderRemoteC.h"
#include "dds/DdsDcpsDataWriterRemoteC.h"

#include "dds/DCPS/DataWriterImpl.h"
#include "dds/DCPS/DataSampleList.h"
#include "dds/DCPS/RepoIdConverter.h"
#include "dds/DCPS/AssociationData.h"

#include "ace/Reverse_Lock_T.h"

namespace OpenDDS {
namespace DCPS {

TransportClient::TransportClient()
{
}

TransportClient::~TransportClient()
{
  if (Transport_debug_level > 5) {
    RepoIdConverter converter(repo_id_);
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) TransportClient::~TransportClient: %C\n"),
               std::string(converter).c_str()));
  }

  for (DataLinkSet::MapType::iterator iter = links_.map().begin();
       iter != links_.map().end(); ++iter) {
    iter->second->remove_listener(repo_id_);
  }

  for (std::vector<TransportImpl_rch>::iterator it = impls_.begin();
       it != impls_.end(); ++it) {
    (*it)->detach_client(this);
  }
}

void
TransportClient::enable_transport(bool reliable)
{
  EntityImpl* ent = dynamic_cast<EntityImpl*>(this);

  TransportConfig_rch tc = ent ? ent->transport_config() : 0;
  for (EntityImpl* p = ent ? ent->parent() : 0;
       p && tc.is_nil(); p = p->parent()) {
    tc = p->transport_config();
  }

  if (tc.is_nil()) {
    TransportRegistry* reg = TransportRegistry::instance();
    tc = reg->global_config();
    if (!tc.is_nil() && tc->instances_.empty()
        && tc->name() == TransportRegistry::DEFAULT_CONFIG_NAME) {
      tc = reg->fix_empty_default();
    }
  }

  if (tc.is_nil()) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: TransportClient::enable_transport ")
               ACE_TEXT("No TransportConfig found.\n")));
    throw Transport::NotConfigured();
  }

  swap_bytes_ = tc->swap_bytes_;
  cdr_encapsulation_ = false;
  reliable_ = reliable;
  passive_connect_duration_.set(tc->passive_connect_duration_ / 1000,
                                (tc->passive_connect_duration_ % 1000) * 1000);

  const size_t n = tc->instances_.size();
  for (size_t i = 0; i < n; ++i) {
    TransportInst_rch inst = tc->instances_[i];
    if (check_transport_qos(*inst.in())) {
      TransportImpl_rch impl = inst->impl();
      impl->attach_client(this);
      impls_.push_back(impl);
      const CORBA::ULong len = conn_info_.length();
      conn_info_.length(len + 1);
      impl->connection_info(conn_info_[len]);
      cdr_encapsulation_ |= inst->requires_cdr();
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
TransportClient::transport_detached(TransportImpl* which)
{
  ACE_GUARD(ACE_Thread_Mutex, guard, lock_);

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
      return;
    }
  }
}

bool
TransportClient::associate(const AssociationData& data, bool active)
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, lock_, false);
  repo_id_ = get_repo_id();
  const TransportImpl::ConnectionAttribs attribs =
    {get_priority_value(data), reliable_};

  MultiReservLock mrl(impls_);
  ACE_GUARD_RETURN(MultiReservLock, guard2, mrl, false);

  // Attempt to find an existing DataLink that can be reused
  for (size_t i = 0; i < impls_.size(); ++i) {

    DataLink_rch link =
      impls_[i]->find_datalink(repo_id_, data, attribs, active);
    if (!link.is_nil()) {
      add_link(link, data.remote_id_);
      return true;
    }
  }

  // Create a new DataLink
  if (active) {

    for (size_t i = 0; i < impls_.size(); ++i) {
      DataLink_rch link = impls_[i]->connect_datalink(repo_id_, data, attribs);
      if (!link.is_nil()) {
        add_link(link, data.remote_id_);
        return true;
      }
    }
  } else { // passive

    TransportImpl::ConnectionEvent ce(repo_id_, data, attribs);
    for (size_t i = 0; i < impls_.size(); ++i) {
      DataLink_rch link = impls_[i]->accept_datalink(ce);
      if (!link.is_nil()) {
        ce.link_ = link;
        break;
      }
    }
    if (ce.link_.is_nil()) {
      // Reservation lock must be released for the loopback case, since the peer
      // may be one of these impls, and it may need to make progress on the
      // active side in order for this wait to complete.
      ACE_Reverse_Lock<MultiReservLock> rev(mrl);
      ACE_GUARD_RETURN(ACE_Reverse_Lock<MultiReservLock>, rguard, rev, false);

      ce.wait(passive_connect_duration_);
    }
    for (size_t i = 0; i < impls_.size(); ++i) {
      if (ce.link_.is_nil() || impls_[i].in() != ce.link_->impl().in()) {
        impls_[i]->stop_accepting(ce);
      }
    }
    if (!ce.link_.is_nil()) {
      add_link(ce.link_, data.remote_id_);
      return true;
    }
  }

  return false;
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
  ACE_GUARD(ACE_Thread_Mutex, guard, lock_);

  DataLinkIndex::iterator found = data_link_index_.find(peerId);
  if (found == data_link_index_.end()) {
    if (DCPS_debug_level > 4) {
      RepoIdConverter converter(peerId);
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) TransportClient::disassociate: ")
                 ACE_TEXT("no link for remote peer %C\n"),
                 std::string(converter).c_str()));
    }
    return;
  }

  DataLink_rch link = found->second;

  MultiReservLock mrl(impls_);
  ACE_GUARD(MultiReservLock, guard2, mrl);

  DataLinkSetMap released;
  link->release_reservations(peerId, repo_id_, released);
  data_link_index_.erase(found);

  if (!released.empty()) {
    // Datalink is no longer used for any remote peer
    link->remove_listener(repo_id_);
    links_.remove_link(link);
  }
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
      RepoIdConverter converter(peer);
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
        RepoIdConverter converter(cur->publication_id_);
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
          GUIDSeq_var ti = itr->second->target_intersection(cur->filter_out_);
          if (ti.ptr() == 0 || ti->length() != itr->second->num_targets()) {
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


// MultiReservLock nested class


TransportClient::MultiReservLock::MultiReservLock(
  const std::vector<TransportImpl_rch>& impls)
  : impls_(impls)
{
  for (size_t i = 0; i < impls_.size(); ++i) {
    sorted_.insert(impls_[i].in());
  }
}

int
TransportClient::MultiReservLock::action_fwd(
  TransportClient::MultiReservLock::PMF function,
  TransportClient::MultiReservLock::PMF undo)
{
  typedef std::set<TransportImpl*>::iterator iter_t;
  for (iter_t iter = sorted_.begin(); iter != sorted_.end(); ++iter) {
    int ret = ((*iter)->reservation_lock().*function)();
    if (ret != 0) {
      while (iter != sorted_.begin()) {
        ((*--iter)->reservation_lock().*undo)();
      }
      return ret;
    }
  }
  return 0;
}

int
TransportClient::MultiReservLock::action_rev(
  TransportClient::MultiReservLock::PMF function)
{
  int ret = 0;
  typedef std::set<TransportImpl*>::reverse_iterator iter_t;
  for (iter_t iter = sorted_.rbegin(); iter != sorted_.rend(); ++iter) {
    ret += ((*iter)->reservation_lock().*function)();
  }
  return ret;
}

int
TransportClient::MultiReservLock::acquire()
{
  typedef TransportImpl::ReservationLockType Lock;
  return action_fwd(&Lock::acquire, &Lock::release);
}

int
TransportClient::MultiReservLock::tryacquire()
{
  typedef TransportImpl::ReservationLockType Lock;
  return action_fwd(&Lock::tryacquire, &Lock::release);
}

int
TransportClient::MultiReservLock::release()
{
  return action_rev(&TransportImpl::ReservationLockType::release);
}

int
TransportClient::MultiReservLock::remove()
{
  return action_rev(&TransportImpl::ReservationLockType::remove);
}

}
}
