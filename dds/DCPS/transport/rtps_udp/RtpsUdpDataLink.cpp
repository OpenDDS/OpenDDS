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
#include "dds/DCPS/transport/framework/RemoveAllVisitor.h"

#include "dds/DCPS/RTPS/RtpsCoreTypeSupportImpl.h"
#include "dds/DCPS/RTPS/BaseMessageUtils.h"
#include "dds/DCPS/RTPS/BaseMessageTypes.h"
#include "dds/DCPS/RTPS/MessageTypes.h"

#ifdef OPENDDS_SECURITY
#include "dds/DCPS/RTPS/SecurityHelpers.h"
#include "dds/DCPS/security/framework/SecurityRegistry.h"
#endif

#include "dds/DdsDcpsCoreTypeSupportImpl.h"

#include "dds/DCPS/Definitions.h"

#include "ace/Default_Constants.h"
#include "ace/Log_Msg.h"
#include "ace/Message_Block.h"
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
  return high < low ? CORBA::ULong(0) : std::min(CORBA::ULong(8), CORBA::ULong((high.getValue() - low.getValue() + 32) / 32));
}

bool bitmapNonEmpty(const OpenDDS::RTPS::SequenceNumberSet& snSet)
{
  for (CORBA::ULong i = 0; i < snSet.bitmap.length(); ++i) {
    if (snSet.bitmap[i]) {
      if (snSet.numBits >= (i + 1) * 32) {
        return true;
      }
      for (int bit = 31; bit >= 0; --bit) {
        if ((snSet.bitmap[i] & (1 << bit))
            && snSet.numBits >= i * 32 + (31 - bit)) {
          return true;
        }
      }
    }
  }
  return false;
}

bool compare_and_update_counts(CORBA::Long incoming, CORBA::Long& existing) {
  static const CORBA::Long ONE_QUARTER_MAX_POSITIVE = 0x20000000;
  static const CORBA::Long THREE_QUARTER_MAX_POSITIVE = 0x60000000;
  if (incoming <= existing &&
      !(incoming < ONE_QUARTER_MAX_POSITIVE && existing > THREE_QUARTER_MAX_POSITIVE)) {
    return false;
  }
  existing = incoming;
  return true;
}

}

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

const double QUICK_REPLY_DELAY_RATIO = 0.1;
const size_t ONE_SAMPLE_PER_PACKET = 1;

RtpsUdpDataLink::RtpsUdpDataLink(RtpsUdpTransport& transport,
                                 const GuidPrefix_t& local_prefix,
                                 const RtpsUdpInst& config,
                                 const ReactorTask_rch& reactor_task)
  : DataLink(transport, // 3 data link "attributes", below, are unused
             0,         // priority
             false,     // is_loopback
             false)     // is_active
  , reactor_task_(reactor_task)
  , job_queue_(DCPS::make_rch<DCPS::JobQueue>(reactor_task->get_reactor()))
  , multi_buff_(this, config.nak_depth_)
  , best_effort_heartbeat_count_(0)
  , nack_reply_(this, &RtpsUdpDataLink::send_nack_replies,
                config.nak_response_delay_)
  , heartbeat_reply_(this, &RtpsUdpDataLink::send_heartbeat_replies,
                     config.heartbeat_response_delay_)
  , heartbeat_(reactor_task->interceptor(), *this, &RtpsUdpDataLink::send_heartbeats)
  , heartbeatchecker_(reactor_task->interceptor(), *this, &RtpsUdpDataLink::check_heartbeats)
  , relay_beacon_(reactor_task->interceptor(), *this, &RtpsUdpDataLink::send_relay_beacon)
  , held_data_delivery_handler_(this)
  , max_bundle_size_(config.max_bundle_size_)
  , quick_reply_delay_(config.heartbeat_response_delay_ * QUICK_REPLY_DELAY_RATIO)
#ifdef OPENDDS_SECURITY
  , security_config_(Security::SecurityRegistry::instance()->default_config())
  , local_crypto_handle_(DDS::HANDLE_NIL)
#endif
{
  send_strategy_ = make_rch<RtpsUdpSendStrategy>(this, local_prefix);
  receive_strategy_ = make_rch<RtpsUdpReceiveStrategy>(this, local_prefix);
  std::memcpy(local_prefix_, local_prefix, sizeof(GuidPrefix_t));
}

RtpsUdpInst&
RtpsUdpDataLink::config() const
{
  return static_cast<RtpsUdpTransport&>(impl()).config();
}

bool
RtpsUdpDataLink::add_delayed_notification(TransportQueueElement* element)
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, writers_lock_, false);
  RtpsWriter_rch writer;
  RtpsWriterMap::iterator iter = writers_.find(element->publication_id());
  if (iter != writers_.end()) {
    writer = iter->second;
  }

  g.release();

  if (writer) {
    writer->add_elem_awaiting_ack(element);
    return true;
  }
  return false;
}

RemoveResult
RtpsUdpDataLink::remove_sample(const DataSampleElement* sample)
{
  RepoId pub_id = sample->get_pub_id();

  ACE_Guard<ACE_Thread_Mutex> g(writers_lock_);
  RtpsWriter_rch writer;
  RtpsWriterMap::iterator iter = writers_.find(pub_id);
  if (iter != writers_.end()) {
    writer = iter->second;
  }

  g.release();

  if (writer) {
    return writer->remove_sample(sample);
  }
  return REMOVE_NOT_FOUND;
}

void RtpsUdpDataLink::remove_all_msgs(const RepoId& pub_id)
{
  ACE_Guard<ACE_Thread_Mutex> g(writers_lock_);
  RtpsWriter_rch writer;
  RtpsWriterMap::iterator iter = writers_.find(pub_id);
  if (iter != writers_.end()) {
    writer = iter->second;
  }

  g.release();

  if (writer) {
    writer->remove_all_msgs();
  }
}

RemoveResult
RtpsUdpDataLink::RtpsWriter::remove_sample(const DataSampleElement* sample)
{
  bool found = false;
  SequenceNumber to_release;
  TransportQueueElement* tqe = 0;

  const SequenceNumber& seq = sample->get_header().sequence_;
  const char* const payload = sample->get_sample()->cont()->rd_ptr();
  const TransportQueueElement::MatchOnDataPayload modp(payload);
  SingleSendBuffer::BufferVec removed;

  ACE_Guard<ACE_Thread_Mutex> g(mutex_);

  RtpsUdpDataLink_rch link = link_.lock();
  if (!link) {
    return REMOVE_NOT_FOUND;
  }

  RemoveResult result = link->send_strategy()->remove_sample(sample);

  ACE_Guard<ACE_Thread_Mutex> g2(elems_not_acked_mutex_);

  if (!elems_not_acked_.empty()) {
    typedef SnToTqeMap::iterator iter_t;
    for (std::pair<iter_t, iter_t> er = elems_not_acked_.equal_range(seq); er.first != er.second; ++er.first) {
      if (modp.matches(*er.first->second)) {
        found = true;
        to_release = seq;
        tqe = er.first->second;
        elems_not_acked_.erase(er.first);
        break;
      }
    }
  }

  g2.release();

  if (found) {
    send_buff_->remove_acked(to_release, removed);
  }

  g.release();

  if (found) {
    for (size_t i = 0; i < removed.size(); ++i) {
      RemoveAllVisitor visitor;
      removed[i].first->accept_remove_visitor(visitor);
      delete removed[i].first;
      removed[i].second->release();
    }
    removed.clear();
    tqe->data_dropped(true);
    result = REMOVE_FOUND;
  }
  return result;
}

void
RtpsUdpDataLink::RtpsWriter::remove_all_msgs()
{
  ACE_GUARD(ACE_Thread_Mutex, g, mutex_);

  RtpsUdpDataLink_rch link = link_.lock();
  if (!link) {
    return;
  }

  send_buff_->retain_all(id_);

  link->send_strategy()->remove_all_msgs(id_);

  ACE_GUARD(ACE_Thread_Mutex, g2, elems_not_acked_mutex_);

  SnToTqeMap sn_tqe_map;
  sn_tqe_map.swap(elems_not_acked_);

  g2.release();

  SequenceNumber prev = SequenceNumber::ZERO();
  typedef SnToTqeMap::iterator iter_t;
  for (iter_t it = sn_tqe_map.begin(); it != sn_tqe_map.end(); ++it) {
    if (it->first != prev) {
      send_buff_->release_acked(it->first);
      prev = it->first;
    }
  }

  g.release();

  for (iter_t it = sn_tqe_map.begin(); it != sn_tqe_map.end(); ++it) {
    it->second->data_dropped(true);
  }
}

bool
RtpsUdpDataLink::open(const ACE_SOCK_Dgram& unicast_socket)
{
  unicast_socket_ = unicast_socket;

  RtpsUdpInst& cfg = config();

  NetworkConfigMonitor_rch ncm = TheServiceParticipant->network_config_monitor();

  DCPS::NetworkInterfaces nics;
  if (ncm) {
    nics = ncm->add_listener(*this);
  }

  if (cfg.use_multicast_) {
#ifdef ACE_HAS_MAC_OSX
    multicast_socket_.opts(ACE_SOCK_Dgram_Mcast::OPT_BINDADDR_NO |
                           ACE_SOCK_Dgram_Mcast::DEFOPT_NULLIFACE);
#endif
    if (ncm) {
      for (DCPS::NetworkInterfaces::const_iterator pos = nics.begin(), limit = nics.end(); pos != limit; ++pos) {
        join_multicast_group(*pos);
      }
    } else {
      NetworkInterface nic(0, cfg.multicast_interface_, true);
      nic.addresses.insert(ACE_INET_Addr());
      join_multicast_group(nic, true);
    }
  }

  if (!OpenDDS::DCPS::set_socket_multicast_ttl(unicast_socket_, cfg.ttl_)) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("RtpsUdpDataLink::open: ")
                      ACE_TEXT("failed to set TTL: %d\n"),
                      cfg.ttl_),
                     false);
  }

  if (cfg.send_buffer_size_ > 0) {
    int snd_size = cfg.send_buffer_size_;
    if (unicast_socket_.set_option(SOL_SOCKET,
                                SO_SNDBUF,
                                (void *) &snd_size,
                                sizeof(snd_size)) < 0
        && errno != ENOTSUP) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) ERROR: ")
                        ACE_TEXT("RtpsUdpDataLink::open: failed to set the send buffer size to %d errno %m\n"),
                        snd_size),
                       false);
    }
  }

  if (cfg.rcv_buffer_size_ > 0) {
    int rcv_size = cfg.rcv_buffer_size_;
    if (unicast_socket_.set_option(SOL_SOCKET,
                                SO_RCVBUF,
                                (void *) &rcv_size,
                                sizeof(int)) < 0
        && errno != ENOTSUP) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) ERROR: ")
                        ACE_TEXT("RtpsUdpDataLink::open: failed to set the receive buffer size to %d errno %m \n"),
                        rcv_size),
                       false);
    }
  }

  send_strategy()->send_buffer(&multi_buff_);

  if (start(send_strategy_,
            receive_strategy_, false) != 0) {
    stop_i();
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: ")
                      ACE_TEXT("UdpDataLink::open: start failed!\n")),
                     false);
  }

  if (cfg.rtps_relay_address() != ACE_INET_Addr() ||
      cfg.use_rtps_relay_) {
    relay_beacon_.enable(false, cfg.rtps_relay_beacon_period_);
  }

  return true;
}

void
RtpsUdpDataLink::add_address(const DCPS::NetworkInterface& nic,
                             const ACE_INET_Addr&)
{
  job_queue_->enqueue(make_rch<ChangeMulticastGroup>(rchandle_from(this), nic,
                                                     ChangeMulticastGroup::CMG_JOIN));
}

void
RtpsUdpDataLink::remove_address(const DCPS::NetworkInterface& nic,
                                const ACE_INET_Addr&)
{
  job_queue_->enqueue(make_rch<ChangeMulticastGroup>(rchandle_from(this), nic,
                                                     ChangeMulticastGroup::CMG_LEAVE));
}

void
RtpsUdpDataLink::join_multicast_group(const DCPS::NetworkInterface& nic,
                                      bool all_interfaces)
{
  network_change();

  if (!config().use_multicast_) {
    return;
  }

  if (joined_interfaces_.count(nic.name()) != 0 || nic.addresses.empty() || !nic.can_multicast()) {
    return;
  }

  if (!config().multicast_interface_.empty() && nic.name() != config().multicast_interface_) {
    return;
  }

  if (DCPS::DCPS_debug_level > 3) {
    ACE_DEBUG((LM_INFO,
               ACE_TEXT("(%P|%t) RtpsUdpDataLink::join_multicast_group ")
               ACE_TEXT("joining group %C %C:%hu\n"),
               nic.name().c_str(),
               config().multicast_group_address_str_.c_str(),
               config().multicast_group_address_.get_port_number()));
  }

  if (0 == multicast_socket_.join(config().multicast_group_address_, 1, all_interfaces ? 0 : ACE_TEXT_CHAR_TO_TCHAR(nic.name().c_str()))) {
    joined_interfaces_.insert(nic.name());
  } else {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: RtpsUdpDataLink::join_multicast_group(): ")
               ACE_TEXT("ACE_SOCK_Dgram_Mcast::join failed: %m\n")));
  }
}

void
RtpsUdpDataLink::leave_multicast_group(const DCPS::NetworkInterface& nic)
{
  if (joined_interfaces_.count(nic.name()) == 0 || !nic.addresses.empty()) {
    return;
  }

  if (DCPS::DCPS_debug_level > 3) {
    ACE_DEBUG((LM_INFO,
               ACE_TEXT("(%P|%t) RtpsUdpDataLink::leave_multicast_group ")
               ACE_TEXT("leaving group %C %C:%hu\n"),
               nic.name().c_str(),
               config().multicast_group_address_str_.c_str(),
               config().multicast_group_address_.get_port_number()));
  }

  if (0 == multicast_socket_.leave(config().multicast_group_address_, ACE_TEXT_CHAR_TO_TCHAR(nic.name().c_str()))) {
    joined_interfaces_.erase(nic.name());
  } else {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: RtpsUdpDataLink::leave_multicast_group(): ")
               ACE_TEXT("ACE_SOCK_Dgram_Mcast::leave failed: %m\n")));
  }
}

void
RtpsUdpDataLink::add_locator(const RepoId& remote_id,
                             const ACE_INET_Addr& address,
                             bool requires_inline_qos)
{
  ACE_GUARD(ACE_Thread_Mutex, g, locators_lock_);
  locators_[remote_id] = RemoteInfo(address, requires_inline_qos);

  if (DCPS::DCPS_debug_level > 3) {
    ACE_TCHAR addr_buff[256] = {};
    address.addr_to_string(addr_buff, 256);
    ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) RtpsUdpDataLink::add_locator %C is now at %s\n"), LogGuid(remote_id).c_str(), addr_buff));
  }
}

void RtpsUdpDataLink::filterBestEffortReaders(const ReceivedDataSample& ds, RepoIdSet& selected, RepoIdSet& withheld)
{
  ACE_GUARD(ACE_Thread_Mutex, g, readers_lock_);
  const RepoId& writer = ds.header_.publication_id_;
  const SequenceNumber& seq = ds.header_.sequence_;
  WriterToSeqReadersMap::iterator w = writer_to_seq_best_effort_readers_.find(writer);
  if (w != writer_to_seq_best_effort_readers_.end()) {
    if (w->second.seq < seq) {
      w->second.seq = seq;
      selected.insert(w->second.readers.begin(), w->second.readers.end());
    } else {
      withheld.insert(w->second.readers.begin(), w->second.readers.end());
    }
  } // else the writer is not associated with best effort readers
}

int
RtpsUdpDataLink::make_reservation(const RepoId& rpi,
                                  const RepoId& lsi,
                                  const TransportReceiveListener_wrch& trl,
                                  bool reliable)
{
  ACE_Guard<ACE_Thread_Mutex> guard(readers_lock_);
  if (reliable) {
    RtpsReaderMap::iterator rr = readers_.find(lsi);
    if (rr == readers_.end()) {
      pending_reliable_readers_.insert(lsi);
    }
  }
  guard.release();
  return DataLink::make_reservation(rpi, lsi, trl, reliable);
}

void
RtpsUdpDataLink::associated(const RepoId& local_id, const RepoId& remote_id,
                            bool local_reliable, bool remote_reliable,
                            bool local_durable, bool remote_durable)
{
  const GuidConverter conv(local_id);

  if (!local_reliable) {
    if (conv.isReader()) {
      ACE_GUARD(ACE_Thread_Mutex, g, readers_lock_);
      WriterToSeqReadersMap::iterator i = writer_to_seq_best_effort_readers_.find(remote_id);
      if (i == writer_to_seq_best_effort_readers_.end()) {
        writer_to_seq_best_effort_readers_.insert(WriterToSeqReadersMap::value_type(remote_id, SeqReaders(local_id)));
      } else if (i->second.readers.find(local_id) == i->second.readers.end()) {
        i->second.readers.insert(local_id);
      }
    }
    return;
  }

  bool enable_heartbeat = false;

  if (conv.isWriter()) {
    if (remote_reliable) {
      ACE_GUARD(ACE_Thread_Mutex, g, writers_lock_);
      // Insert count if not already there.
      RtpsWriterMap::iterator rw = writers_.find(local_id);
      if (rw == writers_.end()) {
        RtpsUdpDataLink_rch link(this, OpenDDS::DCPS::inc_count());
        CORBA::Long hb_start = 0;
        HeartBeatCountMapType::iterator hbc_it = heartbeat_counts_.find(local_id);
        if (hbc_it != heartbeat_counts_.end()) {
          hb_start = hbc_it->second;
          heartbeat_counts_.erase(hbc_it);
        }
        RtpsWriter_rch writer = make_rch<RtpsWriter>(link, local_id, local_durable, hb_start, multi_buff_.capacity());
        rw = writers_.insert(RtpsWriterMap::value_type(local_id, writer)).first;
      }
      RtpsWriter_rch writer = rw->second;
      enable_heartbeat = true;
      g.release();
      writer->add_reader(remote_id, ReaderInfo(remote_durable));
    } else {
      invoke_on_start_callbacks(local_id, remote_id, true);
    }
  } else if (conv.isReader()) {
    if (remote_reliable) {
      ACE_GUARD(ACE_Thread_Mutex, g, readers_lock_);
      RtpsReaderMap::iterator rr = readers_.find(local_id);
      if (rr == readers_.end()) {
        pending_reliable_readers_.erase(local_id);
        RtpsUdpDataLink_rch link(this, OpenDDS::DCPS::inc_count());
        RtpsReader_rch reader = make_rch<RtpsReader>(link, local_id, local_durable);
        rr = readers_.insert(RtpsReaderMap::value_type(local_id, reader)).first;
      }
      RtpsReader_rch reader = rr->second;
      readers_of_writer_.insert(RtpsReaderMultiMap::value_type(remote_id, rr->second));
      g.release();
      reader->add_writer(remote_id, WriterInfo());
    } else {
      invoke_on_start_callbacks(local_id, remote_id, true);
    }
  }

  if (enable_heartbeat) {
    heartbeat_.enable(true, config().heartbeat_period_);
  }
}

bool
RtpsUdpDataLink::check_handshake_complete(const RepoId& local_id,
                                          const RepoId& remote_id)
{
  const GuidConverter conv(local_id);
  if (conv.isWriter()) {
    RtpsWriter_rch writer;
    {
      ACE_Guard<ACE_Thread_Mutex> guard(writers_lock_);
      RtpsWriterMap::iterator rw = writers_.find(local_id);
      if (rw == writers_.end()) {
        return true; // not reliable, no handshaking
      }
      writer = rw->second;
    }
    return writer->is_reader_handshake_done(remote_id);
  } else if (conv.isReader()) {
    RtpsReader_rch reader;
    {
      ACE_Guard<ACE_Thread_Mutex> guard(readers_lock_);
      RtpsReaderMap::iterator rr = readers_.find(local_id);
      if (rr == readers_.end()) {
        return true; // not reliable, no handshaking
      }
      reader = rr->second;
    }
    return reader->is_writer_handshake_done(remote_id);
  }
  return false;
}

void
RtpsUdpDataLink::register_for_reader(const RepoId& writerid,
                                     const RepoId& readerid,
                                     const ACE_INET_Addr& address,
                                     OpenDDS::DCPS::DiscoveryListener* listener)
{
  ACE_GUARD(ACE_Thread_Mutex, g, writers_lock_);
  bool enableheartbeat = interesting_readers_.empty();
  interesting_readers_.insert(
    InterestingRemoteMapType::value_type(
      readerid,
      InterestingRemote(writerid, address, listener)));
  if (heartbeat_counts_.find(writerid) == heartbeat_counts_.end()) {
    heartbeat_counts_[writerid] = 0;
  }
  g.release();
  if (enableheartbeat) {
    heartbeat_.enable(false, config().heartbeat_period_);
  }
}

void
RtpsUdpDataLink::unregister_for_reader(const RepoId& writerid,
                                       const RepoId& readerid)
{
  ACE_GUARD(ACE_Thread_Mutex, g, writers_lock_);
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
  ACE_GUARD(ACE_Thread_Mutex, g, readers_lock_);
  bool enableheartbeatchecker = interesting_writers_.empty();
  interesting_writers_.insert(
    InterestingRemoteMapType::value_type(
      writerid,
      InterestingRemote(readerid, address, listener)));
  g.release();
  if (enableheartbeatchecker) {
    heartbeatchecker_.enable(false, config().heartbeat_period_);
  }
}

void
RtpsUdpDataLink::unregister_for_writer(const RepoId& readerid,
                                       const RepoId& writerid)
{
  ACE_GUARD(ACE_Thread_Mutex, g, readers_lock_);
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
RtpsUdpDataLink::RtpsWriter::pre_stop_helper(OPENDDS_VECTOR(TransportQueueElement*)& to_drop)
{
  typedef SnToTqeMap::iterator iter_t;

  ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
  ACE_GUARD(ACE_Thread_Mutex, g2, elems_not_acked_mutex_);

  if (!elems_not_acked_.empty()) {
    OPENDDS_SET(SequenceNumber) sns_to_release;
    iter_t iter = elems_not_acked_.begin();
    while (iter != elems_not_acked_.end()) {
      to_drop.push_back(iter->second);
      sns_to_release.insert(iter->first);
      elems_not_acked_.erase(iter);
      iter = elems_not_acked_.begin();
    }
    OPENDDS_SET(SequenceNumber)::iterator sns_it = sns_to_release.begin();
    while (sns_it != sns_to_release.end()) {
      send_buff_->release_acked(*sns_it);
      ++sns_it;
    }
  }
}

void
RtpsUdpDataLink::pre_stop_i()
{
  DBG_ENTRY_LVL("RtpsUdpDataLink","pre_stop_i",6);
  DataLink::pre_stop_i();
  OPENDDS_VECTOR(TransportQueueElement*) to_drop;
  {
    ACE_GUARD(ACE_Thread_Mutex, g, writers_lock_);

    RtpsWriterMap::iterator iter = writers_.begin();
    while (iter != writers_.end()) {
      RtpsWriter_rch writer = iter->second;
      writer->pre_stop_helper(to_drop);
      RtpsWriterMap::iterator last = iter;
      ++iter;
      heartbeat_counts_.erase(last->first);
      writers_.erase(last);
    }
  }
  typedef OPENDDS_VECTOR(TransportQueueElement*)::iterator tqe_iter;
  tqe_iter drop_it = to_drop.begin();
  while (drop_it != to_drop.end()) {
    (*drop_it)->data_dropped(true);
    ++drop_it;
  }
  {
    ACE_GUARD(ACE_Thread_Mutex, g, readers_lock_);

    RtpsReaderMap::iterator iter = readers_.begin();
    while (iter != readers_.end()) {
      RtpsReader_rch reader = iter->second;
      reader->pre_stop_helper();
      ++iter;
    }
  }
}

void
RtpsUdpDataLink::release_reservations_i(const RepoId& remote_id,
                                        const RepoId& local_id)
{
  OPENDDS_VECTOR(TransportQueueElement*) to_drop;
  using std::pair;
  const GuidConverter conv(local_id);
  if (conv.isWriter()) {
    ACE_GUARD(ACE_Thread_Mutex, g, writers_lock_);
    RtpsWriterMap::iterator rw = writers_.find(local_id);

    if (rw != writers_.end()) {
      RtpsWriter_rch writer = rw->second;
      g.release();
      writer->remove_reader(remote_id);

      if (writer->reader_count() == 0) {
        writer->pre_stop_helper(to_drop);
        const CORBA::ULong hbc = writer->get_heartbeat_count();

        ACE_GUARD(ACE_Thread_Mutex, h, writers_lock_);
        rw = writers_.find(local_id);
        if (rw != writers_.end()) {
          heartbeat_counts_[rw->first] = hbc;
          writers_.erase(rw);
        }
      } else {
        writer->process_acked_by_all();
      }
    }

  } else if (conv.isReader()) {
    ACE_GUARD(ACE_Thread_Mutex, g, readers_lock_);
    RtpsReaderMap::iterator rr = readers_.find(local_id);

    if (rr != readers_.end()) {
      for (pair<RtpsReaderMultiMap::iterator, RtpsReaderMultiMap::iterator> iters =
             readers_of_writer_.equal_range(remote_id);
           iters.first != iters.second;) {
        if (iters.first->first == local_id) {
          readers_of_writer_.erase(iters.first++);
        } else {
          ++iters.first;
        }
      }

      RtpsReader_rch reader = rr->second;
      g.release();

      reader->remove_writer(remote_id);

      if (reader->writer_count() == 0) {
        {
          ACE_GUARD(ACE_Thread_Mutex, h, readers_lock_);
          rr = readers_.find(local_id);
          if (rr != readers_.end()) {
            readers_.erase(rr);
          }
        }
      }
    } else {
      WriterToSeqReadersMap::iterator w = writer_to_seq_best_effort_readers_.find(remote_id);
      if (w != writer_to_seq_best_effort_readers_.end()) {
        RepoIdSet::iterator r = w->second.readers.find(local_id);
        if (r != w->second.readers.end()) {
          w->second.readers.erase(r);
          if (w->second.readers.empty()) {
            writer_to_seq_best_effort_readers_.erase(w);
          }
        }
      }
    }
  }

  typedef OPENDDS_VECTOR(TransportQueueElement*)::iterator tqe_iter;
  tqe_iter drop_it = to_drop.begin();
  while (drop_it != to_drop.end()) {
    (*drop_it)->data_dropped(true);
    ++drop_it;
  }
}

void
RtpsUdpDataLink::stop_i()
{
  NetworkConfigMonitor_rch ncm = TheServiceParticipant->network_config_monitor();
  if (ncm) {
    ncm->remove_listener(*this);
  }

  nack_reply_.cancel();
  heartbeat_reply_.cancel();
  heartbeat_.disable_and_wait();
  heartbeatchecker_.disable_and_wait();
  relay_beacon_.disable_and_wait();
  unicast_socket_.close();
  multicast_socket_.close();
}

RcHandle<SingleSendBuffer>
RtpsUdpDataLink::get_writer_send_buffer(const RepoId& pub_id)
{
  RcHandle<SingleSendBuffer> result;
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, writers_lock_, result);

  const RtpsWriterMap::iterator wi = writers_.find(pub_id);
  if (wi != writers_.end()) {
    result = wi->second->get_send_buff();
  }
  return result;
}

// Implementing MultiSendBuffer nested class

void
RtpsUdpDataLink::MultiSendBuffer::insert(SequenceNumber /*transport_seq*/,
                                         TransportSendStrategy::QueueType* q,
                                         ACE_Message_Block* chain)
{
  // Called from TransportSendStrategy::send_packet().
  // RtpsUdpDataLink is not locked at this point, and is only locked
  // to grab the appropriate writer send buffer via get_writer_send_buffer()
  const TransportQueueElement* const tqe = q->peek();
  const SequenceNumber seq = tqe->sequence();
  if (seq == SequenceNumber::SEQUENCENUMBER_UNKNOWN()) {
    return;
  }

  const RepoId pub_id = tqe->publication_id();

  RcHandle<SingleSendBuffer> send_buff = outer_->get_writer_send_buffer(pub_id);
  if (send_buff.is_nil()) {
    return;
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
RtpsUdpDataLink::RtpsWriter::customize_queue_element_helper(
  TransportQueueElement* element,
  bool requires_inline_qos,
  MetaSubmessageVec& meta_submessages,
  bool& deliver_after_send)
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, mutex_, 0);

  RtpsUdpDataLink_rch link = link_.lock();
  if (!link) {
    return 0;
  }

  bool gap_ok = true;
  DestToEntityMap gap_receivers;
  if (!remote_readers_.empty()) {
    for (ReaderInfoMap::iterator ri = remote_readers_.begin();
         ri != remote_readers_.end(); ++ri) {
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

  RTPS::SubmessageSeq subm;

  if (gap_ok) {
    add_gap_submsg_i(subm, *element, gap_receivers);
  }

  const SequenceNumber seq = element->sequence();
  if (seq != SequenceNumber::SEQUENCENUMBER_UNKNOWN()) {
    expected_ = seq;
    ++expected_;
  }

  TransportSendElement* tse = dynamic_cast<TransportSendElement*>(element);
  TransportCustomizedElement* tce =
    dynamic_cast<TransportCustomizedElement*>(element);
  TransportSendControlElement* tsce =
    dynamic_cast<TransportSendControlElement*>(element);

  Message_Block_Ptr data;
  bool durable = false;

  const ACE_Message_Block* msg = element->msg();
  const RepoId pub_id = element->publication_id();

  // Based on the type of 'element', find and duplicate the data payload
  // continuation block.
  if (tsce) {        // Control message
    if (RtpsSampleHeader::control_message_supported(tsce->header().message_id_)) {
      data.reset(msg->cont()->duplicate());
      // Create RTPS Submessage(s) in place of the OpenDDS DataSampleHeader
      RtpsSampleHeader::populate_data_control_submessages(
                subm, *tsce, requires_inline_qos);
    } else if (tsce->header().message_id_ == END_HISTORIC_SAMPLES) {
      end_historic_samples_i(tsce->header(), msg->cont());
      g.release();
      element->data_delivered();
      return 0;
    } else if (tsce->header().message_id_ == DATAWRITER_LIVELINESS) {
      send_heartbeats_manual_i(meta_submessages);
      deliver_after_send = true;
      return 0;
    } else {
      g.release();
      element->data_dropped(true /*dropped_by_transport*/);
      return 0;
    }

  } else if (tse) {  // Basic data message
    // {DataSampleHeader} -> {Data Payload}
    data.reset(msg->cont()->duplicate());
    const DataSampleElement* dsle = tse->sample();
    // Create RTPS Submessage(s) in place of the OpenDDS DataSampleHeader
    RtpsSampleHeader::populate_data_sample_submessages(
              subm, *dsle, requires_inline_qos);
    durable = dsle->get_header().historic_sample_;

  } else if (tce) {  // Customized data message
    // {DataSampleHeader} -> {Content Filtering GUIDs} -> {Data Payload}
    data.reset(msg->cont()->cont()->duplicate());
    const DataSampleElement* dsle = tce->original_send_element()->sample();
    // Create RTPS Submessage(s) in place of the OpenDDS DataSampleHeader
    RtpsSampleHeader::populate_data_sample_submessages(
              subm, *dsle, requires_inline_qos);
    durable = dsle->get_header().historic_sample_;

  } else {
    return element;
  }

#ifdef OPENDDS_SECURITY
  {
    GuardType guard(link->strategy_lock_);
    if (link->send_strategy_) {
      link->send_strategy()->encode_payload(pub_id, data, subm);
    }
  }
#endif

  Message_Block_Ptr hdr(submsgs_to_msgblock(subm));
  hdr->cont(data.release());
  RtpsCustomizedElement* rtps =
    new RtpsCustomizedElement(element, move(hdr));

  // Handle durability resends
  if (durable) {
    const RepoId sub = element->subscription_id();
    if (sub != GUID_UNKNOWN) {
      ReaderInfoMap::iterator ri = remote_readers_.find(sub);
      if (ri != remote_readers_.end()) {
        ri->second.durable_data_[rtps->sequence()] = rtps;
        ri->second.durable_timestamp_.set_to_now();
        if (Transport_debug_level > 3) {
          const GuidConverter conv(pub_id), sub_conv(sub);
          ACE_DEBUG((LM_DEBUG,
            "(%P|%t) RtpsUdpDataLink::customize_queue_element() - "
            "storing durable data for local %C remote %C seq %q\n",
            OPENDDS_STRING(conv).c_str(), OPENDDS_STRING(sub_conv).c_str(),
            rtps->sequence().getValue()));
        }
        return 0;
      }
    }
  }

  return rtps;
}

TransportQueueElement*
RtpsUdpDataLink::customize_queue_element_non_reliable_i(
  TransportQueueElement* element,
  bool requires_inline_qos,
  MetaSubmessageVec& meta_submessages,
  bool& deliver_after_send,
  ACE_Guard<ACE_Thread_Mutex>& guard)
{
  RTPS::SubmessageSeq subm;

  TransportSendElement* tse = dynamic_cast<TransportSendElement*>(element);
  TransportCustomizedElement* tce =
    dynamic_cast<TransportCustomizedElement*>(element);
  TransportSendControlElement* tsce =
    dynamic_cast<TransportSendControlElement*>(element);

  Message_Block_Ptr data;

  const ACE_Message_Block* msg = element->msg();

  // Based on the type of 'element', find and duplicate the data payload
  // continuation block.
  if (tsce) {        // Control message
    if (RtpsSampleHeader::control_message_supported(tsce->header().message_id_)) {
      data.reset(msg->cont()->duplicate());
      // Create RTPS Submessage(s) in place of the OpenDDS DataSampleHeader
      RtpsSampleHeader::populate_data_control_submessages(
                subm, *tsce, requires_inline_qos);
    } else if (tsce->header().message_id_ == DATAWRITER_LIVELINESS) {
      send_heartbeats_manual_i(tsce, meta_submessages);
      deliver_after_send = true;
      return 0;
    } else {
      guard.release();
      element->data_dropped(true /*dropped_by_transport*/);
      return 0;
    }

  } else if (tse) {  // Basic data message
    // {DataSampleHeader} -> {Data Payload}
    data.reset(msg->cont()->duplicate());
    const DataSampleElement* dsle = tse->sample();
    // Create RTPS Submessage(s) in place of the OpenDDS DataSampleHeader
    RtpsSampleHeader::populate_data_sample_submessages(
              subm, *dsle, requires_inline_qos);

  } else if (tce) {  // Customized data message
    // {DataSampleHeader} -> {Content Filtering GUIDs} -> {Data Payload}
    data.reset(msg->cont()->cont()->duplicate());
    const DataSampleElement* dsle = tce->original_send_element()->sample();
    // Create RTPS Submessage(s) in place of the OpenDDS DataSampleHeader
    RtpsSampleHeader::populate_data_sample_submessages(
              subm, *dsle, requires_inline_qos);

  } else {
    return element;
  }

#ifdef OPENDDS_SECURITY
  const RepoId pub_id = element->publication_id();

  {
    GuardType guard(strategy_lock_);
    if (send_strategy_) {
      send_strategy()->encode_payload(pub_id, data, subm);
    }
  }
#endif

  Message_Block_Ptr hdr(submsgs_to_msgblock(subm));
  hdr->cont(data.release());
  return new RtpsCustomizedElement(element, move(hdr));
}

TransportQueueElement*
RtpsUdpDataLink::customize_queue_element(TransportQueueElement* element)
{
  const ACE_Message_Block* msg = element->msg();
  if (!msg) {
    return element;
  }

  const RepoId pub_id = element->publication_id();
  GUIDSeq_var peers = peer_ids(pub_id);

  ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, writers_lock_, 0);

  bool require_iq = requires_inline_qos(peers);

  const RtpsWriterMap::iterator rw = writers_.find(pub_id);
  MetaSubmessageVec meta_submessages;
  RtpsWriter_rch writer;
  TransportQueueElement* result;
  bool deliver_after_send = false;
  if (rw != writers_.end()) {
    writer = rw->second;
    guard.release();
    result = writer->customize_queue_element_helper(element, require_iq, meta_submessages, deliver_after_send);
  } else {
    result = customize_queue_element_non_reliable_i(element, require_iq, meta_submessages, deliver_after_send, guard);
    guard.release();
  }

  send_bundled_submessages(meta_submessages);

  if (deliver_after_send) {
    element->data_delivered();
  }

  return result;
}

void
RtpsUdpDataLink::RtpsWriter::end_historic_samples_i(const DataSampleHeader& header,
                                                    ACE_Message_Block* body)
{
  // Set the ReaderInfo::durable_timestamp_ for the case where no
  // durable samples exist in the DataWriter.
  if (durable_) {
    const MonotonicTimePoint now = MonotonicTimePoint::now();
    RepoId sub = GUID_UNKNOWN;
    if (body && header.message_length_ >= sizeof(sub)) {
      std::memcpy(&sub, body->rd_ptr(), header.message_length_);
    }
    typedef ReaderInfoMap::iterator iter_t;
    if (sub == GUID_UNKNOWN) {
      if (Transport_debug_level > 3) {
        const GuidConverter conv(id_);
        ACE_DEBUG((LM_DEBUG, "(%P|%t) RtpsUdpDataLink::end_historic_samples "
                   "local %C all readers\n", OPENDDS_STRING(conv).c_str()));
      }
      for (iter_t iter = remote_readers_.begin();
           iter != remote_readers_.end(); ++iter) {
        if (iter->second.durable_) {
          iter->second.durable_timestamp_ = now;
        }
      }
    } else {
      iter_t iter = remote_readers_.find(sub);
      if (iter != remote_readers_.end()) {
        if (iter->second.durable_) {
          iter->second.durable_timestamp_ = now;
          if (Transport_debug_level > 3) {
            const GuidConverter conv(id_), sub_conv(sub);
            ACE_DEBUG((LM_DEBUG, "(%P|%t) RtpsUdpDataLink::end_historic_samples"
                       " local %C remote %C\n", OPENDDS_STRING(conv).c_str(),
                       OPENDDS_STRING(sub_conv).c_str()));
          }
        }
      }
    }

    // This should always succeed, since this method is called by customize_queue_element_helper,
    // which already holds a RCH to the datalink... this is just to avoid adding another parameter to pass it
    RtpsUdpDataLink_rch link = link_.lock();
    if (link) {
      link->heartbeat_.enable(true, link->config().heartbeat_period_);
    }
  }
}

bool RtpsUdpDataLink::requires_inline_qos(const GUIDSeq_var& peers)
{
  if (force_inline_qos_) {
    // Force true for testing purposes
    return true;
  } else {
    if (!peers.ptr()) {
      return false;
    }
    for (CORBA::ULong i = 0; i < peers->length(); ++i) {
      const RemoteInfoMap::const_iterator iter = locators_.find(peers[i]);
      if (iter != locators_.end() && iter->second.requires_inline_qos_) {
        return true;
      }
    }
    return false;
  }
}

bool RtpsUdpDataLink::force_inline_qos_ = false;

void
RtpsUdpDataLink::RtpsWriter::add_gap_submsg_i(RTPS::SubmessageSeq& msg,
                                              const TransportQueueElement& tqe,
                                              const DestToEntityMap& dtem)
{
  // These are the GAP submessages that we'll send directly in-line with the
  // DATA when we notice that the DataWriter has deliberately skipped seq #s.
  // There are other GAP submessages generated in meta_submessage to reader ACKNACKS,
  // see send_nack_replies().
  using namespace OpenDDS::RTPS;

  const SequenceNumber seq = tqe.sequence();
  const RepoId pub = tqe.publication_id();
  if (seq == SequenceNumber::SEQUENCENUMBER_UNKNOWN() || pub == GUID_UNKNOWN
      || tqe.subscription_id() != GUID_UNKNOWN) {
    return;
  }

  if (seq != expected_) {
    SequenceNumber firstMissing = expected_;

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
      {GAP, FLAG_E, 0 /*length determined below*/},
      ENTITYID_UNKNOWN, // readerId: applies to all matched readers
      pub.entityId,
      gapStart,
      {gapListBase, 1, bitmap}
    };

    size_t size = 0, padding = 0;
    gen_find_size(gap, size, padding);
    gap.smHeader.submessageLength =
      static_cast<CORBA::UShort>(size + padding) - SMHDR_SZ;

    if (!durable_) {
      const CORBA::ULong i = msg.length();
      msg.length(i + 1);
      msg[i].gap_sm(gap);
    } else {
      InfoDestinationSubmessage idst = {
        {INFO_DST, FLAG_E, INFO_DST_SZ},
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
  RepoId local;
  std::memcpy(local.guidPrefix, local_prefix_, sizeof(GuidPrefix_t));
  local.entityId = data.readerId;

  RepoId src;
  std::memcpy(src.guidPrefix, src_prefix, sizeof(GuidPrefix_t));
  src.entityId = data.writerId;

  OPENDDS_VECTOR(RtpsReader_rch) to_call;
  {
    ACE_GUARD(ACE_Thread_Mutex, g, readers_lock_);
    if (local.entityId == ENTITYID_UNKNOWN) {
      typedef std::pair<RtpsReaderMultiMap::iterator, RtpsReaderMultiMap::iterator> RRMM_IterRange;
      for (RRMM_IterRange iters = readers_of_writer_.equal_range(src); iters.first != iters.second; ++iters.first) {
        to_call.push_back(iters.first->second);
      }
      if (!pending_reliable_readers_.empty()) {
        GuardType guard(strategy_lock_);
        RtpsUdpReceiveStrategy* trs = receive_strategy();
        if (trs) {
          for (RepoIdSet::const_iterator it = pending_reliable_readers_.begin();
               it != pending_reliable_readers_.end(); ++it)
          {
            trs->withhold_data_from(*it);
          }
        }
      }
    } else {
      const RtpsReaderMap::iterator rr = readers_.find(local);
      if (rr != readers_.end()) {
        to_call.push_back(rr->second);
      } else if (pending_reliable_readers_.count(local)) {
        GuardType guard(strategy_lock_);
        RtpsUdpReceiveStrategy* trs = receive_strategy();
        if (trs) {
          trs->withhold_data_from(local);
        }
      }
    }
  }
  MetaSubmessageVec meta_submessages;
  for (OPENDDS_VECTOR(RtpsReader_rch)::const_iterator it = to_call.begin(); it < to_call.end(); ++it) {
    (*it)->process_data_i(data, src, meta_submessages);
  }
  send_bundled_submessages(meta_submessages);
}

void
RtpsUdpDataLink::RtpsReader::pre_stop_helper()
{
  ACE_GUARD(ACE_Thread_Mutex, g, mutex_);

  stopping_ = true;

  RtpsUdpDataLink_rch link = link_.lock();

  if (!link) {
    return;
  }

  GuardType guard(link->strategy_lock_);
  if (link->receive_strategy() == 0) {
    return;
  }

  for (WriterInfoMap::iterator it = remote_writers_.begin(); it != remote_writers_.end(); ++it) {
    it->second.held_.clear();
  }
}

bool
RtpsUdpDataLink::RtpsReader::process_data_i(const RTPS::DataSubmessage& data,
                                            const RepoId& src,
                                            MetaSubmessageVec&)
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, mutex_, false);

  if (stopping_) {
    return false;
  }

  RtpsUdpDataLink_rch link = link_.lock();

  if (!link) {
    return false;
  }

  GuardType guard(link->strategy_lock_);
  if (link->receive_strategy() == 0) {
    return false;
  }

  bool on_start = false;
  const WriterInfoMap::iterator wi = remote_writers_.find(src);
  if (wi != remote_writers_.end()) {
    WriterInfo& info = wi->second;
    SequenceNumber seq;
    seq.setValue(data.writerSN.high, data.writerSN.low);

    if (info.first_activity_) {
      on_start = true;
      info.first_activity_ = false;
    }

    const bool no_nack = !(info.hb_range_.second == SequenceNumber::ZERO())
      && info.hb_range_.second < info.hb_range_.first;

    info.frags_.erase(seq);

    if (info.recvd_.empty()) {
      if (durable_) {
        info.hb_range_.first = 1;
        info.hb_range_.second = seq;
        info.recvd_.insert(SequenceNumber::ZERO());
        info.recvd_.insert(seq);

        if (seq != 1) {
          if (Transport_debug_level > 5) {
            GuidConverter writer(src);
            GuidConverter reader(id_);
            ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) RtpsUdpDataLink::process_data_i(DataSubmessage) -")
                                 ACE_TEXT(" data seq: %q from %C being WITHHELD from %C because it's EXPECTING more data")
                                 ACE_TEXT(" (first message, initializing reader)\n"),
                                 seq.getValue(),
                                 OPENDDS_STRING(writer).c_str(),
                                 OPENDDS_STRING(reader).c_str()));
          }
          const ReceivedDataSample* sample =
            link->receive_strategy()->withhold_data_from(id_);
          info.held_.insert(std::make_pair(seq, *sample));

        } else {
          if (Transport_debug_level > 5) {
            GuidConverter writer(src);
            GuidConverter reader(id_);
            ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) RtpsUdpDataLink::process_data_i(DataSubmessage) -")
                                 ACE_TEXT(" data seq: %q from %C to %C OK to deliver")
                                 ACE_TEXT(" (first message, initializing reader)\n"),
                                 seq.getValue(),
                                 OPENDDS_STRING(writer).c_str(),
                                 OPENDDS_STRING(reader).c_str()));
          }
          link->receive_strategy()->do_not_withhold_data_from(id_);
          info.first_delivered_data_ = false;
        }
      } else {
        SequenceNumber low = std::min(seq, info.frags_.empty() ? seq : info.frags_.begin()->first.previous());
        if (seq <= low) {
          info.hb_range_.first = seq;
          info.hb_range_.second = seq;
          info.recvd_.insert(SequenceRange(SequenceNumber::ZERO(), seq));
          info.first_delivered_data_ = false;
          link->receive_strategy()->do_not_withhold_data_from(id_);
        } else {
          info.hb_range_.first = low;
          info.hb_range_.second = seq;
          const ReceivedDataSample* sample =
            link->receive_strategy()->withhold_data_from(id_);
          info.held_.insert(std::make_pair(seq, *sample));
          info.recvd_.insert(seq);
          link->deliver_held_data(id_, info, durable_);
        }
      }

    } else if (info.recvd_.contains(seq)) {
      if (Transport_debug_level > 5) {
        GuidConverter writer(src);
        GuidConverter reader(id_);
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) RtpsUdpDataLink::process_data_i(DataSubmessage) -")
                             ACE_TEXT(" data seq: %q from %C being DROPPED from %C because it's ALREADY received\n"),
                             seq.getValue(),
                             OPENDDS_STRING(writer).c_str(),
                             OPENDDS_STRING(reader).c_str()));
      }
      link->receive_strategy()->withhold_data_from(id_);

    } else if (!info.held_.empty()) {
      const ReceivedDataSample* sample =
        link->receive_strategy()->withhold_data_from(id_);
      info.held_.insert(std::make_pair(seq, *sample));
      info.recvd_.insert(seq);
      link->deliver_held_data(id_, info, durable_);

    } else if (!durable_ && info.first_delivered_data_ && info.hb_range_.second < seq && no_nack) {
      info.hb_range_.first = seq;
      info.hb_range_.second = seq;
      info.recvd_.insert(SequenceRange(SequenceNumber::ZERO(), seq));
      info.first_delivered_data_ = false;
      link->receive_strategy()->do_not_withhold_data_from(id_);

    } else if (info.recvd_.disjoint() || info.recvd_.cumulative_ack() != seq.previous()) {
      if (Transport_debug_level > 5) {
        GuidConverter writer(src);
        GuidConverter reader(id_);
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) RtpsUdpDataLink::process_data_i(DataSubmessage) -")
                             ACE_TEXT(" data seq: %q from %C being WITHHELD from %C because it's EXPECTING more data\n"),
                             seq.getValue(),
                             OPENDDS_STRING(writer).c_str(),
                             OPENDDS_STRING(reader).c_str()));
      }
      const ReceivedDataSample* sample =
        link->receive_strategy()->withhold_data_from(id_);
      info.held_.insert(std::make_pair(seq, *sample));
      info.recvd_.insert(seq);
      link->deliver_held_data(id_, info, durable_);

    } else {
      if (Transport_debug_level > 5) {
        GuidConverter writer(src);
        GuidConverter reader(id_);
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) RtpsUdpDataLink::process_data_i(DataSubmessage) -")
                             ACE_TEXT(" data seq: %q from %C to %C OK to deliver\n"),
                             seq.getValue(),
                             OPENDDS_STRING(writer).c_str(),
                             OPENDDS_STRING(reader).c_str()));
      }
      info.recvd_.insert(seq);
      link->receive_strategy()->do_not_withhold_data_from(id_);
      info.first_delivered_data_ = false;
    }

  } else {
    if (Transport_debug_level > 5) {
      GuidConverter writer(src);
      GuidConverter reader(id_);
      SequenceNumber seq;
      seq.setValue(data.writerSN.high, data.writerSN.low);
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) RtpsUdpDataLink::process_data_i(DataSubmessage) -")
                           ACE_TEXT(" data seq: %q from %C to %C OK to deliver (Writer not currently in Reader remote writer map)\n"),
                           seq.getValue(),
                           OPENDDS_STRING(writer).c_str(),
                           OPENDDS_STRING(reader).c_str()));
    }
    link->receive_strategy()->withhold_data_from(id_);
  }

  guard.release();
  g.release();

  if (on_start) {
    link->invoke_on_start_callbacks(id_, src, true);
  }

  return false;
}

void
RtpsUdpDataLink::deliver_held_data(const RepoId& readerId, WriterInfo& info,
                                   bool durable)
{
  if (durable && (info.recvd_.empty() || info.recvd_.low() > 1)) return;
  held_data_delivery_handler_.notify_delivery(readerId, info);
}

void
RtpsUdpDataLink::received(const RTPS::GapSubmessage& gap,
                          const GuidPrefix_t& src_prefix)
{
  datareader_dispatch(gap, src_prefix, &RtpsReader::process_gap_i);
}

bool
RtpsUdpDataLink::RtpsReader::process_gap_i(const RTPS::GapSubmessage& gap,
                                           const RepoId& src,
                                           MetaSubmessageVec&)
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, mutex_, false);
  RtpsUdpDataLink_rch link = link_.lock();

  if (!link) {
    return false;
  }

  const WriterInfoMap::iterator wi = remote_writers_.find(src);
  if (wi != remote_writers_.end()) {
    if (wi->second.recvd_.empty()) {
      return false;
    }

    SequenceNumber start, base;
    start.setValue(gap.gapStart.high, gap.gapStart.low);
    base.setValue(gap.gapList.bitmapBase.high, gap.gapList.bitmapBase.low);

    SequenceRange sr;
    sr.first = std::max(wi->second.recvd_.low(), start);
    sr.second = base.previous();

    // Insert the GAP range (but not before recvd_.low())
    if (sr.first <= sr.second) {
      if (Transport_debug_level > 5) {
        const GuidConverter conv(src);
        const GuidConverter rdr(id_);
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

    // Insert the GAP bitmap (but not before recvd_.low())
    if (base < sr.first) {
      // Check to see if entire bitmap is below recvd_.low()
      if (sr.first < (base + gap.gapList.numBits)) {
        // If not, partially apply gapList to recvd_ by building temporary
        // disjoint sequence and deriving 'adjusted' bitmap to apply
        DisjointSequence temp;
        temp.insert(base, gap.gapList.numBits,
                          gap.gapList.bitmap.get_buffer());
        temp.insert(SequenceRange(base, sr.first));

        OpenDDS::RTPS::LongSeq8 bitmap;
        CORBA::ULong num_bits = 0;
        bitmap.length((gap.gapList.numBits + 31) / 32); // won't be any larger than original
        memset(&bitmap[0], 0, sizeof (CORBA::ULong) * ((gap.gapList.numBits + 31) / 32));

        (void) temp.to_bitmap(bitmap.get_buffer(), bitmap.length(), num_bits, true);
        if (num_bits) {
          wi->second.recvd_.insert(temp.cumulative_ack(), num_bits, &bitmap[0]);
        }
      }
    } else {
      wi->second.recvd_.insert(base, gap.gapList.numBits,
                               gap.gapList.bitmap.get_buffer());
    }

    link->deliver_held_data(id_, wi->second, durable_);
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
  const MonotonicTimePoint now = MonotonicTimePoint::now();
  OPENDDS_VECTOR(InterestingRemote) callbacks;

  {
    ACE_GUARD(ACE_Thread_Mutex, g, readers_lock_);

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
        interesting_ack_nacks_.insert(InterestingAckNack(writerid, readerid, pos->second.address));
      } else if (riter->second->has_writer(writerid)) {
        // Reader is not associated with this writer.
        interesting_ack_nacks_.insert(InterestingAckNack(writerid, readerid, pos->second.address));
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
                      &RtpsReader::process_heartbeat_i);
}

bool
RtpsUdpDataLink::RtpsReader::process_heartbeat_i(const RTPS::HeartBeatSubmessage& heartbeat,
                                                 const RepoId& src,
                                                 MetaSubmessageVec&)
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, mutex_, false);

  RtpsUdpDataLink_rch link = link_.lock();

  if (!link) {
    return false;
  }

  GuardType guard(link->strategy_lock_);
  if (link->receive_strategy() == 0) {
    return false;
  }

  const WriterInfoMap::iterator wi = remote_writers_.find(src);
  if (wi == remote_writers_.end() || !link) {
    // we may not be associated yet, even if the writer thinks we are
    return false;
  }

  WriterInfo& info = wi->second;

  if (!compare_and_update_counts(heartbeat.count.value, info.heartbeat_recvd_count_)) {
    return false;
  }
  info.heartbeat_recvd_count_ = heartbeat.count.value;

  // Heartbeat Sequence Range
  SequenceNumber hb_first;
  hb_first.setValue(heartbeat.firstSN.high, heartbeat.firstSN.low);
  SequenceNumber hb_last;
  hb_last.setValue(heartbeat.lastSN.high, heartbeat.lastSN.low);

  // Internal Sequence Range
  SequenceNumber& wi_first = info.hb_range_.first;
  SequenceNumber& wi_last = info.hb_range_.second;

  static const SequenceNumber one, zero = SequenceNumber::ZERO();

  bool immediate_reply = false;
  bool first_ever_hb = false;
  if (wi_last.getValue() == 0 && hb_last.getValue() != 0) {
    immediate_reply = true;
  }

  // The first-ever HB can determine the start of our nackable range (wi_first)
  if (info.first_activity_) {
    immediate_reply = true;
    info.first_activity_ = false;
    first_ever_hb = true;
    // Don't re-initialize recvd_ values if a data sample has already done it
    if (info.recvd_.empty()) {
      if (!durable_) {
        if (hb_last > zero) {
          info.recvd_.insert(SequenceRange(zero, hb_last));
        } else {
          info.recvd_.insert(zero);
        }
        wi_first = hb_last; // non-durable reliable connections ignore previous data
      } else {
        info.recvd_.insert(zero);
      }
    }
  }

  // Only valid heartbeats (see spec) will be "fully" applied to writer info
  if (hb_first <= hb_last + 1 || (hb_first == one && wi_last == zero)) {
    if (info.first_valid_hb_) {
      info.first_valid_hb_ = false;
      immediate_reply = true;
    }
    if (!durable_) {
      if (wi_first < hb_first) {
        info.recvd_.insert(SequenceRange(wi_first, hb_first.previous()));
        wi_first = hb_first;
        link->deliver_held_data(id_, info, durable_);
      }
    }
    wi_last = wi_last < hb_last ? hb_last : wi_last;

    info.first_valid_hb_ = false;
  }

  const bool is_final = heartbeat.smHeader.flags & RTPS::FLAG_F,
    liveliness = heartbeat.smHeader.flags & RTPS::FLAG_L;

  bool result = false;
  if (!is_final || (!liveliness && (info.should_nack() ||
      should_nack_durable(info) ||
      link->receive_strategy()->has_fragments(info.hb_range_, wi->first)))) {
    info.ack_pending_ = true;

    if (immediate_reply) {
      link->heartbeat_reply_.schedule(link->quick_reply_delay_);
    } else {
      result = true; // timer will invoke send_heartbeat_replies()
    }
  }

  guard.release();
  g.release();

  if (first_ever_hb) {
    link->invoke_on_start_callbacks(id_, src, true);
  }

  //FUTURE: support assertion of liveliness for MANUAL_BY_TOPIC
  return result;
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
RtpsUdpDataLink::RtpsWriter::add_reader(const RepoId& id, const ReaderInfo& info)
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, mutex_, false);
  ReaderInfoMap::const_iterator iter = remote_readers_.find(id);
  if (iter == remote_readers_.end()) {
    remote_readers_.insert(ReaderInfoMap::value_type(id, info));
    return true;
  }
  return false;
}

bool
RtpsUdpDataLink::RtpsWriter::has_reader(const RepoId& id) const
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, mutex_, false);
  return remote_readers_.count(id) != 0;
}

bool
RtpsUdpDataLink::RtpsWriter::remove_reader(const RepoId& id)
{
  OPENDDS_MAP(SequenceNumber, TransportQueueElement*) dd;
  bool result = false;
  {
    ACE_Guard<ACE_Thread_Mutex> g(mutex_);
    ReaderInfoMap::iterator it = remote_readers_.find(id);
    if (it != remote_readers_.end()) {
      it->second.swap_durable_data(dd);
      remote_readers_.erase(it);
      result = true;
    }
  }
  typedef OPENDDS_MAP(SequenceNumber, TransportQueueElement*)::iterator iter_t;
  for (iter_t it = dd.begin(); it != dd.end(); ++it) {
    it->second->data_dropped();
  }
  return result;
}

size_t
RtpsUdpDataLink::RtpsWriter::reader_count() const
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, mutex_, 0);
  return remote_readers_.size();
}

bool
RtpsUdpDataLink::RtpsWriter::is_reader_handshake_done(const RepoId& id) const
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, mutex_, false);
  ReaderInfoMap::const_iterator iter = remote_readers_.find(id);
  return iter != remote_readers_.end() && iter->second.handshake_done_;
}

bool
RtpsUdpDataLink::RtpsReader::is_writer_handshake_done(const RepoId& id) const
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, mutex_, false);
  WriterInfoMap::const_iterator iter = remote_writers_.find(id);
  return iter != remote_writers_.end() && !iter->second.first_activity_;
}

bool
RtpsUdpDataLink::RtpsReader::add_writer(const RepoId& id, const WriterInfo& info)
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, mutex_, false);
  WriterInfoMap::const_iterator iter = remote_writers_.find(id);
  if (iter == remote_writers_.end()) {
    remote_writers_[id] = info;
    return true;
  }
  return false;
}

bool
RtpsUdpDataLink::RtpsReader::has_writer(const RepoId& id) const
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, mutex_, false);
  return remote_writers_.count(id) != 0;
}

bool
RtpsUdpDataLink::RtpsReader::remove_writer(const RepoId& id)
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, mutex_, false);
  return remote_writers_.erase(id) > 0;
}

size_t
RtpsUdpDataLink::RtpsReader::writer_count() const
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, mutex_, 0);
  return remote_writers_.size();
}

bool
RtpsUdpDataLink::RtpsReader::should_nack_durable(const WriterInfo& info)
{
  return durable_ && (info.recvd_.empty() || info.recvd_.low() > info.hb_range_.first);
}

void
RtpsUdpDataLink::RtpsReader::gather_ack_nacks(MetaSubmessageVec& meta_submessages, bool finalFlag)
{
  ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
  gather_ack_nacks_i(meta_submessages, finalFlag);
}

void
RtpsUdpDataLink::RtpsReader::gather_ack_nacks_i(MetaSubmessageVec& meta_submessages, bool finalFlag)
{
  using namespace OpenDDS::RTPS;

  RtpsUdpDataLink_rch link = link_.lock();

  if (!link) {
    return;
  }

  GuardType guard(link->strategy_lock_);
  if (link->receive_strategy() == 0) {
    return;
  }

  for (WriterInfoMap::iterator wi = remote_writers_.begin(); wi != remote_writers_.end(); ++wi) {

    // if we have some negative acknowledgments, we'll ask for a reply
    DisjointSequence& recvd = wi->second.recvd_;
    const bool nack = wi->second.should_nack() ||
      should_nack_durable(wi->second);
    bool is_final = finalFlag || !nack;

    if (wi->second.ack_pending_ || nack || finalFlag) {
      wi->second.ack_pending_ = false;

      SequenceNumber ack;
      CORBA::ULong num_bits = 0;
      LongSeq8 bitmap;

      const SequenceNumber& hb_low = wi->second.hb_range_.first;
      const SequenceNumber& hb_high = wi->second.hb_range_.second;

      ack = std::max(++SequenceNumber(recvd.cumulative_ack()), hb_low);

      if (recvd.disjoint()) {
        bitmap.length(bitmap_num_longs(ack, recvd.last_ack().previous()));
        if (bitmap.length() > 0) {
          (void)recvd.to_bitmap(bitmap.get_buffer(), bitmap.length(),
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
        link->receive_strategy()->remove_frags_from_bitmap(bitmap.get_buffer(),
                                                 num_bits, ack, wi->first);
      if (frags_modified && !is_final) { // change to is_final if bitmap is empty
        is_final = true;
        for (CORBA::ULong i = 0; i < bitmap.length(); ++i) {
          if ((i + 1) * 32 <= num_bits) {
            if (bitmap[i]) {
              is_final = false;
              break;
            }
          } else {
            if ((0xffffffff << (32 - (num_bits % 32))) & bitmap[i]) {
              is_final = false;
              break;
            }
          }
        }
      }

      EntityId_t reader_id = id_.entityId, writer_id = wi->first.entityId;

      MetaSubmessage meta_submessage(id_, wi->first);

      AckNackSubmessage acknack = {
        {ACKNACK,
         CORBA::Octet(FLAG_E | (is_final ? FLAG_F : 0)),
         0 /*length*/},
        id_.entityId,
        wi->first.entityId,
        { // SequenceNumberSet: acking bitmapBase - 1
          {ack.getHigh(), ack.getLow()},
          num_bits, bitmap
        },
        {++wi->second.acknack_count_}
      };
      meta_submessage.sm_.acknack_sm(acknack);

      meta_submessages.push_back(meta_submessage);

      NackFragSubmessageVec nfsv;
      generate_nack_frags_i(nfsv, wi->second, wi->first);
      for (size_t i = 0; i < nfsv.size(); ++i) {
        nfsv[i].readerId = reader_id;
        nfsv[i].writerId = writer_id;
        meta_submessage.sm_.nack_frag_sm(nfsv[i]);
        meta_submessages.push_back(meta_submessage);
      }
    }
  }
}

#ifdef OPENDDS_SECURITY
namespace {
  const ACE_INET_Addr BUNDLING_PLACEHOLDER;
}
#endif

void
RtpsUdpDataLink::build_meta_submessage_map(MetaSubmessageVec& meta_submessages, AddrDestMetaSubmessageMap& adr_map)
{
  ACE_GUARD(ACE_Thread_Mutex, g, locators_lock_);
  AddrSet addrs;
  // Sort meta_submessages by address set and destination
  for (MetaSubmessageVec::iterator it = meta_submessages.begin(); it != meta_submessages.end(); ++it) {
    if (it->dst_guid_ == GUID_UNKNOWN) {
      addrs = get_addresses_i(it->from_guid_); // This will overwrite, but addrs should always be empty here
    } else {
      accumulate_addresses(it->from_guid_, it->dst_guid_, addrs);
    }
    for (RepoIdSet::iterator it2 = it->to_guids_.begin(); it2 != it->to_guids_.end(); ++it2) {
      accumulate_addresses(it->from_guid_, *it2, addrs);
    }
    if (addrs.empty()) {
      continue;
    }

#ifdef OPENDDS_SECURITY
    if (local_crypto_handle() != DDS::HANDLE_NIL && separate_message(it->from_guid_.entityId)) {
      addrs.insert(BUNDLING_PLACEHOLDER); // removed in bundle_mapped_meta_submessages
    }
#endif

    if (std::memcmp(&(it->dst_guid_.guidPrefix), &GUIDPREFIX_UNKNOWN, sizeof(GuidPrefix_t)) != 0) {
      RepoId dst;
      std::memcpy(dst.guidPrefix, it->dst_guid_.guidPrefix, sizeof(dst.guidPrefix));
      dst.entityId = ENTITYID_UNKNOWN;
      adr_map[addrs][dst].push_back(it);
    } else {
      adr_map[addrs][GUID_UNKNOWN].push_back(it);
    }
    addrs.clear();
  }
}

#ifdef OPENDDS_SECURITY
bool RtpsUdpDataLink::separate_message(EntityId_t entity)
{
  // submessages generated by these entities may not be combined
  // with other submessages when using full-message protection
  // DDS Security v1.1 8.4.2.4 Table 27 is_rtps_protected
  using namespace RTPS;
  return entity == ENTITYID_P2P_BUILTIN_PARTICIPANT_STATELESS_WRITER
    || entity == ENTITYID_P2P_BUILTIN_PARTICIPANT_STATELESS_READER
    || entity == ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_SECURE_WRITER
    || entity == ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_SECURE_READER;
}
#endif

namespace {

struct BundleHelper {
  BundleHelper(size_t max_bundle_size, OPENDDS_VECTOR(size_t)& meta_submessage_bundle_sizes)
  : max_bundle_size_(max_bundle_size)
  , size_(0)
  , padding_(0)
  , prev_size_(0)
  , prev_padding_(0)
  , meta_submessage_bundle_sizes_(meta_submessage_bundle_sizes)
  {
  }

  void end_bundle() {
    meta_submessage_bundle_sizes_.push_back(size_ + padding_);
    size_ = 0; padding_ = 0; prev_size_ = 0; prev_padding_ = 0;
  }

  template <typename T>
  void push_to_next_bundle(const T&) {
    meta_submessage_bundle_sizes_.push_back(prev_size_ + prev_padding_);
    size_ -= prev_size_; padding_ -= prev_padding_; prev_size_ = 0; prev_padding_ = 0;
  }

  template <typename T>
  bool add_to_bundle(const T& val) {
    prev_size_ = size_;
    prev_padding_ = padding_;
    gen_find_size(val, size_, padding_);
    if ((size_ + padding_) > max_bundle_size_) {
      push_to_next_bundle(val);
      return false;
    }
    return true;
  }

  size_t prev_size_diff() const {
    return size_ - prev_size_;
  }

  size_t prev_padding_diff() const {
    return padding_ - prev_padding_;
  }

  size_t max_bundle_size_;
  size_t size_, padding_, prev_size_, prev_padding_;
  OPENDDS_VECTOR(size_t)& meta_submessage_bundle_sizes_;
};

}

void
RtpsUdpDataLink::bundle_mapped_meta_submessages(AddrDestMetaSubmessageMap& adr_map,
                                                MetaSubmessageIterVecVec& meta_submessage_bundles,
                                                OPENDDS_VECTOR(AddrSet)& meta_submessage_bundle_addrs,
                                                OPENDDS_VECTOR(size_t)& meta_submessage_bundle_sizes)
{
  using namespace RTPS;

  // Reusable INFO_DST
  InfoDestinationSubmessage idst = {
    {INFO_DST, FLAG_E, INFO_DST_SZ},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
  };

  BundleHelper helper(max_bundle_size_, meta_submessage_bundle_sizes);
  RepoId prev_dst; // used to determine when we need to write a new info_dst
  for (AddrDestMetaSubmessageMap::iterator addr_it = adr_map.begin(); addr_it != adr_map.end(); ++addr_it) {

    // A new address set always starts a new bundle
    meta_submessage_bundles.push_back(MetaSubmessageIterVec());
    meta_submessage_bundle_addrs.push_back(addr_it->first);

#ifdef OPENDDS_SECURITY
    if (local_crypto_handle() != DDS::HANDLE_NIL) {
      meta_submessage_bundle_addrs.back().erase(BUNDLING_PLACEHOLDER);
    }
#endif

    prev_dst = GUID_UNKNOWN;

    for (DestMetaSubmessageMap::iterator dest_it = addr_it->second.begin(); dest_it != addr_it->second.end(); ++dest_it) {
      for (MetaSubmessageIterVec::iterator resp_it = dest_it->second.begin(); resp_it != dest_it->second.end(); ++resp_it) {
        // Check before every meta_submessage to see if we need to prefix a INFO_DST
        if (dest_it->first != GUID_UNKNOWN && dest_it->first != prev_dst) {
          // If adding an INFO_DST prefix bumped us over the limit, push the
          // size difference into the next bundle, reset prev_dst, and keep
          // going
          if (!helper.add_to_bundle(idst)) {
            meta_submessage_bundles.push_back(MetaSubmessageIterVec());
            meta_submessage_bundle_addrs.push_back(addr_it->first);
          }
        }
        // Attempt to add the submessage meta_submessage to the bundle
        bool result = false;
        MetaSubmessage& res = **resp_it;
        switch (res.sm_._d()) {
          case HEARTBEAT: {
            result = helper.add_to_bundle(res.sm_.heartbeat_sm());
            res.sm_.heartbeat_sm().smHeader.submessageLength =
              static_cast<CORBA::UShort>(helper.prev_size_diff()) - SMHDR_SZ;
            break;
          }
          case ACKNACK: {
            result = helper.add_to_bundle(res.sm_.acknack_sm());
            res.sm_.acknack_sm().smHeader.submessageLength =
              static_cast<CORBA::UShort>(helper.prev_size_diff()) - SMHDR_SZ;
            break;
          }
          case GAP: {
            result = helper.add_to_bundle(res.sm_.gap_sm());
            res.sm_.gap_sm().smHeader.submessageLength = static_cast<CORBA::UShort>(helper.prev_size_diff()) - SMHDR_SZ;
            break;
          }
          case NACK_FRAG: {
            result = helper.add_to_bundle(res.sm_.nack_frag_sm());
            res.sm_.nack_frag_sm().smHeader.submessageLength =
              static_cast<CORBA::UShort>(helper.prev_size_diff()) - SMHDR_SZ;
            break;
          }
          default: {
            break;
          }
        }
        prev_dst = dest_it->first;

        // If adding the submessage bumped us over the limit, push the size
        // difference into the next bundle, reset prev_dst, and keep going
        if (!result) {
          meta_submessage_bundles.push_back(MetaSubmessageIterVec());
          meta_submessage_bundle_addrs.push_back(addr_it->first);
          prev_dst = GUID_UNKNOWN;
        }
        meta_submessage_bundles.back().push_back(*resp_it);
      }
    }
    helper.end_bundle();
  }
}

void
RtpsUdpDataLink::send_bundled_submessages(MetaSubmessageVec& meta_submessages)
{
  using namespace RTPS;

  if (meta_submessages.empty()) {
    return;
  }

  // Sort meta_submessages based on both locator IPs and INFO_DST GUID destination/s
  AddrDestMetaSubmessageMap adr_map;
  build_meta_submessage_map(meta_submessages, adr_map);

  // Build reasonably-sized submessage bundles based on our destination map
  MetaSubmessageIterVecVec meta_submessage_bundles; // a vector of vectors of iterators pointing to meta_submessages
  OPENDDS_VECTOR(AddrSet) meta_submessage_bundle_addrs; // for a bundle's address set
  OPENDDS_VECTOR(size_t) meta_submessage_bundle_sizes; // for allocating the bundle's buffer
  bundle_mapped_meta_submessages(adr_map, meta_submessage_bundles, meta_submessage_bundle_addrs, meta_submessage_bundle_sizes);

  // Reusable INFO_DST
  InfoDestinationSubmessage idst = {
    {INFO_DST, FLAG_E, INFO_DST_SZ},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
  };

  // Allocate buffers, seralize, and send bundles
  RepoId prev_dst; // used to determine when we need to write a new info_dst
  for (size_t i = 0; i < meta_submessage_bundles.size(); ++i) {
    prev_dst = GUID_UNKNOWN;
    ACE_Message_Block mb_bundle(meta_submessage_bundle_sizes[i]); //FUTURE: allocators?
    Serializer ser(&mb_bundle, false, Serializer::ALIGN_CDR);
    for (MetaSubmessageIterVec::const_iterator it = meta_submessage_bundles[i].begin(); it != meta_submessage_bundles[i].end(); ++it) {
      MetaSubmessage& res = **it;
      RepoId dst = res.dst_guid_;
      dst.entityId = ENTITYID_UNKNOWN;
      if (dst != GUID_UNKNOWN && dst != prev_dst) {
        std::memcpy(&idst.guidPrefix, dst.guidPrefix, sizeof(idst.guidPrefix));
        ser << idst;
      }
      ser << res.sm_;
      prev_dst = dst;
    }
    send_strategy()->send_rtps_control(mb_bundle, meta_submessage_bundle_addrs[i]);
  }
}

void
RtpsUdpDataLink::send_heartbeat_replies() // from DR to DW
{
  using namespace OpenDDS::RTPS;

  MetaSubmessageVec meta_submessages;
  RtpsReaderMap readers;

  {
    ACE_GUARD(ACE_Thread_Mutex, g, readers_lock_);

    for (InterestingAckNackSetType::const_iterator pos = interesting_ack_nacks_.begin(),
           limit = interesting_ack_nacks_.end();
         pos != limit;
         ++pos) {

      SequenceNumber ack;
      LongSeq8 bitmap;
      bitmap.length(0);

      AckNackSubmessage acknack = {
        {ACKNACK,
         CORBA::Octet(FLAG_E | FLAG_F),
         0 /*length*/},
        pos->readerid.entityId,
        pos->writerid.entityId,
        { // SequenceNumberSet: acking bitmapBase - 1
          {ack.getHigh(), ack.getLow()},
          0 /* num_bits */, bitmap
        },
        {0 /* acknack count */}
      };

      MetaSubmessage meta_submessage(pos->readerid, pos->writerid);
      meta_submessage.sm_.acknack_sm(acknack);

      meta_submessages.push_back(meta_submessage);
    }
    interesting_ack_nacks_.clear();

    readers = readers_;
  }

  for (RtpsReaderMap::iterator rr = readers.begin(); rr != readers.end(); ++rr) {
    rr->second->gather_ack_nacks(meta_submessages);
  }

  send_bundled_submessages(meta_submessages);
}

void
RtpsUdpDataLink::RtpsReader::generate_nack_frags_i(NackFragSubmessageVec& nf,
                                                   WriterInfo& wi, const RepoId& pub_id)
{
  typedef OPENDDS_MAP(SequenceNumber, RTPS::FragmentNumber_t)::iterator iter_t;
  typedef RtpsUdpReceiveStrategy::FragmentInfo::value_type Frag_t;
  RtpsUdpReceiveStrategy::FragmentInfo frag_info;

  // This is an internal method, locks already locked,
  // we just need a local handle to the link
  RtpsUdpDataLink_rch link = link_.lock();

  // Populate frag_info with two possible sources of NackFrags:
  // 1. sequence #s in the reception gaps that we have partially received
  OPENDDS_VECTOR(SequenceRange) missing = wi.recvd_.missing_sequence_ranges();
  for (size_t i = 0; i < missing.size(); ++i) {
    link->receive_strategy()->has_fragments(missing[i], pub_id, &frag_info);
  }
  // 1b. larger than the last received seq# but less than the heartbeat.lastSN
  if (!wi.recvd_.empty()) {
    const SequenceRange range(wi.recvd_.high(), wi.hb_range_.second);
    link->receive_strategy()->has_fragments(range, pub_id, &frag_info);
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
    if (link->receive_strategy()->has_fragments(range, pub_id, &frag_info)) {
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
    return;
  }

  const RTPS::NackFragSubmessage nackfrag_prototype = {
    {RTPS::NACK_FRAG, RTPS::FLAG_E, 0 /* length set below */},
    ENTITYID_UNKNOWN, // readerId will be filled-in by send_heartbeat_replies()
    ENTITYID_UNKNOWN, // writerId will be filled-in by send_heartbeat_replies()
    {0, 0}, // writerSN set below
    RTPS::FragmentNumberSet(), // fragmentNumberState set below
    {0} // count set below
  };

  for (size_t i = 0; i < frag_info.size(); ++i) {
    nf.push_back(nackfrag_prototype);
    RTPS::NackFragSubmessage& nackfrag = nf.back();
    nackfrag.writerSN.low = frag_info[i].first.getLow();
    nackfrag.writerSN.high = frag_info[i].first.getHigh();
    nackfrag.fragmentNumberState = frag_info[i].second;
    nackfrag.count.value = ++wi.nackfrag_count_;
  }
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
  datareader_dispatch(hb_frag, src_prefix, &RtpsReader::process_hb_frag_i);
}

bool
RtpsUdpDataLink::RtpsReader::process_hb_frag_i(const RTPS::HeartBeatFragSubmessage& hb_frag,
                                               const RepoId& src,
                                               MetaSubmessageVec&)
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, mutex_, false);

  WriterInfoMap::iterator wi = remote_writers_.find(src);
  if (wi == remote_writers_.end()) {
    // we may not be associated yet, even if the writer thinks we are
    return false;
  }

  if (!compare_and_update_counts(hb_frag.count.value, wi->second.hb_frag_recvd_count_)) {
    return false;
  }

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

  const MonotonicTimePoint now = MonotonicTimePoint::now();
  OPENDDS_VECTOR(DiscoveryListener*) callbacks;

  {
    ACE_GUARD(ACE_Thread_Mutex, g, writers_lock_);
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

  datawriter_dispatch(acknack, src_prefix, &RtpsWriter::process_acknack);
}

void
RtpsUdpDataLink::RtpsWriter::gather_gaps_i(const RepoId& reader,
                                           const DisjointSequence& gaps,
                                           MetaSubmessageVec& meta_submessages)
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
    if (bitmap.length() > 0) {
      (void)gaps.to_bitmap(bitmap.get_buffer(), bitmap.length(), num_bits);
    }
  }

  MetaSubmessage meta_submessage(id_, reader);
  GapSubmessage gap = {
    {GAP, FLAG_E, 0 /*length determined later*/},
    reader.entityId,
    id_.entityId,
    gapStart,
    {gapListBase, num_bits, bitmap}
  };
  meta_submessage.sm_.gap_sm(gap);

  if (Transport_debug_level > 5) {
    const GuidConverter conv(id_);
    SequenceRange sr;
    sr.first.setValue(gap.gapStart.high, gap.gapStart.low);
    SequenceNumber srbase;
    srbase.setValue(gap.gapList.bitmapBase.high, gap.gapList.bitmapBase.low);
    sr.second = srbase.previous();
    ACE_DEBUG((LM_DEBUG, "(%P|%t) RtpsUdpDataLink::RtpsWriter::gather_gaps_i "
              "GAP with range [%q, %q] from %C\n",
              sr.first.getValue(), sr.second.getValue(),
              OPENDDS_STRING(conv).c_str()));
  }

  // For durable writers, change a non-directed Gap into multiple directed gaps.
  OPENDDS_VECTOR(RepoId) readers;
  if (durable_ && reader.entityId == ENTITYID_UNKNOWN) {
    if (Transport_debug_level > 5) {
      const GuidConverter local_conv(id_);
      ACE_DEBUG((LM_DEBUG, "RtpsUdpDataLink::RtpsWriter::gather_gaps_i local %C "
                 "durable writer\n", OPENDDS_STRING(local_conv).c_str()));
    }
    for (ReaderInfoMap::iterator ri = remote_readers_.begin();
         ri != remote_readers_.end(); ++ri) {
      if (!ri->second.expecting_durable_data()) {
        readers.push_back(ri->first);
      } else if (Transport_debug_level > 5) {
        const GuidConverter remote_conv(ri->first);
        ACE_DEBUG((LM_DEBUG, "RtpsUdpDataLink::RtpsWriter::gather_gaps_i reader "
                   "%C is expecting durable data, no GAP sent\n",
                   OPENDDS_STRING(remote_conv).c_str()));
      }
    }
    for (size_t i = 0; i < readers.size(); ++i) {
      meta_submessage.dst_guid_ = readers[i];
      gap.readerId = readers[i].entityId;
      // potentially multiple meta_submessages, but all directed
      meta_submessages.push_back(meta_submessage);
    }
  } else {
    // single meta_submessage, possibly non-directed
    meta_submessages.push_back(meta_submessage);
  }
}

void
RtpsUdpDataLink::RtpsWriter::process_acknack(const RTPS::AckNackSubmessage& acknack,
                                             const RepoId& src,
                                             MetaSubmessageVec& meta_submessages)
{
  ACE_GUARD(ACE_Thread_Mutex, g, mutex_);

  RtpsUdpDataLink_rch link = link_.lock();

  if (!link) {
    return;
  }

  RepoId remote = src;

  bool first_ack = false;

  if (Transport_debug_level > 5) {
    GuidConverter local_conv(id_), remote_conv(remote);
    ACE_DEBUG((LM_DEBUG, "(%P|%t) RtpsUdpDataLink::received(ACKNACK) "
      "local %C remote %C\n", OPENDDS_STRING(local_conv).c_str(),
      OPENDDS_STRING(remote_conv).c_str()));
  }

  const ReaderInfoMap::iterator ri = remote_readers_.find(remote);
  if (ri == remote_readers_.end()) {
    VDBG((LM_WARNING, "(%P|%t) RtpsUdpDataLink::received(ACKNACK) "
      "WARNING ReaderInfo not found\n"));
    return;
  }

  if (!compare_and_update_counts(acknack.count.value, ri->second.acknack_recvd_count_)) {
    VDBG((LM_WARNING, "(%P|%t) RtpsUdpDataLink::received(ACKNACK) "
      "WARNING Count indicates duplicate, dropping\n"));
    return;
  }

  if (!ri->second.handshake_done_) {
    ri->second.handshake_done_ = true;
    first_ack = true;
  }

  // For first_acknowledged_by_reader
  SequenceNumber received_sn_base;
  received_sn_base.setValue(acknack.readerSNState.bitmapBase.high, acknack.readerSNState.bitmapBase.low);

  OPENDDS_MAP(SequenceNumber, TransportQueueElement*) pendingCallbacks;
  const bool is_final = acknack.smHeader.flags & RTPS::FLAG_F;

  if (!ri->second.durable_data_.empty()) {
    if (Transport_debug_level > 5) {
      const GuidConverter local_conv(id_), remote_conv(remote);
      ACE_DEBUG((LM_DEBUG, "RtpsUdpDataLink::received(ACKNACK) "
                 "local %C has durable for remote %C\n",
                 OPENDDS_STRING(local_conv).c_str(),
                 OPENDDS_STRING(remote_conv).c_str()));
    }
    SequenceNumber ack;
    ack.setValue(acknack.readerSNState.bitmapBase.high,
                 acknack.readerSNState.bitmapBase.low);
    const SequenceNumber& dd_last = ri->second.durable_data_.rbegin()->first;
    if (Transport_debug_level > 5) {
      ACE_DEBUG((LM_DEBUG, "RtpsUdpDataLink::received(ACKNACK) "
                 "check ack %q against last durable %q\n",
                 ack.getValue(), dd_last.getValue()));
    }
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
          && !is_final && ack == heartbeat_high(ri->second)) {
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
          link->durability_resend(it->second);
          //FUTURE: combine multiple resends into one RTPS Message?
          sent_some = true;
          if (it->first > lastSent + 1) {
            gaps.insert(SequenceRange(lastSent + 1, it->first.previous()));
          }
          lastSent = it->first;
        }
        if (lastSent < psr[i].second && psr[i].second < dd_last) {
          gaps.insert(SequenceRange(lastSent + 1, psr[i].second));
          if (it != ri->second.durable_data_.end()) {
            gaps.insert(SequenceRange(psr[i].second, it->first.previous()));
          }
        }
      }
      if (!gaps.empty()) {
        if (Transport_debug_level > 5) {
          ACE_DEBUG((LM_DEBUG, "RtpsUdpDataLink::received(ACKNACK) "
                     "sending durability gaps:\n"));
          gaps.dump();
        }
        gather_gaps_i(remote, gaps, meta_submessages);
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
                     "sending durability gaps for all requests:\n"));
          requests.dump();
        }
        gather_gaps_i(remote, requests, meta_submessages);
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
                     "sending durability gaps for some requests:\n"));
          gaps.dump();
        }
        gather_gaps_i(remote, gaps, meta_submessages);
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
  if (!is_final || bitmapNonEmpty(acknack.readerSNState)) {
    ri->second.requested_changes_.push_back(acknack.readerSNState);
  }

  TqeSet to_deliver;
  acked_by_all_helper_i(to_deliver);

  if (!is_final) {
    link->nack_reply_.schedule(); // timer will invoke send_nack_replies()
  }

  g.release();

  typedef OPENDDS_MAP(SequenceNumber, TransportQueueElement*)::iterator iter_t;
  for (iter_t it = pendingCallbacks.begin();
       it != pendingCallbacks.end(); ++it) {
    it->second->data_delivered();
  }

  TqeSet::iterator deliver_iter = to_deliver.begin();
  while (deliver_iter != to_deliver.end()) {
    (*deliver_iter)->data_delivered();
    ++deliver_iter;
  }

  if (first_ack) {
    link->invoke_on_start_callbacks(id_, remote, true);
  }
}

void
RtpsUdpDataLink::received(const RTPS::NackFragSubmessage& nackfrag,
                          const GuidPrefix_t& src_prefix)
{
  datawriter_dispatch(nackfrag, src_prefix, &RtpsWriter::process_nackfrag);
}

void RtpsUdpDataLink::RtpsWriter::process_nackfrag(const RTPS::NackFragSubmessage& nackfrag,
                                                   const RepoId& src,
                                                   MetaSubmessageVec&)
{
  ACE_GUARD(ACE_Thread_Mutex, g, mutex_);

  RtpsUdpDataLink_rch link = link_.lock();

  if (!link) {
    return;
  }

  RepoId remote = src;

  if (Transport_debug_level > 5) {
    GuidConverter local_conv(id_), remote_conv(remote);
    ACE_DEBUG((LM_DEBUG, "(%P|%t) RtpsUdpDataLink::received(NACK_FRAG) "
      "local %C remote %C\n", OPENDDS_STRING(local_conv).c_str(),
      OPENDDS_STRING(remote_conv).c_str()));
  }

  const ReaderInfoMap::iterator ri = remote_readers_.find(remote);
  if (ri == remote_readers_.end()) {
    VDBG((LM_WARNING, "(%P|%t) RtpsUdpDataLink::received(NACK_FRAG) "
      "WARNING ReaderInfo not found\n"));
    return;
  }

  if (!compare_and_update_counts(nackfrag.count.value, ri->second.nackfrag_recvd_count_)) {
    VDBG((LM_WARNING, "(%P|%t) RtpsUdpDataLink::received(NACK_FRAG) "
      "WARNING Count indicates duplicate, dropping\n"));
    return;
  }

  SequenceNumber seq;
  seq.setValue(nackfrag.writerSN.high, nackfrag.writerSN.low);
  ri->second.requested_frags_[seq] = nackfrag.fragmentNumberState;

  link->nack_reply_.schedule(); // timer will invoke send_nack_replies()
}

void
RtpsUdpDataLink::RtpsWriter::send_and_gather_nack_replies(MetaSubmessageVec& meta_submessages)
{
  ACE_GUARD(ACE_Thread_Mutex, g, mutex_);

  RtpsUdpDataLink_rch link = link_.lock();

  if (!link) {
    return;
  }

  // consolidate requests from N readers
  AddrSet recipients;
  DisjointSequence requests;

  //track if any messages have been fully acked by all readers
  SequenceNumber all_readers_ack = SequenceNumber::MAX_VALUE;

#ifdef OPENDDS_SECURITY
  const EntityId_t& pvs_writer =
    RTPS::ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_SECURE_WRITER;
  const bool is_pvs_writer =
    0 == std::memcmp(&pvs_writer, &id_.entityId, sizeof pvs_writer);
#endif

  bool gaps_ok = true;
  typedef ReaderInfoMap::iterator ri_iter;
  const ri_iter end = remote_readers_.end();
  for (ri_iter ri = remote_readers_.begin(); ri != end; ++ri) {

    if (ri->second.cur_cumulative_ack_ < all_readers_ack) {
      all_readers_ack = ri->second.cur_cumulative_ack_;
    }

#ifdef OPENDDS_SECURITY
    if (is_pvs_writer && !ri->second.requested_changes_.empty()) {
      send_directed_nack_replies_i(ri->first, ri->second, meta_submessages);
      continue;
    }
#endif

    process_requested_changes_i(requests, ri->second);

    if (!ri->second.requested_changes_.empty()) {
      AddrSet addrs = link->get_addresses(id_, ri->first);
      if (!addrs.empty()) {
        recipients.insert(addrs.begin(), addrs.end());
        if (ri->second.expecting_durable_data()) {
          gaps_ok = false;
        }
        if (Transport_debug_level > 5) {
          const GuidConverter local_conv(id_), remote_conv(ri->first);
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
    if (send_buff_.is_nil() || send_buff_->empty()) {
      gaps = requests;
    } else {
      OPENDDS_VECTOR(SequenceRange) ranges = requests.present_sequence_ranges();
      SingleSendBuffer& sb = *send_buff_;
      ACE_GUARD(TransportSendBuffer::LockType, guard, sb.strategy_lock());
      const RtpsUdpSendStrategy::OverrideToken ot =
        link->send_strategy()->override_destinations(recipients);
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

  send_nackfrag_replies_i(gaps, recipients);

  if (gaps_ok && !gaps.empty()) {
    if (Transport_debug_level > 5) {
      ACE_DEBUG((LM_DEBUG, "RtpsUdpDataLink::send_nack_replies "
                 "GAPs:"));
      gaps.dump();
    }
    gather_gaps_i(GUID_UNKNOWN, gaps, meta_submessages);
  }
}

void
RtpsUdpDataLink::send_nack_replies()
{
  RtpsWriterMap writers;
  {
    ACE_GUARD(ACE_Thread_Mutex, g, writers_lock_);
    writers = writers_;
  }

  MetaSubmessageVec meta_submessages;

  // Reply from local DW to remote DR: GAP or DATA
  using namespace OpenDDS::RTPS;
  typedef RtpsWriterMap::iterator rw_iter;
  for (rw_iter rw = writers.begin(); rw != writers.end(); ++rw) {
    rw->second->send_and_gather_nack_replies(meta_submessages);
  }

  send_bundled_submessages(meta_submessages);
}

void
RtpsUdpDataLink::RtpsWriter::send_nackfrag_replies_i(DisjointSequence& gaps,
                                                     AddrSet& gap_recipients)
{
  RtpsUdpDataLink_rch link = link_.lock();

  if (!link) {
    return;
  }

  typedef OPENDDS_MAP(SequenceNumber, DisjointSequence) FragmentInfo;
  OPENDDS_MAP(ACE_INET_Addr, FragmentInfo) requests;

  typedef ReaderInfoMap::iterator ri_iter;
  const ri_iter end = remote_readers_.end();
  for (ri_iter ri = remote_readers_.begin(); ri != end; ++ri) {

    if (ri->second.requested_frags_.empty()) {
      continue;
    }

    AddrSet remote_addrs = link->get_addresses(id_, ri->first);
    if (remote_addrs.empty()) {
      continue;
    }

    typedef OPENDDS_MAP(SequenceNumber, RTPS::FragmentNumberSet)::iterator rf_iter;
    const rf_iter rf_end = ri->second.requested_frags_.end();
    for (rf_iter rf = ri->second.requested_frags_.begin(); rf != rf_end; ++rf) {

      const SequenceNumber& seq = rf->first;
      if (send_buff_->contains(seq)) {
        for (AddrSet::const_iterator pos = remote_addrs.begin(), limit = remote_addrs.end();
             pos != limit; ++pos) {
          FragmentInfo& fi = requests[*pos];
          fi[seq].insert(rf->second.bitmapBase.value, rf->second.numBits,
                         rf->second.bitmap.get_buffer());
        }
      } else {
        gaps.insert(seq);
        gap_recipients.insert(remote_addrs.begin(), remote_addrs.end());
      }
    }
    ri->second.requested_frags_.clear();
  }

  typedef OPENDDS_MAP(ACE_INET_Addr, FragmentInfo)::iterator req_iter;
  for (req_iter req = requests.begin(); req != requests.end(); ++req) {
    const FragmentInfo& fi = req->second;

    ACE_GUARD(TransportSendBuffer::LockType, guard,
      send_buff_->strategy_lock());
    const RtpsUdpSendStrategy::OverrideToken ot =
      link->send_strategy()->override_destinations(req->first);

    for (FragmentInfo::const_iterator sn_iter = fi.begin();
         sn_iter != fi.end(); ++sn_iter) {
      const SequenceNumber& seq = sn_iter->first;
      send_buff_->resend_fragments_i(seq, sn_iter->second);
    }
  }
}

void
RtpsUdpDataLink::RtpsWriter::process_requested_changes_i(DisjointSequence& requests,
                                                         const ReaderInfo& reader)
{
  for (size_t i = 0; i < reader.requested_changes_.size(); ++i) {
    const RTPS::SequenceNumberSet& sn_state = reader.requested_changes_[i];
    SequenceNumber base;
    base.setValue(sn_state.bitmapBase.high, sn_state.bitmapBase.low);
    if (sn_state.numBits == 1 && !(sn_state.bitmap[0] & 1)
        && base == heartbeat_high(reader)) {
      // Since there is an entry in requested_changes_, the DR must have
      // sent a non-final AckNack.  If the base value is the high end of
      // the heartbeat range, treat it as a request for that seq#.
      if (!send_buff_.is_nil() && send_buff_->contains(base)) {
        requests.insert(base);
      }
    } else {
      requests.insert(base, sn_state.numBits, sn_state.bitmap.get_buffer());
    }
  }
}

void
RtpsUdpDataLink::RtpsWriter::send_directed_nack_replies_i(const RepoId& readerId,
                                                          ReaderInfo& reader,
                                                          MetaSubmessageVec& meta_submessages)
{
  RtpsUdpDataLink_rch link = link_.lock();

  if (!link) {
    return;
  }

  AddrSet addrs = link->get_addresses(id_, readerId);
  if (addrs.empty()) {
    return;
  }

  DisjointSequence requests;
  process_requested_changes_i(requests, reader);
  reader.requested_changes_.clear();

  DisjointSequence gaps;

  if (!requests.empty()) {
    if (send_buff_.is_nil() || send_buff_->empty()) {
      gaps = requests;
    } else {
      OPENDDS_VECTOR(SequenceRange) ranges = requests.present_sequence_ranges();
      SingleSendBuffer& sb = *send_buff_;
      ACE_GUARD(TransportSendBuffer::LockType, guard, sb.strategy_lock());
      const RtpsUdpSendStrategy::OverrideToken ot =
        link->send_strategy()->override_destinations(addrs);
      for (size_t i = 0; i < ranges.size(); ++i) {
        if (Transport_debug_level > 5) {
          ACE_DEBUG((LM_DEBUG, "RtpsUdpDataLink::send_directed_nack_replies "
                     "resend data %d-%d\n", int(ranges[i].first.getValue()),
                     int(ranges[i].second.getValue())));
        }
        sb.resend_i(ranges[i], &gaps, readerId);
      }
    }
  }

  if (gaps.empty()) {
    return;
  }
  if (Transport_debug_level > 5) {
    ACE_DEBUG((LM_DEBUG, "RtpsUdpDataLink::send_directed_nack_replies GAPs: "));
    gaps.dump();
  }
  gather_gaps_i(readerId, gaps, meta_submessages);
}

void
RtpsUdpDataLink::RtpsWriter::process_acked_by_all()
{
  TqeSet to_deliver;
  {
    ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
    acked_by_all_helper_i(to_deliver);
  }

  TqeSet::iterator deliver_iter = to_deliver.begin();
  while (deliver_iter != to_deliver.end()) {
    (*deliver_iter)->data_delivered();
    ++deliver_iter;
  }
}

void
RtpsUdpDataLink::RtpsWriter::acked_by_all_helper_i(TqeSet& to_deliver)
{
  using namespace OpenDDS::RTPS;
  typedef OPENDDS_MULTIMAP(SequenceNumber, TransportQueueElement*)::iterator iter_t;
  OPENDDS_VECTOR(RepoId) to_check;

  RtpsUdpDataLink_rch link = link_.lock();

  if (!link) {
    return;
  }

  //start with the max sequence number writer knows about and decrease
  //by what the min over all readers is
  SequenceNumber all_readers_ack = SequenceNumber::MAX_VALUE;

  typedef ReaderInfoMap::iterator ri_iter;
  const ri_iter end = remote_readers_.end();
  for (ri_iter ri = remote_readers_.begin(); ri != end; ++ri) {
    if (ri->second.cur_cumulative_ack_ < all_readers_ack) {
      all_readers_ack = ri->second.cur_cumulative_ack_;
    }
  }
  if (all_readers_ack == SequenceNumber::MAX_VALUE) {
    return;
  }

  ACE_GUARD(ACE_Thread_Mutex, g2, elems_not_acked_mutex_);

  if (!elems_not_acked_.empty()) {

    OPENDDS_SET(SequenceNumber) sns_to_release;
    iter_t it = elems_not_acked_.begin();
    while (it != elems_not_acked_.end()) {
      if (it->first < all_readers_ack) {
        to_deliver.insert(it->second);
        sns_to_release.insert(it->first);
        iter_t last = it;
        ++it;
        elems_not_acked_.erase(last);
      } else {
        break;
      }
    }
    OPENDDS_SET(SequenceNumber)::iterator sns_it = sns_to_release.begin();
    while (sns_it != sns_to_release.end()) {
      send_buff_->release_acked(*sns_it);
      ++sns_it;
    }
  }
}

void
RtpsUdpDataLink::durability_resend(TransportQueueElement* element)
{
  ACE_Message_Block* msg = const_cast<ACE_Message_Block*>(element->msg());
  AddrSet addrs = get_addresses(element->publication_id(), element->subscription_id());
  if (addrs.empty()) {
    const GuidConverter conv(element->subscription_id());
    ACE_ERROR((LM_ERROR,
               "(%P|%t) RtpsUdpDataLink::durability_resend() - "
               "no locator for remote %C\n", OPENDDS_STRING(conv).c_str()));
  } else {
    send_strategy()->send_rtps_control(*msg, addrs);
  }
}

void
RtpsUdpDataLink::send_heartbeats(const DCPS::MonotonicTimePoint& /*now*/)
{
  OPENDDS_VECTOR(CallbackType) readerDoesNotExistCallbacks;
  OPENDDS_VECTOR(TransportQueueElement*) pendingCallbacks;

  const MonotonicTimePoint now = MonotonicTimePoint::now();
  RtpsWriterMap writers;

  typedef OPENDDS_MAP_CMP(RepoId, RepoIdSet, GUID_tKeyLessThan) WtaMap;
  WtaMap writers_to_advertise;

  {
    ACE_GUARD(ACE_Thread_Mutex, g, writers_lock_);

    RtpsUdpInst& cfg = config();

    const MonotonicTimePoint tv = now - 10 * cfg.heartbeat_period_;
    const MonotonicTimePoint tv3 = now - 3 * cfg.heartbeat_period_;

    for (InterestingRemoteMapType::iterator pos = interesting_readers_.begin(),
           limit = interesting_readers_.end();
         pos != limit;
         ++pos) {
      if (pos->second.status == InterestingRemote::DOES_NOT_EXIST ||
          (pos->second.status == InterestingRemote::EXISTS && pos->second.last_activity < tv3)) {
          writers_to_advertise[pos->second.localid].insert(pos->first);
      }
      if (pos->second.status == InterestingRemote::EXISTS && pos->second.last_activity < tv) {
        CallbackType callback(pos->first, pos->second);
        readerDoesNotExistCallbacks.push_back(callback);
        pos->second.status = InterestingRemote::DOES_NOT_EXIST;
      }
    }

    if (writers_.empty() && interesting_readers_.empty()) {
      heartbeat_.disable_and_wait();
    }

    writers = writers_;
  }

  using namespace OpenDDS::RTPS;

  MetaSubmessageVec meta_submessages;

  using namespace OpenDDS::RTPS;
  typedef RtpsWriterMap::iterator rw_iter;
  for (rw_iter rw = writers.begin(); rw != writers.end(); ++rw) {
    WtaMap::iterator it = writers_to_advertise.find(rw->first);
    if (it == writers_to_advertise.end()) {
      rw->second->gather_heartbeats(pendingCallbacks, RepoIdSet(), true, meta_submessages);
    } else {
      if (rw->second->gather_heartbeats(pendingCallbacks, it->second, false, meta_submessages)) {
        writers_to_advertise.erase(it);
      }
    }
  }

  for (WtaMap::const_iterator pos = writers_to_advertise.begin(),
         limit = writers_to_advertise.end();
       pos != limit;
       ++pos) {
    const SequenceNumber SN = 1, lastSN = SequenceNumber::ZERO();
    const HeartBeatSubmessage hb = {
      {HEARTBEAT, FLAG_E, HEARTBEAT_SZ},
      ENTITYID_UNKNOWN, // any matched reader may be interested in this
      pos->first.entityId,
      {SN.getHigh(), SN.getLow()},
      {lastSN.getHigh(), lastSN.getLow()},
      {++heartbeat_counts_[pos->first]}
    };

    MetaSubmessage meta_submessage(pos->first, GUID_UNKNOWN, pos->second);
    meta_submessage.sm_.heartbeat_sm(hb);

    meta_submessages.push_back(meta_submessage);
  }

  send_bundled_submessages(meta_submessages);

  for (OPENDDS_VECTOR(CallbackType)::iterator iter = readerDoesNotExistCallbacks.begin();
      iter != readerDoesNotExistCallbacks.end(); ++iter) {
    const InterestingRemote& remote = iter->second;
    remote.listener->reader_does_not_exist(iter->first, remote.localid);
  }

  for (size_t i = 0; i < pendingCallbacks.size(); ++i) {
    pendingCallbacks[i]->data_dropped();
  }
}

bool
RtpsUdpDataLink::RtpsWriter::gather_heartbeats(OPENDDS_VECTOR(TransportQueueElement*)& pendingCallbacks,
                                               const RepoIdSet& additional_guids,
                                               bool allow_final,
                                               MetaSubmessageVec& meta_submessages)
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, mutex_, false);

  RtpsUdpDataLink_rch link = link_.lock();
  if (!link) {
    return false;
  }

  const bool has_data = !send_buff_.is_nil()
                        && !send_buff_->empty();
  bool is_final = allow_final, has_durable_data = false;
  SequenceNumber durable_max = SequenceNumber::ZERO();

  MetaSubmessage meta_submessage(id_, GUID_UNKNOWN);
  meta_submessage.to_guids_ = additional_guids;

  const MonotonicTimePoint now = MonotonicTimePoint::now();
  RtpsUdpInst& cfg = link->config();

  // Directed, non-final pre-association heartbeats
  RepoIdSet pre_assoc_hb_guids;

  typedef ReaderInfoMap::iterator ri_iter;
  const ri_iter end = remote_readers_.end();
  for (ri_iter ri = remote_readers_.begin(); ri != end; ++ri) {
    bool marked_to = false;
    if (has_data) {
      meta_submessage.to_guids_.insert(ri->first);
      marked_to = true;
    }
    if (!ri->second.durable_data_.empty()) {
      const MonotonicTimePoint expiration =
        ri->second.durable_timestamp_ + cfg.durable_data_timeout_;
      if (now > expiration) {
        typedef OPENDDS_MAP(SequenceNumber, TransportQueueElement*)::iterator
          dd_iter;
        for (dd_iter it = ri->second.durable_data_.begin();
             it != ri->second.durable_data_.end(); ++it) {
          pendingCallbacks.push_back(it->second);
        }
        ri->second.durable_data_.clear();
        if (Transport_debug_level > 3) {
          const GuidConverter gw(id_), gr(ri->first);
          VDBG_LVL((LM_INFO, "(%P|%t) RtpsUdpDataLink::send_heartbeats - "
            "removed expired durable data for %C -> %C\n",
            OPENDDS_STRING(gw).c_str(), OPENDDS_STRING(gr).c_str()), 3);
        }
      } else {
        has_durable_data = true;
        is_final = false;
        if (ri->second.durable_data_.rbegin()->first > durable_max) {
          durable_max = ri->second.durable_data_.rbegin()->first;
        }
        meta_submessage.to_guids_.insert(ri->first);
        marked_to = true;
      }
    }
    if (!marked_to && !ri->second.handshake_done_) {
      pre_assoc_hb_guids.insert(ri->first);
    }
  }

  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g2, elems_not_acked_mutex_, false);

    if (!elems_not_acked_.empty()) {
      is_final = false;
    }
  }

  const SequenceNumber firstSN = (durable_ || !has_data) ? 1 : send_buff_->low(),
    lastSN = std::max(durable_max, has_data ? send_buff_->high() : SequenceNumber::ZERO());
  using namespace OpenDDS::RTPS;

  const HeartBeatSubmessage hb = {
    {HEARTBEAT,
     CORBA::Octet(FLAG_E | (is_final ? FLAG_F : 0)),
     HEARTBEAT_SZ},
    ENTITYID_UNKNOWN, // any matched reader may be interested in this
    id_.entityId,
    {firstSN.getHigh(), firstSN.getLow()},
    {lastSN.getHigh(), lastSN.getLow()},
    {++heartbeat_count_}
  };
  meta_submessage.sm_.heartbeat_sm(hb);

  // Directed, non-final pre-association heartbeats
  MetaSubmessage pre_assoc_hb = meta_submessage;
  pre_assoc_hb.to_guids_.clear();
  pre_assoc_hb.sm_.heartbeat_sm().smHeader.flags &= ~(FLAG_F);
  for (RepoIdSet::const_iterator it = pre_assoc_hb_guids.begin(); it != pre_assoc_hb_guids.end(); ++it) {
    pre_assoc_hb.dst_guid_ = (*it);
    pre_assoc_hb.sm_.heartbeat_sm().readerId = it->entityId;
    meta_submessages.push_back(pre_assoc_hb);
  }

  if (is_final && !has_data && !has_durable_data) {
    return true;
  }

#ifdef OPENDDS_SECURITY
  const EntityId_t& volatile_writer =
    RTPS::ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_SECURE_WRITER;
  if (std::memcmp(&id_.entityId, &volatile_writer, sizeof(EntityId_t)) == 0) {
    RepoIdSet guids = meta_submessage.to_guids_;
    meta_submessage.to_guids_.clear();
    for (RepoIdSet::const_iterator it = guids.begin(); it != guids.end(); ++it) {
      meta_submessage.dst_guid_ = (*it);
      meta_submessage.sm_.heartbeat_sm().readerId = it->entityId;
      meta_submessages.push_back(meta_submessage);
    }
  } else {
    meta_submessages.push_back(meta_submessage);
  }
#else
  meta_submessages.push_back(meta_submessage);
#endif
  return true;
}

void
RtpsUdpDataLink::check_heartbeats(const DCPS::MonotonicTimePoint& now)
{
  OPENDDS_VECTOR(CallbackType) writerDoesNotExistCallbacks;

  // Have any interesting writers timed out?
  const MonotonicTimePoint tv(now - 10 * config().heartbeat_period_);
  {
    ACE_GUARD(ACE_Thread_Mutex, g, readers_lock_);

    for (InterestingRemoteMapType::iterator pos = interesting_writers_.begin(), limit = interesting_writers_.end();
         pos != limit;
         ++pos) {
      if (pos->second.status == InterestingRemote::EXISTS && pos->second.last_activity < tv) {
        CallbackType callback(pos->first, pos->second);
        writerDoesNotExistCallbacks.push_back(callback);
        pos->second.status = InterestingRemote::DOES_NOT_EXIST;
      }
    }
  }

  OPENDDS_VECTOR(CallbackType)::iterator iter;
  for (iter = writerDoesNotExistCallbacks.begin(); iter != writerDoesNotExistCallbacks.end(); ++iter) {
    const RepoId& rid = iter->first;
    const InterestingRemote& remote = iter->second;
    remote.listener->writer_does_not_exist(rid, remote.localid);
  }
}

void
RtpsUdpDataLink::send_relay_beacon(const DCPS::MonotonicTimePoint& /*now*/)
{
  const ACE_INET_Addr rra = config().rtps_relay_address();
  if (rra == ACE_INET_Addr()) {
    return;
  }

  // Create a message with a few bytes of data for the beacon
  ACE_Message_Block mb(reinterpret_cast<const char*>(OpenDDS::RTPS::BEACON_MESSAGE), OpenDDS::RTPS::BEACON_MESSAGE_LENGTH);
  mb.wr_ptr(OpenDDS::RTPS::BEACON_MESSAGE_LENGTH);
  send_strategy()->send_rtps_control(mb, rra);
}

void
RtpsUdpDataLink::send_heartbeats_manual_i(const TransportSendControlElement* tsce, MetaSubmessageVec& meta_submessages)
{
  using namespace OpenDDS::RTPS;

  const RepoId pub_id = tsce->publication_id();

  SequenceNumber firstSN, lastSN;
  CORBA::Long counter;

  firstSN = 1;
  lastSN = tsce->sequence();

  counter = ++best_effort_heartbeat_count_;

  const HeartBeatSubmessage hb = {
    {HEARTBEAT,
     CORBA::Octet(FLAG_E | FLAG_F | FLAG_L),
     HEARTBEAT_SZ},
    ENTITYID_UNKNOWN, // any matched reader may be interested in this
    pub_id.entityId,
    {firstSN.getHigh(), firstSN.getLow()},
    {lastSN.getHigh(), lastSN.getLow()},
    {counter}
  };

  MetaSubmessage meta_submessage(pub_id, GUID_UNKNOWN);
  meta_submessage.sm_.heartbeat_sm(hb);

  meta_submessages.push_back(meta_submessage);
}

void
RtpsUdpDataLink::RtpsWriter::send_heartbeats_manual_i(MetaSubmessageVec& meta_submessages)
{
  using namespace OpenDDS::RTPS;

  RtpsUdpDataLink_rch link = link_.lock();
  if (!link) {
    return;
  }

  SequenceNumber firstSN, lastSN;
  CORBA::Long counter;

  const bool has_data = !send_buff_.is_nil() && !send_buff_->empty();
  SequenceNumber durable_max;
  const MonotonicTimePoint now = MonotonicTimePoint::now();
  for (ReaderInfoMap::const_iterator ri = remote_readers_.begin(), end = remote_readers_.end();
       ri != end;
       ++ri) {
    if (!ri->second.durable_data_.empty()) {
      const MonotonicTimePoint expiration = ri->second.durable_timestamp_ + link->config().durable_data_timeout_;
      if (now <= expiration &&
          ri->second.durable_data_.rbegin()->first > durable_max) {
        durable_max = ri->second.durable_data_.rbegin()->first;
      }
    }
  }

  firstSN = (durable_ || !has_data) ? 1 : send_buff_->low();
  lastSN = std::max(durable_max, has_data ? send_buff_->high() : 1);
  counter = ++heartbeat_count_;

  const HeartBeatSubmessage hb = {
    {HEARTBEAT,
     CORBA::Octet(FLAG_E | FLAG_F | FLAG_L),
     HEARTBEAT_SZ},
    ENTITYID_UNKNOWN, // any matched reader may be interested in this
    id_.entityId,
    {firstSN.getHigh(), firstSN.getLow()},
    {lastSN.getHigh(), lastSN.getLow()},
    {counter}
  };

  MetaSubmessage meta_submessage(id_, GUID_UNKNOWN);
  meta_submessage.sm_.heartbeat_sm(hb);

  meta_submessages.push_back(meta_submessage);
}

#ifdef OPENDDS_SECURITY
void
RtpsUdpDataLink::populate_security_handles(const RepoId& local_id,
                                           const RepoId& remote_id,
                                           const unsigned char* buffer,
                                           unsigned int buffer_size)
{
  using DDS::Security::ParticipantCryptoHandle;
  using DDS::Security::DatawriterCryptoHandle;
  using DDS::Security::DatareaderCryptoHandle;

  ACE_Data_Block db(buffer_size, ACE_Message_Block::MB_DATA,
    reinterpret_cast<const char*>(buffer),
    0 /*alloc*/, 0 /*lock*/, ACE_Message_Block::DONT_DELETE, 0 /*db_alloc*/);
  ACE_Message_Block mb(&db, ACE_Message_Block::DONT_DELETE, 0 /*mb_alloc*/);
  mb.wr_ptr(mb.space());
  DCPS::Serializer ser(&mb, ACE_CDR_BYTE_ORDER, DCPS::Serializer::ALIGN_CDR);

  const bool local_is_writer = GuidConverter(local_id).isWriter();
  const RepoId& writer_id = local_is_writer ? local_id : remote_id;
  const RepoId& reader_id = local_is_writer ? remote_id : local_id;

  ACE_GUARD(ACE_Thread_Mutex, g, ch_lock_);

  while (mb.length()) {
    DDS::BinaryProperty_t prop;
    if (!(ser >> prop)) {
      ACE_ERROR((LM_ERROR, "(%P|%t) RtpsUdpDataLink::populate_security_handles()"
                 " - failed to deserialize BinaryProperty_t\n"));
      return;
    }

    if (std::strcmp(prop.name.in(), RTPS::BLOB_PROP_PART_CRYPTO_HANDLE) == 0
        && prop.value.length() >= sizeof(ParticipantCryptoHandle)) {
      unsigned int handle = 0;
      for (unsigned int i = 0; i < prop.value.length(); ++i) {
        handle = handle << 8 | prop.value[i];
      }

      RepoId remote_participant;
      RTPS::assign(remote_participant.guidPrefix, remote_id.guidPrefix);
      remote_participant.entityId = ENTITYID_PARTICIPANT;
      peer_crypto_handles_[remote_participant] = handle;
      if (security_debug.bookkeeping) {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) {bookkeeping} RtpsUdpDataLink::populate_security_handles() ")
                   ACE_TEXT("RPCH %C = %d\n"),
                   OPENDDS_STRING(GuidConverter(remote_participant)).c_str(), handle));
      }

    } else if (std::strcmp(prop.name.in(), RTPS::BLOB_PROP_DW_CRYPTO_HANDLE) == 0
               && prop.value.length() >= sizeof(DatawriterCryptoHandle)) {
      unsigned int handle = 0;
      for (unsigned int i = 0; i < prop.value.length(); ++i) {
        handle = handle << 8 | prop.value[i];
      }
      peer_crypto_handles_[writer_id] = handle;
      if (security_debug.bookkeeping) {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) {bookkeeping} RtpsUdpDataLink::populate_security_handles() ")
                   ACE_TEXT("DWCH %C = %d\n"),
                   OPENDDS_STRING(GuidConverter(writer_id)).c_str(), handle));
      }

    } else if (std::strcmp(prop.name.in(), RTPS::BLOB_PROP_DR_CRYPTO_HANDLE) == 0
               && prop.value.length() >= sizeof(DatareaderCryptoHandle)) {
      unsigned int handle = 0;
      for (unsigned int i = 0; i < prop.value.length(); ++i) {
        handle = handle << 8 | prop.value[i];
      }
      peer_crypto_handles_[reader_id] = handle;
      if (security_debug.bookkeeping) {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) {bookkeeping} RtpsUdpDataLink::populate_security_handles() ")
                   ACE_TEXT("DRCH %C = %d\n"),
                   std::string(GuidConverter(reader_id)).c_str(), handle));
      }

    } else if (std::strcmp(prop.name.in(), RTPS::BLOB_PROP_ENDPOINT_SEC_ATTR) == 0
               && prop.value.length() >= max_marshaled_size_ulong()) {
      DDS::Security::EndpointSecurityAttributesMask esa;
      std::memcpy(&esa, prop.value.get_buffer(), prop.value.length());
      endpoint_security_attributes_[writer_id] = endpoint_security_attributes_[reader_id] = esa;
    }

  }
}
#endif

RtpsUdpDataLink::ReaderInfo::~ReaderInfo()
{
  expire_durable_data();
}

void
RtpsUdpDataLink::ReaderInfo::swap_durable_data(OPENDDS_MAP(SequenceNumber, TransportQueueElement*)& dd)
{
  durable_data_.swap(dd);
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
    (durable_timestamp_.is_zero() // DW hasn't resent yet
     || !durable_data_.empty()); // DW resent, not sent to reader
}

RtpsUdpDataLink::RtpsWriter::RtpsWriter(RcHandle<RtpsUdpDataLink> link, const RepoId& id, bool durable, CORBA::Long hbc, size_t capacity)
 : send_buff_(make_rch<SingleSendBuffer>(capacity, ONE_SAMPLE_PER_PACKET))
 , link_(link)
 , id_(id)
 , durable_(durable)
 , heartbeat_count_(hbc)
{
  send_buff_->bind(link->send_strategy());
}

RtpsUdpDataLink::RtpsWriter::~RtpsWriter()
{
  ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
  ACE_GUARD(ACE_Thread_Mutex, g2, elems_not_acked_mutex_);

  if (!elems_not_acked_.empty()) {
    ACE_DEBUG((LM_WARNING, ACE_TEXT("(%P|%t) WARNING: RtpsWriter::~RtpsWriter - ")
      ACE_TEXT("deleting with %d elements left not fully acknowledged\n"),
      elems_not_acked_.size()));
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
  ACE_GUARD(ACE_Thread_Mutex, g, elems_not_acked_mutex_);
  elems_not_acked_.insert(SnToTqeMap::value_type(element->sequence(), element));
}


// Implementing TimedDelay and HeartBeat nested classes (for ACE timers)

void
RtpsUdpDataLink::TimedDelay::schedule(const TimeDuration& timeout)
{
  const TimeDuration& next_to = (!timeout.is_zero() && timeout < timeout_) ? timeout : timeout_;
  const MonotonicTimePoint next_to_point(MonotonicTimePoint::now() + next_to);

  if (!scheduled_.is_zero() && (next_to_point < scheduled_)) {
    cancel();
  }

  if (scheduled_.is_zero()) {
    const long timer = outer_->get_reactor()->schedule_timer(this, 0, next_to.value());

    if (timer == -1) {
      ACE_ERROR((LM_ERROR, "(%P|%t) RtpsUdpDataLink::TimedDelay::schedule "
        "failed to schedule timer %p\n", ACE_TEXT("")));
    } else {
      scheduled_ = next_to_point;
    }
  }
}

void
RtpsUdpDataLink::TimedDelay::cancel()
{
  if (!scheduled_.is_zero()) {
    outer_->get_reactor()->cancel_timer(this);
    scheduled_ = MonotonicTimePoint::zero_value;
  }
}

void
RtpsUdpDataLink::send_final_acks(const RepoId& readerid)
{
  RtpsReader_rch reader;
  {
    ACE_GUARD(ACE_Thread_Mutex, g, readers_lock_);
    RtpsReaderMap::iterator rr = readers_.find(readerid);
    if (rr != readers_.end()) {
      reader = rr->second;
    }
  }

  if (reader) {
    MetaSubmessageVec meta_submessages;
    reader->gather_ack_nacks(meta_submessages, true);
    send_bundled_submessages(meta_submessages);
  }
}


int
RtpsUdpDataLink::HeldDataDeliveryHandler::handle_exception(ACE_HANDLE /* fd */)
{
  OPENDDS_ASSERT(link_->reactor_task_->get_reactor_owner() == ACE_Thread::self());

  HeldData::iterator itr;
  for (itr = held_data_.begin(); itr != held_data_.end(); ++itr) {
    link_->data_received(itr->first, itr->second);
  }
  held_data_.clear();

  return 0;
}

void RtpsUdpDataLink::HeldDataDeliveryHandler::notify_delivery(const RepoId& readerId, WriterInfo& info)
{
  OPENDDS_ASSERT(link_->reactor_task_->get_reactor_owner() == ACE_Thread::self());

  const SequenceNumber ca = info.recvd_.cumulative_ack();
  typedef OPENDDS_MAP(SequenceNumber, ReceivedDataSample)::iterator iter;
  const iter end = info.held_.upper_bound(ca);

  if (info.held_.begin() != end) {
    info.first_delivered_data_ = false;
  }

  for (iter it = info.held_.begin(); it != end; /*increment in loop body*/) {
    if (Transport_debug_level > 5) {
      GuidConverter reader(readerId);
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) RtpsUdpDataLink::HeldDataDeliveryHandler::notify_delivery -")
                           ACE_TEXT(" deliver sequence: %q to %C\n"),
                           it->second.header_.sequence_.getValue(),
                           OPENDDS_STRING(reader).c_str()));
    }
    // The head_data_ is not protected by a mutex because it is always accessed from the reactor task thread.
    held_data_.push_back(HeldDataEntry(it->second, readerId));
    info.held_.erase(it++);
  }
  link_->reactor_task_->get_reactor()->notify(this);
}

ACE_Event_Handler::Reference_Count
RtpsUdpDataLink::HeldDataDeliveryHandler::add_reference()
{
  return link_->add_reference();
}

ACE_Event_Handler::Reference_Count
RtpsUdpDataLink::HeldDataDeliveryHandler::remove_reference()
{
  return link_->remove_reference();
}

RtpsUdpSendStrategy*
RtpsUdpDataLink::send_strategy()
{
  return static_cast<RtpsUdpSendStrategy*>(send_strategy_.in());
}

RtpsUdpReceiveStrategy*
RtpsUdpDataLink::receive_strategy()
{
  return static_cast<RtpsUdpReceiveStrategy*>(receive_strategy_.in());
}

RtpsUdpDataLink::AddrSet
RtpsUdpDataLink::get_addresses(const RepoId& local, const RepoId& remote) const {
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, locators_lock_, AddrSet());
  return get_addresses_i(local, remote);
}

RtpsUdpDataLink::AddrSet
RtpsUdpDataLink::get_addresses(const RepoId& local) const {
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, locators_lock_, AddrSet());
  return get_addresses_i(local);
}

RtpsUdpDataLink::AddrSet
RtpsUdpDataLink::get_addresses_i(const RepoId& local, const RepoId& remote) const {
  AddrSet retval;

  accumulate_addresses(local, remote, retval);

  return retval;
}

RtpsUdpDataLink::AddrSet
RtpsUdpDataLink::get_addresses_i(const RepoId& local) const {
  AddrSet retval;

  const GUIDSeq_var peers = peer_ids(local);
  if (peers.ptr()) {
    for (CORBA::ULong i = 0; i < peers->length(); ++i) {
      accumulate_addresses(local, peers[i], retval);
    }
  }

  return retval;
}

void
RtpsUdpDataLink::accumulate_addresses(const RepoId& local, const RepoId& remote,
                                                     AddrSet& addresses) const {
  ACE_UNUSED_ARG(local);
  OPENDDS_ASSERT(local != GUID_UNKNOWN);
  OPENDDS_ASSERT(remote != GUID_UNKNOWN);

  ACE_INET_Addr normal_addr;
  ACE_INET_Addr ice_addr;
  static const ACE_INET_Addr NO_ADDR;

  typedef OPENDDS_MAP_CMP(RepoId, RemoteInfo, GUID_tKeyLessThan)::const_iterator iter_t;
  iter_t pos = locators_.find(remote);
  if (pos != locators_.end()) {
    normal_addr = pos->second.addr_;
  } else {
    const GuidConverter conv(remote);
    if (conv.isReader()) {
      InterestingRemoteMapType::const_iterator ipos = interesting_readers_.find(remote);
      if (ipos != interesting_readers_.end()) {
        normal_addr = ipos->second.address;
      }
    } else if (conv.isWriter()) {
      InterestingRemoteMapType::const_iterator ipos = interesting_writers_.find(remote);
      if (ipos != interesting_writers_.end()) {
        normal_addr = ipos->second.address;
      }
    }
  }

#ifdef OPENDDS_SECURITY
  ICE::Endpoint* endpoint = get_ice_endpoint();
  if (endpoint) {
    ice_addr = ICE::Agent::instance()->get_address(endpoint, local, remote);
  }
#endif

  if (ice_addr == NO_ADDR) {
    if (normal_addr != NO_ADDR) {
      addresses.insert(normal_addr);
    }
    const ACE_INET_Addr relay_addr = config().rtps_relay_address();
    if (relay_addr != NO_ADDR) {
      addresses.insert(relay_addr);
    }
    return;
  }

  if (ice_addr != normal_addr) {
    addresses.insert(ice_addr);
    return;
  }

  if (normal_addr != NO_ADDR) {
    addresses.insert(normal_addr);
  }
}

ICE::Endpoint*
RtpsUdpDataLink::get_ice_endpoint() const {
  return impl().get_ice_endpoint();
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
