/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "RtpsUdpDataLink.h"
#include "RtpsUdpTransport.h"
#include "RtpsUdpInst.h"
#include "RtpsUdpSendStrategy.h"
#include "RtpsUdpReceiveStrategy.h"

#include "dds/DCPS/transport/framework/TransportCustomizedElement.h"
#include "dds/DCPS/transport/framework/TransportSendElement.h"
#include "dds/DCPS/transport/framework/TransportSendControlElement.h"
#include "dds/DCPS/transport/framework/NetworkAddress.h"

#include "dds/DCPS/RTPS/RtpsCoreTypeSupportImpl.h"
#include "dds/DCPS/RTPS/BaseMessageUtils.h"
#include "dds/DCPS/RTPS/BaseMessageTypes.h"
#include "dds/DCPS/RTPS/MessageTypes.h"

#include "ace/Default_Constants.h"
#include "ace/Log_Msg.h"
#include "ace/Message_Block.h"
#include "ace/Reverse_Lock_T.h"
#include "ace/Reactor.h"

#include <string.h>

#ifndef __ACE_INLINE__
# include "RtpsUdpDataLink.inl"
#endif  /* __ACE_INLINE__ */

namespace {

/// Return the number of CORBA::Longs required for the bitmap representation of
/// sequence numbers between low and high, inclusive (maximum 8 longs).
CORBA::ULong
bitmap_num_longs(const OpenDDS::DCPS::SequenceNumber& low,
                 const OpenDDS::DCPS::SequenceNumber& high)
{
  return std::min(CORBA::ULong(8),
                  CORBA::ULong((high.getValue() - low.getValue() + 32) / 32));
}

}

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

RtpsUdpDataLink::RtpsUdpDataLink(const RtpsUdpTransport_rch& transport,
                                 const GuidPrefix_t& local_prefix,
                                 const RtpsUdpInst_rch& config,
                                 const TransportReactorTask_rch& reactor_task)
  : DataLink(transport, // 3 data link "attributes", below, are unused
             0,         // priority
             false,     // is_loopback
             false),    // is_active
    config_(config),
    reactor_task_(reactor_task),
    send_strategy_(make_rch<RtpsUdpSendStrategy>(this, config, local_prefix)),
    recv_strategy_(make_rch<RtpsUdpReceiveStrategy>(this, local_prefix)),
    rtps_customized_element_allocator_(40, sizeof(RtpsCustomizedElement)),
    multi_buff_(this, config->nak_depth_),
    best_effort_heartbeat_count_(0),
    nack_reply_(this, &RtpsUdpDataLink::send_nack_replies,
                config->nak_response_delay_),
    heartbeat_reply_(this, &RtpsUdpDataLink::send_heartbeat_replies,
                     config->heartbeat_response_delay_),
  heartbeat_(make_rch<HeartBeat>(reactor_task->get_reactor(), reactor_task->get_reactor_owner(), this, &RtpsUdpDataLink::send_heartbeats)),
  heartbeatchecker_(make_rch<HeartBeat>(reactor_task->get_reactor(), reactor_task->get_reactor_owner(), this, &RtpsUdpDataLink::check_heartbeats))
{
  std::memcpy(local_prefix_, local_prefix, sizeof(GuidPrefix_t));
}

RtpsUdpInst_rch
RtpsUdpDataLink::config() const
{
  return config_;
}

bool
RtpsUdpDataLink::add_delayed_notification(TransportQueueElement* element)
{
  RtpsWriterMap::iterator iter = writers_.find(element->publication_id());
  if (iter != writers_.end()) {

    iter->second.add_elem_awaiting_ack(element);
    return true;
  }
  return false;
}

void RtpsUdpDataLink::do_remove_sample(const RepoId& pub_id,
  const TransportQueueElement::MatchCriteria& criteria)
{
  RtpsWriter::SnToTqeMap sn_tqe_map;
  RtpsWriter::SnToTqeMap to_deliver;
  typedef RtpsWriter::SnToTqeMap::iterator iter_t;

  {
    ACE_GUARD(ACE_Thread_Mutex, g, lock_);

    RtpsWriterMap::iterator iter = writers_.find(pub_id);
    if (iter != writers_.end() && !iter->second.elems_not_acked_.empty()) {
      to_deliver.insert(iter->second.to_deliver_.begin(), iter->second.to_deliver_.end());
      iter->second.to_deliver_.clear();
      iter_t it = iter->second.elems_not_acked_.begin();
      OPENDDS_SET(SequenceNumber) sns_to_release;
      while (it != iter->second.elems_not_acked_.end()) {
        if (criteria.matches(*it->second)) {
          sn_tqe_map.insert(RtpsWriter::SnToTqeMap::value_type(it->first, it->second));
          sns_to_release.insert(it->first);
          iter_t last = it;
          ++it;
          iter->second.elems_not_acked_.erase(last);
        } else {
          ++it;
        }
      }
      OPENDDS_SET(SequenceNumber)::iterator sns_it = sns_to_release.begin();
      while (sns_it != sns_to_release.end()) {
        iter->second.send_buff_->release_acked(*sns_it);
        ++sns_it;
      }
    }
  }
  iter_t deliver_iter = to_deliver.begin();
  while (deliver_iter != to_deliver.end()) {
    deliver_iter->second->data_delivered();
    ++deliver_iter;
  }
  iter_t drop_iter = sn_tqe_map.begin();
  while (drop_iter != sn_tqe_map.end()) {
    drop_iter->second->data_dropped(true);
    ++drop_iter;
  }
}

bool
RtpsUdpDataLink::open(const ACE_SOCK_Dgram& unicast_socket)
{
  unicast_socket_ = unicast_socket;

  if (config_->use_multicast_) {
    const OPENDDS_STRING& net_if = config_->multicast_interface_;
#ifdef ACE_HAS_MAC_OSX
    multicast_socket_.opts(ACE_SOCK_Dgram_Mcast::OPT_BINDADDR_NO |
                           ACE_SOCK_Dgram_Mcast::DEFOPT_NULLIFACE);
#endif
    if (multicast_socket_.join(config_->multicast_group_address_, 1,
                               net_if.empty() ? 0 :
                               ACE_TEXT_CHAR_TO_TCHAR(net_if.c_str())) != 0) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) ERROR: ")
                        ACE_TEXT("RtpsUdpDataLink::open: ")
                        ACE_TEXT("ACE_SOCK_Dgram_Mcast::join failed: %m\n")),
                       false);
    }
  }

  if (!OpenDDS::DCPS::set_socket_multicast_ttl(unicast_socket_, config_->ttl_)) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("RtpsUdpDataLink::open: ")
                      ACE_TEXT("failed to set TTL: %d\n"),
                      config_->ttl_),
                     false);
  }

  send_strategy_->send_buffer(&multi_buff_);

  if (start(static_rchandle_cast<TransportSendStrategy>(send_strategy_),
            static_rchandle_cast<TransportStrategy>(recv_strategy_)) != 0) {
    stop_i();
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("UdpDataLink::open: start failed!\n")),
                     false);
  }

  return true;
}

void
RtpsUdpDataLink::add_locator(const RepoId& remote_id,
                             const ACE_INET_Addr& address,
                             bool requires_inline_qos)
{
  ACE_GUARD(ACE_Thread_Mutex, g, lock_);
  locators_[remote_id] = RemoteInfo(address, requires_inline_qos);
}

void
RtpsUdpDataLink::get_locators(const RepoId& local_id,
                              OPENDDS_SET(ACE_INET_Addr)& addrs) const
{
  typedef OPENDDS_MAP_CMP(RepoId, RemoteInfo, GUID_tKeyLessThan)::const_iterator iter_t;

  if (local_id == GUID_UNKNOWN) {
    for (iter_t iter = locators_.begin(); iter != locators_.end(); ++iter) {
      addrs.insert(iter->second.addr_);
    }
    return;
  }

  const GUIDSeq_var peers = peer_ids(local_id);
  if (!peers.ptr()) {
    return;
  }
  for (CORBA::ULong i = 0; i < peers->length(); ++i) {
    const ACE_INET_Addr addr = get_locator(peers[i]);
    if (addr != ACE_INET_Addr()) {
      addrs.insert(addr);
    }
  }
}

ACE_INET_Addr
RtpsUdpDataLink::get_locator(const RepoId& remote_id) const
{
  typedef OPENDDS_MAP_CMP(RepoId, RemoteInfo, GUID_tKeyLessThan)::const_iterator iter_t;
  const iter_t iter = locators_.find(remote_id);
  if (iter == locators_.end()) {
    const GuidConverter conv(remote_id);
    ACE_DEBUG((LM_ERROR, "(%P|%t) RtpsUdpDataLink::get_locator_i() - "
      "no locator found for peer %C\n", OPENDDS_STRING(conv).c_str()));
    return ACE_INET_Addr();
  }
  return iter->second.addr_;
}

void
RtpsUdpDataLink::associated(const RepoId& local_id, const RepoId& remote_id,
                            bool local_reliable, bool remote_reliable,
                            bool local_durable, bool remote_durable)
{
  if (!local_reliable) {
    return;
  }

  bool enable_heartbeat = false;

  ACE_GUARD(ACE_Thread_Mutex, g, lock_);
  const GuidConverter conv(local_id);
  const EntityKind kind = conv.entityKind();
  if (kind == KIND_WRITER && remote_reliable) {
    // Insert count if not already there.
    heartbeat_counts_.insert(HeartBeatCountMapType::value_type(local_id, 0));
    RtpsWriter& w = writers_[local_id];
    w.remote_readers_[remote_id].durable_ = remote_durable;
    w.durable_ = local_durable;
    enable_heartbeat = true;

  } else if (kind == KIND_READER) {
    RtpsReaderMap::iterator rr = readers_.find(local_id);
    if (rr == readers_.end()) {
      rr = readers_.insert(RtpsReaderMap::value_type(local_id, RtpsReader()))
        .first;
      rr->second.durable_ = local_durable;
    }
    rr->second.remote_writers_[remote_id];
    reader_index_.insert(RtpsReaderIndex::value_type(remote_id, rr));
  }

  g.release();
  if (enable_heartbeat) {
    heartbeat_->schedule_enable();
  }
}

bool
RtpsUdpDataLink::check_handshake_complete(const RepoId& local_id,
                                          const RepoId& remote_id)
{
  const GuidConverter conv(local_id);
  const EntityKind kind = conv.entityKind();
  if (kind == KIND_WRITER) {
    RtpsWriterMap::iterator rw = writers_.find(local_id);
    if (rw == writers_.end()) {
      return true; // not reliable, no handshaking
    }
    ReaderInfoMap::iterator ri = rw->second.remote_readers_.find(remote_id);
    if (ri == rw->second.remote_readers_.end()) {
      return true; // not reliable, no handshaking
    }
    return ri->second.handshake_done_;

  } else if (kind == KIND_READER) {
    return true; // no handshaking for local reader
  }
  return false;
}

void
RtpsUdpDataLink::register_for_reader(const RepoId& writerid,
                                     const RepoId& readerid,
                                     const ACE_INET_Addr& address,
                                     OpenDDS::DCPS::DiscoveryListener* listener)
{
  ACE_GUARD(ACE_Thread_Mutex, g, lock_);
  bool enableheartbeat = interesting_readers_.empty();
  interesting_readers_.insert(InterestingRemoteMapType::value_type(readerid, InterestingRemote(writerid, address, listener)));
  heartbeat_counts_[writerid] = 0;
  g.release();
  if (enableheartbeat) {
    heartbeat_->schedule_enable();
  }
}

void
RtpsUdpDataLink::unregister_for_reader(const RepoId& writerid,
                                       const RepoId& readerid)
{
  OPENDDS_VECTOR(CallbackType) to_notify;
  {
    ACE_GUARD(ACE_Thread_Mutex, c, reader_no_longer_exists_lock_);
    to_notify.swap(readerDoesNotExistCallbacks_);
  }
  OPENDDS_VECTOR(CallbackType)::iterator iter = to_notify.begin();
  while(iter != to_notify.end()) {
    const RepoId& rid = iter->first;
    const InterestingRemote& remote = iter->second;
    remote.listener->reader_does_not_exist(rid, remote.localid);
    ++iter;
  }
  ACE_GUARD(ACE_Thread_Mutex, g, lock_);
  for (InterestingRemoteMapType::iterator pos = interesting_readers_.lower_bound(readerid),
         limit = interesting_readers_.upper_bound(readerid);
       pos != limit;
       ) {
    if (pos->second.localid == writerid) {
      interesting_readers_.erase(pos++);
    } else {
      ++pos;
    }
  }
}

void
RtpsUdpDataLink::register_for_writer(const RepoId& readerid,
                                     const RepoId& writerid,
                                     const ACE_INET_Addr& address,
                                     OpenDDS::DCPS::DiscoveryListener* listener)
{
  ACE_GUARD(ACE_Thread_Mutex, g, lock_);
  bool enableheartbeatchecker = interesting_writers_.empty();
  interesting_writers_.insert(InterestingRemoteMapType::value_type(writerid, InterestingRemote(readerid, address, listener)));
  g.release();
  if (enableheartbeatchecker) {
    heartbeatchecker_->schedule_enable();
  }
}

void
RtpsUdpDataLink::unregister_for_writer(const RepoId& readerid,
                                       const RepoId& writerid)
{
  OPENDDS_VECTOR(CallbackType) to_notify;
  {
    ACE_GUARD(ACE_Thread_Mutex, c, writer_no_longer_exists_lock_);
    to_notify.swap(writerDoesNotExistCallbacks_);
  }
  OPENDDS_VECTOR(CallbackType)::iterator iter = to_notify.begin();
  while(iter != to_notify.end()) {
    const RepoId& rid = iter->first;
    const InterestingRemote& remote = iter->second;
    remote.listener->writer_does_not_exist(rid, remote.localid);
    ++iter;
  }
  ACE_GUARD(ACE_Thread_Mutex, g, lock_);
  for (InterestingRemoteMapType::iterator pos = interesting_writers_.lower_bound(writerid),
         limit = interesting_writers_.upper_bound(writerid);
       pos != limit;
       ) {
    if (pos->second.localid == readerid) {
      interesting_writers_.erase(pos++);
    } else {
      ++pos;
    }
  }
}

void
RtpsUdpDataLink::pre_stop_i()
{
  DBG_ENTRY_LVL("RtpsUdpDataLink","pre_stop_i",6);
  DataLink::pre_stop_i();
  OPENDDS_VECTOR(TransportQueueElement*) to_deliver;
  OPENDDS_VECTOR(TransportQueueElement*) to_drop;
  {
    ACE_GUARD(ACE_Thread_Mutex, g, lock_);

    typedef OPENDDS_MULTIMAP(SequenceNumber, TransportQueueElement*)::iterator iter_t;

    RtpsWriterMap::iterator iter = writers_.begin();
    while (iter != writers_.end()) {
      RtpsWriter& writer = iter->second;
      if (!writer.to_deliver_.empty()) {
        iter_t iter = writer.to_deliver_.begin();
        while (iter != writer.to_deliver_.end()) {
          to_deliver.push_back(iter->second);
          writer.to_deliver_.erase(iter);
          iter = writer.to_deliver_.begin();
        }
      }
      if (!writer.elems_not_acked_.empty()) {
        OPENDDS_SET(SequenceNumber) sns_to_release;
        iter_t iter = writer.elems_not_acked_.begin();
        while (iter != writer.elems_not_acked_.end()) {
          to_drop.push_back(iter->second);
          sns_to_release.insert(iter->first);
          writer.elems_not_acked_.erase(iter);
          iter = writer.elems_not_acked_.begin();
        }
        OPENDDS_SET(SequenceNumber)::iterator sns_it = sns_to_release.begin();
        while (sns_it != sns_to_release.end()) {
          writer.send_buff_->release_acked(*sns_it);
          ++sns_it;
        }
      }
      RtpsWriterMap::iterator last = iter;
      ++iter;
      heartbeat_counts_.erase(last->first);
      writers_.erase(last);
    }
  }
  typedef OPENDDS_VECTOR(TransportQueueElement*)::iterator tqe_iter;
  tqe_iter deliver_it = to_deliver.begin();
  while (deliver_it != to_deliver.end()) {
    (*deliver_it)->data_delivered();
    ++deliver_it;
  }
  tqe_iter drop_it = to_drop.begin();
  while (drop_it != to_drop.end()) {
    (*drop_it)->data_dropped(true);
    ++drop_it;
  }
}

void
RtpsUdpDataLink::send_i(TransportQueueElement* element, bool relink)
{
  // Lock here to maintain the locking order:
  // RtpsUdpDataLink before RtpsUdpSendStrategy
  // which is required for resending due to nacks
  ACE_GUARD(ACE_Thread_Mutex, g, lock_);
  DataLink::send_i(element, relink);
}

void
RtpsUdpDataLink::release_reservations_i(const RepoId& remote_id,
                                        const RepoId& local_id)
{
  OPENDDS_VECTOR(TransportQueueElement*) to_deliver;
  OPENDDS_VECTOR(TransportQueueElement*) to_drop;
  ACE_GUARD(ACE_Thread_Mutex, g, lock_);
  using std::pair;
  const GuidConverter conv(local_id);
  const EntityKind kind = conv.entityKind();
  if (kind == KIND_WRITER) {
    const RtpsWriterMap::iterator rw = writers_.find(local_id);

    if (rw != writers_.end()) {
      rw->second.remote_readers_.erase(remote_id);

      if (rw->second.remote_readers_.empty()) {
        RtpsWriter& writer = rw->second;
        typedef OPENDDS_MULTIMAP(SequenceNumber, TransportQueueElement*)::iterator iter_t;

        if (!writer.to_deliver_.empty()) {
          iter_t iter = writer.to_deliver_.begin();
          while (iter != writer.to_deliver_.end()) {
            to_deliver.push_back(iter->second);
            writer.to_deliver_.erase(iter);
            iter = writer.to_deliver_.begin();
          }
        }
        if (!writer.elems_not_acked_.empty()) {
          OPENDDS_SET(SequenceNumber) sns_to_release;
          iter_t iter = writer.elems_not_acked_.begin();
          while (iter != writer.elems_not_acked_.end()) {
            to_drop.push_back(iter->second);
            sns_to_release.insert(iter->first);
            writer.elems_not_acked_.erase(iter);
            iter = writer.elems_not_acked_.begin();
          }
          OPENDDS_SET(SequenceNumber)::iterator sns_it = sns_to_release.begin();
          while (sns_it != sns_to_release.end()) {
            writer.send_buff_->release_acked(*sns_it);
            ++sns_it;
          }
        }
        heartbeat_counts_.erase(rw->first);
        writers_.erase(rw);
      }
    }

  } else if (kind == KIND_READER) {
    const RtpsReaderMap::iterator rr = readers_.find(local_id);

    if (rr != readers_.end()) {
      rr->second.remote_writers_.erase(remote_id);

      for (pair<RtpsReaderIndex::iterator, RtpsReaderIndex::iterator> iters =
             reader_index_.equal_range(remote_id);
           iters.first != iters.second;) {
        if (iters.first->second == rr) {
          reader_index_.erase(iters.first++);
        } else {
          ++iters.first;
        }
      }

      if (rr->second.remote_writers_.empty()) {
        readers_.erase(rr);
      }
    }
  }
  g.release();
  typedef OPENDDS_VECTOR(TransportQueueElement*)::iterator tqe_iter;
  tqe_iter deliver_it = to_deliver.begin();
  while (deliver_it != to_deliver.end()) {
    (*deliver_it)->data_delivered();
    ++deliver_it;
  }
  tqe_iter drop_it = to_drop.begin();
  while (drop_it != to_drop.end()) {
    (*drop_it)->data_dropped(true);
    ++drop_it;
  }
}

void
RtpsUdpDataLink::stop_i()
{
  nack_reply_.cancel();
  heartbeat_reply_.cancel();
  heartbeat_->disable();
  unicast_socket_.close();
  multicast_socket_.close();
}


// Implementing MultiSendBuffer nested class

void
RtpsUdpDataLink::MultiSendBuffer::retain_all(RepoId pub_id)
{
  ACE_GUARD(ACE_Thread_Mutex, g, outer_->lock_);
  const RtpsWriterMap::iterator wi = outer_->writers_.find(pub_id);
  if (wi != outer_->writers_.end() && !wi->second.send_buff_.is_nil()) {
    wi->second.send_buff_->retain_all(pub_id);
  }
}

void
RtpsUdpDataLink::MultiSendBuffer::insert(SequenceNumber /*transport_seq*/,
                                         TransportSendStrategy::QueueType* q,
                                         ACE_Message_Block* chain)
{
  // Called from TransportSendStrategy::send_packet().
  // RtpsUdpDataLink is already locked.
  const TransportQueueElement* const tqe = q->peek();
  const SequenceNumber seq = tqe->sequence();
  if (seq == SequenceNumber::SEQUENCENUMBER_UNKNOWN()) {
    return;
  }

  const RepoId pub_id = tqe->publication_id();

  const RtpsWriterMap::iterator wi = outer_->writers_.find(pub_id);
  if (wi == outer_->writers_.end()) {
    return; // this datawriter is not reliable
  }

  RcHandle<SingleSendBuffer>& send_buff = wi->second.send_buff_;

  if (send_buff.is_nil()) {
    send_buff = make_rch<SingleSendBuffer>(SingleSendBuffer::UNLIMITED, 1 /*mspp*/);

    send_buff->bind(outer_->send_strategy_.in());
  }

  if (Transport_debug_level > 5) {
    const GuidConverter pub(pub_id);
    ACE_DEBUG((LM_DEBUG, "(%P|%t) RtpsUdpDataLink::MultiSendBuffer::insert() - "
      "pub_id %C seq %q frag %d\n", OPENDDS_STRING(pub).c_str(), seq.getValue(),
      (int)tqe->is_fragment()));
  }

  if (tqe->is_fragment()) {
    const RtpsCustomizedElement* const rce =
      dynamic_cast<const RtpsCustomizedElement*>(tqe);
    if (rce) {
      send_buff->insert_fragment(seq, rce->last_fragment(), q, chain);
    } else if (Transport_debug_level) {
      const GuidConverter pub(pub_id);
      ACE_ERROR((LM_ERROR, "(%P|%t) RtpsUdpDataLink::MultiSendBuffer::insert()"
        " - ERROR: couldn't get fragment number for pub_id %C seq %q\n",
        OPENDDS_STRING(pub).c_str(), seq.getValue()));
    }
  } else {
    send_buff->insert(seq, q, chain);
  }
}


// Support for the send() data handling path
namespace {
  ACE_Message_Block* submsgs_to_msgblock(const RTPS::SubmessageSeq& subm)
  {
    size_t size = 0, padding = 0;
    for (CORBA::ULong i = 0; i < subm.length(); ++i) {
      if ((size + padding) % 4) {
        padding += 4 - ((size + padding) % 4);
      }
      gen_find_size(subm[i], size, padding);
    }

    ACE_Message_Block* hdr = new ACE_Message_Block(size + padding);

    for (CORBA::ULong i = 0; i < subm.length(); ++i) {
      // byte swapping is handled in the operator<<() implementation
      Serializer ser(hdr, false, Serializer::ALIGN_CDR);
      ser << subm[i];
      const size_t len = hdr->length();
      if (len % 4) {
        hdr->wr_ptr(4 - (len % 4));
      }
    }
    return hdr;
  }
}

TransportQueueElement*
RtpsUdpDataLink::customize_queue_element(TransportQueueElement* element)
{
  const ACE_Message_Block* msg = element->msg();
  if (!msg) {
    return element;
  }

  const RepoId pub_id = element->publication_id();

  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, 0);

  RTPS::SubmessageSeq subm;

  const RtpsWriterMap::iterator rw = writers_.find(pub_id);

  bool gap_ok = true;
  DestToEntityMap gap_receivers;
  if (rw != writers_.end() && !rw->second.remote_readers_.empty()) {
    for (ReaderInfoMap::iterator ri = rw->second.remote_readers_.begin();
         ri != rw->second.remote_readers_.end(); ++ri) {
      RepoId tmp;
      std::memcpy(tmp.guidPrefix, ri->first.guidPrefix, sizeof(GuidPrefix_t));
      tmp.entityId = ENTITYID_UNKNOWN;
      gap_receivers[tmp].push_back(ri->first);

      if (ri->second.expecting_durable_data()) {
        // Can't add an in-line GAP if some Data Reader is expecting durable
        // data, the GAP could cause that Data Reader to ignore the durable
        // data.  The other readers will eventually learn about the GAP by
        // sending an ACKNACK and getting a GAP reply.
        gap_ok = false;
        break;
      }
    }
  }

  if (gap_ok) {
    add_gap_submsg(subm, *element, gap_receivers);
  }

  const SequenceNumber seq = element->sequence();
  if (rw != writers_.end() && seq != SequenceNumber::SEQUENCENUMBER_UNKNOWN()) {
    rw->second.expected_ = seq;
    ++rw->second.expected_;
  }

  TransportSendElement* tse = dynamic_cast<TransportSendElement*>(element);
  TransportCustomizedElement* tce =
    dynamic_cast<TransportCustomizedElement*>(element);
  TransportSendControlElement* tsce =
    dynamic_cast<TransportSendControlElement*>(element);

  ACE_Message_Block* data = 0;
  bool durable = false;

  // Based on the type of 'element', find and duplicate the data payload
  // continuation block.
  if (tsce) {        // Control message
    if (RtpsSampleHeader::control_message_supported(tsce->header().message_id_)) {
      data = msg->cont()->duplicate();
      // Create RTPS Submessage(s) in place of the OpenDDS DataSampleHeader
      RtpsSampleHeader::populate_data_control_submessages(
                subm, *tsce, requires_inline_qos(pub_id));
    } else if (tsce->header().message_id_ == END_HISTORIC_SAMPLES) {
      end_historic_samples(rw, tsce->header(), msg->cont());
      element->data_delivered();
      return 0;
    } else if (tsce->header().message_id_ == DATAWRITER_LIVELINESS) {
      send_heartbeats_manual(tsce);
      element->data_delivered();
      return 0;
    } else {
      element->data_dropped(true /*dropped_by_transport*/);
      return 0;
    }

  } else if (tse) {  // Basic data message
    // {DataSampleHeader} -> {Data Payload}
    data = msg->cont()->duplicate();
    const DataSampleElement* dsle = tse->sample();
    // Create RTPS Submessage(s) in place of the OpenDDS DataSampleHeader
    RtpsSampleHeader::populate_data_sample_submessages(
              subm, *dsle, requires_inline_qos(pub_id));
    durable = dsle->get_header().historic_sample_;

  } else if (tce) {  // Customized data message
    // {DataSampleHeader} -> {Content Filtering GUIDs} -> {Data Payload}
    data = msg->cont()->cont()->duplicate();
    const DataSampleElement* dsle = tce->original_send_element()->sample();
    // Create RTPS Submessage(s) in place of the OpenDDS DataSampleHeader
    RtpsSampleHeader::populate_data_sample_submessages(
              subm, *dsle, requires_inline_qos(pub_id));
    durable = dsle->get_header().historic_sample_;

  } else {
    return element;
  }

  ACE_Message_Block* hdr = submsgs_to_msgblock(subm);
  hdr->cont(data);
  RtpsCustomizedElement* rtps =
    RtpsCustomizedElement::alloc(element, hdr,
      &rtps_customized_element_allocator_);

  // Handle durability resends
  if (durable && rw != writers_.end()) {
    const RepoId sub = element->subscription_id();
    if (sub != GUID_UNKNOWN) {
      ReaderInfoMap::iterator ri = rw->second.remote_readers_.find(sub);
      if (ri != rw->second.remote_readers_.end()) {
        ri->second.durable_data_[rtps->sequence()] = rtps;
        ri->second.durable_timestamp_ = ACE_OS::gettimeofday();
        if (Transport_debug_level > 3) {
          const GuidConverter conv(pub_id), sub_conv(sub);
          ACE_DEBUG((LM_DEBUG,
            "(%P|%t) RtpsUdpDataLink::customize_queue_element() - "
            "storing durable data for local %C remote %C\n",
            OPENDDS_STRING(conv).c_str(), OPENDDS_STRING(sub_conv).c_str()));
        }
        return 0;
      }
    }
  } else if (durable && (Transport_debug_level)) {
    const GuidConverter conv(pub_id);
    ACE_DEBUG((LM_ERROR,
      "(%P|%t) RtpsUdpDataLink::customize_queue_element() - "
      "WARNING: no RtpsWriter to store durable data for local %C\n",
      OPENDDS_STRING(conv).c_str()));
  }

  return rtps;
}

void
RtpsUdpDataLink::end_historic_samples(RtpsWriterMap::iterator writer,
                                      const DataSampleHeader& header,
                                      ACE_Message_Block* body)
{
  // Set the ReaderInfo::durable_timestamp_ for the case where no
  // durable samples exist in the DataWriter.
  if (writer != writers_.end() && writer->second.durable_) {
    const ACE_Time_Value now = ACE_OS::gettimeofday();
    RepoId sub = GUID_UNKNOWN;
    if (body && header.message_length_ >= sizeof(sub)) {
      std::memcpy(&sub, body->rd_ptr(), header.message_length_);
    }
    typedef ReaderInfoMap::iterator iter_t;
    if (sub == GUID_UNKNOWN) {
      if (Transport_debug_level > 3) {
        const GuidConverter conv(writer->first);
        ACE_DEBUG((LM_DEBUG, "(%P|%t) RtpsUdpDataLink::end_historic_samples "
                   "local %C all readers\n", OPENDDS_STRING(conv).c_str()));
      }
      for (iter_t iter = writer->second.remote_readers_.begin();
           iter != writer->second.remote_readers_.end(); ++iter) {
        if (iter->second.durable_) {
          iter->second.durable_timestamp_ = now;
        }
      }
    } else {
      iter_t iter = writer->second.remote_readers_.find(sub);
      if (iter != writer->second.remote_readers_.end()) {
        if (iter->second.durable_) {
          iter->second.durable_timestamp_ = now;
          if (Transport_debug_level > 3) {
            const GuidConverter conv(writer->first), sub_conv(sub);
            ACE_DEBUG((LM_DEBUG, "(%P|%t) RtpsUdpDataLink::end_historic_samples"
                       " local %C remote %C\n", OPENDDS_STRING(conv).c_str(),
                       OPENDDS_STRING(sub_conv).c_str()));
          }
        }
      }
    }
  }
}

bool
RtpsUdpDataLink::requires_inline_qos(const PublicationId& pub_id)
{
  if (force_inline_qos_) {
    // Force true for testing purposes
    return true;
  } else {
    const GUIDSeq_var peers = peer_ids(pub_id);
    if (!peers.ptr()) {
      return false;
    }
    typedef OPENDDS_MAP_CMP(RepoId, RemoteInfo, GUID_tKeyLessThan)::iterator iter_t;
    for (CORBA::ULong i = 0; i < peers->length(); ++i) {
      const iter_t iter = locators_.find(peers[i]);
      if (iter != locators_.end() && iter->second.requires_inline_qos_) {
        return true;
      }
    }
    return false;
  }
}

bool RtpsUdpDataLink::force_inline_qos_ = false;

void
RtpsUdpDataLink::add_gap_submsg(RTPS::SubmessageSeq& msg,
                                const TransportQueueElement& tqe,
                                const DestToEntityMap& dtem)
{
  // These are the GAP submessages that we'll send directly in-line with the
  // DATA when we notice that the DataWriter has deliberately skipped seq #s.
  // There are other GAP submessages generated in response to reader ACKNACKS,
  // see send_nack_replies().
  using namespace OpenDDS::RTPS;

  const SequenceNumber seq = tqe.sequence();
  const RepoId pub = tqe.publication_id();
  if (seq == SequenceNumber::SEQUENCENUMBER_UNKNOWN() || pub == GUID_UNKNOWN
      || tqe.subscription_id() != GUID_UNKNOWN) {
    return;
  }

  const RtpsWriterMap::iterator wi = writers_.find(pub);
  if (wi == writers_.end()) {
    return; // not a reliable writer, does not send GAPs
  }

  RtpsWriter& rw = wi->second;

  if (seq != rw.expected_) {
    SequenceNumber firstMissing = rw.expected_;

    // RTPS v2.1 8.3.7.4: the Gap sequence numbers are those in the range
    // [gapStart, gapListBase) and those in the SNSet.
    const SequenceNumber_t gapStart = {firstMissing.getHigh(),
                                       firstMissing.getLow()},
                           gapListBase = {seq.getHigh(),
                                          seq.getLow()};

    // We are not going to enable any bits in the "bitmap" of the SNSet,
    // but the "numBits" and the bitmap.length must both be > 0.
    LongSeq8 bitmap;
    bitmap.length(1);
    bitmap[0] = 0;

    GapSubmessage gap = {
      {GAP, 1 /*FLAG_E*/, 0 /*length determined below*/},
      ENTITYID_UNKNOWN, // readerId: applies to all matched readers
      pub.entityId,
      gapStart,
      {gapListBase, 1, bitmap}
    };

    size_t size = 0, padding = 0;
    gen_find_size(gap, size, padding);
    gap.smHeader.submessageLength =
      static_cast<CORBA::UShort>(size + padding) - SMHDR_SZ;

    if (!rw.durable_) {
      const CORBA::ULong i = msg.length();
      msg.length(i + 1);
      msg[i].gap_sm(gap);
    } else {
      InfoDestinationSubmessage idst = {
        {INFO_DST, 1 /*FLAG_E*/, INFO_DST_SZ},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
      };
      CORBA::ULong ml = msg.length();

      //Change the non-directed Gap into multiple directed gaps to prevent
      //delivering to currently undiscovered durable readers
      DestToEntityMap::const_iterator iter = dtem.begin();
      while (iter != dtem.end()) {
        std::memcpy(idst.guidPrefix, iter->first.guidPrefix, sizeof(GuidPrefix_t));
        msg.length(ml + 1);
        msg[ml++].info_dst_sm(idst);

        const OPENDDS_VECTOR(RepoId)& readers = iter->second;
        for (size_t i = 0; i < readers.size(); ++i) {
          gap.readerId = readers.at(i).entityId;
          msg.length(ml + 1);
          msg[ml++].gap_sm(gap);
        } //END iter over reader entity ids
        ++iter;
      } //END iter over reader GuidPrefix_t's
    }
  }
}


// DataReader's side of Reliability

void
RtpsUdpDataLink::received(const RTPS::DataSubmessage& data,
                          const GuidPrefix_t& src_prefix)
{
  datareader_dispatch(data, src_prefix, &RtpsUdpDataLink::process_data_i);
}

bool
RtpsUdpDataLink::process_data_i(const RTPS::DataSubmessage& data,
                                const RepoId& src,
                                RtpsReaderMap::value_type& rr)
{
  const WriterInfoMap::iterator wi = rr.second.remote_writers_.find(src);
  if (wi != rr.second.remote_writers_.end()) {
    WriterInfo& info = wi->second;
    SequenceNumber seq;
    seq.setValue(data.writerSN.high, data.writerSN.low);
    info.frags_.erase(seq);
    const RepoId& readerId = rr.first;
    if (info.recvd_.contains(seq)) {
      if (Transport_debug_level > 5) {
        GuidConverter writer(src);
        GuidConverter reader(readerId);
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) RtpsUdpDataLink::process_data_i(DataSubmessage) -")
                             ACE_TEXT(" data seq: %q from %C being WITHHELD from %C because ALREADY received\n"),
                             seq.getValue(),
                             OPENDDS_STRING(writer).c_str(),
                             OPENDDS_STRING(reader).c_str()));
      }
      recv_strategy_->withhold_data_from(readerId);
    } else if (info.recvd_.disjoint() ||
        (!info.recvd_.empty() && info.recvd_.cumulative_ack() != seq.previous())
        || (rr.second.durable_ && !info.recvd_.empty() && info.recvd_.low() > 1)
        || (rr.second.durable_ && info.recvd_.empty() && seq > 1)) {
      if (Transport_debug_level > 5) {
        GuidConverter writer(src);
        GuidConverter reader(readerId);
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) RtpsUdpDataLink::process_data_i(DataSubmessage) -")
                             ACE_TEXT(" data seq: %q from %C being WITHHELD from %C because can't receive yet\n"),
                             seq.getValue(),
                             OPENDDS_STRING(writer).c_str(),
                             OPENDDS_STRING(reader).c_str()));
      }
      const ReceivedDataSample* sample =
        recv_strategy_->withhold_data_from(readerId);
      info.held_.insert(std::make_pair(seq, *sample));
    } else {
      if (Transport_debug_level > 5) {
        GuidConverter writer(src);
        GuidConverter reader(readerId);
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) RtpsUdpDataLink::process_data_i(DataSubmessage) -")
                             ACE_TEXT(" data seq: %q from %C to %C OK to deliver\n"),
                             seq.getValue(),
                             OPENDDS_STRING(writer).c_str(),
                             OPENDDS_STRING(reader).c_str()));
      }
      recv_strategy_->do_not_withhold_data_from(readerId);
    }
    info.recvd_.insert(seq);
    deliver_held_data(readerId, info, rr.second.durable_);
  } else {
    if (Transport_debug_level > 5) {
      GuidConverter writer(src);
      GuidConverter reader(rr.first);
      SequenceNumber seq;
      seq.setValue(data.writerSN.high, data.writerSN.low);
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) RtpsUdpDataLink::process_data_i(DataSubmessage) -")
                           ACE_TEXT(" data seq: %q from %C to %C OK to deliver (Writer not currently in Reader remote writer map)\n"),
                           seq.getValue(),
                           OPENDDS_STRING(writer).c_str(),
                           OPENDDS_STRING(reader).c_str()));
    }
    recv_strategy_->do_not_withhold_data_from(rr.first);
  }
  return false;
}

void
RtpsUdpDataLink::deliver_held_data(const RepoId& readerId, WriterInfo& info,
                                   bool durable)
{
  if (durable && (info.recvd_.empty() || info.recvd_.low() > 1)) return;
  const SequenceNumber ca = info.recvd_.cumulative_ack();
  typedef OPENDDS_MAP(SequenceNumber, ReceivedDataSample)::iterator iter;
  const iter end = info.held_.upper_bound(ca);
  for (iter it = info.held_.begin(); it != end; /*increment in loop body*/) {
    if (Transport_debug_level > 5) {
      GuidConverter reader(readerId);
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) RtpsUdpDataLink::deliver_held_data -")
                           ACE_TEXT(" deliver sequence: %q to %C\n"),
                           it->second.header_.sequence_.getValue(),
                           OPENDDS_STRING(reader).c_str()));
    }
    data_received(it->second, readerId);
    info.held_.erase(it++);
  }
}

void
RtpsUdpDataLink::received(const RTPS::GapSubmessage& gap,
                          const GuidPrefix_t& src_prefix)
{
  datareader_dispatch(gap, src_prefix, &RtpsUdpDataLink::process_gap_i);
}

bool
RtpsUdpDataLink::process_gap_i(const RTPS::GapSubmessage& gap,
                               const RepoId& src, RtpsReaderMap::value_type& rr)
{
  const WriterInfoMap::iterator wi = rr.second.remote_writers_.find(src);
  if (wi != rr.second.remote_writers_.end()) {
    SequenceRange sr;
    sr.first.setValue(gap.gapStart.high, gap.gapStart.low);
    SequenceNumber base;
    base.setValue(gap.gapList.bitmapBase.high, gap.gapList.bitmapBase.low);
    SequenceNumber first_received = SequenceNumber::MAX_VALUE;
    if (!wi->second.recvd_.empty()) {
      OPENDDS_VECTOR(SequenceRange) missing = wi->second.recvd_.missing_sequence_ranges();
      if (!missing.empty()) {
        first_received = missing.front().second;
      }
    }
    sr.second = std::min(first_received, base.previous());
    if (sr.first <= sr.second) {
      if (Transport_debug_level > 5) {
        const GuidConverter conv(src);
        const GuidConverter rdr(rr.first);
        ACE_DEBUG((LM_DEBUG, "(%P|%t) RtpsUdpDataLink::process_gap_i "
                  "Reader %C received GAP with range [%q, %q] (inserting range [%q, %q]) from %C\n",
                  OPENDDS_STRING(rdr).c_str(),
                  sr.first.getValue(), base.previous().getValue(),
                  sr.first.getValue(), sr.second.getValue(),
                  OPENDDS_STRING(conv).c_str()));
      }
      wi->second.recvd_.insert(sr);
    } else {
      const GuidConverter conv(src);
      VDBG_LVL((LM_WARNING, "(%P|%t) RtpsUdpDataLink::process_gap_i "
                "received GAP with invalid range [%q, %q] from %C\n",
                sr.first.getValue(), sr.second.getValue(),
                OPENDDS_STRING(conv).c_str()), 2);
    }
    wi->second.recvd_.insert(base, gap.gapList.numBits,
                             gap.gapList.bitmap.get_buffer());
    deliver_held_data(rr.first, wi->second, rr.second.durable_);
    //FUTURE: to support wait_for_acks(), notify DCPS layer of the GAP
  }
  return false;
}

void
RtpsUdpDataLink::received(const RTPS::HeartBeatSubmessage& heartbeat,
                          const GuidPrefix_t& src_prefix)
{
  RepoId src;
  std::memcpy(src.guidPrefix, src_prefix, sizeof(GuidPrefix_t));
  src.entityId = heartbeat.writerId;

  bool schedule_acknack = false;
  const ACE_Time_Value now = ACE_OS::gettimeofday();
  OPENDDS_VECTOR(InterestingRemote) callbacks;

  {
    ACE_GUARD(ACE_Thread_Mutex, g, lock_);

    // We received a heartbeat from a writer.
    // We should ACKNACK if the writer is interesting and there is no association.

    for (InterestingRemoteMapType::iterator pos = interesting_writers_.lower_bound(src),
           limit = interesting_writers_.upper_bound(src);
         pos != limit;
         ++pos) {
      const RepoId& writerid = src;
      const RepoId& readerid = pos->second.localid;

      RtpsReaderMap::const_iterator riter = readers_.find(readerid);
      if (riter == readers_.end()) {
        // Reader has no associations.
        interesting_ack_nacks_.insert (InterestingAckNack(writerid, readerid, pos->second.address));
      } else if (riter->second.remote_writers_.find(writerid) == riter->second.remote_writers_.end()) {
        // Reader is not associated with this writer.
        interesting_ack_nacks_.insert (InterestingAckNack(writerid, readerid, pos->second.address));
      }
      pos->second.last_activity = now;
      if (pos->second.status == InterestingRemote::DOES_NOT_EXIST) {
        callbacks.push_back(pos->second);
        pos->second.status = InterestingRemote::EXISTS;
      }
    }

    schedule_acknack = !interesting_ack_nacks_.empty();
  }

  for (size_t i = 0; i < callbacks.size(); ++i) {
    callbacks[i].listener->writer_exists(src, callbacks[i].localid);
  }

  if (schedule_acknack) {
    heartbeat_reply_.schedule();
  }

  datareader_dispatch(heartbeat, src_prefix,
                      &RtpsUdpDataLink::process_heartbeat_i);
}

bool
RtpsUdpDataLink::process_heartbeat_i(const RTPS::HeartBeatSubmessage& heartbeat,
                                     const RepoId& src,
                                     RtpsReaderMap::value_type& rr)
{
  const WriterInfoMap::iterator wi = rr.second.remote_writers_.find(src);
  if (wi == rr.second.remote_writers_.end()) {
    // we may not be associated yet, even if the writer thinks we are
    return false;
  }

  WriterInfo& info = wi->second;

  if (heartbeat.count.value <= info.heartbeat_recvd_count_) {
    return false;
  }
  info.heartbeat_recvd_count_ = heartbeat.count.value;

  SequenceNumber& first = info.hb_range_.first;
  first.setValue(heartbeat.firstSN.high, heartbeat.firstSN.low);
  SequenceNumber& last = info.hb_range_.second;
  last.setValue(heartbeat.lastSN.high, heartbeat.lastSN.low);
  static const SequenceNumber starting, zero = SequenceNumber::ZERO();

  DisjointSequence& recvd = info.recvd_;
  if (!rr.second.durable_ && info.initial_hb_) {
    if (last.getValue() < starting.getValue()) {
      // this is an invalid heartbeat -- last must be positive
      return false;
    }
    // For the non-durable reader, the first received HB or DATA establishes
    // a baseline of the lowest sequence number we'd ever need to NACK.
    if (recvd.empty() || recvd.low() >= last) {
      recvd.insert(SequenceRange(zero,
                                 (last > starting) ? last.previous() : zero));
    } else {
      recvd.insert(SequenceRange(zero, recvd.low()));
    }
  } else if (!recvd.empty()) {
    // All sequence numbers below 'first' should not be NACKed.
    // The value of 'first' may not decrease with subsequent HBs.
    recvd.insert(SequenceRange(zero,
                               (first > starting) ? first.previous() : zero));
  }

  deliver_held_data(rr.first, info, rr.second.durable_);

  //FUTURE: to support wait_for_acks(), notify DCPS layer of the sequence
  //        numbers we no longer expect to receive due to HEARTBEAT

  info.initial_hb_ = false;

  const bool final = heartbeat.smHeader.flags & 2 /* FLAG_F */,
    liveliness = heartbeat.smHeader.flags & 4 /* FLAG_L */;

  if (!final || (!liveliness && (info.should_nack() ||
      rr.second.nack_durable(info) ||
      recv_strategy_->has_fragments(info.hb_range_, wi->first)))) {
    info.ack_pending_ = true;
    return true; // timer will invoke send_heartbeat_replies()
  }

  //FUTURE: support assertion of liveliness for MANUAL_BY_TOPIC
  return false;
}

bool
RtpsUdpDataLink::WriterInfo::should_nack() const
{
  if (recvd_.disjoint() && recvd_.cumulative_ack() < hb_range_.second) {
    return true;
  }
  if (!recvd_.empty()) {
    return recvd_.high() < hb_range_.second;
  }
  return false;
}

bool
RtpsUdpDataLink::RtpsReader::nack_durable(const WriterInfo& info)
{
  return durable_ && (info.recvd_.empty() ||
                      info.recvd_.low() > info.hb_range_.first);
}

void
RtpsUdpDataLink::send_ack_nacks(RtpsReaderMap::iterator rr, bool finalFlag)
{
  using namespace OpenDDS::RTPS;

  WriterInfoMap& writers = rr->second.remote_writers_;
  for (WriterInfoMap::iterator wi = writers.begin(); wi != writers.end();
       ++wi) {

    // if we have some negative acknowledgments, we'll ask for a reply
    DisjointSequence& recvd = wi->second.recvd_;
    const bool nack = wi->second.should_nack() ||
      rr->second.nack_durable(wi->second);
    bool final = finalFlag || !nack;

    if (wi->second.ack_pending_ || nack || finalFlag) {
      const bool prev_ack_pending = wi->second.ack_pending_;
      wi->second.ack_pending_ = false;

      SequenceNumber ack;
      CORBA::ULong num_bits = 1;
      LongSeq8 bitmap;
      bitmap.length(1);
      bitmap[0] = 0;

      const SequenceNumber& hb_low = wi->second.hb_range_.first;
      const SequenceNumber& hb_high = wi->second.hb_range_.second;
      const SequenceNumber::Value hb_low_val = hb_low.getValue(),
        hb_high_val = hb_high.getValue();

      if (recvd.empty()) {
        // Nack the entire heartbeat range.  Only reached when durable.
        ack = hb_low;
        bitmap.length(bitmap_num_longs(ack, hb_high));
        const CORBA::ULong idx = (hb_high_val > hb_low_val + 255)
          ? 255
          : CORBA::ULong(hb_high_val - hb_low_val);
        DisjointSequence::fill_bitmap_range(0, idx,
                                            bitmap.get_buffer(),
                                            bitmap.length(), num_bits);
      } else if (((prev_ack_pending && !nack) || rr->second.nack_durable(wi->second)) && recvd.low() > hb_low) {
        // Nack the range between the heartbeat low and the recvd low.
        ack = hb_low;
        const SequenceNumber& rec_low = recvd.low();
        const SequenceNumber::Value rec_low_val = rec_low.getValue();
        bitmap.length(bitmap_num_longs(ack, rec_low));
        const CORBA::ULong idx = (rec_low_val > hb_low_val + 255)
          ? 255
          : CORBA::ULong(rec_low_val - hb_low_val);
        DisjointSequence::fill_bitmap_range(0, idx,
                                            bitmap.get_buffer(),
                                            bitmap.length(), num_bits);

      } else {
        ack = ++SequenceNumber(recvd.cumulative_ack());
        if (recvd.low().getValue() > 1) {
          // since the "ack" really is cumulative, we need to make
          // sure that a lower discontinuity is not possible later
          recvd.insert(SequenceRange(SequenceNumber::ZERO(), recvd.low()));
        }

        if (recvd.disjoint()) {
          bitmap.length(bitmap_num_longs(ack, recvd.last_ack().previous()));
          recvd.to_bitmap(bitmap.get_buffer(), bitmap.length(),
                          num_bits, true);
        }
      }

      const SequenceNumber::Value ack_val = ack.getValue();

      if (!recvd.empty() && hb_high > recvd.high()) {
        const SequenceNumber eff_high =
          (hb_high <= ack_val + 255) ? hb_high : (ack_val + 255);
        const SequenceNumber::Value eff_high_val = eff_high.getValue();
        // Nack the range between the received high and the effective high.
        const CORBA::ULong old_len = bitmap.length(),
          new_len = bitmap_num_longs(ack, eff_high);
        if (new_len > old_len) {
          bitmap.length(new_len);
          for (CORBA::ULong i = old_len; i < new_len; ++i) {
            bitmap[i] = 0;
          }
        }
        const CORBA::ULong idx_hb_high = CORBA::ULong(eff_high_val - ack_val),
          idx_recv_high = recvd.disjoint() ?
          CORBA::ULong(recvd.high().getValue() - ack_val) : 0;
        DisjointSequence::fill_bitmap_range(idx_recv_high, idx_hb_high,
                                            bitmap.get_buffer(), new_len,
                                            num_bits);
      }

      // If the receive strategy is holding any fragments, those should
      // not be "nacked" in the ACKNACK reply.  They will be accounted for
      // in the NACK_FRAG(s) instead.
      bool frags_modified =
        recv_strategy_->remove_frags_from_bitmap(bitmap.get_buffer(),
                                                 num_bits, ack, wi->first);
      if (frags_modified && !final) { // change to final if bitmap is empty
        final = true;
        for (CORBA::ULong i = 0; i < bitmap.length(); ++i) {
          if ((i + 1) * 32 <= num_bits) {
            if (bitmap[i]) {
              final = false;
              break;
            }
          } else {
            if ((0xffffffff << (32 - (num_bits % 32))) & bitmap[i]) {
              final = false;
              break;
            }
          }
        }
      }

      AckNackSubmessage acknack = {
        {ACKNACK,
         CORBA::Octet(1 /*FLAG_E*/ | (final ? 2 /*FLAG_F*/ : 0)),
         0 /*length*/},
        rr->first.entityId,
        wi->first.entityId,
        { // SequenceNumberSet: acking bitmapBase - 1
          {ack.getHigh(), ack.getLow()},
          num_bits, bitmap
        },
        {++wi->second.acknack_count_}
      };

      size_t size = 0, padding = 0;
      gen_find_size(acknack, size, padding);
      acknack.smHeader.submessageLength =
        static_cast<CORBA::UShort>(size + padding) - SMHDR_SZ;
      InfoDestinationSubmessage info_dst = {
        {INFO_DST, 1 /*FLAG_E*/, INFO_DST_SZ},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
      };
      gen_find_size(info_dst, size, padding);

      OPENDDS_VECTOR(NackFragSubmessage) nack_frags;
      size += generate_nack_frags(nack_frags, wi->second, wi->first);

      ACE_Message_Block mb_acknack(size + padding); //FUTURE: allocators?
      // byte swapping is handled in the operator<<() implementation
      Serializer ser(&mb_acknack, false, Serializer::ALIGN_CDR);
      std::memcpy(info_dst.guidPrefix, wi->first.guidPrefix,
                  sizeof(GuidPrefix_t));
      ser << info_dst;
      // Interoperability note: we used to insert INFO_REPLY submessage here, but
      // testing indicated that other DDS implementations didn't accept it.
      ser << acknack;
      for (size_t i = 0; i < nack_frags.size(); ++i) {
        nack_frags[i].readerId = rr->first.entityId;
        nack_frags[i].writerId = wi->first.entityId;
        ser << nack_frags[i]; // always 4-byte aligned
      }

      if (!locators_.count(wi->first)) {
        if (Transport_debug_level) {
          const GuidConverter conv(wi->first);
          ACE_DEBUG((LM_ERROR,
                     "(%P|%t) RtpsUdpDataLink::send_heartbeat_replies() - "
                     "no locator for remote %C\n", OPENDDS_STRING(conv).c_str()));
        }
      } else {
        send_strategy_->send_rtps_control(mb_acknack,
                                          locators_[wi->first].addr_);
      }
    }
  }
}

void
RtpsUdpDataLink::send_heartbeat_replies() // from DR to DW
{
  using namespace OpenDDS::RTPS;
  ACE_GUARD(ACE_Thread_Mutex, g, lock_);

  for (InterestingAckNackSetType::const_iterator pos = interesting_ack_nacks_.begin(),
         limit = interesting_ack_nacks_.end();
       pos != limit;
       ++pos) {

    SequenceNumber ack;
    LongSeq8 bitmap;
    bitmap.length(0);

    AckNackSubmessage acknack = {
      {ACKNACK,
       CORBA::Octet(1 /*FLAG_E*/ | 2 /*FLAG_F*/),
       0 /*length*/},
      pos->readerid.entityId,
      pos->writerid.entityId,
      { // SequenceNumberSet: acking bitmapBase - 1
        {ack.getHigh(), ack.getLow()},
        0 /* num_bits */, bitmap
      },
      {0 /* acknack count */}
    };

    size_t size = 0, padding = 0;
    gen_find_size(acknack, size, padding);
    acknack.smHeader.submessageLength =
      static_cast<CORBA::UShort>(size + padding) - SMHDR_SZ;
    InfoDestinationSubmessage info_dst = {
      {INFO_DST, 1 /*FLAG_E*/, INFO_DST_SZ},
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
    };
    gen_find_size(info_dst, size, padding);

    ACE_Message_Block mb_acknack(size + padding); //FUTURE: allocators?
    // byte swapping is handled in the operator<<() implementation
    Serializer ser(&mb_acknack, false, Serializer::ALIGN_CDR);
    std::memcpy(info_dst.guidPrefix, pos->writerid.guidPrefix,
                sizeof(GuidPrefix_t));
    ser << info_dst;
    // Interoperability note: we used to insert INFO_REPLY submessage here, but
    // testing indicated that other DDS implementations didn't accept it.
    ser << acknack;

    send_strategy_->send_rtps_control(mb_acknack, pos->writer_address);
  }
  interesting_ack_nacks_.clear();

  for (RtpsReaderMap::iterator rr = readers_.begin(); rr != readers_.end();
       ++rr) {
    send_ack_nacks (rr);
  }
}

size_t
RtpsUdpDataLink::generate_nack_frags(OPENDDS_VECTOR(RTPS::NackFragSubmessage)& nf,
                                     WriterInfo& wi, const RepoId& pub_id)
{
  typedef OPENDDS_MAP(SequenceNumber, RTPS::FragmentNumber_t)::iterator iter_t;
  typedef RtpsUdpReceiveStrategy::FragmentInfo::value_type Frag_t;
  RtpsUdpReceiveStrategy::FragmentInfo frag_info;

  // Populate frag_info with two possible sources of NackFrags:
  // 1. sequence #s in the reception gaps that we have partially received
  OPENDDS_VECTOR(SequenceRange) missing = wi.recvd_.missing_sequence_ranges();
  for (size_t i = 0; i < missing.size(); ++i) {
    recv_strategy_->has_fragments(missing[i], pub_id, &frag_info);
  }
  // 1b. larger than the last received seq# but less than the heartbeat.lastSN
  if (!wi.recvd_.empty()) {
    const SequenceRange range(wi.recvd_.high(), wi.hb_range_.second);
    recv_strategy_->has_fragments(range, pub_id, &frag_info);
  }
  for (size_t i = 0; i < frag_info.size(); ++i) {
    // If we've received a HeartbeatFrag, we know the last (available) frag #
    const iter_t heartbeat_frag = wi.frags_.find(frag_info[i].first);
    if (heartbeat_frag != wi.frags_.end()) {
      extend_bitmap_range(frag_info[i].second, heartbeat_frag->second.value);
    }
  }

  // 2. sequence #s outside the recvd_ gaps for which we have a HeartbeatFrag
  const iter_t low = wi.frags_.lower_bound(wi.recvd_.cumulative_ack()),
              high = wi.frags_.upper_bound(wi.recvd_.last_ack()),
               end = wi.frags_.end();
  for (iter_t iter = wi.frags_.begin(); iter != end; ++iter) {
    if (iter == low) {
      // skip over the range covered by step #1 above
      if (high == end) {
        break;
      }
      iter = high;
    }

    const SequenceRange range(iter->first, iter->first);
    if (recv_strategy_->has_fragments(range, pub_id, &frag_info)) {
      extend_bitmap_range(frag_info.back().second, iter->second.value);
    } else {
      // it was not in the recv strategy, so the entire range is "missing"
      frag_info.push_back(Frag_t(iter->first, RTPS::FragmentNumberSet()));
      RTPS::FragmentNumberSet& fnSet = frag_info.back().second;
      fnSet.bitmapBase.value = 1;
      fnSet.numBits = std::min(CORBA::ULong(256), iter->second.value);
      fnSet.bitmap.length((fnSet.numBits + 31) / 32);
      for (CORBA::ULong i = 0; i < fnSet.bitmap.length(); ++i) {
        fnSet.bitmap[i] = 0xFFFFFFFF;
      }
    }
  }

  if (frag_info.empty()) {
    return 0;
  }

  const RTPS::NackFragSubmessage nackfrag_prototype = {
    {RTPS::NACK_FRAG, 1 /*FLAG_E*/, 0 /* length set below */},
    ENTITYID_UNKNOWN, // readerId will be filled-in by send_heartbeat_replies()
    ENTITYID_UNKNOWN, // writerId will be filled-in by send_heartbeat_replies()
    {0, 0}, // writerSN set below
    RTPS::FragmentNumberSet(), // fragmentNumberState set below
    {0} // count set below
  };

  size_t size = 0, padding = 0;
  for (size_t i = 0; i < frag_info.size(); ++i) {
    nf.push_back(nackfrag_prototype);
    RTPS::NackFragSubmessage& nackfrag = nf.back();
    nackfrag.writerSN.low = frag_info[i].first.getLow();
    nackfrag.writerSN.high = frag_info[i].first.getHigh();
    nackfrag.fragmentNumberState = frag_info[i].second;
    nackfrag.count.value = ++wi.nackfrag_count_;
    const size_t before_size = size;
    gen_find_size(nackfrag, size, padding);
    nackfrag.smHeader.submessageLength =
      static_cast<CORBA::UShort>(size - before_size) - RTPS::SMHDR_SZ;
  }
  return size;
}

void
RtpsUdpDataLink::extend_bitmap_range(RTPS::FragmentNumberSet& fnSet,
                                     CORBA::ULong extent)
{
  if (extent < fnSet.bitmapBase.value) {
    return; // can't extend to some number under the base
  }
  // calculate the index to the extent to determine the new_num_bits
  const CORBA::ULong new_num_bits = std::min(CORBA::ULong(255),
                                             extent - fnSet.bitmapBase.value + 1),
                     len = (new_num_bits + 31) / 32;
  if (new_num_bits < fnSet.numBits) {
    return; // bitmap already extends past "extent"
  }
  fnSet.bitmap.length(len);
  // We are missing from one past old bitmap end to the new end
  DisjointSequence::fill_bitmap_range(fnSet.numBits + 1, new_num_bits,
                                      fnSet.bitmap.get_buffer(), len,
                                      fnSet.numBits);
}

void
RtpsUdpDataLink::received(const RTPS::HeartBeatFragSubmessage& hb_frag,
                          const GuidPrefix_t& src_prefix)
{
  datareader_dispatch(hb_frag, src_prefix, &RtpsUdpDataLink::process_hb_frag_i);
}

bool
RtpsUdpDataLink::process_hb_frag_i(const RTPS::HeartBeatFragSubmessage& hb_frag,
                                   const RepoId& src,
                                   RtpsReaderMap::value_type& rr)
{
  WriterInfoMap::iterator wi = rr.second.remote_writers_.find(src);
  if (wi == rr.second.remote_writers_.end()) {
    // we may not be associated yet, even if the writer thinks we are
    return false;
  }

  if (hb_frag.count.value <= wi->second.hb_frag_recvd_count_) {
    return false;
  }

  wi->second.hb_frag_recvd_count_ = hb_frag.count.value;

  SequenceNumber seq;
  seq.setValue(hb_frag.writerSN.high, hb_frag.writerSN.low);

  // If seq is outside the heartbeat range or we haven't completely received
  // it yet, send a NackFrag along with the AckNack.  The heartbeat range needs
  // to be checked first because recvd_ contains the numbers below the
  // heartbeat range (so that we don't NACK those).
  if (seq < wi->second.hb_range_.first || seq > wi->second.hb_range_.second
      || !wi->second.recvd_.contains(seq)) {
    wi->second.frags_[seq] = hb_frag.lastFragmentNum;
    wi->second.ack_pending_ = true;
    return true; // timer will invoke send_heartbeat_replies()
  }
  return false;
}


// DataWriter's side of Reliability

void
RtpsUdpDataLink::received(const RTPS::AckNackSubmessage& acknack,
                          const GuidPrefix_t& src_prefix)
{
  // local side is DW
  RepoId local;
  std::memcpy(local.guidPrefix, local_prefix_, sizeof(GuidPrefix_t));
  local.entityId = acknack.writerId; // can't be ENTITYID_UNKNOWN

  RepoId remote;
  std::memcpy(remote.guidPrefix, src_prefix, sizeof(GuidPrefix_t));
  remote.entityId = acknack.readerId;

  const ACE_Time_Value now = ACE_OS::gettimeofday();
  OPENDDS_VECTOR(DiscoveryListener*) callbacks;

  {
    ACE_GUARD(ACE_Thread_Mutex, g, lock_);
    for (InterestingRemoteMapType::iterator pos = interesting_readers_.lower_bound(remote),
           limit = interesting_readers_.upper_bound(remote);
         pos != limit;
         ++pos) {
      pos->second.last_activity = now;
      // Ensure the acknack was for the writer.
      if (local == pos->second.localid) {
        if (pos->second.status == InterestingRemote::DOES_NOT_EXIST) {
          callbacks.push_back(pos->second.listener);
          pos->second.status = InterestingRemote::EXISTS;
        }
      }
    }
  }

  for (size_t i = 0; i < callbacks.size(); ++i) {
    callbacks[i]->reader_exists(remote, local);
  }

  ACE_GUARD(ACE_Thread_Mutex, g, lock_);
  const RtpsWriterMap::iterator rw = writers_.find(local);
  if (rw == writers_.end()) {
    if (Transport_debug_level > 5) {
      GuidConverter local_conv(local);
      ACE_DEBUG((LM_WARNING, "(%P|%t) RtpsUdpDataLink::received(ACKNACK) "
        "WARNING local %C no RtpsWriter\n", OPENDDS_STRING(local_conv).c_str()));
    }
    return;
  }

  if (Transport_debug_level > 5) {
    GuidConverter local_conv(local), remote_conv(remote);
    ACE_DEBUG((LM_DEBUG, "(%P|%t) RtpsUdpDataLink::received(ACKNACK) "
      "local %C remote %C\n", OPENDDS_STRING(local_conv).c_str(),
      OPENDDS_STRING(remote_conv).c_str()));
  }

  const ReaderInfoMap::iterator ri = rw->second.remote_readers_.find(remote);
  if (ri == rw->second.remote_readers_.end()) {
    VDBG((LM_WARNING, "(%P|%t) RtpsUdpDataLink::received(ACKNACK) "
      "WARNING ReaderInfo not found\n"));
    return;
  }

  if (acknack.count.value <= ri->second.acknack_recvd_count_) {
    VDBG((LM_WARNING, "(%P|%t) RtpsUdpDataLink::received(ACKNACK) "
      "WARNING Count indicates duplicate, dropping\n"));
    return;
  }

  ri->second.acknack_recvd_count_ = acknack.count.value;

  if (!ri->second.handshake_done_) {
    ri->second.handshake_done_ = true;
    invoke_on_start_callbacks(true);
  }

  OPENDDS_MAP(SequenceNumber, TransportQueueElement*) pendingCallbacks;
  const bool final = acknack.smHeader.flags & 2 /* FLAG_F */;

  if (!ri->second.durable_data_.empty()) {
    if (Transport_debug_level > 5) {
      const GuidConverter local_conv(local), remote_conv(remote);
      ACE_DEBUG((LM_DEBUG, "RtpsUdpDataLink::received(ACKNACK) "
                 "local %C has durable for remote %C\n",
                 OPENDDS_STRING(local_conv).c_str(),
                 OPENDDS_STRING(remote_conv).c_str()));
    }
    SequenceNumber ack;
    ack.setValue(acknack.readerSNState.bitmapBase.high,
                 acknack.readerSNState.bitmapBase.low);
    const SequenceNumber& dd_last = ri->second.durable_data_.rbegin()->first;
    if (ack > dd_last) {
      // Reader acknowledges durable data, we no longer need to store it
      ri->second.durable_data_.swap(pendingCallbacks);
      if (Transport_debug_level > 5) {
        ACE_DEBUG((LM_DEBUG, "RtpsUdpDataLink::received(ACKNACK) "
                   "durable data acked\n"));
      }
    } else {
      DisjointSequence requests;
      if (!requests.insert(ack, acknack.readerSNState.numBits,
                           acknack.readerSNState.bitmap.get_buffer())
          && !final && ack == rw->second.heartbeat_high(ri->second)) {
        // This is a non-final AckNack with no bits in the bitmap.
        // Attempt to reply to a request for the "base" value which
        // is neither Acked nor Nacked, only when it's the HB high.
        if (ri->second.durable_data_.count(ack)) requests.insert(ack);
      }
      // Attempt to reply to nacks for durable data
      bool sent_some = false;
      typedef OPENDDS_MAP(SequenceNumber, TransportQueueElement*)::iterator iter_t;
      iter_t it = ri->second.durable_data_.begin();
      const OPENDDS_VECTOR(SequenceRange) psr = requests.present_sequence_ranges();
      SequenceNumber lastSent = SequenceNumber::ZERO();
      if (!requests.empty()) {
        lastSent = requests.low().previous();
      }
      DisjointSequence gaps;
      for (size_t i = 0; i < psr.size(); ++i) {
        for (; it != ri->second.durable_data_.end()
             && it->first < psr[i].first; ++it) ; // empty for-loop
        for (; it != ri->second.durable_data_.end()
             && it->first <= psr[i].second; ++it) {
          if (Transport_debug_level > 5) {
            ACE_DEBUG((LM_DEBUG, "RtpsUdpDataLink::received(ACKNACK) "
                       "durable resend %d\n", int(it->first.getValue())));
          }
          durability_resend(it->second);
          //FUTURE: combine multiple resends into one RTPS Message?
          sent_some = true;
          if (it->first > lastSent + 1) {
            gaps.insert(SequenceRange(lastSent + 1, it->first.previous()));
          }
          lastSent = it->first;
        }
        if (sent_some && lastSent < psr[i].second && psr[i].second < dd_last) {
          gaps.insert(SequenceRange(lastSent + 1, psr[i].second));
        }
      }
      if (!gaps.empty()) {
        if (Transport_debug_level > 5) {
          ACE_DEBUG((LM_DEBUG, "RtpsUdpDataLink::received(ACKNACK) "
                     "sending durability gaps: "));
          gaps.dump();
        }
        send_durability_gaps(local, remote, gaps);
      }
      if (sent_some) {
        return;
      }
      const SequenceNumber& dd_first = ri->second.durable_data_.begin()->first;
      if (!requests.empty() && requests.high() < dd_first) {
        // All nacks were below the start of the durable data.
          requests.insert(SequenceRange(requests.high(), dd_first.previous()));
        if (Transport_debug_level > 5) {
          ACE_DEBUG((LM_DEBUG, "RtpsUdpDataLink::received(ACKNACK) "
                     "sending durability gaps for all requests: "));
          requests.dump();
        }
        send_durability_gaps(local, remote, requests);
        return;
      }
      if (!requests.empty() && requests.low() < dd_first) {
        // Lowest nack was below the start of the durable data.
        for (size_t i = 0; i < psr.size(); ++i) {
          if (psr[i].first > dd_first) {
            break;
          }
          gaps.insert(SequenceRange(psr[i].first,
                                    std::min(psr[i].second, dd_first)));
        }
        if (Transport_debug_level > 5) {
          ACE_DEBUG((LM_DEBUG, "RtpsUdpDataLink::received(ACKNACK) "
                     "sending durability gaps for some requests: "));
          gaps.dump();
        }
        send_durability_gaps(local, remote, gaps);
        return;
      }
    }
  }
  SequenceNumber ack;
  ack.setValue(acknack.readerSNState.bitmapBase.high,
               acknack.readerSNState.bitmapBase.low);
  if (ack != SequenceNumber::SEQUENCENUMBER_UNKNOWN()
      && ack != SequenceNumber::ZERO()) {
    ri->second.cur_cumulative_ack_ = ack;
  }
  // If this ACKNACK was final, the DR doesn't expect a reply, and therefore
  // we don't need to do anything further.
  if (!final) {
    ri->second.requested_changes_.push_back(acknack.readerSNState);
  }
  process_acked_by_all_i(g, local);
  g.release();
  if (!final) {
    nack_reply_.schedule(); // timer will invoke send_nack_replies()
  }
  typedef OPENDDS_MAP(SequenceNumber, TransportQueueElement*)::iterator iter_t;
  for (iter_t it = pendingCallbacks.begin();
       it != pendingCallbacks.end(); ++it) {
    it->second->data_delivered();
  }
}

void
RtpsUdpDataLink::received(const RTPS::NackFragSubmessage& nackfrag,
                          const GuidPrefix_t& src_prefix)
{
  // local side is DW
  RepoId local;
  std::memcpy(local.guidPrefix, local_prefix_, sizeof(GuidPrefix_t));
  local.entityId = nackfrag.writerId; // can't be ENTITYID_UNKNOWN

  ACE_GUARD(ACE_Thread_Mutex, g, lock_);
  const RtpsWriterMap::iterator rw = writers_.find(local);
  if (rw == writers_.end()) {
    if (Transport_debug_level > 5) {
      GuidConverter local_conv(local);
      ACE_DEBUG((LM_WARNING, "(%P|%t) RtpsUdpDataLink::received(NACK_FRAG) "
        "WARNING local %C no RtpsWriter\n", OPENDDS_STRING(local_conv).c_str()));
    }
    return;
  }

  RepoId remote;
  std::memcpy(remote.guidPrefix, src_prefix, sizeof(GuidPrefix_t));
  remote.entityId = nackfrag.readerId;

  if (Transport_debug_level > 5) {
    GuidConverter local_conv(local), remote_conv(remote);
    ACE_DEBUG((LM_DEBUG, "(%P|%t) RtpsUdpDataLink::received(NACK_FRAG) "
      "local %C remote %C\n", OPENDDS_STRING(local_conv).c_str(),
      OPENDDS_STRING(remote_conv).c_str()));
  }

  const ReaderInfoMap::iterator ri = rw->second.remote_readers_.find(remote);
  if (ri == rw->second.remote_readers_.end()) {
    VDBG((LM_WARNING, "(%P|%t) RtpsUdpDataLink::received(NACK_FRAG) "
      "WARNING ReaderInfo not found\n"));
    return;
  }

  if (nackfrag.count.value <= ri->second.nackfrag_recvd_count_) {
    VDBG((LM_WARNING, "(%P|%t) RtpsUdpDataLink::received(NACK_FRAG) "
      "WARNING Count indicates duplicate, dropping\n"));
    return;
  }

  ri->second.nackfrag_recvd_count_ = nackfrag.count.value;

  SequenceNumber seq;
  seq.setValue(nackfrag.writerSN.high, nackfrag.writerSN.low);
  ri->second.requested_frags_[seq] = nackfrag.fragmentNumberState;
  g.release();
  nack_reply_.schedule(); // timer will invoke send_nack_replies()
}

void
RtpsUdpDataLink::send_nack_replies()
{
  ACE_GUARD(ACE_Thread_Mutex, g, lock_);
  // Reply from local DW to remote DR: GAP or DATA
  using namespace OpenDDS::RTPS;
  typedef RtpsWriterMap::iterator rw_iter;
  for (rw_iter rw = writers_.begin(); rw != writers_.end(); ++rw) {

    // consolidate requests from N readers
    OPENDDS_SET(ACE_INET_Addr) recipients;
    DisjointSequence requests;
    RtpsWriter& writer = rw->second;

    //track if any messages have been fully acked by all readers
    SequenceNumber all_readers_ack = SequenceNumber::MAX_VALUE;

    typedef ReaderInfoMap::iterator ri_iter;
    const ri_iter end = writer.remote_readers_.end();
    for (ri_iter ri = writer.remote_readers_.begin(); ri != end; ++ri) {

      if (ri->second.cur_cumulative_ack_ < all_readers_ack) {
        all_readers_ack = ri->second.cur_cumulative_ack_;
      }

      for (size_t i = 0; i < ri->second.requested_changes_.size(); ++i) {
        const SequenceNumberSet& sn_state = ri->second.requested_changes_[i];
        SequenceNumber base;
        base.setValue(sn_state.bitmapBase.high, sn_state.bitmapBase.low);
        if (sn_state.numBits == 1 && !(sn_state.bitmap[0] & 1)
            && base == writer.heartbeat_high(ri->second)) {
          // Since there is an entry in requested_changes_, the DR must have
          // sent a non-final AckNack.  If the base value is the high end of
          // the heartbeat range, treat it as a request for that seq#.
          if (!writer.send_buff_.is_nil() && writer.send_buff_->contains(base)) {
            requests.insert(base);
          }
        } else {
          requests.insert(base, sn_state.numBits, sn_state.bitmap.get_buffer());
        }
      }

      if (ri->second.requested_changes_.size()) {
        if (locators_.count(ri->first)) {
          recipients.insert(locators_[ri->first].addr_);
          if (Transport_debug_level > 5) {
            const GuidConverter local_conv(rw->first), remote_conv(ri->first);
            ACE_DEBUG((LM_DEBUG, "RtpsUdpDataLink::send_nack_replies "
                       "local %C remote %C requested resend\n",
                       OPENDDS_STRING(local_conv).c_str(),
                       OPENDDS_STRING(remote_conv).c_str()));
          }
        }
        ri->second.requested_changes_.clear();
      }
    }

    DisjointSequence gaps;
    if (!requests.empty()) {
      if (writer.send_buff_.is_nil() || writer.send_buff_->empty()) {
        gaps = requests;
      } else {
        OPENDDS_VECTOR(SequenceRange) ranges = requests.present_sequence_ranges();
        SingleSendBuffer& sb = *writer.send_buff_;
        ACE_GUARD(TransportSendBuffer::LockType, guard, sb.strategy_lock());
        const RtpsUdpSendStrategy::OverrideToken ot =
          send_strategy_->override_destinations(recipients);
        for (size_t i = 0; i < ranges.size(); ++i) {
          if (Transport_debug_level > 5) {
            ACE_DEBUG((LM_DEBUG, "RtpsUdpDataLink::send_nack_replies "
                       "resend data %d-%d\n", int(ranges[i].first.getValue()),
                       int(ranges[i].second.getValue())));
          }
          sb.resend_i(ranges[i], &gaps);
        }
      }
    }

    send_nackfrag_replies(writer, gaps, recipients);

    if (!gaps.empty()) {
      if (Transport_debug_level > 5) {
        ACE_DEBUG((LM_DEBUG, "RtpsUdpDataLink::send_nack_replies "
                   "GAPs:"));
        gaps.dump();
      }
      ACE_Message_Block* mb_gap =
        marshal_gaps(rw->first, GUID_UNKNOWN, gaps, writer.durable_);
      if (mb_gap) {
        send_strategy_->send_rtps_control(*mb_gap, recipients);
        mb_gap->release();
      }
    }
    if (all_readers_ack == SequenceNumber::MAX_VALUE) {
      continue;
    }
  }
}

void
RtpsUdpDataLink::send_nackfrag_replies(RtpsWriter& writer,
                                       DisjointSequence& gaps,
                                       OPENDDS_SET(ACE_INET_Addr)& gap_recipients)
{
  typedef OPENDDS_MAP(SequenceNumber, DisjointSequence) FragmentInfo;
  OPENDDS_MAP(ACE_INET_Addr, FragmentInfo) requests;

  typedef ReaderInfoMap::iterator ri_iter;
  const ri_iter end = writer.remote_readers_.end();
  for (ri_iter ri = writer.remote_readers_.begin(); ri != end; ++ri) {

    if (ri->second.requested_frags_.empty() || !locators_.count(ri->first)) {
      continue;
    }

    const ACE_INET_Addr& remote_addr = locators_[ri->first].addr_;

    typedef OPENDDS_MAP(SequenceNumber, RTPS::FragmentNumberSet)::iterator rf_iter;
    const rf_iter rf_end = ri->second.requested_frags_.end();
    for (rf_iter rf = ri->second.requested_frags_.begin(); rf != rf_end; ++rf) {

      const SequenceNumber& seq = rf->first;
      if (writer.send_buff_->contains(seq)) {
        FragmentInfo& fi = requests[remote_addr];
        fi[seq].insert(rf->second.bitmapBase.value, rf->second.numBits,
                       rf->second.bitmap.get_buffer());
      } else {
        gaps.insert(seq);
        gap_recipients.insert(remote_addr);
      }
    }
    ri->second.requested_frags_.clear();
  }

  typedef OPENDDS_MAP(ACE_INET_Addr, FragmentInfo)::iterator req_iter;
  for (req_iter req = requests.begin(); req != requests.end(); ++req) {
    const FragmentInfo& fi = req->second;

    ACE_GUARD(TransportSendBuffer::LockType, guard,
      writer.send_buff_->strategy_lock());
    const RtpsUdpSendStrategy::OverrideToken ot =
      send_strategy_->override_destinations(req->first);

    for (FragmentInfo::const_iterator sn_iter = fi.begin();
         sn_iter != fi.end(); ++sn_iter) {
      const SequenceNumber& seq = sn_iter->first;
      writer.send_buff_->resend_fragments_i(seq, sn_iter->second);
    }
  }
}

void
RtpsUdpDataLink::process_acked_by_all_i(ACE_Guard<ACE_Thread_Mutex>& g, const RepoId& pub_id)
{
  using namespace OpenDDS::RTPS;
  typedef RtpsWriterMap::iterator rw_iter;
  typedef OPENDDS_MULTIMAP(SequenceNumber, TransportQueueElement*)::iterator iter_t;
  OPENDDS_VECTOR(RepoId) to_check;
  rw_iter rw = writers_.find(pub_id);
  if (rw == writers_.end()) {
    return;
  }
  RtpsWriter& writer = rw->second;
  if (!writer.elems_not_acked_.empty()) {

    //start with the max sequence number writer knows about and decrease
    //by what the min over all readers is
    SequenceNumber all_readers_ack = SequenceNumber::MAX_VALUE;

    typedef ReaderInfoMap::iterator ri_iter;
    const ri_iter end = writer.remote_readers_.end();
    for (ri_iter ri = writer.remote_readers_.begin(); ri != end; ++ri) {
      if (ri->second.cur_cumulative_ack_ < all_readers_ack) {
        all_readers_ack = ri->second.cur_cumulative_ack_;
      }
    }
    if (all_readers_ack == SequenceNumber::MAX_VALUE) {
      return;
    }
    OPENDDS_VECTOR(SequenceNumber) sns;
    //if any messages fully acked, call data delivered and remove from map

    iter_t it = writer.elems_not_acked_.begin();
    OPENDDS_SET(SequenceNumber) sns_to_release;
    while (it != writer.elems_not_acked_.end()) {
      if (it->first < all_readers_ack) {
        writer.to_deliver_.insert(RtpsWriter::SnToTqeMap::value_type(it->first, it->second));
        sns_to_release.insert(it->first);
        iter_t last = it;
        ++it;
        writer.elems_not_acked_.erase(last);
      } else {
        break;
      }
    }
    OPENDDS_SET(SequenceNumber)::iterator sns_it = sns_to_release.begin();
    while (sns_it != sns_to_release.end()) {
      writer.send_buff_->release_acked(*sns_it);
      ++sns_it;
    }
    TransportQueueElement* tqe_to_deliver;

    while (true) {
      rw_iter deliver_on_writer = writers_.find(pub_id);
      if (deliver_on_writer == writers_.end()) {
        break;
      }
      RtpsWriter& writer = deliver_on_writer->second;
      iter_t to_deliver_iter = writer.to_deliver_.begin();
      if (to_deliver_iter == writer.to_deliver_.end()) {
        break;
      }
      tqe_to_deliver = to_deliver_iter->second;
      writer.to_deliver_.erase(to_deliver_iter);
      g.release();

      tqe_to_deliver->data_delivered();
      g.acquire();
    }
  }
}

ACE_Message_Block*
RtpsUdpDataLink::marshal_gaps(const RepoId& writer, const RepoId& reader,
                              const DisjointSequence& gaps, bool durable)
{
  using namespace RTPS;
  // RTPS v2.1 8.3.7.4: the Gap sequence numbers are those in the range
  // [gapStart, gapListBase) and those in the SNSet.
  const SequenceNumber firstMissing = gaps.low(),
                       base = ++SequenceNumber(gaps.cumulative_ack());
  const SequenceNumber_t gapStart = {firstMissing.getHigh(),
                                     firstMissing.getLow()},
                         gapListBase = {base.getHigh(), base.getLow()};
  CORBA::ULong num_bits = 0;
  LongSeq8 bitmap;

  if (gaps.disjoint()) {
    bitmap.length(bitmap_num_longs(base, gaps.high()));
    gaps.to_bitmap(bitmap.get_buffer(), bitmap.length(), num_bits);

  } else {
    bitmap.length(1);
    bitmap[0] = 0;
    num_bits = 1;
  }

  GapSubmessage gap = {
    {GAP, 1 /*FLAG_E*/, 0 /*length determined below*/},
    reader.entityId,
    writer.entityId,
    gapStart,
    {gapListBase, num_bits, bitmap}
  };

  if (Transport_debug_level > 5) {
    const GuidConverter conv(writer);
    SequenceRange sr;
    sr.first.setValue(gap.gapStart.high, gap.gapStart.low);
    SequenceNumber srbase;
    srbase.setValue(gap.gapList.bitmapBase.high, gap.gapList.bitmapBase.low);
    sr.second = srbase.previous();
    ACE_DEBUG((LM_DEBUG, "(%P|%t) RtpsUdpDataLink::marshal_gaps "
              "GAP with range [%q, %q] from %C\n",
              sr.first.getValue(), sr.second.getValue(),
              OPENDDS_STRING(conv).c_str()));
  }

  size_t gap_size = 0, padding = 0;
  gen_find_size(gap, gap_size, padding);
  gap.smHeader.submessageLength =
    static_cast<CORBA::UShort>(gap_size + padding) - SMHDR_SZ;

  // For durable writers, change a non-directed Gap into multiple directed gaps.
  OPENDDS_VECTOR(RepoId) readers;
  if (durable && reader.entityId == ENTITYID_UNKNOWN) {
    if (Transport_debug_level > 5) {
      const GuidConverter local_conv(writer);
      ACE_DEBUG((LM_DEBUG, "RtpsUdpDataLink::marshal_gaps local %C "
                 "durable writer\n", OPENDDS_STRING(local_conv).c_str()));
    }
    const RtpsWriterMap::iterator iter = writers_.find(writer);
    RtpsWriter& rw = iter->second;
    for (ReaderInfoMap::iterator ri = rw.remote_readers_.begin();
         ri != rw.remote_readers_.end(); ++ri) {
      if (!ri->second.expecting_durable_data()) {
        readers.push_back(ri->first);
      } else if (Transport_debug_level > 5) {
        const GuidConverter remote_conv(ri->first);
        ACE_DEBUG((LM_DEBUG, "RtpsUdpDataLink::marshal_gaps reader "
                   "%C is expecting durable data, no GAP sent\n",
                   OPENDDS_STRING(remote_conv).c_str()));
      }
    }
    if (readers.empty()) return 0;
  }

  const size_t size_per_idst = INFO_DST_SZ + SMHDR_SZ,
    prefix_sz = sizeof(reader.guidPrefix);
  // no additional padding needed for INFO_DST
  const size_t total_sz = readers.empty() ? (gap_size + padding) :
    (readers.size() * (gap_size + padding + size_per_idst));

  ACE_Message_Block* mb_gap = new ACE_Message_Block(total_sz);
  //FUTURE: allocators?
  // byte swapping is handled in the operator<<() implementation
  Serializer ser(mb_gap, false, Serializer::ALIGN_CDR);
  if (readers.empty()) {
    ser << gap;
  } else {
    InfoDestinationSubmessage idst = {
      {INFO_DST, 1 /*FLAG_E*/, INFO_DST_SZ},
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
    };
    for (size_t i = 0; i < readers.size(); ++i) {
      std::memcpy(idst.guidPrefix, readers[i].guidPrefix, prefix_sz);
      gap.readerId = readers[i].entityId;
      ser << idst;
      ser << gap;
    }
  }
  return mb_gap;
}

void
RtpsUdpDataLink::durability_resend(TransportQueueElement* element)
{
  ACE_Message_Block* msg = const_cast<ACE_Message_Block*>(element->msg());
  send_strategy_->send_rtps_control(*msg,
                                    get_locator(element->subscription_id()));
}

void
RtpsUdpDataLink::send_durability_gaps(const RepoId& writer,
                                      const RepoId& reader,
                                      const DisjointSequence& gaps)
{
  ACE_Message_Block mb(RTPS::INFO_DST_SZ + RTPS::SMHDR_SZ);
  Serializer ser(&mb, false, Serializer::ALIGN_CDR);
  RTPS::InfoDestinationSubmessage info_dst = {
    {RTPS::INFO_DST, 1 /*FLAG_E*/, RTPS::INFO_DST_SZ},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
  };
  std::memcpy(info_dst.guidPrefix, reader.guidPrefix, sizeof(GuidPrefix_t));
  ser << info_dst;
  mb.cont(marshal_gaps(writer, reader, gaps));
  send_strategy_->send_rtps_control(mb, get_locator(reader));
  mb.cont()->release();
}

void
RtpsUdpDataLink::send_heartbeats()
{
  ACE_GUARD(ACE_Thread_Mutex, g, lock_);
  ACE_GUARD(ACE_Thread_Mutex, c, reader_no_longer_exists_lock_);

  if (writers_.empty() && interesting_readers_.empty()) {
    heartbeat_->disable();
  }

  using namespace OpenDDS::RTPS;
  OPENDDS_VECTOR(HeartBeatSubmessage) subm;
  OPENDDS_SET(ACE_INET_Addr) recipients;
  OPENDDS_VECTOR(TransportQueueElement*) pendingCallbacks;
  const ACE_Time_Value now = ACE_OS::gettimeofday();

  RepoIdSet writers_to_advertise;

  const ACE_Time_Value tv = ACE_OS::gettimeofday() - 10 * config_->heartbeat_period_;
  const ACE_Time_Value tv3 = ACE_OS::gettimeofday() - 3 * config_->heartbeat_period_;
  for (InterestingRemoteMapType::iterator pos = interesting_readers_.begin(),
         limit = interesting_readers_.end();
       pos != limit;
       ++pos) {
    if (pos->second.status == InterestingRemote::DOES_NOT_EXIST ||
        (pos->second.status == InterestingRemote::EXISTS && pos->second.last_activity < tv3)) {
      recipients.insert(pos->second.address);
      writers_to_advertise.insert(pos->second.localid);
    }
    if (pos->second.status == InterestingRemote::EXISTS && pos->second.last_activity < tv) {
      CallbackType callback(pos->first, pos->second);
      readerDoesNotExistCallbacks_.push_back(callback);
      pos->second.status = InterestingRemote::DOES_NOT_EXIST;
    }
  }

  typedef RtpsWriterMap::iterator rw_iter;
  for (rw_iter rw = writers_.begin(); rw != writers_.end(); ++rw) {
    const bool has_data = !rw->second.send_buff_.is_nil()
                          && !rw->second.send_buff_->empty();
    bool final = true, has_durable_data = false;
    SequenceNumber durable_max;

    typedef ReaderInfoMap::iterator ri_iter;
    const ri_iter end = rw->second.remote_readers_.end();
    for (ri_iter ri = rw->second.remote_readers_.begin(); ri != end; ++ri) {
      if ((has_data || !ri->second.handshake_done_)
          && locators_.count(ri->first)) {
        recipients.insert(locators_[ri->first].addr_);
        if (final && !ri->second.handshake_done_) {
          final = false;
        }
      }
      if (!ri->second.durable_data_.empty()) {
        const ACE_Time_Value expiration =
          ri->second.durable_timestamp_ + config_->durable_data_timeout_;
        if (now > expiration) {
          typedef OPENDDS_MAP(SequenceNumber, TransportQueueElement*)::iterator
            dd_iter;
          for (dd_iter it = ri->second.durable_data_.begin();
               it != ri->second.durable_data_.end(); ++it) {
            pendingCallbacks.push_back(it->second);
          }
          ri->second.durable_data_.clear();
          if (Transport_debug_level > 3) {
            const GuidConverter gw(rw->first), gr(ri->first);
            VDBG_LVL((LM_INFO, "(%P|%t) RtpsUdpDataLink::send_heartbeats - "
              "removed expired durable data for %C -> %C\n",
              OPENDDS_STRING(gw).c_str(), OPENDDS_STRING(gr).c_str()), 3);
          }
        } else {
          has_durable_data = true;
          if (ri->second.durable_data_.rbegin()->first > durable_max) {
            durable_max = ri->second.durable_data_.rbegin()->first;
          }
          if (locators_.count(ri->first)) {
            recipients.insert(locators_[ri->first].addr_);
          }
        }
      }
    }

    if (!rw->second.elems_not_acked_.empty()) {
      final = false;
    }

    if (writers_to_advertise.count(rw->first)) {
      final = false;
      writers_to_advertise.erase(rw->first);
    }

    if (final && !has_data && !has_durable_data) {
      continue;
    }

    const SequenceNumber firstSN = (rw->second.durable_ || !has_data)
                                   ? 1 : rw->second.send_buff_->low(),
        lastSN = std::max(durable_max,
                          has_data ? rw->second.send_buff_->high() : 1);

    const HeartBeatSubmessage hb = {
      {HEARTBEAT,
       CORBA::Octet(1 /*FLAG_E*/ | (final ? 2 /*FLAG_F*/ : 0)),
       HEARTBEAT_SZ},
      ENTITYID_UNKNOWN, // any matched reader may be interested in this
      rw->first.entityId,
      {firstSN.getHigh(), firstSN.getLow()},
      {lastSN.getHigh(), lastSN.getLow()},
      {++heartbeat_counts_[rw->first]}
    };
    subm.push_back(hb);
  }

  for (RepoIdSet::const_iterator pos = writers_to_advertise.begin(),
         limit = writers_to_advertise.end();
       pos != limit;
       ++pos) {
    const SequenceNumber SN = 1;
    const HeartBeatSubmessage hb = {
      {HEARTBEAT,
       CORBA::Octet(1 /*FLAG_E*/),
       HEARTBEAT_SZ},
      ENTITYID_UNKNOWN, // any matched reader may be interested in this
      pos->entityId,
      {SN.getHigh(), SN.getLow()},
      {SN.getHigh(), SN.getLow()},
      {++heartbeat_counts_[*pos]}
    };
    subm.push_back(hb);
  }

  if (!subm.empty()) {
    ACE_Message_Block mb((HEARTBEAT_SZ + SMHDR_SZ) * subm.size()); //FUTURE: allocators?
    // byte swapping is handled in the operator<<() implementation
    Serializer ser(&mb, false, Serializer::ALIGN_CDR);
    bool send_ok = true;
    for (size_t i = 0; i < subm.size(); ++i) {
      if (!(ser << subm[i])) {
        ACE_DEBUG((LM_ERROR, "(%P|%t) RtpsUdpDataLink::send_heartbeats() - "
          "failed to serialize HEARTBEAT submessage %B\n", i));
        send_ok = false;
        break;
      }
    }
    if (send_ok) {
      send_strategy_->send_rtps_control(mb, recipients);
    }
  }
  c.release();
  g.release();

  while(true) {
    c.acquire();
    if (readerDoesNotExistCallbacks_.empty()) {
      break;
    }
    OPENDDS_VECTOR(CallbackType)::iterator iter = readerDoesNotExistCallbacks_.begin();
    const RepoId& rid = iter->first;
    const InterestingRemote& remote = iter->second;
    readerDoesNotExistCallbacks_.erase(iter);
    c.release();
    remote.listener->reader_does_not_exist(rid, remote.localid);
  }

  for (size_t i = 0; i < pendingCallbacks.size(); ++i) {
    pendingCallbacks[i]->data_dropped();
  }
}

void
RtpsUdpDataLink::check_heartbeats()
{
  // Have any interesting writers timed out?
  const ACE_Time_Value tv = ACE_OS::gettimeofday() - 10 * config_->heartbeat_period_;
  ACE_GUARD(ACE_Thread_Mutex, g, lock_);
  ACE_GUARD(ACE_Thread_Mutex, c, writer_no_longer_exists_lock_);
  for (InterestingRemoteMapType::iterator pos = interesting_writers_.begin(), limit = interesting_writers_.end();
       pos != limit;
       ++pos) {
    if (pos->second.status == InterestingRemote::EXISTS && pos->second.last_activity < tv) {
      CallbackType callback(pos->first, pos->second);
      writerDoesNotExistCallbacks_.push_back(callback);
      pos->second.status = InterestingRemote::DOES_NOT_EXIST;
    }
  }
  c.release();
  g.release();
  while(true) {
    c.acquire();
    if (writerDoesNotExistCallbacks_.empty()) {
      break;
    }
    OPENDDS_VECTOR(CallbackType)::iterator iter = writerDoesNotExistCallbacks_.begin();
    const RepoId& rid = iter->first;
    const InterestingRemote& remote = iter->second;
    writerDoesNotExistCallbacks_.erase(iter);
    c.release();
    remote.listener->writer_does_not_exist(rid, remote.localid);
  }
}

void
RtpsUdpDataLink::send_heartbeats_manual(const TransportSendControlElement* tsce)
{
  using namespace OpenDDS::RTPS;

  const RepoId pub_id = tsce->publication_id();

  // Populate the recipients.
  OPENDDS_SET(ACE_INET_Addr) recipients;
  get_locators (pub_id, recipients);
  if (recipients.empty()) {
    return;
  }

  // Populate the sequence numbers and counter.

  SequenceNumber firstSN, lastSN;
  CORBA::Long counter;
  RtpsWriterMap::iterator pos = writers_.find (pub_id);
  if (pos != writers_.end ()) {
    // Reliable.
    const bool has_data = !pos->second.send_buff_.is_nil() && !pos->second.send_buff_->empty();
    SequenceNumber durable_max;
    const ACE_Time_Value now = ACE_OS::gettimeofday();
    for (ReaderInfoMap::const_iterator ri = pos->second.remote_readers_.begin(), end = pos->second.remote_readers_.end();
         ri != end;
         ++ri) {
      if (!ri->second.durable_data_.empty()) {
        const ACE_Time_Value expiration = ri->second.durable_timestamp_ + config_->durable_data_timeout_;
        if (now <= expiration &&
            ri->second.durable_data_.rbegin()->first > durable_max) {
          durable_max = ri->second.durable_data_.rbegin()->first;
        }
      }
    }
    firstSN = (pos->second.durable_ || !has_data) ? 1 : pos->second.send_buff_->low();
    lastSN = std::max(durable_max, has_data ? pos->second.send_buff_->high() : 1);
    counter = ++heartbeat_counts_[pos->first];
  } else {
    // Unreliable.
    firstSN = 1;
    lastSN = tsce->sequence();
    counter = ++this->best_effort_heartbeat_count_;
  }

  const HeartBeatSubmessage hb = {
    {HEARTBEAT,
     CORBA::Octet(1 /*FLAG_E*/ | 2 /*FLAG_F*/ | 4 /*FLAG_L*/),
     HEARTBEAT_SZ},
    ENTITYID_UNKNOWN, // any matched reader may be interested in this
    pub_id.entityId,
    {firstSN.getHigh(), firstSN.getLow()},
    {lastSN.getHigh(), lastSN.getLow()},
    {counter}
  };

  ACE_Message_Block mb((HEARTBEAT_SZ + SMHDR_SZ) * 1); //FUTURE: allocators?
  // byte swapping is handled in the operator<<() implementation
  Serializer ser(&mb, false, Serializer::ALIGN_CDR);
  if ((ser << hb)) {
    send_strategy_->send_rtps_control(mb, recipients);
  }
  else {
    ACE_DEBUG((LM_ERROR, "(%P|%t) RtpsUdpDataLink::send_heartbeats_manual() - "
               "failed to serialize HEARTBEAT submessage\n"));
  }
}

RtpsUdpDataLink::ReaderInfo::~ReaderInfo()
{
  expire_durable_data();
}

void
RtpsUdpDataLink::ReaderInfo::expire_durable_data()
{
  typedef OPENDDS_MAP(SequenceNumber, TransportQueueElement*)::iterator iter_t;
  for (iter_t it = durable_data_.begin(); it != durable_data_.end(); ++it) {
    it->second->data_dropped();
  }
}

bool
RtpsUdpDataLink::ReaderInfo::expecting_durable_data() const
{
  return durable_ &&
    (durable_timestamp_ == ACE_Time_Value::zero // DW hasn't resent yet
     || !durable_data_.empty());                // DW resent, not sent to reader
}

RtpsUdpDataLink::RtpsWriter::~RtpsWriter()
{
  if (!to_deliver_.empty()) {
    ACE_DEBUG((LM_WARNING, "(%P|%t) WARNING: RtpsWriter::~RtpsWriter - deleting with %d elements left to deliver\n", to_deliver_.size()));
  }
  if (!elems_not_acked_.empty()) {
    ACE_DEBUG((LM_WARNING, "(%P|%t) WARNING: RtpsWriter::~RtpsWriter - deleting with %d elements left not fully acknowledged\n", elems_not_acked_.size()));
  }
}

SequenceNumber
RtpsUdpDataLink::RtpsWriter::heartbeat_high(const ReaderInfo& ri) const
{
  const SequenceNumber durable_max =
    ri.durable_data_.empty() ? 0 : ri.durable_data_.rbegin()->first;
  const SequenceNumber data_max =
    send_buff_.is_nil() ? 0 : (send_buff_->empty() ? 0 : send_buff_->high());
  return std::max(durable_max, data_max);
}

void
RtpsUdpDataLink::RtpsWriter::add_elem_awaiting_ack(TransportQueueElement* element)
{
  elems_not_acked_.insert(SnToTqeMap::value_type(element->sequence(), element));
}


// Implementing TimedDelay and HeartBeat nested classes (for ACE timers)

void
RtpsUdpDataLink::TimedDelay::schedule()
{
  if (!scheduled_) {
    const long timer = outer_->get_reactor()->schedule_timer(this, 0, timeout_);

    if (timer == -1) {
      ACE_DEBUG((LM_ERROR, "(%P|%t) RtpsUdpDataLink::TimedDelay::schedule "
        "failed to schedule timer %p\n", ACE_TEXT("")));
    } else {
      scheduled_ = true;
    }
  }
}

void
RtpsUdpDataLink::TimedDelay::cancel()
{
  if (scheduled_) {
    outer_->get_reactor()->cancel_timer(this);
    scheduled_ = false;
  }
}

void
RtpsUdpDataLink::HeartBeat::enable()
{
  if (!enabled_) {
    const ACE_Time_Value& per = outer_->config_->heartbeat_period_;
    const long timer =
      outer_->get_reactor()->schedule_timer(this, 0, ACE_Time_Value::zero, per);

    if (timer == -1) {
      ACE_DEBUG((LM_ERROR, "(%P|%t) RtpsUdpDataLink::HeartBeat::enable"
        " failed to schedule timer %p\n", ACE_TEXT("")));
    } else {
      enabled_ = true;
    }
  }
}

void
RtpsUdpDataLink::HeartBeat::disable()
{
  if (enabled_) {
    outer_->get_reactor()->cancel_timer(this);
    enabled_ = false;
  }
}

void
RtpsUdpDataLink::send_final_acks (const RepoId& readerid)
{
  ACE_GUARD(ACE_Thread_Mutex, g, lock_);
  RtpsReaderMap::iterator rr = readers_.find (readerid);
  if (rr != readers_.end ()) {
    send_ack_nacks (rr, true);
  }
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
