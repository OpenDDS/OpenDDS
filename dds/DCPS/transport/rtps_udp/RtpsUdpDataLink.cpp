/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "RtpsUdpDataLink.h"

#include "RtpsUdpTransport.h"
#include "RtpsUdpInst.h"
#include "RtpsUdpSendStrategy.h"
#include "RtpsUdpReceiveStrategy.h"

#include <dds/DCPS/LogAddr.h>
#include <dds/DCPS/Definitions.h>
#include <dds/DCPS/Util.h>
#include <dds/DCPS/Logging.h>
#include <dds/DCPS/NetworkResource.h>
#include <dds/DCPS/Qos_Helper.h>
#include <dds/DCPS/transport/framework/TransportCustomizedElement.h>
#include <dds/DCPS/transport/framework/TransportSendElement.h>
#include <dds/DCPS/transport/framework/TransportSendControlElement.h>
#include <dds/DCPS/transport/framework/RemoveAllVisitor.h>
#include <dds/DCPS/RTPS/MessageUtils.h>
#include <dds/DCPS/RTPS/MessageTypes.h>
#ifdef OPENDDS_SECURITY
#  include <dds/DCPS/security/framework/SecurityRegistry.h>
#endif

#include <dds/DCPS/RTPS/RtpsCoreTypeSupportImpl.h>
#include <dds/DdsDcpsCoreTypeSupportImpl.h>

#include <ace/Default_Constants.h>
#include <ace/Log_Msg.h>
#include <ace/Message_Block.h>
#include <ace/Reactor.h>

#include <string.h>

#ifndef __ACE_INLINE__
# include "RtpsUdpDataLink.inl"
#endif  /* __ACE_INLINE__ */

namespace {

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

using RTPS::to_opendds_seqnum;
using RTPS::to_rtps_seqnum;

const size_t ONE_SAMPLE_PER_PACKET = 1;

RtpsUdpDataLink::RtpsUdpDataLink(const RtpsUdpTransport_rch& transport,
                                 const GuidPrefix_t& local_prefix,
                                 const RtpsUdpInst_rch& config,
                                 const ReactorTask_rch& reactor_task)
  : DataLink(transport, // 3 data link "attributes", below, are unused
             0,         // priority
             false,     // is_loopback
             false)     // is_active
  , reactor_task_(reactor_task)
  , job_queue_(make_rch<JobQueue>(reactor_task->get_reactor()))
  , event_dispatcher_(transport->event_dispatcher())
  , mb_allocator_(TheServiceParticipant->association_chunk_multiplier())
  , db_allocator_(TheServiceParticipant->association_chunk_multiplier())
  , custom_allocator_(TheServiceParticipant->association_chunk_multiplier() * config->anticipated_fragments(), RtpsSampleHeader::FRAG_SIZE)
  , bundle_allocator_(TheServiceParticipant->association_chunk_multiplier(), config->max_message_size())
  , db_lock_pool_(new DataBlockLockPool(static_cast<unsigned long>(TheServiceParticipant->n_chunks())))
  , multi_buff_(this, config->nak_depth())
  , fsq_vec_size_(0)
  , harvest_send_queue_sporadic_(make_rch<SporadicEvent>(event_dispatcher_, make_rch<PmfNowEvent<RtpsUdpDataLink> >(rchandle_from(this), &RtpsUdpDataLink::harvest_send_queue)))
  , flush_send_queue_sporadic_(make_rch<SporadicEvent>(event_dispatcher_, make_rch<PmfNowEvent<RtpsUdpDataLink> >(rchandle_from(this), &RtpsUdpDataLink::flush_send_queue)))
  , best_effort_heartbeat_count_(0)
  , heartbeat_(make_rch<PeriodicEvent>(event_dispatcher_, make_rch<PmfNowEvent<RtpsUdpDataLink> >(rchandle_from(this), &RtpsUdpDataLink::send_heartbeats)))
  , heartbeatchecker_(make_rch<PeriodicEvent>(event_dispatcher_, make_rch<PmfNowEvent<RtpsUdpDataLink> >(rchandle_from(this), &RtpsUdpDataLink::check_heartbeats)))
  , max_bundle_size_(config->max_message_size() - RTPS::RTPSHDR_SZ) // default maximum bundled message size is max udp message size (see TransportStrategy) minus RTPS header
#ifdef OPENDDS_SECURITY
  , security_config_(Security::SecurityRegistry::instance()->default_config())
  , local_crypto_handle_(DDS::HANDLE_NIL)
  , ice_agent_(ICE::Agent::instance())
#endif
  , network_interface_address_reader_(make_rch<InternalDataReader<NetworkInterfaceAddress> >(DCPS::DataReaderQosBuilder().reliability_reliable().durability_transient_local(), rchandle_from(this)))
{
#ifdef OPENDDS_SECURITY
  const GUID_t guid = make_id(local_prefix, ENTITYID_PARTICIPANT);
  handle_registry_ = security_config_->get_handle_registry(guid);
#endif

  send_strategy_ = make_rch<RtpsUdpSendStrategy>(this, local_prefix);
  receive_strategy_ = make_rch<RtpsUdpReceiveStrategy>(this, local_prefix, ref(TheServiceParticipant->get_thread_status_manager()));
  assign(local_prefix_, local_prefix);

  this->job_queue(job_queue_);
}

RtpsUdpDataLink::~RtpsUdpDataLink()
{
  harvest_send_queue_sporadic_->cancel();
  flush_send_queue_sporadic_->cancel();
}

RtpsUdpInst_rch
RtpsUdpDataLink::config() const
{
  return dynamic_rchandle_cast<RtpsUdpTransport>(impl())->config();
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
  GUID_t pub_id = sample->get_pub_id();

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

void RtpsUdpDataLink::remove_all_msgs(const GUID_t& pub_id)
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

  RemoveResult result = REMOVE_NOT_FOUND;
  {
    GuardType guard(link->strategy_lock_);
    if (link->send_strategy_) {
      ACE_Reverse_Lock<ACE_Thread_Mutex> rev_lock(mutex_);
      ACE_Guard<ACE_Reverse_Lock<ACE_Thread_Mutex> > rg(rev_lock);
      result = link->send_strategy_->remove_sample(sample);
      guard.release();
    }
  }

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

  {
    ACE_Reverse_Lock<ACE_Thread_Mutex> rev_lock(mutex_);
    ACE_Guard<ACE_Reverse_Lock<ACE_Thread_Mutex> > rg(rev_lock);
    GuardType guard(link->strategy_lock_);
    if (link->send_strategy_) {
      link->send_strategy_->remove_all_msgs(id_);
    }
  }

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
RtpsUdpDataLink::open(const ACE_SOCK_Dgram& unicast_socket
#ifdef ACE_HAS_IPV6
                      , const ACE_SOCK_Dgram& ipv6_unicast_socket
#endif
                      )
{
  unicast_socket_ = unicast_socket;
#ifdef ACE_HAS_IPV6
  ipv6_unicast_socket_ = ipv6_unicast_socket;
#endif

  RtpsUdpInst_rch cfg = config();
  if (!cfg) {
    return false;
  }

  if (cfg->use_multicast()) {
#ifdef ACE_HAS_MAC_OSX
    multicast_socket_.opts(ACE_SOCK_Dgram_Mcast::OPT_BINDADDR_NO |
                           ACE_SOCK_Dgram_Mcast::DEFOPT_NULLIFACE);
#ifdef ACE_HAS_IPV6
    ipv6_multicast_socket_.opts(ACE_SOCK_Dgram_Mcast::OPT_BINDADDR_NO |
                                ACE_SOCK_Dgram_Mcast::DEFOPT_NULLIFACE);
#endif
#endif
  }

  if (cfg->use_multicast()) {
    if (!set_socket_multicast_ttl(unicast_socket_, cfg->ttl())) {
      if (DCPS_debug_level > 0) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("(%P|%t) ERROR: ")
                   ACE_TEXT("RtpsUdpDataLink::open: ")
                   ACE_TEXT("failed to set TTL: %d\n"),
                   cfg->ttl()));
      }
      return false;
    }
#ifdef ACE_HAS_IPV6
    if (!set_socket_multicast_ttl(ipv6_unicast_socket_, cfg->ttl())) {
      if (DCPS_debug_level > 0) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("(%P|%t) ERROR: ")
                   ACE_TEXT("RtpsUdpDataLink::open: ")
                   ACE_TEXT("failed to set TTL: %d\n"),
                   cfg->ttl()));
      }
      return false;
    }
#endif
  }

  if (cfg->send_buffer_size() > 0) {
    const int snd_size = cfg->send_buffer_size();
    if (unicast_socket_.set_option(SOL_SOCKET,
                                SO_SNDBUF,
                                (void *) &snd_size,
                                sizeof(snd_size)) < 0
        && errno != ENOTSUP) {
      if (DCPS_debug_level > 0) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("(%P|%t) ERROR: ")
                   ACE_TEXT("RtpsUdpDataLink::open: failed to set the send buffer size to %d errno %m\n"),
                   snd_size));
      }
      return false;
    }
#ifdef ACE_HAS_IPV6
    if (ipv6_unicast_socket_.set_option(SOL_SOCKET,
                                        SO_SNDBUF,
                                        (void *) &snd_size,
                                        sizeof(snd_size)) < 0
        && errno != ENOTSUP) {
      if (DCPS_debug_level > 0) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("(%P|%t) ERROR: ")
                   ACE_TEXT("RtpsUdpDataLink::open: failed to set the send buffer size to %d errno %m\n"),
                   snd_size));
      }
      return false;
    }
#endif
  }

  if (cfg->rcv_buffer_size() > 0) {
    const int rcv_size = cfg->rcv_buffer_size();
    if (unicast_socket_.set_option(SOL_SOCKET,
                                SO_RCVBUF,
                                (void *) &rcv_size,
                                sizeof(int)) < 0
        && errno != ENOTSUP) {
      if (DCPS_debug_level > 0) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("(%P|%t) ERROR: ")
                   ACE_TEXT("RtpsUdpDataLink::open: failed to set the receive buffer size to %d errno %m\n"),
                   rcv_size));
      }
      return false;
    }
#ifdef ACE_HAS_IPV6
    if (ipv6_unicast_socket_.set_option(SOL_SOCKET,
                                        SO_RCVBUF,
                                        (void *) &rcv_size,
                                        sizeof(int)) < 0
        && errno != ENOTSUP) {
      if (DCPS_debug_level > 0) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("(%P|%t) ERROR: ")
                   ACE_TEXT("RtpsUdpDataLink::open: failed to set the receive buffer size to %d errno %m\n"),
                   rcv_size));
      }
      return false;
    }
#endif
  }

  send_strategy()->send_buffer(&multi_buff_);

  if (start(send_strategy_,
            receive_strategy_, false) != 0) {
    stop_i();
    if (DCPS_debug_level > 0) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: ")
                 ACE_TEXT("UdpDataLink::open: start failed!\n")));
    }
    return false;
  }

  TheServiceParticipant->network_interface_address_topic()->connect(network_interface_address_reader_);

  return true;
}

void RtpsUdpDataLink::on_data_available(RcHandle<InternalDataReader<NetworkInterfaceAddress> >)
{
  InternalDataReader<NetworkInterfaceAddress>::SampleSequence samples;
  InternalSampleInfoSequence infos;

  network_interface_address_reader_->take(samples, infos, DDS::LENGTH_UNLIMITED, DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE);

  RtpsUdpInst_rch cfg = config();
  if (!cfg || !cfg->use_multicast()) {
    return;
  }

  RtpsUdpTransport_rch tport = transport();

  multicast_manager_.process(samples,
                             infos,
                             cfg->multicast_interface(),
                             get_reactor(),
                             receive_strategy().in(),
                             cfg->multicast_group_address(tport->domain()),
                             multicast_socket_
#ifdef ACE_HAS_IPV6
                             , cfg->ipv6_multicast_group_address(),
                             ipv6_multicast_socket_
#endif
                             );

  if (!samples.empty()) {
    // FUTURE: This is propagating info to discovery.  Write instead.
    network_change();
  }
}

void
RtpsUdpDataLink::remove_locator_and_bundling_cache(const GUID_t& remote_id)
{
  locator_cache_.remove_id(remote_id);
  bundling_cache_.remove_id(remote_id);
}

NetworkAddress
RtpsUdpDataLink::get_last_recv_address(const GUID_t& remote_id)
{
  ACE_Guard<ACE_Thread_Mutex> guard(locators_lock_);
  const RemoteInfoMap::const_iterator pos = locators_.find(remote_id);
  if (pos != locators_.end()) {
    RtpsUdpTransport_rch tport = transport();
    const TimeDuration threshold = tport ? tport->core().receive_address_duration() : TimeDuration();
    const bool valid_last_recv_addr = (MonotonicTimePoint::now() - pos->second.last_recv_time_) <= threshold;
    return valid_last_recv_addr ? pos->second.last_recv_addr_ : NetworkAddress();
  }
  return NetworkAddress();
}

void
RtpsUdpDataLink::update_locators(const GUID_t& remote_id,
                                 NetworkAddressSet& unicast_addresses,
                                 NetworkAddressSet& multicast_addresses,
                                 bool requires_inline_qos,
                                 bool add_ref)
{
  if (unicast_addresses.empty() && multicast_addresses.empty()) {
    if (DCPS_debug_level > 0) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: RtpsUdpDataLink::update_locators: no addresses for %C\n"), LogGuid(remote_id).c_str()));
    }
  }

  remove_locator_and_bundling_cache(remote_id);

  ACE_GUARD(ACE_Thread_Mutex, g, locators_lock_);

  RemoteInfo& info = locators_[remote_id];
  const bool log_unicast_change = DCPS_debug_level > 3 && info.unicast_addrs_ != unicast_addresses;
  const bool log_multicast_change = DCPS_debug_level > 3 && info.multicast_addrs_ != multicast_addresses;
  info.unicast_addrs_.swap(unicast_addresses);
  info.multicast_addrs_.swap(multicast_addresses);
  info.requires_inline_qos_ = requires_inline_qos;
  if (add_ref) {
    ++info.ref_count_;
  }

  g.release();

  if (log_unicast_change) {
    for (NetworkAddressSet::const_iterator pos = unicast_addresses.begin(), limit = unicast_addresses.end();
         pos != limit; ++pos) {
      ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) RtpsUdpDataLink::update_locators %C is now at %C\n"),
                 LogGuid(remote_id).c_str(), LogAddr(*pos).c_str()));
    }
  }
  if (log_multicast_change) {
    for (NetworkAddressSet::const_iterator pos = multicast_addresses.begin(), limit = multicast_addresses.end();
         pos != limit; ++pos) {
      ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) RtpsUdpDataLink::update_locators %C is now at %C\n"),
                 LogGuid(remote_id).c_str(), LogAddr(*pos).c_str()));
    }
  }
}

void RtpsUdpDataLink::filterBestEffortReaders(const ReceivedDataSample& ds, RepoIdSet& selected, RepoIdSet& withheld)
{
  ACE_GUARD(ACE_Thread_Mutex, g, readers_lock_);
  const GUID_t& writer = ds.header_.publication_id_;
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
RtpsUdpDataLink::make_reservation(const GUID_t& rpi,
                                  const GUID_t& lsi,
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

bool
RtpsUdpDataLink::associated(const GUID_t& local_id, const GUID_t& remote_id,
                            bool local_reliable, bool remote_reliable,
                            bool local_durable, bool remote_durable,
                            const MonotonicTime_t& participant_discovered_at,
                            ACE_CDR::ULong participant_flags,
                            SequenceNumber max_sn,
                            const TransportClient_rch& client,
                            NetworkAddressSet& unicast_addresses,
                            NetworkAddressSet& multicast_addresses,
                            const NetworkAddress& last_addr_hint,
                            bool requires_inline_qos)
{
  sq_.ignore(local_id, remote_id);

  update_locators(remote_id, unicast_addresses, multicast_addresses, requires_inline_qos, true);
  if (last_addr_hint) {
    update_last_recv_addr(remote_id, last_addr_hint);
  }

  const GuidConverter conv(local_id);

  if (!local_reliable) {
    if (conv.isReader()) {
      ACE_GUARD_RETURN(ACE_Thread_Mutex, g, readers_lock_, true);
      WriterToSeqReadersMap::iterator i = writer_to_seq_best_effort_readers_.find(remote_id);
      if (i == writer_to_seq_best_effort_readers_.end()) {
        writer_to_seq_best_effort_readers_.insert(WriterToSeqReadersMap::value_type(remote_id, SeqReaders(local_id)));
      } else if (i->second.readers.find(local_id) == i->second.readers.end()) {
        i->second.readers.insert(local_id);
      }
    }
    return true;
  }

  bool associated = true;

  if (conv.isWriter()) {
    if (transport_debug.log_progress) {
      log_progress("RTPS writer/reader association", local_id, remote_id, participant_discovered_at);
    }

    if (remote_reliable) {
      ACE_GUARD_RETURN(ACE_Thread_Mutex, g, writers_lock_, true);
      // Insert count if not already there.
      RtpsWriterMap::iterator rw = writers_.find(local_id);
      if (rw == writers_.end()) {
        RtpsUdpDataLink_rch link(this, inc_count());
        CORBA::Long hb_start = 0;
        CountMapType::iterator hbc_it = heartbeat_counts_.find(local_id.entityId);
        if (hbc_it != heartbeat_counts_.end()) {
          hb_start = hbc_it->second;
          heartbeat_counts_.erase(hbc_it);
        }
        RtpsWriter_rch writer = make_rch<RtpsWriter>(client, link, local_id, local_durable,
                                                     max_sn, hb_start, multi_buff_.capacity());
        rw = writers_.insert(RtpsWriterMap::value_type(local_id, writer)).first;
      }
      RtpsWriter_rch writer = rw->second;
      g.release();
      const SequenceNumber writer_max_sn = writer->update_max_sn(remote_id, max_sn);
      writer->add_reader(make_rch<ReaderInfo>(remote_id, remote_durable, participant_discovered_at, participant_flags, writer_max_sn + 1));
    }
    invoke_on_start_callbacks(local_id, remote_id, true);
  } else if (conv.isReader()) {
    if (transport_debug.log_progress) {
      log_progress("RTPS reader/writer association", local_id, remote_id, participant_discovered_at);
    }
    {
      GuardType guard(strategy_lock_);
      if (receive_strategy()) {
        receive_strategy()->clear_completed_fragments(remote_id);
      }
    }
    if (remote_reliable) {
      ACE_GUARD_RETURN(ACE_Thread_Mutex, g, readers_lock_, true);
      RtpsReaderMap::iterator rr = readers_.find(local_id);
      if (rr == readers_.end()) {
        pending_reliable_readers_.erase(local_id);
        RtpsUdpDataLink_rch link(this, inc_count());
        RtpsReader_rch reader = make_rch<RtpsReader>(link, local_id);
        rr = readers_.insert(RtpsReaderMap::value_type(local_id, reader)).first;
      }
      RtpsReader_rch reader = rr->second;
      readers_of_writer_.insert(RtpsReaderMultiMap::value_type(remote_id, rr->second));
      g.release();
      add_on_start_callback(client, remote_id);
      reader->add_writer(make_rch<WriterInfo>(remote_id, participant_discovered_at, participant_flags));
      associated = false;
    } else {
      invoke_on_start_callbacks(local_id, remote_id, true);
    }
  }

  return associated;
}

void
RtpsUdpDataLink::disassociated(const GUID_t& local_id,
                               const GUID_t& remote_id)
{
  release_reservations_i(remote_id, local_id);
  remove_locator_and_bundling_cache(remote_id);
  sq_.ignore_remote(remote_id);

  ACE_GUARD(ACE_Thread_Mutex, g, locators_lock_);

  RemoteInfoMap::iterator pos = locators_.find(remote_id);
  if (pos != locators_.end()) {
    OPENDDS_ASSERT(pos->second.ref_count_ > 0);

    --pos->second.ref_count_;
    if (pos->second.ref_count_ == 0) {
      locators_.erase(pos);
    }
  } else if (Transport_debug_level > 3) {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) RtpsUdpDataLink::disassociated: local id %C does not have any locators\n", LogGuid(local_id).c_str()));
  }
}

void
RtpsUdpDataLink::register_for_reader(const GUID_t& writerid,
                                     const GUID_t& readerid,
                                     const NetworkAddressSet& addresses,
                                     DiscoveryListener* listener)
{
  ACE_GUARD(ACE_Thread_Mutex, g, writers_lock_);
  const bool enableheartbeat = interesting_readers_.empty();
  interesting_readers_.insert(
    InterestingRemoteMapType::value_type(
      readerid,
      InterestingRemote(writerid, addresses, listener)));
  if (heartbeat_counts_.find(writerid.entityId) == heartbeat_counts_.end()) {
    heartbeat_counts_[writerid.entityId] = 0;
  }
  g.release();
  if (enableheartbeat) {
    RtpsUdpTransport_rch tport = transport();
    heartbeat_->enable(tport ? tport->core().heartbeat_period() : TimeDuration(RtpsUdpInst::DEFAULT_HEARTBEAT_PERIOD_SEC));
  }
}

void
RtpsUdpDataLink::unregister_for_reader(const GUID_t& writerid,
                                       const GUID_t& readerid)
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
RtpsUdpDataLink::register_for_writer(const GUID_t& readerid,
                                     const GUID_t& writerid,
                                     const NetworkAddressSet& addresses,
                                     DiscoveryListener* listener)
{
  ACE_GUARD(ACE_Thread_Mutex, g, readers_lock_);
  bool enableheartbeatchecker = interesting_writers_.empty();
  interesting_writers_.insert(
    InterestingRemoteMapType::value_type(
      writerid,
      InterestingRemote(readerid, addresses, listener)));
  g.release();
  if (enableheartbeatchecker) {
    RtpsUdpTransport_rch tport = transport();
    heartbeatchecker_->enable(tport ? tport->core().heartbeat_period() : TimeDuration(RtpsUdpInst::DEFAULT_HEARTBEAT_PERIOD_SEC));
  }
}

void
RtpsUdpDataLink::unregister_for_writer(const GUID_t& readerid,
                                       const GUID_t& writerid)
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

void RtpsUdpDataLink::client_stop(const GUID_t& localId)
{
  const GuidConverter conv(localId);

  if (conv.isReader()) {
    ACE_GUARD(ACE_Thread_Mutex, gr, readers_lock_);
    RtpsReaderMap::iterator rr = readers_.find(localId);
    if (rr != readers_.end()) {
      for (RtpsReaderMultiMap::iterator iter = readers_of_writer_.begin();
          iter != readers_of_writer_.end();) {
        if (iter->second->id() == localId) {
          readers_of_writer_.erase(iter++);
        } else {
          ++iter;
        }
      }

      RtpsReader_rch reader = rr->second;
      readers_.erase(rr);
      gr.release();

      reader->pre_stop_helper();

    } else {
      for (WriterToSeqReadersMap::iterator w = writer_to_seq_best_effort_readers_.begin();
          w != writer_to_seq_best_effort_readers_.end();) {
        RepoIdSet::iterator r = w->second.readers.find(localId);
        if (r != w->second.readers.end()) {
          w->second.readers.erase(r);
          if (w->second.readers.empty()) {
            writer_to_seq_best_effort_readers_.erase(w++);
            continue;
          }
        }
        ++w;
      }
    }

  } else {
    RtpsWriter_rch writer;
    {
      // Don't hold the writers lock when destroying a writer.
      ACE_GUARD(ACE_Thread_Mutex, gw, writers_lock_);
      RtpsWriterMap::iterator pos = writers_.find(localId);
      if (pos != writers_.end()) {
        writer = pos->second;
        writers_.erase(pos);
      }
    }

    if (writer) {
      TqeVector to_drop;
      writer->pre_stop_helper(to_drop, true);

      TqeVector::iterator drop_it = to_drop.begin();
      while (drop_it != to_drop.end()) {
        (*drop_it)->data_dropped(true);
        ++drop_it;
      }
      writer->remove_all_msgs();
    } else {
      GuardType guard(strategy_lock_);
      if (send_strategy_) {
        send_strategy_->remove_all_msgs(localId);
      }
    }
  }
  sq_.ignore_local(localId);
}

void
RtpsUdpDataLink::RtpsWriter::pre_stop_helper(TqeVector& to_drop, bool true_stop)
{
  typedef SnToTqeMap::iterator iter_t;

  ACE_GUARD(ACE_Thread_Mutex, g, mutex_);
  ACE_GUARD(ACE_Thread_Mutex, g2, elems_not_acked_mutex_);

  stopping_ = true_stop;

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

  send_buff_->pre_clear();

  g2.release();
  g.release();

  if (stopping_) {
    heartbeat_->cancel();
    nack_response_->cancel();
  }
}

void
RtpsUdpDataLink::pre_stop_i()
{
  DBG_ENTRY_LVL("RtpsUdpDataLink","pre_stop_i",6);
  DataLink::pre_stop_i();
  TqeVector to_drop;

  RtpsWriterMap writers;
  {
    ACE_GUARD(ACE_Thread_Mutex, g, writers_lock_);
    writers_.swap(writers);
    for (RtpsWriterMap::const_iterator it = writers.begin(); it != writers.end(); ++it) {
      heartbeat_counts_.erase(it->first.entityId);
    }
  }

  RtpsWriterMap::iterator w_iter = writers.begin();
  while (w_iter != writers.end()) {
    w_iter->second->pre_stop_helper(to_drop, true);
    writers.erase(w_iter++);
  }

  TqeVector::iterator drop_it = to_drop.begin();
  while (drop_it != to_drop.end()) {
    (*drop_it)->data_dropped(true);
    ++drop_it;
  }

  RtpsReaderMap readers;
  {
    ACE_GUARD(ACE_Thread_Mutex, g, readers_lock_);
    readers = readers_;
  }

  RtpsReaderMap::iterator r_iter = readers.begin();
  while (r_iter != readers.end()) {
    r_iter->second->pre_stop_helper();
    ++r_iter;
  }
}

void
RtpsUdpDataLink::release_reservations_i(const GUID_t& remote_id,
                                        const GUID_t& local_id)
{
  TqeVector to_drop;
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
        writer->pre_stop_helper(to_drop, false);
      }
      writer->process_acked_by_all();
    }

  } else if (conv.isReader()) {
    ACE_GUARD(ACE_Thread_Mutex, g, readers_lock_);
    RtpsReaderMap::iterator rr = readers_.find(local_id);

    if (rr != readers_.end()) {
      for (pair<RtpsReaderMultiMap::iterator, RtpsReaderMultiMap::iterator> iters =
             readers_of_writer_.equal_range(remote_id);
           iters.first != iters.second;) {
        if (iters.first->second->id() == local_id) {
          readers_of_writer_.erase(iters.first++);
        } else {
          ++iters.first;
        }
      }

      RtpsReader_rch reader = rr->second;
      g.release();

      reader->remove_writer(remote_id);

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

  remove_locator_and_bundling_cache(remote_id);

  for (TqeVector::iterator drop_it = to_drop.begin(); drop_it != to_drop.end(); ++drop_it) {
    (*drop_it)->data_dropped(true);
  }
}

void
RtpsUdpDataLink::stop_i()
{
  TheServiceParticipant->network_interface_address_topic()->disconnect(network_interface_address_reader_);

  heartbeat_->disable();
  heartbeatchecker_->disable();
  unicast_socket_.close();
  multicast_socket_.close();
#ifdef ACE_HAS_IPV6
  ipv6_unicast_socket_.close();
  ipv6_multicast_socket_.close();
#endif
}

RcHandle<SingleSendBuffer>
RtpsUdpDataLink::get_writer_send_buffer(const GUID_t& pub_id)
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

  const GUID_t pub_id = tqe->publication_id();

  RcHandle<SingleSendBuffer> send_buff = outer_->get_writer_send_buffer(pub_id);
  if (send_buff.is_nil()) {
    return;
  }

  if (Transport_debug_level > 5) {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) RtpsUdpDataLink::MultiSendBuffer::insert() - "
      "pub_id %C seq %q frag %d\n", LogGuid(pub_id).c_str(), seq.getValue(),
      (int)tqe->is_fragment()));
  }

  if (tqe->is_fragment()) {
    const RtpsCustomizedElement* const rce =
      dynamic_cast<const RtpsCustomizedElement*>(tqe);
    if (rce) {
      send_buff->insert_fragment(seq, rce->last_fragment(), rce->is_last_fragment(), q, chain);
    } else if (Transport_debug_level) {
      ACE_ERROR((LM_ERROR, "(%P|%t) RtpsUdpDataLink::MultiSendBuffer::insert()"
        " - ERROR: couldn't get fragment number for pub_id %C seq %q\n",
        LogGuid(pub_id).c_str(), seq.getValue()));
    }
  } else {
    send_buff->insert(seq, q, chain);
  }
}

ACE_Message_Block*
RtpsUdpDataLink::alloc_msgblock(size_t size, ACE_Allocator* data_allocator) {
  ACE_Message_Block* result;
  ACE_NEW_MALLOC_RETURN(result,
    static_cast<ACE_Message_Block*>(
      mb_allocator_.malloc(sizeof(ACE_Message_Block))),
    ACE_Message_Block(size,
                      ACE_Message_Block::MB_DATA,
                      0, // cont
                      0, // data
                      data_allocator,
                      db_lock_pool_->get_lock(), // locking_strategy
                      ACE_DEFAULT_MESSAGE_BLOCK_PRIORITY,
                      ACE_Time_Value::zero,
                      ACE_Time_Value::max_time,
                      &db_allocator_,
                      &mb_allocator_),
    0);
  return result;
}

ACE_Message_Block*
RtpsUdpDataLink::submsgs_to_msgblock(const RTPS::SubmessageSeq& subm)
{
  // byte swapping is handled in the operator<<() implementation
  const Encoding encoding(Encoding::KIND_XCDR1);
  size_t size = 0;
  for (CORBA::ULong i = 0; i < subm.length(); ++i) {
    encoding.align(size, RTPS::SMHDR_SZ);
    serialized_size(encoding, size, subm[i]);
  }

  ACE_Message_Block* hdr = alloc_msgblock(size, &custom_allocator_);

  Serializer ser(hdr, encoding);
  for (CORBA::ULong i = 0; i < subm.length(); ++i) {
    ser << subm[i];
    ser.align_w(RTPS::SMHDR_SZ);
  }
  return hdr;
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
  if (stopping_ || !link) {
    g.release();
    element->data_dropped(true);
    return 0;
  }

  OPENDDS_ASSERT(element->publication_id() == id_);

  const SequenceNumber previous_max_sn = max_sn_;
  RTPS::SubmessageSeq subm;

  const SequenceNumber seq = element->sequence();
  if (seq != SequenceNumber::SEQUENCENUMBER_UNKNOWN()) {
    if (!element->is_fragment() || element->is_last_fragment()) {
      max_sn_ = std::max(max_sn_, seq);
    }
    if (!durable_ && !is_pvs_writer() &&
        element->subscription_id() == GUID_UNKNOWN &&
        previous_max_sn < max_sn_.previous()) {
      add_gap_submsg_i(subm, previous_max_sn + 1);
    }
  }

  make_leader_lagger(element->subscription_id(), previous_max_sn);
  check_leader_lagger();

  TransportSendElement* tse = dynamic_cast<TransportSendElement*>(element);
  TransportCustomizedElement* tce =
    dynamic_cast<TransportCustomizedElement*>(element);
  TransportSendControlElement* tsce =
    dynamic_cast<TransportSendControlElement*>(element);

  Message_Block_Ptr data;
  bool durable = false;

  const ACE_Message_Block* msg = element->msg();
  const GUID_t pub_id = element->publication_id();

  // Based on the type of 'element', find and duplicate the data payload
  // continuation block.
  if (tsce) {        // Control message
    if (RtpsSampleHeader::control_message_supported(tsce->header().message_id_)) {
      data.reset(msg->cont()->duplicate());
      // Create RTPS Submessage(s) in place of the OpenDDS DataSampleHeader
      RtpsSampleHeader::populate_data_control_submessages(
        subm, *tsce, requires_inline_qos);
      record_directed(element->subscription_id(), seq);
    } else if (tsce->header().message_id_ == END_HISTORIC_SAMPLES) {
      end_historic_samples_i(tsce->header(), msg->cont(), meta_submessages);
      g.release();
      element->data_delivered();
      return 0;
    } else if (tsce->header().message_id_ == REQUEST_ACK) {
      request_ack_i(tsce->header(), msg->cont(), meta_submessages);
      deliver_after_send = true;
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
    record_directed(element->subscription_id(), seq);
    durable = dsle->get_header().historic_sample_;

  } else if (tce) {  // Customized data message
    // {DataSampleHeader} -> {Content Filtering GUIDs} -> {Data Payload}
    data.reset(msg->cont()->cont()->duplicate());
    const DataSampleElement* dsle = tce->original_send_element()->sample();
    // Create RTPS Submessage(s) in place of the OpenDDS DataSampleHeader
    RtpsSampleHeader::populate_data_sample_submessages(
      subm, *dsle, requires_inline_qos);
    record_directed(element->subscription_id(), seq);
    durable = dsle->get_header().historic_sample_;

  } else {
    send_buff_->pre_insert(seq);
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

  if (stopping_) {
    g.release();
    element->data_dropped(true);
    return 0;
  }

  if (transport_debug.log_messages) {
    link->send_strategy()->append_submessages(subm);
  }

  Message_Block_Ptr hdr(link->submsgs_to_msgblock(subm));
  hdr->cont(data.release());
  RtpsCustomizedElement* rtps =
    new RtpsCustomizedElement(element, move(hdr));

  // Handle durability resends
  if (durable) {
    const GUID_t sub = element->subscription_id();
    if (sub != GUID_UNKNOWN) {
      ReaderInfoMap::iterator ri = remote_readers_.find(sub);
      if (ri != remote_readers_.end()) {
        ri->second->durable_data_[rtps->sequence()] = rtps;
        ri->second->durable_timestamp_.set_to_now();
        if (Transport_debug_level > 3) {
          const LogGuid conv(pub_id), sub_conv(sub);
          ACE_DEBUG((LM_DEBUG,
            "(%P|%t) RtpsUdpDataLink::customize_queue_element() - "
            "storing durable data for local %C remote %C seq %q\n",
            conv.c_str(), sub_conv.c_str(),
            rtps->sequence().getValue()));
        }
        return 0;
      }
    }
  }

  send_buff_->pre_insert(seq);
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
  const GUID_t pub_id = element->publication_id();

  {
    GuardType guard(strategy_lock_);
    if (send_strategy_) {
      send_strategy()->encode_payload(pub_id, data, subm);
    }
  }
#endif

  if (transport_debug.log_messages) {
    send_strategy()->append_submessages(subm);
  }

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

  const GUID_t pub_id = element->publication_id();
  GUIDSeq_var peers = peer_ids(pub_id);

  bool require_iq = requires_inline_qos(peers);

  ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, writers_lock_, 0);

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
    guard.release();
    result = customize_queue_element_non_reliable_i(element, require_iq, meta_submessages, deliver_after_send, guard);
  }

  queue_submessages(meta_submessages);

  if (deliver_after_send) {
    element->data_delivered();
  }

  return result;
}

void
RtpsUdpDataLink::RtpsWriter::end_historic_samples_i(const DataSampleHeader& header,
                                                    ACE_Message_Block* body,
                                                    MetaSubmessageVec& meta_submessages)
{
  // Set the ReaderInfo::durable_timestamp_ for the case where no
  // durable samples exist in the DataWriter.
  if (durable_) {
    const MonotonicTimePoint now = MonotonicTimePoint::now();
    GUID_t sub = GUID_UNKNOWN;
    if (body && header.message_length_ >= sizeof(sub)) {
      std::memcpy(&sub, body->rd_ptr(), sizeof(sub));
    }
    typedef ReaderInfoMap::iterator iter_t;
    if (sub == GUID_UNKNOWN) {
      if (Transport_debug_level > 3) {
        ACE_DEBUG((LM_DEBUG, "(%P|%t) RtpsUdpDataLink::end_historic_samples "
                   "local %C all readers\n", LogGuid(id_).c_str()));
      }
      for (iter_t iter = remote_readers_.begin();
           iter != remote_readers_.end(); ++iter) {
        if (iter->second->durable_) {
          iter->second->durable_timestamp_ = now;
          if (transport_debug.log_progress) {
            log_progress("durable data queued", id_, iter->first, iter->second->participant_discovered_at_);
          }
        }
      }
    } else {
      iter_t iter = remote_readers_.find(sub);
      if (iter != remote_readers_.end()) {
        if (iter->second->durable_) {
          iter->second->durable_timestamp_ = now;
          if (transport_debug.log_progress) {
            log_progress("durable data queued", id_, iter->first, iter->second->participant_discovered_at_);
          }
          const SingleSendBuffer::Proxy proxy(*send_buff_);
          MetaSubmessage meta_submessage(id_, GUID_UNKNOWN);
          initialize_heartbeat(proxy, meta_submessage);
          gather_directed_heartbeat_i(proxy, meta_submessages, meta_submessage, iter->second);
          if (Transport_debug_level > 3) {
            ACE_DEBUG((LM_DEBUG, "(%P|%t) RtpsUdpDataLink::end_historic_samples"
                       " local %C remote %C\n", LogGuid(id_).c_str(), LogGuid(sub).c_str()));
          }
        }
      }
    }
  }
}

void
RtpsUdpDataLink::RtpsWriter::request_ack_i(const DataSampleHeader& header,
                                           ACE_Message_Block* body,
                                           MetaSubmessageVec& meta_submessages)
{
  // Set the ReaderInfo::durable_timestamp_ for the case where no
  // durable samples exist in the DataWriter.
  GUID_t sub = GUID_UNKNOWN;
  if (body && header.message_length_ >= sizeof(sub)) {
    std::memcpy(&sub, body->rd_ptr(), sizeof(sub));
  }
  typedef ReaderInfoMap::iterator iter_t;
  if (sub == GUID_UNKNOWN) {
    gather_heartbeats_i(meta_submessages);
    if (Transport_debug_level > 3) {
      ACE_DEBUG((LM_DEBUG, "(%P|%t) RtpsUdpDataLink::request_ack "
                 "local %C all readers\n", LogGuid(id_).c_str()));
    }
  } else {
    iter_t iter = remote_readers_.find(sub);
    if (iter != remote_readers_.end()) {
      const SingleSendBuffer::Proxy proxy(*send_buff_);
      MetaSubmessage meta_submessage(id_, GUID_UNKNOWN);
      initialize_heartbeat(proxy, meta_submessage);
      gather_directed_heartbeat_i(proxy, meta_submessages, meta_submessage, iter->second);
      if (Transport_debug_level > 3) {
        const LogGuid conv(id_), sub_conv(sub);
        ACE_DEBUG((LM_DEBUG, "(%P|%t) RtpsUdpDataLink::request_ack"
                   " local %C remote %C\n", conv.c_str(), sub_conv.c_str()));
      }
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
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, locators_lock_, false);
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
RtpsUdpDataLink::RtpsWriter::send_heartbeats(const MonotonicTimePoint& /*now*/)
{
  ACE_GUARD(ACE_Thread_Mutex, g, mutex_);

  if (stopping_) {
    return;
  }

  RtpsUdpDataLink_rch link = link_.lock();

  if (!link) {
    return;
  }

  MetaSubmessageVec meta_submessages;
  const bool not_sending = !link->send_strategy()->is_sending(id_);
  if (not_sending) {
    gather_heartbeats_i(meta_submessages);
  }

  if (!preassociation_readers_.empty() || !lagging_readers_.empty()) {
    heartbeat_->schedule(fallback_.get());
    if (not_sending) {
      fallback_.advance();
    } else {
      fallback_.set(initial_fallback_);
    }
  } else {
    fallback_.set(initial_fallback_);
  }

  g.release();

  link->queue_submessages(meta_submessages);
}

void
RtpsUdpDataLink::RtpsWriter::send_nack_responses(const MonotonicTimePoint& /*now*/)
{
  RtpsUdpDataLink_rch link = link_.lock();
  if (!link) {
    return;
  }

  MetaSubmessageVec meta_submessages;
  {
    ACE_GUARD(ACE_Thread_Mutex, g, mutex_);

    if (stopping_) {
      return;
    }

    gather_nack_replies_i(meta_submessages);
  }

  link->queue_submessages(meta_submessages);
}

void
RtpsUdpDataLink::RtpsWriter::add_gap_submsg_i(RTPS::SubmessageSeq& msg,
                                              SequenceNumber gap_start)
{
  // These are the GAP submessages that we'll send directly in-line with the
  // DATA when we notice that the DataWriter has deliberately skipped seq #s.
  // There are other GAP submessages generated in meta_submessage to reader ACKNACKS,
  // see send_nack_replies().
  using namespace OpenDDS::RTPS;

  const LongSeq8 bitmap;

  // RTPS v2.1 8.3.7.4: the Gap sequence numbers are those in the range
  // [gapStart, gapListBase) and those in the SNSet.
  GapSubmessage gap = {
    {GAP, FLAG_E, 0 /*length determined below*/},
    ENTITYID_UNKNOWN, // readerId: applies to all matched readers
    id_.entityId,
    to_rtps_seqnum(gap_start),
    {to_rtps_seqnum(max_sn_), 0, bitmap}
  };
  OPENDDS_ASSERT(gap_start < max_sn_);

  const size_t size = serialized_size(Encoding(Encoding::KIND_XCDR1), gap);
  gap.smHeader.submessageLength =
      static_cast<CORBA::UShort>(size) - SMHDR_SZ;

  const CORBA::ULong idx = grow(msg) - 1;
  msg[idx].gap_sm(gap);
}

void RtpsUdpDataLink::update_last_recv_addr(const GUID_t& src, const NetworkAddress& addr, const MonotonicTimePoint& now)
{
  RtpsUdpTransport_rch tport = transport();

  if (!tport) {
    return;
  }

  if (addr == tport->core().rtps_relay_address()) {
    return;
  }

  bool remove_cache = false;
  {
    ACE_GUARD(ACE_Thread_Mutex, g, locators_lock_);
    const RemoteInfoMap::iterator pos = locators_.find(src);
    if (pos != locators_.end()) {
      const bool expired = tport->core().receive_address_duration() < (MonotonicTimePoint::now() - pos->second.last_recv_time_);
      const bool allow_update = expired ||
                                pos->second.last_recv_addr_ == addr ||
                                is_more_local(pos->second.last_recv_addr_, addr);
      if (allow_update) {
        remove_cache = pos->second.last_recv_addr_ != addr;
        pos->second.last_recv_addr_ = addr;
        pos->second.last_recv_time_ = now;
      }
    }
  }
  if (remove_cache) {
    remove_locator_and_bundling_cache(src);
  }
}

// DataReader's side of Reliability

void
RtpsUdpDataLink::received(const RTPS::DataSubmessage& data,
                          const GuidPrefix_t& src_prefix,
                          const NetworkAddress& remote_addr)
{
  const GUID_t local = make_id(local_prefix_, data.readerId);
  const GUID_t src = make_id(src_prefix, data.writerId);

  update_last_recv_addr(src, remote_addr);

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
        RtpsUdpReceiveStrategy_rch trs = receive_strategy();
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
        RtpsUdpReceiveStrategy_rch trs = receive_strategy();
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
  queue_submessages(meta_submessages);
}

void
RtpsUdpDataLink::RtpsReader::pre_stop_helper()
{
  ACE_GUARD(ACE_Thread_Mutex, g, mutex_);

  if (stopping_) {
    return;
  }

  stopping_ = true;

  preassociation_writers_.clear();
  log_remote_counts("pre_stop_helper");

  RtpsUdpDataLink_rch link = link_.lock();

  if (!link) {
    return;
  }

  GuardType guard(link->strategy_lock_);
  if (link->receive_strategy() == 0) {
    return;
  }

  for (WriterInfoMap::iterator it = remote_writers_.begin(); it != remote_writers_.end(); ++it) {
    it->second->held_.clear();
  }

  guard.release();
  g.release();

  preassociation_task_->cancel();
}

RtpsUdpDataLink::RtpsReader::RtpsReader(const RtpsUdpDataLink_rch& link, const GUID_t& id)
  : link_(link)
  , id_(id)
  , stopping_(false)
  , nackfrag_count_(0)
  , preassociation_task_(make_rch<SporadicEvent>(link->event_dispatcher(), make_rch<PmfNowEvent<RtpsReader> >(rchandle_from(this), &RtpsUdpDataLink::RtpsReader::send_preassociation_acknacks)))
  , heartbeat_period_(link ? link->config()->heartbeat_period() : TimeDuration(RtpsUdpInst::DEFAULT_HEARTBEAT_PERIOD_SEC))
{
}

RtpsUdpDataLink::RtpsReader::~RtpsReader()
{
}

bool
RtpsUdpDataLink::RtpsReader::process_data_i(const RTPS::DataSubmessage& data,
                                            const GUID_t& src,
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

  const SequenceNumber seq = to_opendds_seqnum(data.writerSN);
  DeliverHeldData dhd;
  const WriterInfoMap::iterator wi = remote_writers_.find(src);
  if (wi != remote_writers_.end()) {
    const WriterInfo_rch& writer = wi->second;

    DeliverHeldData dhd2(rchandle_from(this), src);
    std::swap(dhd, dhd2);

    writer->frags_.erase(seq);

    if (writer->recvd_.empty()) {
      if (Transport_debug_level > 5) {
        ACE_DEBUG((LM_DEBUG,
                   ACE_TEXT("(%P|%t) RtpsUdpDataLink::process_data_i(DataSubmessage) -")
                   ACE_TEXT(" data seq: %q from %C being from %C expecting heartbeat\n"),
                   seq.getValue(),
                   LogGuid(src).c_str(),
                   LogGuid(id_).c_str()));
      }
      const ReceivedDataSample* sample =
        link->receive_strategy()->withhold_data_from(id_);
      writer->held_.insert(std::make_pair(seq, *sample));

    } else if (writer->recvd_.contains(seq)) {
      if (transport_debug.log_dropped_messages) {
        ACE_DEBUG((LM_DEBUG, "(%P|%t) {transport_debug.log_dropped_messages} RtpsUdpDataLink::RtpsReader::process_data_i: %C -> %C duplicate sample\n", LogGuid(src).c_str(), LogGuid(id_).c_str()));
      }
      if (Transport_debug_level > 5) {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) RtpsUdpDataLink::process_data_i(DataSubmessage) -")
                             ACE_TEXT(" data seq: %q from %C being DROPPED from %C because it's ALREADY received\n"),
                             seq.getValue(),
                             LogGuid(src).c_str(),
                             LogGuid(id_).c_str()));
      }
      link->receive_strategy()->withhold_data_from(id_);

    } else if (!writer->held_.empty()) {
      const ReceivedDataSample* sample =
        link->receive_strategy()->withhold_data_from(id_);
      if (Transport_debug_level > 5) {
        ACE_DEBUG((LM_DEBUG, "(%P|%t) RtpsUdpDataLink::process_data_i(DataSubmessage) WITHHOLD %q\n", seq.getValue()));
        writer->recvd_.dump();
      }
      writer->held_.insert(std::make_pair(seq, *sample));
      writer->recvd_.insert(seq);

    } else if (writer->recvd_.disjoint() || writer->recvd_.cumulative_ack() != seq.previous()) {
      if (Transport_debug_level > 5) {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) RtpsUdpDataLink::process_data_i(DataSubmessage) -")
                             ACE_TEXT(" data seq: %q from %C being WITHHELD from %C because it's EXPECTING more data\n"),
                             seq.getValue(),
                             LogGuid(src).c_str(),
                             LogGuid(id_).c_str()));
      }
      const ReceivedDataSample* sample =
        link->receive_strategy()->withhold_data_from(id_);
      writer->held_.insert(std::make_pair(seq, *sample));
      writer->recvd_.insert(seq);

    } else {
      if (Transport_debug_level > 5) {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) RtpsUdpDataLink::process_data_i(DataSubmessage) -")
                             ACE_TEXT(" data seq: %q from %C to %C OK to deliver\n"),
                             seq.getValue(),
                             LogGuid(src).c_str(),
                             LogGuid(id_).c_str()));
      }
      writer->recvd_.insert(seq);
      link->receive_strategy()->do_not_withhold_data_from(id_);
    }

  } else {
    if (transport_debug.log_dropped_messages) {
      ACE_DEBUG((LM_DEBUG, "(%P|%t) {transport_debug.log_dropped_messages} RtpsUdpDataLink::RtpsReader::process_data_i: %C -> %C unknown remote writer\n", LogGuid(src).c_str(), LogGuid(id_).c_str()));
    }
    if (Transport_debug_level > 5) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) RtpsUdpDataLink::process_data_i(DataSubmessage) -")
                           ACE_TEXT(" data seq: %q from %C to %C dropped because of unknown writer\n"),
                           to_opendds_seqnum(data.writerSN).getValue(),
                           LogGuid(src).c_str(),
                           LogGuid(id_).c_str()));
    }
    link->receive_strategy()->withhold_data_from(id_);
  }

  // Release for delivering held data.
  guard.release();
  g.release();

  return false;
}

void
RtpsUdpDataLink::received(const RTPS::GapSubmessage& gap,
                          const GuidPrefix_t& src_prefix,
                          bool directed,
                          const NetworkAddress& remote_addr)
{
  update_last_recv_addr(make_id(src_prefix, gap.writerId), remote_addr);
  datareader_dispatch(gap, src_prefix, directed, &RtpsReader::process_gap_i);
}

void
RtpsUdpDataLink::RtpsReader::process_gap_i(const RTPS::GapSubmessage& gap,
                                           const GUID_t& src,
                                           bool /*directed*/,
                                           MetaSubmessageVec&)
{
  ACE_GUARD(ACE_Thread_Mutex, g, mutex_);

  RtpsUdpDataLink_rch link = link_.lock();

  if (!link) {
    return;
  }

  GuardType guard(link->strategy_lock_);
  if (link->receive_strategy() == 0) {
    return;
  }

  const WriterInfoMap::iterator wi = remote_writers_.find(src);
  if (wi == remote_writers_.end()) {
    if (transport_debug.log_dropped_messages) {
      ACE_DEBUG((LM_DEBUG, "(%P|%t) {transport_debug.log_dropped_messages} RtpsUdpDataLink::RtpsReader::process_gap_i: %C -> %C unknown remote writer\n", LogGuid(src).c_str(), LogGuid(id_).c_str()));
    }
    return;
  }

  const WriterInfo_rch& writer = wi->second;

  if (writer->recvd_.empty()) {
    if (transport_debug.log_dropped_messages) {
      ACE_DEBUG((LM_DEBUG, "(%P|%t) {transport_debug.log_dropped_messages} RtpsUdpDataLink::RtpsReader::process_gap_i: %C -> %C preassociation writer\n", LogGuid(src).c_str(), LogGuid(id_).c_str()));
    }
    return;
  }

  const SequenceNumber start = to_opendds_seqnum(gap.gapStart);
  const SequenceNumber base = to_opendds_seqnum(gap.gapList.bitmapBase);

  if (start < base) {
    writer->recvd_.insert(SequenceRange(start, base.previous()));
  } else if (start != base) {
    ACE_ERROR((LM_ERROR, "(%P|%t) RtpsUdpDataLink::RtpsReader::process_gap_i: ERROR - Incoming GAP has inverted start (%q) & base (%q) values, ignoring start value\n", start.getValue(), base.getValue()));
  }
  writer->recvd_.insert(base, gap.gapList.numBits, gap.gapList.bitmap.get_buffer());

  DisjointSequence gaps;
  if (start < base) {
    gaps.insert(SequenceRange(start, base.previous()));
  }
  gaps.insert(base, gap.gapList.numBits, gap.gapList.bitmap.get_buffer());

  if (!gaps.empty()) {
    for (WriterInfo::HeldMap::iterator pos = writer->held_.lower_bound(gaps.low()),
           limit = writer->held_.upper_bound(gaps.high()); pos != limit;) {
      if (gaps.contains(pos->first)) {
        writer->held_.erase(pos++);
      } else {
        ++pos;
      }
    }
  }

  const OPENDDS_VECTOR(SequenceRange) psr = gaps.present_sequence_ranges();
  for (OPENDDS_VECTOR(SequenceRange)::const_iterator pos = psr.begin(), limit = psr.end(); pos != limit; ++pos) {
    link->receive_strategy()->remove_fragments(*pos, writer->id_);
  }

  guard.release();
  g.release();

  DeliverHeldData dhd(rchandle_from(this), src);
}

void
RtpsUdpDataLink::received(const RTPS::HeartBeatSubmessage& heartbeat,
                          const GuidPrefix_t& src_prefix,
                          bool directed,
                          const NetworkAddress& remote_addr)
{
  const GUID_t src = make_id(src_prefix, heartbeat.writerId);
  const MonotonicTimePoint now = MonotonicTimePoint::now();

  update_last_recv_addr(src, remote_addr, now);

  MetaSubmessageVec meta_submessages;
  OPENDDS_VECTOR(InterestingRemote) callbacks;
  {
    ACE_GUARD(ACE_Thread_Mutex, g, readers_lock_);

    // We received a heartbeat from a writer.
    // We should ACKNACK if the writer is interesting and there is no association.

    for (InterestingRemoteMapType::iterator pos = interesting_writers_.lower_bound(src),
           limit = interesting_writers_.upper_bound(src);
         pos != limit;
         ++pos) {
      pos->second.last_activity = now;
      if (pos->second.status == InterestingRemote::DOES_NOT_EXIST) {
        callbacks.push_back(pos->second);
        pos->second.status = InterestingRemote::EXISTS;
      }
    }
  }
  queue_submessages(meta_submessages);

  for (size_t i = 0; i < callbacks.size(); ++i) {
    callbacks[i].listener->writer_exists(src, callbacks[i].localid);
  }

  datareader_dispatch(heartbeat, src_prefix, directed, &RtpsReader::process_heartbeat_i);
}

void
RtpsUdpDataLink::RtpsReader::process_heartbeat_i(const RTPS::HeartBeatSubmessage& heartbeat,
                                                 const GUID_t& src,
                                                 bool directed,
                                                 MetaSubmessageVec& meta_submessages)
{
  // TODO: Delay responses by heartbeat_response_delay_.
  ACE_GUARD(ACE_Thread_Mutex, g, mutex_);

  RtpsUdpDataLink_rch link = link_.lock();

  if (!link) {
    return;
  }

  GuardType guard(link->strategy_lock_);
  if (link->receive_strategy() == 0) {
    return;
  }

  // Heartbeat Sequence Range
  const SequenceNumber hb_first = to_opendds_seqnum(heartbeat.firstSN);
  const SequenceNumber hb_last = to_opendds_seqnum(heartbeat.lastSN);

  if (Transport_debug_level > 5) {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) RtpsUdpDataLink::RtpsReader::process_heartbeat_i - %C -> %C first %q last %q count %d\n",
      LogGuid(src).c_str(), LogGuid(id_).c_str(), hb_first.getValue(), hb_last.getValue(), heartbeat.count.value));
  }

  const WriterInfoMap::iterator wi = remote_writers_.find(src);
  if (wi == remote_writers_.end()) {
    if (transport_debug.log_dropped_messages) {
      ACE_DEBUG((LM_DEBUG, "(%P|%t) {transport_debug.log_dropped_messages} RtpsUdpDataLink::RtpsReader::process_heartbeat_i: %C -> %C unknown remote writer\n", LogGuid(src).c_str(), LogGuid(id_).c_str()));
    }
    return;
  }

  const WriterInfo_rch& writer = wi->second;

  if (!compare_and_update_counts(heartbeat.count.value, writer->heartbeat_recvd_count_)) {
    if (transport_debug.log_dropped_messages) {
      const GUID_t dst = heartbeat.readerId == DCPS::ENTITYID_UNKNOWN ? GUID_UNKNOWN : id_;
      ACE_DEBUG((LM_DEBUG, "(%P|%t) {transport_debug.log_dropped_messages} RtpsUdpDataLink::RtpsReader::process_heartbeat_i: %C -> %C stale/duplicate message (%d vs %d)\n",
        LogGuid(src).c_str(), LogGuid(dst).c_str(), heartbeat.count.value, writer->heartbeat_recvd_count_));
    }
    VDBG((LM_WARNING, "(%P|%t) RtpsUdpDataLink::process_heartbeat_i "
          "WARNING Count indicates duplicate, dropping\n"));
    return;
  }

  const bool is_final = heartbeat.smHeader.flags & RTPS::FLAG_F;

  static const SequenceNumber one, zero = SequenceNumber::ZERO();

  bool first_ever_hb = false;

  if (!is_final && transport_debug.log_nonfinal_messages) {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) {transport_debug.log_nonfinal_messages} RtpsUdpDataLink::RtpsReader::process_heartbeat_i - %C -> %C first %q last %q count %d\n",
      LogGuid(src).c_str(), LogGuid(id_).c_str(), hb_first.getValue(), hb_last.getValue(), heartbeat.count.value));
  }

  // Only valid heartbeats (see spec) will be "fully" applied to writer info
  if (!(hb_first < 1 || hb_last < 0 || hb_last < hb_first.previous())) {
    if (writer->recvd_.empty() && (directed || !writer->sends_directed_hb())) {
      OPENDDS_ASSERT(preassociation_writers_.count(writer));
      preassociation_writers_.erase(writer);
      if (transport_debug.log_progress) {
        log_progress("RTPS reader/writer association complete", id_, writer->id_, writer->participant_discovered_at_);
      }
      log_remote_counts("process_heartbeat_i");
      first_ever_hb = true;
    }


    ACE_CDR::ULong cumulative_bits_added = 0;
    if (!writer->recvd_.empty() || first_ever_hb) {
      // "gap" everything before the heartbeat range
      const SequenceRange sr(zero, hb_first.previous());
      writer->recvd_.insert(sr);
      while (!writer->held_.empty() && writer->held_.begin()->first <= sr.second) {
        writer->held_.erase(writer->held_.begin());
      }
      for (WriterInfo::HeldMap::const_iterator it = writer->held_.begin(); it != writer->held_.end(); ++it) {
        writer->recvd_.insert(it->first);
      }
      link->receive_strategy()->remove_fragments(sr, writer->id_);

      writer->hb_last_ = std::max(writer->hb_last_, hb_last);
      gather_ack_nacks_i(writer, link, !is_final, meta_submessages, cumulative_bits_added);
    }
    if (cumulative_bits_added) {
      link->transport()->core().reader_nack_count(id_, cumulative_bits_added);
    }
  } else {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: RtpsUdpDataLink::RtpsReader::process_heartbeat_i: %C -> %C - INVALID - first %q last %q count %d\n", LogGuid(writer->id_).c_str(), LogGuid(id_).c_str(), hb_first.getValue(), hb_last.getValue(), heartbeat.count.value));
  }

  guard.release();
  g.release();

  if (first_ever_hb) {
    link->invoke_on_start_callbacks(id_, src, true);
  }

  DeliverHeldData dhd(rchandle_from(this), src);

  //FUTURE: support assertion of liveliness for MANUAL_BY_TOPIC
  return;
}

bool
RtpsUdpDataLink::WriterInfo::should_nack() const
{
  if (recvd_.empty() || (recvd_.disjoint() && recvd_.cumulative_ack() < hb_last_)) {
    return true;
  }
  if (!recvd_.empty()) {
    return recvd_.high() < hb_last_;
  }
  return false;
}

bool RtpsUdpDataLink::WriterInfo::sends_directed_hb() const
{
  return participant_flags_ & RTPS::PFLAGS_DIRECTED_HEARTBEAT;
}

bool
RtpsUdpDataLink::RtpsWriter::add_reader(const ReaderInfo_rch& reader)
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, mutex_, false);
  OPENDDS_ASSERT(!reader->durable_ || durable_);

  if (stopping_) {
    return false;
  }

  ReaderInfoMap::const_iterator iter = remote_readers_.find(reader->id_);
  if (iter == remote_readers_.end()) {
#ifdef OPENDDS_SECURITY
    if (is_pvs_writer_) {
      reader->max_pvs_sn_ = max_sn_;
    }
#endif
    remote_readers_.insert(ReaderInfoMap::value_type(reader->id_, reader));
    update_remote_guids_cache_i(true, reader->id_);
    preassociation_readers_.insert(reader);
    preassociation_reader_start_sns_.insert(reader->start_sn_);
    log_remote_counts("add_reader");

    RtpsUdpDataLink_rch link = link_.lock();
    if (!link) {
      return false;
    }

    fallback_.set(initial_fallback_);
    heartbeat_->schedule(fallback_.get());
    // Durable readers will get their heartbeat from end historic samples.
    if (!reader->durable_) {
      MetaSubmessageVec meta_submessages;
      MetaSubmessage meta_submessage(id_, GUID_UNKNOWN);
      const SingleSendBuffer::Proxy proxy(*send_buff_);
      initialize_heartbeat(proxy, meta_submessage);
      gather_directed_heartbeat_i(proxy, meta_submessages, meta_submessage, reader);
      g.release();
      link->queue_submessages(meta_submessages);
    }

    return true;
  }
  return false;
}

bool
RtpsUdpDataLink::RtpsWriter::has_reader(const GUID_t& id) const
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, mutex_, false);
  return remote_readers_.count(id) != 0;
}

bool
RtpsUdpDataLink::RtpsWriter::remove_reader(const GUID_t& id)
{
  OPENDDS_MAP(SequenceNumber, TransportQueueElement*) dd;
  TqeSet to_drop;

  bool result = false;
  {
    ACE_Guard<ACE_Thread_Mutex> g(mutex_);
    ReaderInfoMap::iterator it = remote_readers_.find(id);
    if (it != remote_readers_.end()) {
      const ReaderInfo_rch& reader = it->second;
      reader->swap_durable_data(dd);
      remove_preassociation_reader(reader);
      const SequenceNumber acked_sn = reader->acked_sn();
      const SequenceNumber max_sn = expected_max_sn(reader);
      readers_expecting_data_.erase(reader);
      readers_expecting_heartbeat_.erase(reader);
      snris_erase(acked_sn == max_sn ? leading_readers_ : lagging_readers_, acked_sn, reader);
      check_leader_lagger();

#ifdef OPENDDS_SECURITY
      if (is_pvs_writer_ &&
          !reader->pvs_outstanding_.empty()) {
        const OPENDDS_VECTOR(SequenceRange) psr = reader->pvs_outstanding_.present_sequence_ranges();
        for (OPENDDS_VECTOR(SequenceRange)::const_iterator pos = psr.begin(), limit = psr.end(); pos != limit; ++pos) {
          ACE_GUARD_RETURN(ACE_Thread_Mutex, g, elems_not_acked_mutex_, result);
          for (SequenceNumber seq = pos->first; seq <= pos->second; ++seq) {
            OPENDDS_MULTIMAP(SequenceNumber, TransportQueueElement*)::iterator iter = elems_not_acked_.find(seq);
            if (iter != elems_not_acked_.end()) {
              send_buff_->release_acked(iter->first);
              to_drop.insert(iter->second);
              elems_not_acked_.erase(iter);
            }
          }
        }
      }
#endif

      remote_readers_.erase(it);
      update_remote_guids_cache_i(false, id);
      result = true;
      log_remote_counts("remove_reader");
    }
  }
  typedef OPENDDS_MAP(SequenceNumber, TransportQueueElement*)::iterator iter_t;
  for (iter_t it = dd.begin(); it != dd.end(); ++it) {
    it->second->data_dropped();
  }

  for (TqeSet::iterator pos = to_drop.begin(), limit = to_drop.end(); pos != limit; ++pos) {
    (*pos)->data_dropped();
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
RtpsUdpDataLink::RtpsReader::add_writer(const WriterInfo_rch& writer)
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, mutex_, false);

  if (stopping_) {
    return false;
  }

  WriterInfoMap::const_iterator iter = remote_writers_.find(writer->id_);
  if (iter == remote_writers_.end()) {
    remote_writers_[writer->id_] = writer;
    preassociation_writers_.insert(writer);
    log_remote_counts("add_writer");

    RtpsUdpDataLink_rch link = link_.lock();
    if (!link) {
      return false;
    }

    preassociation_task_->schedule(heartbeat_period_);
    MetaSubmessageVec meta_submessages;
    gather_preassociation_acknack_i(meta_submessages, writer);
    g.release();
    link->queue_submessages(meta_submessages);

    return true;
  }
  return false;
}

bool
RtpsUdpDataLink::RtpsReader::has_writer(const GUID_t& id) const
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, mutex_, false);
  return remote_writers_.count(id) != 0;
}

bool
RtpsUdpDataLink::RtpsReader::remove_writer(const GUID_t& id)
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, mutex_, false);
  WriterInfoMap::iterator pos = remote_writers_.find(id);
  if (pos != remote_writers_.end()) {
    preassociation_writers_.erase(pos->second);
    remote_writers_.erase(pos);
    log_remote_counts("remove_writer");
    return true;
  }

  return false;
}

size_t
RtpsUdpDataLink::RtpsReader::writer_count() const
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, mutex_, 0);
  return remote_writers_.size();
}

bool
RtpsUdpDataLink::RtpsReader::should_nack_fragments(const RcHandle<RtpsUdpDataLink>& link,
                                                   const WriterInfo_rch& info)
{
  if (!info->frags_.empty()) {
    return true;
  }

  if (!info->recvd_.empty()) {
    const SequenceRange range(info->recvd_.cumulative_ack() + 1, info->hb_last_);
    if (link->receive_strategy()->has_fragments(range, info->id_)) {
      return true;
    }
  }

  return false;
}

void
RtpsUdpDataLink::RtpsReader::send_preassociation_acknacks(const MonotonicTimePoint& /*now*/)
{
  RtpsUdpDataLink_rch link = link_.lock();
  if (!link) {
    return;
  }

  MetaSubmessageVec meta_submessages;
  {
    ACE_GUARD(ACE_Thread_Mutex, g, mutex_);

    if (stopping_ || preassociation_writers_.empty()) {
      return;
    }

    // We want a heartbeat from these writers.
    meta_submessages.reserve(preassociation_writers_.size());
    for (WriterInfoSet::const_iterator pos = preassociation_writers_.begin(), limit = preassociation_writers_.end();
         pos != limit; ++pos) {
      gather_preassociation_acknack_i(meta_submessages, *pos);
    }
  }

  link->queue_submessages(meta_submessages);

  preassociation_task_->schedule(heartbeat_period_);
}

void
RtpsUdpDataLink::RtpsReader::gather_preassociation_acknack_i(MetaSubmessageVec& meta_submessages,
                                                             const WriterInfo_rch& writer)
{
  using namespace OpenDDS::RTPS;

  OPENDDS_ASSERT(writer->recvd_.empty());
  const CORBA::ULong num_bits = 0;
  const LongSeq8 bitmap;
  const EntityId_t reader_id = id_.entityId;
  const EntityId_t writer_id = writer->id_.entityId;

  MetaSubmessage meta_submessage(id_, writer->id_);

  AckNackSubmessage acknack = {
    {ACKNACK,
     CORBA::Octet(FLAG_E),
     0 /*length*/},
    reader_id,
    writer_id,
    { // SequenceNumberSet: acking bitmapBase - 1
      {0, 1},
      num_bits, bitmap
    },
    {writer->heartbeat_recvd_count_}
  };
  meta_submessage.sm_.acknack_sm(acknack);
  meta_submessages.push_back(meta_submessage);
}

void
RtpsUdpDataLink::RtpsReader::gather_ack_nacks_i(const WriterInfo_rch& writer,
                                                const RtpsUdpDataLink_rch& link,
                                                bool heartbeat_was_non_final,
                                                MetaSubmessageVec& meta_submessages,
                                                ACE_CDR::ULong& cumulative_bits_added)
{
  const bool should_nack_frags = should_nack_fragments(link, writer);
  if (writer->should_nack() ||
      should_nack_frags) {
    using namespace OpenDDS::RTPS;
    const EntityId_t reader_id = id_.entityId;
    const EntityId_t writer_id = writer->id_.entityId;
    MetaSubmessage meta_submessage(id_, writer->id_);

    const DisjointSequence& recvd = writer->recvd_;
    const SequenceNumber& hb_high = writer->hb_last_;
    const SequenceNumber ack = recvd.empty() ? 1 : ++SequenceNumber(recvd.cumulative_ack());
    const SequenceNumber::Value ack_val = ack.getValue();
    CORBA::ULong num_bits = 0;
    LongSeq8 bitmap;

    if (recvd.disjoint()) {
      bitmap.length(DisjointSequence::bitmap_num_longs(ack, recvd.last_ack().previous()));
      if (bitmap.length() > 0) {
        (void)recvd.to_bitmap(bitmap.get_buffer(), bitmap.length(),
                              num_bits, cumulative_bits_added, true);
      }
    }

    if (!recvd.empty() && hb_high > recvd.high()) {
      const SequenceNumber eff_high =
        (hb_high <= ack_val + 255) ? hb_high : (ack_val + 255);
      const SequenceNumber::Value eff_high_val = eff_high.getValue();
      // Nack the range between the received high and the effective high.
      const CORBA::ULong old_len = bitmap.length(),
        new_len = DisjointSequence::bitmap_num_longs(ack, eff_high);
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
                                          num_bits, cumulative_bits_added);
    }

    // If the receive strategy is holding any fragments, those should
    // not be "nacked" in the ACKNACK reply.  They will be accounted for
    // in the NACK_FRAG(s) instead.
    const bool frags_modified =
      link->receive_strategy()->remove_frags_from_bitmap(bitmap.get_buffer(),
                                                         num_bits, ack, writer->id_, cumulative_bits_added);
    if (frags_modified) {
      for (CORBA::ULong i = 0; i < bitmap.length(); ++i) {
        if ((i + 1) * 32 <= num_bits) {
          if (bitmap[i]) {
            break;
          }
        } else {
          if ((0xffffffff << (32 - (num_bits % 32))) & bitmap[i]) {
            break;
          }
        }
      }
    }

    AckNackSubmessage acknack = {
      {ACKNACK,
       CORBA::Octet(FLAG_E),
       0 /*length*/},
      reader_id,
      writer_id,
      { // SequenceNumberSet: acking bitmapBase - 1
        to_rtps_seqnum(ack),
        num_bits, bitmap
      },
      {writer->heartbeat_recvd_count_}
    };
    meta_submessage.sm_.acknack_sm(acknack);
    meta_submessages.push_back(meta_submessage);

    if (should_nack_frags) {
      generate_nack_frags_i(meta_submessages, writer, reader_id, writer_id, cumulative_bits_added);
    }
  } else if (heartbeat_was_non_final) {
    using namespace OpenDDS::RTPS;
    const DisjointSequence& recvd = writer->recvd_;
    const CORBA::ULong num_bits = 0;
    const LongSeq8 bitmap;
    const SequenceNumber ack = recvd.empty() ? 1 : ++SequenceNumber(recvd.cumulative_ack());
    const EntityId_t reader_id = id_.entityId;
    const EntityId_t writer_id = writer->id_.entityId;

    MetaSubmessage meta_submessage(id_, writer->id_);

    AckNackSubmessage acknack = {
      {ACKNACK,
       CORBA::Octet(FLAG_E | FLAG_F),
       0 /*length*/},
      reader_id,
      writer_id,
      { // SequenceNumberSet: acking bitmapBase - 1
        to_rtps_seqnum(ack),
        num_bits, bitmap
      },
      {writer->heartbeat_recvd_count_}
    };
    meta_submessage.sm_.acknack_sm(acknack);
    meta_submessages.push_back(meta_submessage);
  }
}

#ifdef OPENDDS_SECURITY
namespace {
  const NetworkAddress BUNDLING_PLACEHOLDER;
}
#endif

void
RtpsUdpDataLink::build_meta_submessage_map(MetaSubmessageVec& meta_submessages, AddrDestMetaSubmessageMap& addr_map)
{
  size_t cache_hits = 0;
  size_t cache_misses = 0;
  size_t addrset_min_size = std::numeric_limits<size_t>::max();
  size_t addrset_max_size = 0;

  BundlingCache::ScopedAccess global_access(bundling_cache_);
  const MonotonicTimePoint now = MonotonicTimePoint::now();

  // Sort meta_submessages by address set and destination
  for (MetaSubmessageVec::iterator it = meta_submessages.begin(), limit = meta_submessages.end(); it != limit; ++it) {
    if (it->ignore_) {
      continue;
    }

    const BundlingCacheKey key(it->src_guid_, it->dst_guid_);
    BundlingCache::ScopedAccess entry(bundling_cache_, key, false, now);
    if (entry.is_new_) {

      NetworkAddressSet& addrs = entry.value().addrs_;
      ACE_GUARD(ACE_Thread_Mutex, g, locators_lock_);

      const bool directed = it->dst_guid_ != GUID_UNKNOWN;
      if (directed) {
        accumulate_addresses(it->src_guid_, it->dst_guid_, addrs, true);
      } else {
        addrs = get_addresses_i(it->src_guid_);
      }
#ifdef OPENDDS_SECURITY
      if (local_crypto_handle() != DDS::HANDLE_NIL && separate_message(it->src_guid_.entityId)) {
        addrs.insert(BUNDLING_PLACEHOLDER); // removed in bundle_mapped_meta_submessages
      }
#endif
#if defined ACE_HAS_CPP11
      entry.recalculate_hash();
#endif
      ++cache_misses;
    } else {
      ++cache_hits;
    }

    const BundlingCache::ScopedAccess& const_entry = entry;
    const NetworkAddressSet& addrs = const_entry.value().addrs_;
    addrset_min_size = std::min(addrset_min_size, static_cast<size_t>(addrs.size()));
    addrset_max_size = std::max(addrset_max_size, static_cast<size_t>(addrs.size()));
    if (addrs.empty()) {
      continue;
#ifdef OPENDDS_SECURITY
    } else if (addrs.size() == 1 && *addrs.begin() == BUNDLING_PLACEHOLDER) {
      continue;
#endif
    }

    DestMetaSubmessageMap& dest_map = addr_map[AddressCacheEntryProxy(const_entry.rch_)];
    if (std::memcmp(&(it->dst_guid_.guidPrefix), &GUIDPREFIX_UNKNOWN, sizeof(GuidPrefix_t)) != 0) {
      MetaSubmessageIterVec& vec = dest_map[make_unknown_guid(it->dst_guid_.guidPrefix)];
      vec.reserve(meta_submessages.size());
      vec.push_back(it);
    } else {
      MetaSubmessageIterVec& vec = dest_map[GUID_UNKNOWN];
      vec.reserve(meta_submessages.size());
      vec.push_back(it);
    }
  }

  VDBG((LM_DEBUG, "(%P|%t) RtpsUdpDataLink::build_meta_submessage_map()"
                  "- Bundling Cache Stats: hits = %B, misses = %B, min = %B, max = %B\n",
                  cache_hits, cache_misses, addrset_min_size, addrset_max_size));
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
  static const size_t initial_size =
#ifdef OPENDDS_SECURITY
    RtpsUdpSendStrategy::MaxSecureFullMessageLeadingSize;
#else
    0;
#endif

  BundleHelper(
    const Encoding& encoding, size_t max_bundle_size,
    RtpsUdpDataLink::BundleVec& bundles)
  : encoding_(encoding)
  , max_bundle_size_(max_bundle_size)
  , size_(initial_size)
  , bundles_(bundles)
  {
  }

  void end_bundle()
  {
    bundles_.back().size_ = size_;
    size_ = initial_size;
  }

  template <typename T>
  bool add_to_bundle(T& submessage)
  {
    const size_t prev_size = size_;
#ifdef OPENDDS_SECURITY
    // Could be an encoded submessage (encoding happens later)
    size_ += RtpsUdpSendStrategy::MaxSecureSubmessageLeadingSize;
#endif
    const size_t submessage_size = serialized_size(encoding_, submessage);
    submessage.smHeader.submessageLength = static_cast<CORBA::UShort>(submessage_size - RTPS::SMHDR_SZ);
    align(size_, RTPS::SM_ALIGN);
    size_ += submessage_size;
#ifdef OPENDDS_SECURITY
    // Could be an encoded submessage (encoding happens later)
    align(size_, RTPS::SM_ALIGN);
    size_ += RtpsUdpSendStrategy::MaxSecureSubmessageFollowingSize;
#endif
    size_t compare_size = size_;
#ifdef OPENDDS_SECURITY
    // Could be an encoded rtps message (encoding happens later)
    align(compare_size, RTPS::SM_ALIGN);
    compare_size += RtpsUdpSendStrategy::MaxSecureFullMessageFollowingSize;
#endif
    if (compare_size > max_bundle_size_) {
      const size_t chunk_size = size_ - prev_size;
      bundles_.back().size_ = prev_size;
      size_ = initial_size + chunk_size;
      return false;
    }
    return true;
  }

  const Encoding& encoding_;
  const size_t max_bundle_size_;
  size_t size_;
  RtpsUdpDataLink::BundleVec& bundles_;
};

}

void
RtpsUdpDataLink::bundle_mapped_meta_submessages(const Encoding& encoding,
                                                AddrDestMetaSubmessageMap& addr_map,
                                                BundleVec& bundles,
                                                CountKeeper& counts)
{
  using namespace RTPS;

  // Reusable INFO_DST
  InfoDestinationSubmessage idst = {
    {INFO_DST, FLAG_E, INFO_DST_SZ},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
  };

  RtpsUdpTransport_rch tport = transport();

  const bool new_bundle_per_dest_guid = tport && tport->core().rtps_relay_only();

  BundleHelper helper(encoding, max_bundle_size_, bundles);
  GUID_t prev_dst; // used to determine when we need to write a new info_dst
  for (AddrDestMetaSubmessageMap::iterator addr_it = addr_map.begin(), limit = addr_map.end(); addr_it != limit; ++addr_it) {

    // A new address set always starts a new bundle
    bundles.push_back(Bundle(addr_it->first));

    prev_dst = GUID_UNKNOWN;

    for (DestMetaSubmessageMap::iterator dest_it = addr_it->second.begin(), limit2 = addr_it->second.end(); dest_it != limit2; ++dest_it) {

      if (dest_it->second.empty()) {
        continue;
      }

      // Check to see if we're sending separate messages per destination guid
      if (new_bundle_per_dest_guid && bundles.back().submessages_.size()) {
        helper.end_bundle();
        bundles.push_back(Bundle(addr_it->first));
        prev_dst = GUID_UNKNOWN;
      }

      for (MetaSubmessageIterVec::iterator resp_it = dest_it->second.begin(), limit3 = dest_it->second.end(); resp_it != limit3; ++resp_it) {

        // Check before every meta_submessage to see if we need to prefix a INFO_DST
        if (dest_it->first != prev_dst) {
          // If adding an INFO_DST prefix bumped us over the limit, push the
          // size difference into the next bundle, reset prev_dst, and keep going
          if (!helper.add_to_bundle(idst)) {
            bundles.push_back(Bundle(addr_it->first));
          }
        }

        // Attempt to add the submessage meta_submessage to the bundle
        bool result = false, unique = false;
        ACE_UNUSED_ARG(unique);
        MetaSubmessage& res = **resp_it;
        switch (res.sm_._d()) {
          case HEARTBEAT: {
            const EntityId_t id = res.sm_.heartbeat_sm().writerId;
            result = helper.add_to_bundle(res.sm_.heartbeat_sm());
            CountMapPair& map_pair = counts.heartbeat_counts_[id].map_[res.sm_.heartbeat_sm().count.value];
            if (res.dst_guid_ == GUID_UNKNOWN) {
              map_pair.undirected_ = true;
            }
            break;
          }
          case ACKNACK: {
            result = helper.add_to_bundle(res.sm_.acknack_sm());
            break;
          }
          case GAP: {
            result = helper.add_to_bundle(res.sm_.gap_sm());
            break;
          }
          case NACK_FRAG: {
            const EntityId_t id = res.sm_.nack_frag_sm().readerId;
            unique = counts.nackfrag_counts_[id].insert(res.sm_.nack_frag_sm().count.value).second;
            OPENDDS_ASSERT(unique);
            result = helper.add_to_bundle(res.sm_.nack_frag_sm());
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
          bundles.push_back(Bundle(addr_it->first));
          prev_dst = GUID_UNKNOWN;
        }
        bundles.back().submessages_.push_back(*resp_it);
      }
    }
    helper.end_bundle();
  }
}

void
RtpsUdpDataLink::harvest_send_queue(const MonotonicTimePoint& /*now*/)
{
  sq_.begin_transaction();
  sq_.ready_to_send();

  disable_response_queue(true);
}

void
RtpsUdpDataLink::flush_send_queue(const MonotonicTimePoint& /*now*/)
{
  ACE_Guard<ACE_Thread_Mutex> fsq_guard(fsq_mutex_);
  flush_send_queue_i();
}

void
RtpsUdpDataLink::flush_send_queue_i()
{
  for (size_t idx = 0; idx != fsq_vec_size_; ++idx) {
    dedup(fsq_vec_[idx]);
    bundle_and_send_submessages(fsq_vec_[idx]);
    fsq_vec_[idx].clear();
  }
  fsq_vec_size_ = 0;
}

void
RtpsUdpDataLink::enable_response_queue()
{
  sq_.begin_transaction();
}

void
RtpsUdpDataLink::disable_response_queue(bool send_immediately)
{
  MetaSubmessageVec vec;
  ACE_Guard<ACE_Thread_Mutex> fsq_guard(fsq_mutex_);
  sq_.end_transaction(vec);
  if (!vec.empty()) {
    if (fsq_vec_.size() == fsq_vec_size_) {
      fsq_vec_.resize(fsq_vec_.size() + 1);
    }
    fsq_vec_[fsq_vec_size_++].swap(vec);
  }

  if (fsq_vec_size_) {
    if (send_immediately) {
      flush_send_queue_i();
    } else {
      flush_send_queue_sporadic_->schedule(TimeDuration::zero_value);
    }
  }

}

void
RtpsUdpDataLink::queue_submessages(MetaSubmessageVec& in)
{
  if (in.empty()) {
    return;
  }

  if (sq_.enqueue(in)) {
    RtpsUdpTransport_rch tport = transport();
    if (tport) {
      harvest_send_queue_sporadic_->schedule(tport->core().send_delay());
    }
  }
}

void
RtpsUdpDataLink::RtpsWriter::update_required_acknack_count(const GUID_t& id, CORBA::Long current)
{
  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
  ReaderInfoMap::iterator ri = remote_readers_.find(id);
  if (ri != remote_readers_.end()) {
    ri->second->required_acknack_count_ = current;
  }
}

void
RtpsUdpDataLink::update_required_acknack_count(const GUID_t& local_id, const GUID_t& remote_id, CORBA::Long current)
{
  RtpsWriter_rch writer;
  {
    ACE_Guard<ACE_Thread_Mutex> guard(writers_lock_);
    RtpsWriterMap::iterator rw = writers_.find(local_id);
    if (rw != writers_.end()) {
      writer = rw->second;
    }
  }
  if (writer) {
    writer->update_required_acknack_count(remote_id, current);
  }
}

void
RtpsUdpDataLink::bundle_and_send_submessages(MetaSubmessageVec& meta_submessages)
{
  using namespace RTPS;

  // Sort meta_submessages based on both locator IPs and INFO_DST GUID destination/s
  AddrDestMetaSubmessageMap addr_map;
  build_meta_submessage_map(meta_submessages, addr_map);

  const Encoding encoding(Encoding::KIND_XCDR1);

  // Build reasonably-sized submessage bundles based on our destination map
  BundleVec bundles;
  bundles.reserve(meta_submessages.size());

  CountKeeper counts;
  bundle_mapped_meta_submessages(encoding, addr_map, bundles, counts);

  // Reusable INFO_DST
  InfoDestinationSubmessage idst = {
    {INFO_DST, FLAG_E, INFO_DST_SZ},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
  };

  for (IdCountMapping::iterator it = counts.heartbeat_counts_.begin(), limit = counts.heartbeat_counts_.end(); it != limit; ++it) {
    it->second.next_directed_unassigned_ = it->second.map_.begin();
    it->second.next_undirected_unassigned_ = it->second.map_.begin();
    for (CountMap::iterator it2 = it->second.map_.begin(), limit2 = it->second.map_.end(); it2 != limit2; ++it2) {
      if (it2->second.undirected_) {
        ++(it->second.next_directed_unassigned_);
      }
    }
  }

  // Allocate buffers, seralize, and send bundles
  GUID_t prev_dst; // used to determine when we need to write a new info_dst
  for (size_t i = 0; i < bundles.size(); ++i) {
    RTPS::Message rtps_message;
    prev_dst = GUID_UNKNOWN;
    Message_Block_Ptr mb_bundle(alloc_msgblock(bundles[i].size_, &bundle_allocator_));
    Serializer ser(mb_bundle.get(), encoding);
    const MetaSubmessageIterVec& bundle_vec = bundles[i].submessages_;
    for (MetaSubmessageIterVec::const_iterator it = bundle_vec.begin(), limit = bundle_vec.end(); it != limit; ++it) {
      MetaSubmessage& res = **it;
      const GUID_t dst = make_unknown_guid(res.dst_guid_);
      if (dst != prev_dst) {
        assign(idst.guidPrefix, dst.guidPrefix);
        ser << idst;
        if (transport_debug.log_messages) {
          append_submessage(rtps_message, idst);
        }
      }
      switch (res.sm_._d()) {
        case HEARTBEAT: {
          CountMapping& mapping = counts.heartbeat_counts_[res.sm_.heartbeat_sm().writerId];
          CountMapPair& map_pair = mapping.map_[res.sm_.heartbeat_sm().count.value];
          if (!map_pair.is_new_assigned_) {
            if (map_pair.undirected_) {
              OPENDDS_ASSERT(mapping.next_undirected_unassigned_ != mapping.map_.end());
              map_pair.new_ = mapping.next_undirected_unassigned_->first;
              ++mapping.next_undirected_unassigned_;
            } else {
              OPENDDS_ASSERT(mapping.next_directed_unassigned_ != mapping.map_.end());
              map_pair.new_ = mapping.next_directed_unassigned_->first;
              ++mapping.next_directed_unassigned_;
              if (res.sm_.heartbeat_sm().smHeader.flags & RTPS::OPENDDS_FLAG_R) {
                if (res.sm_.heartbeat_sm().count.value != map_pair.new_) {
                  update_required_acknack_count(res.src_guid_, res.dst_guid_, map_pair.new_);
                }
                res.sm_.heartbeat_sm().smHeader.flags &= ~RTPS::OPENDDS_FLAG_R;
              }
            }
            map_pair.is_new_assigned_ = true;
          }
          res.sm_.heartbeat_sm().count.value = map_pair.new_;
          const HeartBeatSubmessage& heartbeat = res.sm_.heartbeat_sm();
          if (transport_debug.log_nonfinal_messages && !(heartbeat.smHeader.flags & RTPS::FLAG_F)) {
            const SequenceNumber hb_first = to_opendds_seqnum(heartbeat.firstSN);
            const SequenceNumber hb_last = to_opendds_seqnum(heartbeat.lastSN);
            ACE_DEBUG((LM_DEBUG, "(%P|%t) {transport_debug.log_nonfinal_messages} RtpsUdpDataLink::bundle_and_send_submessages: HEARTBEAT: %C -> %C first %q last %q count %d\n",
              LogGuid(res.src_guid_).c_str(), LogGuid(res.dst_guid_).c_str(), hb_first.getValue(), hb_last.getValue(), heartbeat.count.value));
          }
          break;
        }
        case ACKNACK: {
          const AckNackSubmessage& acknack = res.sm_.acknack_sm();
          if (transport_debug.log_nonfinal_messages && !(acknack.smHeader.flags & RTPS::FLAG_F)) {
            const SequenceNumber ack = to_opendds_seqnum(acknack.readerSNState.bitmapBase);
            ACE_DEBUG((LM_DEBUG, "(%P|%t) {transport_debug.log_nonfinal_messages} RtpsUdpDataLink::bundle_and_send_submessages: ACKNACK: %C -> %C base %q bits %u count %d\n",
              LogGuid(res.src_guid_).c_str(), LogGuid(res.dst_guid_).c_str(), ack.getValue(), acknack.readerSNState.numBits, acknack.count.value));
          }
          break;
        }
        case NACK_FRAG: {
          CountSet& set = counts.nackfrag_counts_[res.sm_.nack_frag_sm().readerId];
          OPENDDS_ASSERT(!set.empty());
          res.sm_.nack_frag_sm().count.value = *set.begin();
          set.erase(set.begin());
          const NackFragSubmessage& nackfrag = res.sm_.nack_frag_sm();
          // All NackFrag messages are technically 'non-final' since they are only used to negatively acknowledge fragments and expect a response
          if (transport_debug.log_nonfinal_messages) {
            const SequenceNumber seq = to_opendds_seqnum(nackfrag.writerSN);
            ACE_DEBUG((LM_DEBUG, "(%P|%t) {transport_debug.log_nonfinal_messages} RtpsUdpDataLink::bundle_and_send_submessages: NACKFRAG: %C -> %C seq %q base %u bits %u\n",
              LogGuid(res.src_guid_).c_str(), LogGuid(res.dst_guid_).c_str(), seq.getValue(), nackfrag.fragmentNumberState.bitmapBase.value, nackfrag.fragmentNumberState.numBits));
          }
          break;
        }
        default: {
          break;
        }
      }
      ser << res.sm_;
      if (transport_debug.log_messages) {
        push_back(rtps_message.submessages, res.sm_);
      }
      prev_dst = dst;
    }
    RtpsUdpSendStrategy_rch ss = send_strategy();
    if (ss) {
      ss->send_rtps_control(rtps_message, *(mb_bundle.get()), bundles[i].proxy_.addrs());
    }
  }
}

void
RtpsUdpDataLink::RtpsReader::generate_nack_frags_i(MetaSubmessageVec& meta_submessages,
                                                   const WriterInfo_rch& wi,
                                                   EntityId_t reader_id,
                                                   EntityId_t writer_id,
                                                   ACE_CDR::ULong& cumulative_bits_added)
{
  typedef OPENDDS_MAP(SequenceNumber, RTPS::FragmentNumber_t)::iterator iter_t;
  typedef RtpsUdpReceiveStrategy::FragmentInfo::value_type Frag_t;
  RtpsUdpReceiveStrategy::FragmentInfo frag_info;

  // This is an internal method, locks already locked,
  // we just need a local handle to the link
  RtpsUdpDataLink_rch link = link_.lock();

  // Populate frag_info with two possible sources of NackFrags:
  // 1. sequence #s in the reception gaps that we have partially received
  OPENDDS_VECTOR(SequenceRange) missing = wi->recvd_.missing_sequence_ranges();
  for (size_t i = 0; i < missing.size(); ++i) {
    link->receive_strategy()->has_fragments(missing[i], wi->id_, &frag_info);
  }
  // 1b. larger than the last received seq# but less than the heartbeat.lastSN
  if (!wi->recvd_.empty() && wi->recvd_.high() < wi->hb_last_) {
    const SequenceRange range(wi->recvd_.high() + 1, wi->hb_last_);
    link->receive_strategy()->has_fragments(range, wi->id_, &frag_info);
  }
  for (size_t i = 0; i < frag_info.size(); ++i) {
    // If we've received a HeartbeatFrag, we know the last (available) frag #
    const iter_t heartbeat_frag = wi->frags_.find(frag_info[i].first);
    if (heartbeat_frag != wi->frags_.end()) {
      extend_bitmap_range(frag_info[i].second, heartbeat_frag->second.value, cumulative_bits_added);
    }
  }

  // 2. sequence #s outside the recvd_ gaps for which we have a HeartbeatFrag
  const iter_t low = wi->frags_.lower_bound(wi->recvd_.cumulative_ack()),
              high = wi->frags_.upper_bound(wi->recvd_.last_ack()),
               end = wi->frags_.end();
  for (iter_t iter = wi->frags_.begin(); iter != end; ++iter) {
    if (iter == low) {
      // skip over the range covered by step #1 above
      if (high == end) {
        break;
      }
      iter = high;
    }

    const SequenceRange range(iter->first, iter->first);
    if (!link->receive_strategy()->has_fragments(range, wi->id_, &frag_info)) {
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
    reader_id,
    writer_id,
    {0, 0}, // writerSN set below
    RTPS::FragmentNumberSet(), // fragmentNumberState set below
    {0} // count set below
  };

  meta_submessages.reserve(meta_submessages.size() + frag_info.size());
  for (size_t i = 0; i < frag_info.size(); ++i) {
    MetaSubmessage meta_submessage(id_, wi->id_);
    meta_submessage.sm_.nack_frag_sm(nackfrag_prototype);
    RTPS::NackFragSubmessage& nackfrag = meta_submessage.sm_.nack_frag_sm();
    nackfrag.writerSN = to_rtps_seqnum(frag_info[i].first);
    nackfrag.fragmentNumberState = frag_info[i].second;
    nackfrag.count.value = ++nackfrag_count_;
    meta_submessages.push_back(meta_submessage);
  }
}

void
RtpsUdpDataLink::extend_bitmap_range(RTPS::FragmentNumberSet& fnSet,
                                     CORBA::ULong extent,
                                     ACE_CDR::ULong& samples_requested)
{
  if (extent < fnSet.bitmapBase.value) {
    return; // can't extend to some number under the base
  }
  // calculate the index to the extent to determine the new_num_bits
  const CORBA::ULong new_num_bits = std::min(CORBA::ULong(256),
                                             extent - fnSet.bitmapBase.value + 1),
                     len = (new_num_bits + 31) / 32;
  if (new_num_bits < fnSet.numBits) {
    return; // bitmap already extends past "extent"
  }
  fnSet.bitmap.length(len);
  // We are missing from one past old bitmap end to the new end
  DisjointSequence::fill_bitmap_range(fnSet.numBits, new_num_bits,
                                      fnSet.bitmap.get_buffer(), len,
                                      fnSet.numBits, samples_requested);
}

void
RtpsUdpDataLink::received(const RTPS::HeartBeatFragSubmessage& hb_frag,
                          const GuidPrefix_t& src_prefix,
                          bool directed,
                          const NetworkAddress& remote_addr)
{
  update_last_recv_addr(make_id(src_prefix, hb_frag.writerId), remote_addr);
  datareader_dispatch(hb_frag, src_prefix, directed, &RtpsReader::process_heartbeat_frag_i);
}

void
RtpsUdpDataLink::RtpsReader::process_heartbeat_frag_i(const RTPS::HeartBeatFragSubmessage& hb_frag,
                                                      const GUID_t& src,
                                                      bool /*directed*/,
                                                      MetaSubmessageVec& meta_submessages)
{
  ACE_GUARD(ACE_Thread_Mutex, g, mutex_);

  RtpsUdpDataLink_rch link = link_.lock();

  if (!link) {
    return;
  }

  GuardType guard(link->strategy_lock_);
  if (link->receive_strategy() == 0) {
    return;
  }

  const WriterInfoMap::iterator wi = remote_writers_.find(src);
  if (wi == remote_writers_.end()) {
    // we may not be associated yet, even if the writer thinks we are
    if (transport_debug.log_dropped_messages) {
      ACE_DEBUG((LM_DEBUG, "(%P|%t) {transport_debug.log_dropped_messages} RtpsUdpDataLink::RtpsReader::process_heartbeat_frag_i: %C -> %C unknown remote writer\n", LogGuid(src).c_str(), LogGuid(id_).c_str()));
    }
    return;
  }

  const WriterInfo_rch& writer = wi->second;

  if (!compare_and_update_counts(hb_frag.count.value, writer->hb_frag_recvd_count_)) {
    if (transport_debug.log_dropped_messages) {
      ACE_DEBUG((LM_DEBUG, "(%P|%t) {transport_debug.log_dropped_messages} RtpsUdpDataLink::RtpsReader::process_heartbeat_frag_i: %C -> %C stale/duplicate message\n", LogGuid(src).c_str(), LogGuid(id_).c_str()));
    }
    VDBG((LM_WARNING, "(%P|%t) RtpsUdpDataLink::process_heartbeat_frag_i "
          "WARNING Count indicates duplicate, dropping\n"));
    return;
  }

  // If seq is outside the heartbeat range or we haven't completely received
  // it yet, send a NackFrag along with the AckNack.  The heartbeat range needs
  // to be checked first because recvd_ contains the numbers below the
  // heartbeat range (so that we don't NACK those).
  const SequenceNumber seq = to_opendds_seqnum(hb_frag.writerSN);
  if (seq > writer->hb_last_ || !writer->recvd_.contains(seq)) {
    writer->frags_[seq] = hb_frag.lastFragmentNum;
    ACE_CDR::ULong cumulative_bits_added = 0;
    gather_ack_nacks_i(writer, link, !(hb_frag.smHeader.flags & RTPS::FLAG_F), meta_submessages, cumulative_bits_added);
    if (cumulative_bits_added) {
      link->transport()->core().reader_nack_count(id_, cumulative_bits_added);
    }

  }
}


// DataWriter's side of Reliability

void
RtpsUdpDataLink::received(const RTPS::AckNackSubmessage& acknack,
                          const GuidPrefix_t& src_prefix,
                          const NetworkAddress& remote_addr)
{
  // local side is DW
  const GUID_t local = make_id(local_prefix_, acknack.writerId); // can't be ENTITYID_UNKNOWN
  const GUID_t remote = make_id(src_prefix, acknack.readerId);
  const MonotonicTimePoint now = MonotonicTimePoint::now();

  update_last_recv_addr(remote, remote_addr, now);

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
RtpsUdpDataLink::RtpsWriter::gather_gaps_i(const ReaderInfo_rch& reader,
                                           const DisjointSequence& gaps,
                                           MetaSubmessageVec& meta_submessages)
{
  using namespace RTPS;

  OPENDDS_ASSERT(reader || !durable_);

  if (gaps.empty()) {
    return;
  }

  // RTPS v2.1 8.3.7.4: the Gap sequence numbers are those in the range
  // [gapStart, gapListBase) and those in the SNSet.
  const SequenceNumber firstMissing = gaps.low(),
                       base = ++SequenceNumber(gaps.cumulative_ack());
  const SequenceNumber_t gapStart = to_rtps_seqnum(firstMissing);
  const SequenceNumber_t gapListBase = to_rtps_seqnum(base);
  CORBA::ULong num_bits = 0;
  LongSeq8 bitmap;

  if (gaps.disjoint()) {
    bitmap.length(DisjointSequence::bitmap_num_longs(base, gaps.high()));
    if (bitmap.length() > 0) {
      ACE_CDR::ULong cumulative_bits_added = 0;
      (void)gaps.to_bitmap(bitmap.get_buffer(), bitmap.length(), num_bits, cumulative_bits_added);
    }
  }

  MetaSubmessage meta_submessage(id_, reader ? reader->id_ : GUID_UNKNOWN);
  GapSubmessage gap = {
    {GAP, FLAG_E, 0 /*length determined later*/},
    reader ? reader->id_.entityId : ENTITYID_UNKNOWN,
    id_.entityId,
    gapStart,
    {gapListBase, num_bits, bitmap}
  };
  OPENDDS_ASSERT(firstMissing < base);
  meta_submessage.sm_.gap_sm(gap);

  if (Transport_debug_level > 5) {
    const LogGuid conv(id_);
    SequenceRange sr;
    sr.first = to_opendds_seqnum(gap.gapStart);
    const SequenceNumber srbase = to_opendds_seqnum(gap.gapList.bitmapBase);
    sr.second = srbase.previous();
    ACE_DEBUG((LM_DEBUG, "(%P|%t) RtpsUdpDataLink::RtpsWriter::gather_gaps_i "
              "GAP with range [%q, %q] from %C\n",
              sr.first.getValue(), sr.second.getValue(),
              conv.c_str()));
  }

  meta_submessages.push_back(meta_submessage);
}

void
RtpsUdpDataLink::RtpsWriter::process_acknack(const RTPS::AckNackSubmessage& acknack,
                                             const GUID_t& src,
                                             MetaSubmessageVec&)
{
  ACE_GUARD(ACE_Thread_Mutex, g, mutex_);

  if (stopping_) {
    return;
  }

  RtpsUdpDataLink_rch link = link_.lock();

  if (!link) {
    return;
  }

  const SequenceNumber ack = to_opendds_seqnum(acknack.readerSNState.bitmapBase);

  if (Transport_debug_level > 5) {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) RtpsUdpDataLink::RtpsWriter::process_acknack: %C -> %C base %q bits %u count %d\n",
      LogGuid(src).c_str(), LogGuid(id_).c_str(), ack.getValue(), acknack.readerSNState.numBits, acknack.count.value));
  }

  ReaderInfoMap::iterator ri = remote_readers_.find(src);
  if (ri == remote_readers_.end()) {
    if (transport_debug.log_dropped_messages) {
      ACE_DEBUG((LM_DEBUG, "(%P|%t) {transport_debug.log_dropped_messages} RtpsUdpDataLink::RtpsWriter::process_acknack: %C -> %C unknown remote reader\n", LogGuid(src).c_str(), LogGuid(id_).c_str()));
    }
    VDBG((LM_WARNING, "(%P|%t) RtpsUdpDataLink::received(ACKNACK) "
      "WARNING ReaderInfo not found\n"));
    return;
  }

  const ReaderInfo_rch& reader = ri->second;
  const SequenceNumber max_sn = expected_max_sn(reader);
  const SequenceNumber sn_received_by_reader = ack.previous();
  if (sn_received_by_reader > max_sn) {
    if (transport_debug.log_dropped_messages) {
      ACE_DEBUG((LM_DEBUG, "(%P|%t) {transport_debug.log_dropped_messages} "
                 "RtpsUdpDataLink::RtpsWriter::process_acknack: %C -> %C "
                 "Received sequence number (%q) > expected max sequence number (%q)\n",
                 LogGuid(src).c_str(), LogGuid(id_).c_str(), sn_received_by_reader.getValue(), max_sn.getValue()));
    }
    return;
  }

  SequenceNumber previous_acked_sn = reader->acked_sn();
  const bool count_is_not_zero = acknack.count.value != 0;
  const CORBA::Long previous_count = reader->acknack_recvd_count_;
  bool dont_schedule_nack_response = false;

  if (count_is_not_zero) {
    if (!compare_and_update_counts(acknack.count.value, reader->acknack_recvd_count_) &&
        (!reader->reflects_heartbeat_count() || acknack.count.value != 0 || reader->acknack_recvd_count_ != 0)) {
      if (transport_debug.log_dropped_messages) {
        ACE_DEBUG((LM_DEBUG, "(%P|%t) {transport_debug.log_dropped_messages} RtpsUdpDataLink::RtpsWriter::process_acknack: %C -> %C stale/duplicate message\n", LogGuid(src).c_str(), LogGuid(id_).c_str()));
      }
      VDBG((LM_WARNING, "(%P|%t) RtpsUdpDataLink::received(ACKNACK) "
            "WARNING Count indicates duplicate, dropping\n"));
      return;
    }

    if (reader->reflects_heartbeat_count()) {
      if (acknack.count.value < reader->required_acknack_count_) {
        if (transport_debug.log_dropped_messages) {
          ACE_DEBUG((LM_DEBUG, "(%P|%t) {transport_debug.log_dropped_messages} RtpsUdpDataLink::RtpsWriter::process_acknack: %C -> %C stale message (reflect %d < %d)\n", LogGuid(src).c_str(), LogGuid(id_).c_str(), acknack.count.value, reader->required_acknack_count_));
        }
        dont_schedule_nack_response = true;
      }
    }
  }

  fallback_.set(initial_fallback_);

  const bool is_final = acknack.smHeader.flags & RTPS::FLAG_F;
  const bool is_postassociation = count_is_not_zero && (is_final || bitmapNonEmpty(acknack.readerSNState) || ack != 1);

  if (preassociation_readers_.count(reader) && is_postassociation) {
    remove_preassociation_reader(reader);
    if (transport_debug.log_progress) {
      log_progress("RTPS writer/reader association complete", id_, reader->id_, reader->participant_discovered_at_);
    }
    log_remote_counts("process_acknack");

    const SequenceNumber max_sn = expected_max_sn(reader);
    const SequenceNumber acked_sn = reader->acked_sn();
    snris_insert(acked_sn == max_sn ? leading_readers_ : lagging_readers_, reader);
    check_leader_lagger();
    // Heartbeat is already scheduled.
  }

  OPENDDS_MAP(SequenceNumber, TransportQueueElement*) pendingCallbacks;

  if (!is_final && transport_debug.log_nonfinal_messages) {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) {transport_debug.log_nonfinal_messages} RtpsUdpDataLink::RtpsWriter::process_acknack: %C -> %C base %q bits %u count %d\n",
      LogGuid(src).c_str(), LogGuid(id_).c_str(), ack.getValue(), acknack.readerSNState.numBits, acknack.count.value));
  }

  // Process the ack.
  bool inform_send_listener = false;
  if (ack != SequenceNumber::SEQUENCENUMBER_UNKNOWN()) {

    if (ack >= reader->cur_cumulative_ack_) {
      reader->cur_cumulative_ack_ = ack;
      inform_send_listener = true;
    } else if (count_is_not_zero) {
      // Count increased but ack decreased.  Reset.
      ACE_ERROR((LM_WARNING, "(%P|%t) WARNING RtpsUdpDataLink::RtpsWriter::process_acknack: "
                 "%C -> %C reset detected count %d > %d ack %q < %q\n",
                 LogGuid(id_).c_str(), LogGuid(reader->id_).c_str(),
                 acknack.count.value, previous_count, ack.getValue(), reader->cur_cumulative_ack_.getValue()));
      const SequenceNumber max_sn = expected_max_sn(reader);
      snris_erase(previous_acked_sn == max_sn ? leading_readers_ : lagging_readers_, previous_acked_sn, reader);
      reader->cur_cumulative_ack_ = ack;
      const SequenceNumber acked_sn = reader->acked_sn();
      snris_insert(acked_sn == max_sn ? leading_readers_ : lagging_readers_, reader);
      previous_acked_sn = acked_sn;
      check_leader_lagger();
      heartbeat_->schedule(fallback_.get());

      if (reader->durable_) {
        if (Transport_debug_level > 5) {
          ACE_DEBUG((LM_DEBUG, "(%P|%t) RtpsUdpDataLink::RtpsWriter::process_acknack: enqueuing ReplayDurableData\n"));
        }
        reader->durable_data_.swap(pendingCallbacks);
        link->event_dispatcher()->dispatch(make_rch<ReplayDurableData>(link_, id_, src));
        reader->durable_timestamp_ = MonotonicTimePoint::zero_value;
      }
    }

    if (!reader->durable_data_.empty()) {
      if (Transport_debug_level > 5) {
        const LogGuid local_conv(id_), remote_conv(src);
        ACE_DEBUG((LM_DEBUG, "(%P|%t) RtpsUdpDataLink::RtpsWriter::process_acknack: "
                   "local %C has durable for remote %C\n",
                   local_conv.c_str(),
                   remote_conv.c_str()));
      }
      const SequenceNumber& dd_last = reader->durable_data_.rbegin()->first;
      if (Transport_debug_level > 5) {
        ACE_DEBUG((LM_DEBUG, "(%P|%t) RtpsUdpDataLink::RtpsWriter::process_acknack: "
                   "check base %q against last durable %q\n",
                   ack.getValue(), dd_last.getValue()));
      }
      if (ack > dd_last) {
        if (transport_debug.log_progress) {
          log_progress("durable delivered", id_, reader->id_, reader->participant_discovered_at_);
        }
        // Reader acknowledges durable data, we no longer need to store it
        if (Transport_debug_level > 5) {
          ACE_DEBUG((LM_DEBUG, "(%P|%t) RtpsUdpDataLink::RtpsWriter::process_acknack: "
                     "durable data acked\n"));
        }
        reader->durable_data_.swap(pendingCallbacks);
      } else {
        for (OPENDDS_MAP(SequenceNumber, TransportQueueElement*)::iterator pos = reader->durable_data_.begin(),
               limit = reader->durable_data_.end(); pos != limit && pos->first < ack;) {
          pendingCallbacks.insert(*pos);
          reader->durable_data_.erase(pos++);
        }
      }
    }
  } else {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: RtpsUdpDataLink::RtpsWriter::process_acknack: %C -> %C invalid acknack\n", LogGuid(src).c_str(), LogGuid(id_).c_str()));
  }

  // Process the nack.
  bool schedule_nack_response = false;
  if (!dont_schedule_nack_response) {
    if (count_is_not_zero) {
      reader->requests_.reset();
      {
        const SingleSendBuffer::Proxy proxy(*send_buff_);
        if ((acknack.readerSNState.numBits == 0 ||
             (acknack.readerSNState.numBits == 1 && !(acknack.readerSNState.bitmap[0] & 1)))
            && ack == max_data_seq(proxy, reader)) {
          // Since there is an entry in requested_changes_, the DR must have
          // sent a non-final AckNack.  If the base value is the high end of
          // the heartbeat range, treat it as a request for that seq#.
          if (reader->durable_data_.count(ack) || proxy.contains(ack) || proxy.pre_contains(ack)) {
            reader->requests_.insert(ack);
          }
        } else {
          reader->requests_.insert(ack, acknack.readerSNState.numBits, acknack.readerSNState.bitmap.get_buffer());
        }

        if (!reader->requests_.empty()) {
          readers_expecting_data_.insert(reader);
          schedule_nack_response = true;
        } else if (reader->requested_frags_.empty()) {
          readers_expecting_data_.erase(reader);
        }
      }
    }

    if (!is_final) {
      readers_expecting_heartbeat_.insert(reader);
      schedule_nack_response = true;
    }
  }

  if (preassociation_readers_.count(reader) == 0) {
    make_lagger_leader(reader, previous_acked_sn);
    check_leader_lagger();
  }

  TqeSet to_deliver;
  acked_by_all_helper_i(to_deliver);

#ifdef OPENDDS_SECURITY
  if (is_pvs_writer_ &&
      !reader->pvs_outstanding_.empty() &&
      reader->pvs_outstanding_.low() < reader->cur_cumulative_ack_) {
    const OPENDDS_VECTOR(SequenceRange) psr = reader->pvs_outstanding_.present_sequence_ranges();
    for (OPENDDS_VECTOR(SequenceRange)::const_iterator pos = psr.begin(), limit = psr.end();
         pos != limit && pos->first < reader->cur_cumulative_ack_; ++pos) {
      ACE_GUARD(ACE_Thread_Mutex, g, elems_not_acked_mutex_);
      for (SequenceNumber seq = pos->first; seq <= pos->second && seq < reader->cur_cumulative_ack_; ++seq) {
        reader->pvs_outstanding_.erase(seq);
        OPENDDS_MULTIMAP(SequenceNumber, TransportQueueElement*)::iterator iter = elems_not_acked_.find(seq);
        if (iter != elems_not_acked_.end()) {
          send_buff_->release_acked(iter->first);
          to_deliver.insert(iter->second);
          elems_not_acked_.erase(iter);
        }
      }
    }
  }
#endif

  if (!dont_schedule_nack_response && schedule_nack_response) {
    RtpsUdpTransport_rch tport = link->transport();
    nack_response_->schedule(tport ? tport->core().nak_response_delay() : TimeDuration(0, RtpsUdpInst::DEFAULT_NAK_RESPONSE_DELAY_USEC));
  }

  TransportClient_rch client = client_.lock();

  g.release();

  if (inform_send_listener && client) {
    client->data_acked(src);
  }

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
}

void
RtpsUdpDataLink::received(const RTPS::NackFragSubmessage& nackfrag,
                          const GuidPrefix_t& src_prefix,
                          const NetworkAddress& remote_addr)
{
  update_last_recv_addr(make_id(src_prefix, nackfrag.readerId), remote_addr);
  datawriter_dispatch(nackfrag, src_prefix, &RtpsWriter::process_nackfrag);
}

void RtpsUdpDataLink::RtpsWriter::process_nackfrag(const RTPS::NackFragSubmessage& nackfrag,
                                                   const GUID_t& src,
                                                   MetaSubmessageVec& /*meta_submessages*/)
{
  ACE_GUARD(ACE_Thread_Mutex, g, mutex_);

  if (stopping_) {
    return;
  }

  RtpsUdpDataLink_rch link = link_.lock();

  if (!link) {
    return;
  }

  if (Transport_debug_level > 5) {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) RtpsUdpDataLink::RtpsWriter::process_nackfrag: %C -> %C\n",
      LogGuid(src).c_str(), LogGuid(id_).c_str()));
  }

  const ReaderInfoMap::iterator ri = remote_readers_.find(src);
  if (ri == remote_readers_.end()) {
    if (Transport_debug_level > 5 || transport_debug.log_dropped_messages) {
      ACE_DEBUG((LM_DEBUG, "(%P|%t) {transport_debug.log_dropped_messages} RtpsUdpDataLink::RtpsWriter::process_nackfrag: %C -> %C unknown remote reader\n",
        LogGuid(src).c_str(), LogGuid(id_).c_str()));
    }
    return;
  }

  const ReaderInfo_rch& reader = ri->second;

  if (!compare_and_update_counts(nackfrag.count.value, reader->nackfrag_recvd_count_)) {
    if (Transport_debug_level > 5 || transport_debug.log_dropped_messages) {
      ACE_DEBUG((LM_DEBUG, "(%P|%t) {transport_debug.log_dropped_messages} RtpsUdpDataLink::RtpsWriter::process_nackfrag: %C -> %C stale/duplicate message\n",
        LogGuid(src).c_str(), LogGuid(id_).c_str()));
    }
    return;
  }

  const SequenceNumber seq = to_opendds_seqnum(nackfrag.writerSN);

  // All NackFrag messages are technically 'non-final' since they are only used to negatively acknowledge fragments and expect a response
  if (transport_debug.log_nonfinal_messages) {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) {transport_debug.log_nonfinal_messages} RtpsUdpDataLink::RtpsWriter::process_nackfrag: %C -> %C seq %q base %u bits %u\n",
      LogGuid(src).c_str(), LogGuid(id_).c_str(), seq.getValue(), nackfrag.fragmentNumberState.bitmapBase.value, nackfrag.fragmentNumberState.numBits));
  }

  reader->requested_frags_[seq][nackfrag.fragmentNumberState.bitmapBase.value] = nackfrag.fragmentNumberState;
  readers_expecting_data_.insert(reader);
  RtpsUdpTransport_rch tport = link->transport();
  nack_response_->schedule(tport ? tport->core().nak_response_delay() : TimeDuration(0, RtpsUdpInst::DEFAULT_NAK_RESPONSE_DELAY_USEC));
}

void
RtpsUdpDataLink::RtpsWriter::gather_nack_replies_i(MetaSubmessageVec& meta_submessages)
{
  RtpsUdpDataLink_rch link = link_.lock();

  if (!link) {
    return;
  }

  // Process naks requests from each reader replying with either data or a gap.
  // Requests for directed data are answered with a directed reply.
  // Requests for undirected data are answered with an undirected and consolidated reply.
  // Directed data includes things like the PVS writer.
  // The requests_ for a reader should not contain requests for durable data.

  typedef OPENDDS_MAP(SequenceNumber, DisjointSequence) FragmentInfo;

  // Consolidated non-directed requests and address sets to be sent together at the end, after directed replies
  typedef OPENDDS_MAP(SequenceNumber, NetworkAddressSet) RecipientMap;
  typedef OPENDDS_MAP(SequenceNumber, RepoIdSet) ReaderMap;
  DisjointSequence consolidated_requests;
  ReaderMap consolidated_request_readers;
  RecipientMap consolidated_recipients_unicast;
  RecipientMap consolidated_recipients_multicast;
  FragmentInfo consolidated_fragment_requests;
  ReaderMap consolidated_fragment_request_readers;
  RecipientMap consolidated_fragment_recipients_unicast;
  RecipientMap consolidated_fragment_recipients_multicast;
  DisjointSequence consolidated_gaps;

  ACE_GUARD(TransportSendBuffer::LockType, guard, send_buff_->strategy_lock());
  SingleSendBuffer::Proxy proxy(*send_buff_);

  size_t cumulative_send_count = 0;

  for (ReaderInfoSet::const_iterator pos = readers_expecting_data_.begin(), limit = readers_expecting_data_.end();
       pos != limit; ++pos) {

    const ReaderInfo_rch& reader = *pos;
    const NetworkAddressSet addrs = link->get_addresses(id_, reader->id_);

    DisjointSequence gaps;

    if (reader->expecting_durable_data()) {

      if (!reader->requests_.empty() && !reader->durable_data_.empty()) {
        const SequenceNumber dd_first = reader->durable_data_.begin()->first;
        const SequenceNumber dd_last = reader->durable_data_.rbegin()->first;

        if (reader->requests_.high() < dd_first) {
          gaps.insert(SequenceRange(reader->requests_.low(), dd_first.previous()));
          reader->requests_.reset();
        } else {
          const OPENDDS_VECTOR(SequenceRange) psr = reader->requests_.present_sequence_ranges();
          for (OPENDDS_VECTOR(SequenceRange)::const_iterator iter = psr.begin(), limit = psr.end();
               iter != limit && iter->first <= dd_last; ++iter) {
            for (SequenceNumber s = iter->first; s <= iter->second; ++s) {
              if (s <= dd_last) {
                const OPENDDS_MAP(SequenceNumber, TransportQueueElement*)::iterator dd_iter = reader->durable_data_.find(s);
                if (dd_iter != reader->durable_data_.end()) {
                  link->durability_resend(dd_iter->second, cumulative_send_count);
                } else {
                  gaps.insert(s);
                }
                reader->requests_.erase(s);
              }
            }
          }
        }
      }

      typedef RequestedFragSeqMap::const_iterator rfs_iter;
      for (rfs_iter rfs = reader->requested_frags_.begin(), limit = reader->requested_frags_.end(); rfs != limit; ++rfs) {
        const OPENDDS_MAP(SequenceNumber, TransportQueueElement*)::iterator dd_iter = reader->durable_data_.find(rfs->first);
        if (dd_iter != reader->durable_data_.end()) {
          for (RequestedFragMap::const_iterator rf = rfs->second.begin(); rf != rfs->second.end(); ++rf) {
            link->durability_resend(dd_iter->second, rf->second, cumulative_send_count);
          }
        } else if ((!reader->durable_data_.empty() && rfs->first < reader->durable_data_.begin()->first)) {
          gaps.insert(rfs->first);
        }
      }

      gather_gaps_i(reader, gaps, meta_submessages);

      // The writer may not be done replaying durable data.
      // Do not send a gap.
      // TODO: If we have all of the durable data, adjust the request so that we can answer the non-durable part.
      reader->requests_.reset();
      reader->requested_frags_.clear();
      continue;
    }

    const SequenceNumber first_sn = std::max(non_durable_first_sn(proxy), reader->start_sn_);
    if (!reader->requests_.empty() &&
        reader->requests_.high() < first_sn) {
      // The reader is not going to get any data.
      // Send a gap that is going to to catch them up.
      gaps.insert(SequenceRange(reader->requests_.low(), first_sn.previous()));
      reader->requests_.reset();
    }

    const OPENDDS_VECTOR(SequenceRange) ranges = reader->requests_.present_sequence_ranges();
    for (OPENDDS_VECTOR(SequenceRange)::const_iterator iter = ranges.begin(), limit = ranges.end();
         iter != limit; ++iter) {
      for (SequenceNumber seq = iter->first; seq <= iter->second; ++seq) {
        GUID_t destination;
        if (proxy.contains(seq, destination)) {
          if (destination == GUID_UNKNOWN) {
            // Not directed.
            consolidated_requests.insert(seq);
            consolidated_request_readers[seq].insert(reader->id_);
            consolidated_recipients_unicast[seq].insert(addrs.begin(), addrs.end());
            ACE_Guard<ACE_Thread_Mutex> g(link->locators_lock_);
            link->accumulate_addresses(id_, reader->id_, consolidated_recipients_multicast[seq], false);
            continue;
          } else if (destination != reader->id_) {
            // Directed at another reader.
            gaps.insert(seq);
            continue;
          } else {
            // Directed at the reader.
            const RtpsUdpSendStrategy::OverrideToken ot =
              link->send_strategy()->override_destinations(addrs);
            proxy.resend_i(SequenceRange(seq, seq), 0, reader->id_);
            ++cumulative_send_count;
            continue;
          }
        } else if (proxy.pre_contains(seq) || seq > max_sn_) {
          // Can't answer, don't gap.
          continue;
        }

        if (durable_ || is_pvs_writer()) {
          // Must send directed gap.
          gaps.insert(seq);
        } else {
          // Non-directed gap.
          consolidated_gaps.insert(seq);
        }
      }
    }

    reader->requests_.reset();

    typedef RequestedFragSeqMap::iterator rfs_iter;
    const rfs_iter rfs_end = reader->requested_frags_.end();
    for (rfs_iter rfs = reader->requested_frags_.begin(); rfs != rfs_end; ++rfs) {
      const SequenceNumber& seq = rfs->first;
      GUID_t destination;
      if (proxy.contains(seq, destination)) {
        if (destination == GUID_UNKNOWN) {
          for (RequestedFragMap::iterator rf = rfs->second.begin(); rf != rfs->second.end(); ++rf) {
            consolidated_fragment_requests[seq].insert(rf->second.bitmapBase.value, rf->second.numBits,
                                                       rf->second.bitmap.get_buffer());
          }
          consolidated_fragment_request_readers[seq].insert(reader->id_);
          consolidated_fragment_recipients_unicast[seq].insert(addrs.begin(), addrs.end());
          ACE_Guard<ACE_Thread_Mutex> g(link->locators_lock_);
          link->accumulate_addresses(id_, reader->id_, consolidated_fragment_recipients_multicast[seq], false);
          continue;
        } else if (destination != reader->id_) {
          // Directed at another reader.
          gaps.insert(seq);
          continue;
        } else {
          // Directed at the reader.
          const RtpsUdpSendStrategy::OverrideToken ot =
            link->send_strategy()->override_destinations(addrs);
          for (RequestedFragMap::iterator rf = rfs->second.begin(); rf != rfs->second.end(); ++rf) {
            DisjointSequence x;
            x.insert(rf->second.bitmapBase.value, rf->second.numBits,
                     rf->second.bitmap.get_buffer());
            proxy.resend_fragments_i(seq, x, cumulative_send_count);
          }
          continue;
        }
      } else if (proxy.pre_contains(seq) || seq > max_sn_) {
        // Can't answer, don't gap.
        continue;
      }

      if (durable_ || is_pvs_writer()) {
        // Must send directed gap.
        gaps.insert(seq);
      } else {
        // Non-directed gap.
        consolidated_gaps.insert(seq);
      }
    }

    reader->requested_frags_.clear();

    // Gather directed gaps.
    gather_gaps_i(reader, gaps, meta_submessages);
  }

  {
    // Send the consolidated requests.
    const OPENDDS_VECTOR(SequenceRange) ranges = consolidated_requests.present_sequence_ranges();
    for (OPENDDS_VECTOR(SequenceRange)::const_iterator pos = ranges.begin(), limit = ranges.end();
         pos != limit; ++pos) {
      if (Transport_debug_level > 5) {
        ACE_DEBUG((LM_DEBUG, "(%P|%t) RtpsUdpDataLink::RtpsWriter::gather_nack_replies_i: "
                   "resend data %q-%q\n", pos->first.getValue(),
                   pos->second.getValue()));
      }
      for (SequenceNumber seq = pos->first; seq <= pos->second; ++seq) {
        const NetworkAddressSet& uni = consolidated_recipients_unicast[seq];
        const NetworkAddressSet& multi = consolidated_recipients_multicast[seq];
        const RepoIdSet& readers = consolidated_request_readers[seq];

        if (proxy.has_frags(seq)) {
          if (consolidated_fragment_requests.find(seq) == consolidated_fragment_requests.end()) {
            consolidated_fragment_requests[seq].insert(1);
          }
          consolidated_fragment_recipients_unicast[seq].insert(uni.begin(), uni.end());
          consolidated_fragment_recipients_multicast[seq].insert(multi.begin(), multi.end());
          consolidated_fragment_request_readers[seq].insert(readers.begin(), readers.end());
        } else {
          const RtpsUdpSendStrategy::OverrideToken ot =
            link->send_strategy()->override_destinations(readers.size() * 2 > remote_readers_.size() ? multi : uni);

          proxy.resend_i(SequenceRange(seq, seq));
          ++cumulative_send_count;
        }
      }
    }

    for (FragmentInfo::const_iterator pos = consolidated_fragment_requests.begin(),
           limit = consolidated_fragment_requests.end(); pos != limit; ++pos) {
      const NetworkAddressSet& uni = consolidated_fragment_recipients_unicast[pos->first];
      const NetworkAddressSet& multi = consolidated_fragment_recipients_multicast[pos->first];
      const RepoIdSet& readers = consolidated_fragment_request_readers[pos->first];
      const RtpsUdpSendStrategy::OverrideToken ot =
        link->send_strategy()->override_destinations(readers.size() * 2 > remote_readers_.size() ? multi : uni);

      proxy.resend_fragments_i(pos->first, pos->second, cumulative_send_count);
    }
  }

  if (cumulative_send_count) {
    link->transport()->core().writer_resend_count(id_, static_cast<ACE_CDR::ULong>(cumulative_send_count));
  }

  // Gather the consolidated gaps.
  if (!consolidated_gaps.empty()) {
    if (Transport_debug_level > 5) {
      ACE_DEBUG((LM_DEBUG,
        "(%P|%t) RtpsUdpDataLink::RtpsWriter::gather_nack_replies_i: GAPs:\n"));
      consolidated_gaps.dump();
    }
    gather_gaps_i(ReaderInfo_rch(), consolidated_gaps, meta_submessages);
  } else if (Transport_debug_level > 5) {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) RtpsUdpDataLink::RtpsWriter::gather_nack_replies_i: "
      "no GAPs to send\n"));
  }

  MetaSubmessage meta_submessage(id_, GUID_UNKNOWN);
  initialize_heartbeat(proxy, meta_submessage);

  // Directed, non-final.
  for (ReaderInfoSet::const_iterator pos = readers_expecting_data_.begin(), limit = readers_expecting_data_.end();
       pos != limit; ++pos) {
    const ReaderInfo_rch& reader = *pos;
    readers_expecting_heartbeat_.erase(reader);
    if (reader->reflects_heartbeat_count()) {
      meta_submessage.sm_.heartbeat_sm().smHeader.flags |= RTPS::OPENDDS_FLAG_R;
      gather_directed_heartbeat_i(proxy, meta_submessages, meta_submessage, reader);
      reader->required_acknack_count_ = heartbeat_count_;
      meta_submessage.sm_.heartbeat_sm().smHeader.flags &= ~RTPS::OPENDDS_FLAG_R;
    } else {
      gather_directed_heartbeat_i(proxy, meta_submessages, meta_submessage, reader);
    }
  }
  readers_expecting_data_.clear();

  // Directed, final.
  meta_submessage.sm_.heartbeat_sm().smHeader.flags |= RTPS::FLAG_F;
  meta_submessages.reserve(meta_submessages.size() + readers_expecting_heartbeat_.size());
  for (ReaderInfoSet::const_iterator pos = readers_expecting_heartbeat_.begin(), limit = readers_expecting_heartbeat_.end();
       pos != limit; ++pos) {
    const ReaderInfo_rch& reader = *pos;
    gather_directed_heartbeat_i(proxy, meta_submessages, meta_submessage, reader);
  }
  readers_expecting_heartbeat_.clear();
}

SequenceNumber
RtpsUdpDataLink::RtpsWriter::expected_max_sn(const ReaderInfo_rch& reader) const
{
  ACE_UNUSED_ARG(reader);
#ifdef OPENDDS_SECURITY
  if (is_pvs_writer_) {
    return reader->max_pvs_sn_;
  } else {
#endif
    return max_sn_;
#ifdef OPENDDS_SECURITY
  }
#endif
}

void
RtpsUdpDataLink::RtpsWriter::snris_insert(RtpsUdpDataLink::SNRIS& snris,
                                          const ReaderInfo_rch& reader)
{
  const SequenceNumber sn = reader->acked_sn();
  SNRIS::iterator pos = snris.find(sn);
  if (pos == snris.end()) {
    pos = snris.insert(RtpsUdpDataLink::SNRIS::value_type(sn, make_rch<ReaderInfoSetHolder>())).first;
  }
  pos->second->readers.insert(reader);
}

void
RtpsUdpDataLink::RtpsWriter::snris_erase(RtpsUdpDataLink::SNRIS& snris,
                                         const SequenceNumber sn,
                                         const ReaderInfo_rch& reader)
{
  SNRIS::iterator pos = snris.find(sn);
  if (pos != snris.end()) {
    pos->second->readers.erase(reader);
    if (pos->second->readers.empty()) {
      snris.erase(pos);
    }
  }
}

void
RtpsUdpDataLink::RtpsWriter::make_leader_lagger(const GUID_t& reader_id,
                                                SequenceNumber previous_max_sn)
{
  ACE_UNUSED_ARG(reader_id);

  if (stopping_) {
    return;
  }

  RtpsUdpDataLink_rch link = link_.lock();
  if (!link) {
    return;
  }

#ifdef OPENDDS_SECURITY
    if (!is_pvs_writer_) {
#endif
      if (previous_max_sn != max_sn_) {
        // All readers that have acked previous_max_sn are now lagging.
        // Move leader_readers_[previous_max_sn] to lagging_readers_[previous_max_sn].
        SNRIS::iterator leading_pos = leading_readers_.find(previous_max_sn);
        SNRIS::iterator lagging_pos = lagging_readers_.find(previous_max_sn);
        if (leading_pos != leading_readers_.end()) {
          if (lagging_pos != lagging_readers_.end()) {
            lagging_pos->second->readers.insert(leading_pos->second->readers.begin(), leading_pos->second->readers.end());
          } else {
            lagging_readers_[previous_max_sn] = leading_pos->second;
          }
          leading_readers_.erase(leading_pos);
          heartbeat_->schedule(fallback_.get());
        }
      }
#ifdef OPENDDS_SECURITY
    } else {
      // Move a specific reader.
      const ReaderInfoMap::iterator iter = remote_readers_.find(reader_id);
      if (iter == remote_readers_.end()) {
        return;
      }

      const ReaderInfo_rch& reader = iter->second;
      previous_max_sn = reader->max_pvs_sn_;
      reader->max_pvs_sn_ = max_sn_;
      if (preassociation_readers_.count(reader)) {
        // Will be inserted once association is complete.
        return;
      }

      const SequenceNumber acked_sn = reader->acked_sn();
      if (acked_sn == previous_max_sn && previous_max_sn != max_sn_) {
        snris_erase(leading_readers_, acked_sn, reader);
        snris_insert(lagging_readers_, reader);
        heartbeat_->schedule(fallback_.get());
      }
    }
#endif
}

void
RtpsUdpDataLink::RtpsWriter::make_lagger_leader(const ReaderInfo_rch& reader,
                                                const SequenceNumber previous_acked_sn)
{
  RtpsUdpDataLink_rch link = link_.lock();
  if (!link) {
    return;
  }

  const SequenceNumber acked_sn = reader->acked_sn();
  if (previous_acked_sn == acked_sn) { return; }
  const SequenceNumber previous_max_sn = expected_max_sn(reader);
#ifdef OPENDDS_SECURITY
  if (is_pvs_writer_ && acked_sn > previous_max_sn) {
    reader->max_pvs_sn_ = acked_sn;
  }
#endif
  const SequenceNumber max_sn = expected_max_sn(reader);

  snris_erase(previous_acked_sn == previous_max_sn ? leading_readers_ : lagging_readers_, previous_acked_sn, reader);
  snris_insert(acked_sn == max_sn ? leading_readers_ : lagging_readers_, reader);
  if (acked_sn != max_sn) {
    heartbeat_->schedule(fallback_.get());
  }
}

bool
RtpsUdpDataLink::RtpsWriter::is_lagging(const ReaderInfo_rch& reader) const
{
  return reader->acked_sn() != expected_max_sn(reader);
}

bool
RtpsUdpDataLink::RtpsWriter::is_leading(const ReaderInfo_rch& reader) const
{
  return reader->acked_sn() == expected_max_sn(reader);
}

void
RtpsUdpDataLink::RtpsWriter::check_leader_lagger() const
{
#ifndef OPENDDS_SAFETY_PROFILE
#ifndef NDEBUG
  static const SequenceNumber negative_one = SequenceNumber::ZERO().previous();
  for (SNRIS::const_iterator pos1 = lagging_readers_.begin(), limit = lagging_readers_.end();
       pos1 != limit; ++pos1) {
    const SequenceNumber& sn = pos1->first;
    const ReaderInfoSetHolder_rch& readers = pos1->second;
    for (ReaderInfoSet::const_iterator pos2 = readers->readers.begin(), limit = readers->readers.end();
         pos2 != limit; ++pos2) {
      const ReaderInfo_rch& reader = *pos2;
      OPENDDS_ASSERT(reader->acked_sn() == sn);
      const SequenceNumber expect_max_sn = expected_max_sn(reader);
      OPENDDS_ASSERT(sn == negative_one || sn < expect_max_sn);
      OPENDDS_ASSERT(preassociation_readers_.count(reader) == 0);
    }
  }

  for (SNRIS::const_iterator pos1 = leading_readers_.begin(), limit = leading_readers_.end();
       pos1 != limit; ++pos1) {
    const SequenceNumber& sn = pos1->first;
    const ReaderInfoSetHolder_rch& readers = pos1->second;
    for (ReaderInfoSet::const_iterator pos2 = readers->readers.begin(), limit = readers->readers.end();
         pos2 != limit; ++pos2) {
      const ReaderInfo_rch& reader = *pos2;
      OPENDDS_ASSERT(reader->acked_sn() == sn);
      const SequenceNumber expect_max_sn = expected_max_sn(reader);
      OPENDDS_ASSERT(sn == expect_max_sn);
      OPENDDS_ASSERT(preassociation_readers_.count(reader) == 0);
    }
  }
#endif
#endif
}

void
RtpsUdpDataLink::RtpsWriter::record_directed(const GUID_t& reader_id, SequenceNumber seq)
{
  ACE_UNUSED_ARG(reader_id);
  ACE_UNUSED_ARG(seq);
#ifdef OPENDDS_SECURITY
  if (!is_pvs_writer_) {
    return;
  }

  const ReaderInfoMap::iterator iter = remote_readers_.find(reader_id);
  if (iter == remote_readers_.end()) {
    return;
  }

  const ReaderInfo_rch& reader = iter->second;
  reader->pvs_outstanding_.insert(seq);
#endif
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
  OPENDDS_VECTOR(GUID_t) to_check;

  RtpsUdpDataLink_rch link = link_.lock();

  if (!link) {
    return;
  }

  //start with the max sequence number writer knows about and decrease
  //by what the min over all readers is
  SequenceNumber all_readers_ack = SequenceNumber::MAX_VALUE;
  if (!preassociation_readers_.empty()) {
    all_readers_ack = std::min(all_readers_ack, *preassociation_reader_start_sns_.begin());
  }
  if (!lagging_readers_.empty()) {
    all_readers_ack = std::min(all_readers_ack, lagging_readers_.begin()->first + 1);
  }
  if (!leading_readers_.empty()) {
    // When is_pvs_writer_ is true, the leading_readers_ will all be
    // at different sequence numbers.  The minimum could actually be
    // before the preassociation readers or lagging readers.  Use the
    // largest sequence number to avoid holding onto samples.
    all_readers_ack = std::min(all_readers_ack, leading_readers_.rbegin()->first + 1);
  }

  if (all_readers_ack == SequenceNumber::MAX_VALUE) {
    return;
  }

  ACE_GUARD(ACE_Thread_Mutex, g2, elems_not_acked_mutex_);

  if (!elems_not_acked_.empty()) {
    for (iter_t it = elems_not_acked_.begin(), limit = elems_not_acked_.end();
         it != limit && it->first < all_readers_ack;) {
      send_buff_->release_acked(it->first);
      to_deliver.insert(it->second);
      elems_not_acked_.erase(it++);
    }
  }
}

void RtpsUdpDataLink::durability_resend(TransportQueueElement* element,
                                        size_t& cumulative_send_count)
{
  static CORBA::Long buffer[8];
  static const RTPS::FragmentNumberSet none = { {0}, 0, RTPS::LongSeq8(0, buffer) };
  durability_resend(element, none, cumulative_send_count);
}

void RtpsUdpDataLink::durability_resend(TransportQueueElement* element,
                                        const RTPS::FragmentNumberSet& fragmentSet,
                                        size_t& cumulative_send_count)
{
  if (Transport_debug_level > 5) {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) TRACK RtpsUdpDataLink::durability_resend %q\n", element->sequence().getValue()));
  }
  const NetworkAddressSet addrs = get_addresses(element->publication_id(), element->subscription_id());
  if (addrs.empty()) {
    const LogGuid conv(element->subscription_id());
    ACE_ERROR((LM_ERROR,
               "(%P|%t) ERROR: RtpsUdpDataLink::durability_resend() - "
               "no locator for remote %C\n", conv.c_str()));
    return;
  }

  TqeVector to_send;
  if (!send_strategy()->fragmentation_helper(element, to_send)) {
    return;
  }

  DisjointSequence fragments;
  fragments.insert(fragmentSet.bitmapBase.value, fragmentSet.numBits,
                   fragmentSet.bitmap.get_buffer());
  SequenceNumber lastFragment = 0;

  const TqeVector::iterator end = to_send.end();
  for (TqeVector::iterator i = to_send.begin(); i != end; ++i) {
    if (fragments.empty() || include_fragment(**i, fragments, lastFragment)) {
      RTPS::Message message;
      send_strategy()->send_rtps_control(message, *const_cast<ACE_Message_Block*>((*i)->msg()), addrs);
      ++cumulative_send_count;
    }

    (*i)->data_delivered();
  }
}

bool RtpsUdpDataLink::include_fragment(const TransportQueueElement& element,
                                       const DisjointSequence& fragments,
                                       SequenceNumber& lastFragment)
{
  if (!element.is_fragment()) {
    return true;
  }

  const RtpsCustomizedElement* const rce = dynamic_cast<const RtpsCustomizedElement*>(&element);
  if (!rce) {
    return true;
  }

  const SequenceRange thisElement(lastFragment + 1, rce->last_fragment());
  lastFragment = thisElement.second;
  return fragments.contains_any(thisElement);
}

void
RtpsUdpDataLink::send_heartbeats(const MonotonicTimePoint& now)
{
  OPENDDS_VECTOR(CallbackType) readerDoesNotExistCallbacks;

  RtpsUdpTransport_rch tport = transport();

  MetaSubmessageVec meta_submessages;

  {
    ACE_GUARD(ACE_Thread_Mutex, g, writers_lock_);

    const MonotonicTimePoint tv = now - 10 * tport->core().heartbeat_period();
    const MonotonicTimePoint tv3 = now - 3 * tport->core().heartbeat_period();

    typedef OPENDDS_MAP_CMP(GUID_t, RcHandle<ConstSharedRepoIdSet>, GUID_tKeyLessThan) WtaMap;
    WtaMap writers_to_advertise;

    for (InterestingRemoteMapType::iterator pos = interesting_readers_.begin(),
           limit = interesting_readers_.end();
         pos != limit;
         ++pos) {
      if (pos->second.status == InterestingRemote::DOES_NOT_EXIST ||
          (pos->second.status == InterestingRemote::EXISTS && pos->second.last_activity < tv3)) {
        RcHandle<ConstSharedRepoIdSet>& tg = writers_to_advertise[pos->second.localid];
        if (!tg) {
          tg = make_rch<ConstSharedRepoIdSet>();
        }
        const_cast<RepoIdSet&>(tg->guids_).insert(pos->first);
      }
      if (pos->second.status == InterestingRemote::EXISTS && pos->second.last_activity < tv) {
        CallbackType callback(pos->first, pos->second);
        readerDoesNotExistCallbacks.push_back(callback);
        pos->second.status = InterestingRemote::DOES_NOT_EXIST;
      }
    }

    for (WtaMap::const_iterator pos = writers_to_advertise.begin(),
           limit = writers_to_advertise.end();
         pos != limit;
         ++pos) {
      RtpsWriterMap::const_iterator wpos = writers_.find(pos->first);
      if (wpos != writers_.end()) {
        wpos->second->gather_heartbeats(pos->second, meta_submessages);
      } else {
        using namespace OpenDDS::RTPS;
        const int count = ++heartbeat_counts_[pos->first.entityId];
        const HeartBeatSubmessage hb = {
          {HEARTBEAT, FLAG_E, HEARTBEAT_SZ},
          ENTITYID_UNKNOWN, // any matched reader may be interested in this
          pos->first.entityId,
          to_rtps_seqnum(SequenceNumber(1)),
          to_rtps_seqnum(SequenceNumber::ZERO()),
          {count}
        };

        for (RepoIdSet::const_iterator it = pos->second->guids_.begin(),
          limit = pos->second->guids_.end(); it != limit; ++it) {

          MetaSubmessage meta_submessage(pos->first, *it);
          meta_submessage.sm_.heartbeat_sm(hb);
          meta_submessages.push_back(meta_submessage);
        }
      }
    }
  }

  queue_submessages(meta_submessages);

  for (OPENDDS_VECTOR(CallbackType)::iterator iter = readerDoesNotExistCallbacks.begin();
       iter != readerDoesNotExistCallbacks.end(); ++iter) {
    const InterestingRemote& remote = iter->second;
    remote.listener->reader_does_not_exist(iter->first, remote.localid);
  }
}

void
RtpsUdpDataLink::RtpsWriter::initialize_heartbeat(const SingleSendBuffer::Proxy& proxy,
                                                  MetaSubmessage& meta_submessage)
{
  using namespace OpenDDS::RTPS;

  // Assume no samples are available.
  const SequenceNumber nonDurableFirstSN = non_durable_first_sn(proxy);
  const SequenceNumber firstSN = durable_ ? 1 : nonDurableFirstSN;

  const HeartBeatSubmessage hb = {
    {HEARTBEAT,
     FLAG_E,
     HEARTBEAT_SZ},
    ENTITYID_UNKNOWN, // any matched reader may be interested in this
    id_.entityId,
    to_rtps_seqnum(firstSN),
    to_rtps_seqnum(max_sn_),
    {0}
  };

  meta_submessage.sm_.heartbeat_sm(hb);
}

void
RtpsUdpDataLink::RtpsWriter::gather_directed_heartbeat_i(const SingleSendBuffer::Proxy& proxy,
                                                         MetaSubmessageVec& meta_submessages,
                                                         MetaSubmessage& meta_submessage,
                                                         const ReaderInfo_rch& reader)
{
  const SequenceNumber first_sn = reader->durable_ ? 1 : std::max(non_durable_first_sn(proxy), reader->start_sn_);
  SequenceNumber last_sn = expected_max_sn(reader);
#ifdef OPENDDS_SECURITY
  if (is_pvs_writer_ && last_sn < first_sn.previous()) {
    // This can happen if the reader get's reset.
    // Adjust the heartbeat to be valid.
    // Make lagger_leader will eventually correct the problem.
    last_sn = first_sn.previous();
  }
#endif
  meta_submessage.dst_guid_ = reader->id_;
  meta_submessage.sm_.heartbeat_sm().count.value = ++heartbeat_count_;
  meta_submessage.sm_.heartbeat_sm().readerId = reader->id_.entityId;
  meta_submessage.sm_.heartbeat_sm().firstSN = to_rtps_seqnum(first_sn);
  meta_submessage.sm_.heartbeat_sm().lastSN = to_rtps_seqnum(last_sn);
  OPENDDS_ASSERT(!(first_sn < 1 || last_sn < 0 || last_sn < first_sn.previous()));
  meta_submessages.push_back(meta_submessage);
  meta_submessage.reset_destination();
}
void
RtpsUdpDataLink::RtpsWriter::update_remote_guids_cache_i(bool add, const GUID_t& guid)
{
  RtpsUdpDataLink_rch link = link_.lock();
  if (!link) {
    return;
  }

  {
    ACE_Guard<ACE_Thread_Mutex> rrg_guard(remote_reader_guids_mutex_);

    // We make a new RcHandle to prevent changing what's being pointed to by existing references in the send queue (i.e. to preserve historic values)
    RcHandle<ConstSharedRepoIdSet> temp = make_rch<ConstSharedRepoIdSet>();
    if (remote_reader_guids_) {
      const_cast<RepoIdSet&>(temp->guids_) = remote_reader_guids_->guids_;
    }
    if (add) {
      const_cast<RepoIdSet&>(temp->guids_).insert(guid);
    } else {
      const_cast<RepoIdSet&>(temp->guids_).erase(guid);
    }
    remote_reader_guids_ = temp;
  }

  link->bundling_cache_.remove_id(GUID_UNKNOWN);
}

void
RtpsUdpDataLink::RtpsWriter::gather_heartbeats_i(MetaSubmessageVec& meta_submessages)
{
  if (preassociation_readers_.empty() && lagging_readers_.empty()) {
    return;
  }

  check_leader_lagger();

  RtpsUdpDataLink_rch link = link_.lock();
  if (!link) {
    return;
  }

  using namespace OpenDDS::RTPS;

  const SingleSendBuffer::Proxy proxy(*send_buff_);

  // Assume no samples are available.
  const SequenceNumber nonDurableFirstSN = non_durable_first_sn(proxy);
  const SequenceNumber firstSN = durable_ ? 1 : nonDurableFirstSN;
  const SequenceNumber lastSN = max_sn_;

  MetaSubmessage meta_submessage(id_, GUID_UNKNOWN);
  initialize_heartbeat(proxy, meta_submessage);

  // Directed, non-final.
  if (!preassociation_readers_.empty()) {
    meta_submessages.reserve(meta_submessages.size() + preassociation_readers_.size());
    for (ReaderInfoSet::const_iterator pos = preassociation_readers_.begin(), limit = preassociation_readers_.end();
         pos != limit; ++pos) {
      const ReaderInfo_rch& reader = *pos;
      gather_directed_heartbeat_i(proxy, meta_submessages, meta_submessage, reader);
    }
  }

  if (!lagging_readers_.empty()) {
    if (leading_readers_.empty() && remote_readers_.size() > 1
#ifdef OPENDDS_SECURITY
        && !is_pvs_writer_
        && !is_ps_writer_
#endif
        ) {
      // Every reader is lagging and there is more than one.
      meta_submessage.sm_.heartbeat_sm().count.value = ++heartbeat_count_;
      meta_submessage.sm_.heartbeat_sm().readerId = ENTITYID_UNKNOWN;
      meta_submessage.sm_.heartbeat_sm().firstSN = to_rtps_seqnum(firstSN);
      meta_submessage.sm_.heartbeat_sm().lastSN = to_rtps_seqnum(lastSN);

      meta_submessages.push_back(meta_submessage);
      meta_submessage.reset_destination();
    } else {
      for (SNRIS::const_iterator snris_pos = lagging_readers_.begin(), snris_limit = lagging_readers_.end();
           snris_pos != snris_limit; ++snris_pos) {
        meta_submessages.reserve(meta_submessages.size() + snris_pos->second->readers.size());
        for (ReaderInfoSet::const_iterator pos = snris_pos->second->readers.begin(),
               limit = snris_pos->second->readers.end();
             pos != limit; ++pos) {
          const ReaderInfo_rch& reader = *pos;
          gather_directed_heartbeat_i(proxy, meta_submessages, meta_submessage, reader);
        }
      }
    }
  }
}

void
RtpsUdpDataLink::RtpsWriter::gather_heartbeats(RcHandle<ConstSharedRepoIdSet> additional_guids,
                                               MetaSubmessageVec& meta_submessages)
{
  OPENDDS_ASSERT(!additional_guids->guids_.empty());

  ACE_GUARD(ACE_Thread_Mutex, g, mutex_);

  RtpsUdpDataLink_rch link = link_.lock();
  if (!link) {
    return;
  }

  const SingleSendBuffer::Proxy proxy(*send_buff_);

  MetaSubmessage meta_submessage(id_, GUID_UNKNOWN);
  initialize_heartbeat(proxy, meta_submessage);

  for (RepoIdSet::const_iterator it = additional_guids->guids_.begin(),
    limit = additional_guids->guids_.end(); it != limit; ++it) {

    // Semi-directed (INFO_DST but ENTITYID_UNKNOWN, non-final.
    meta_submessage.dst_guid_ = *it;
    meta_submessage.sm_.heartbeat_sm().count.value = ++heartbeat_count_;
    meta_submessages.push_back(meta_submessage);
    meta_submessage.reset_destination();
  }
}

void
RtpsUdpDataLink::check_heartbeats(const MonotonicTimePoint& now)
{
  OPENDDS_VECTOR(CallbackType) writerDoesNotExistCallbacks;

  RtpsUdpTransport_rch tport = transport();

  // Have any interesting writers timed out?
  const MonotonicTimePoint tv(now - 10 * (tport ? tport->core().heartbeat_period() : TimeDuration(RtpsUdpInst::DEFAULT_HEARTBEAT_PERIOD_SEC)));
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
    const GUID_t& rid = iter->first;
    const InterestingRemote& remote = iter->second;
    remote.listener->writer_does_not_exist(rid, remote.localid);
  }
}

void
RtpsUdpDataLink::send_heartbeats_manual_i(const TransportSendControlElement* tsce, MetaSubmessageVec& meta_submessages)
{
  using namespace OpenDDS::RTPS;

  const GUID_t pub_id = tsce->publication_id();
  const HeartBeatSubmessage hb = {
    {HEARTBEAT,
     CORBA::Octet(FLAG_E | FLAG_F | FLAG_L),
     HEARTBEAT_SZ},
    ENTITYID_UNKNOWN, // any matched reader may be interested in this
    pub_id.entityId,
    // This liveliness heartbeat is from a best-effort Writer, the sequence numbers are not used
    to_rtps_seqnum(SequenceNumber(1)),
    to_rtps_seqnum(tsce->sequence()),
    {++best_effort_heartbeat_count_}
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

  const SingleSendBuffer::Proxy proxy(*send_buff_);
  const SequenceNumber firstSN = durable_ ? 1 : non_durable_first_sn(proxy);
  const SequenceNumber lastSN = max_sn_;
  const int counter = ++heartbeat_count_;

  const HeartBeatSubmessage hb = {
    {HEARTBEAT,
     CORBA::Octet(FLAG_E | FLAG_F | FLAG_L),
     HEARTBEAT_SZ},
    ENTITYID_UNKNOWN, // any matched reader may be interested in this
    id_.entityId,
    to_rtps_seqnum(firstSN),
    to_rtps_seqnum(lastSN),
    {counter}
  };

  MetaSubmessage meta_submessage(id_, GUID_UNKNOWN);
  meta_submessage.sm_.heartbeat_sm(hb);

  meta_submessages.push_back(meta_submessage);
}

RtpsUdpDataLink::ReaderInfo::~ReaderInfo()
{
  expunge_durable_data();
}

void
RtpsUdpDataLink::ReaderInfo::swap_durable_data(OPENDDS_MAP(SequenceNumber, TransportQueueElement*)& dd)
{
  durable_data_.swap(dd);
}

void
RtpsUdpDataLink::ReaderInfo::expunge_durable_data()
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

bool
RtpsUdpDataLink::ReaderInfo::reflects_heartbeat_count() const
{
  return participant_flags_ & RTPS::PFLAGS_REFLECT_HEARTBEAT_COUNT;
}

RtpsUdpDataLink::RtpsWriter::RtpsWriter(const TransportClient_rch& client, const RtpsUdpDataLink_rch& link,
                                        const GUID_t& id,
                                        bool durable, SequenceNumber max_sn, int heartbeat_count, size_t capacity)
 : send_buff_(make_rch<SingleSendBuffer>(capacity, ONE_SAMPLE_PER_PACKET))
 , max_sn_(max_sn == SequenceNumber::SEQUENCENUMBER_UNKNOWN() ? SequenceNumber::ZERO() : max_sn)
 , client_(client)
 , link_(link)
 , id_(id)
 , durable_(durable)
 , stopping_(false)
 , heartbeat_count_(heartbeat_count)
#ifdef OPENDDS_SECURITY
 , is_pvs_writer_(id_.entityId == RTPS::ENTITYID_P2P_BUILTIN_PARTICIPANT_VOLATILE_SECURE_WRITER)
 , is_ps_writer_(id_.entityId == RTPS::ENTITYID_SPDP_RELIABLE_BUILTIN_PARTICIPANT_SECURE_WRITER)
#endif
 , heartbeat_(make_rch<SporadicEvent>(link->event_dispatcher(), make_rch<PmfNowEvent<RtpsWriter> >(rchandle_from(this), &RtpsWriter::send_heartbeats)))
 , nack_response_(make_rch<SporadicEvent>(link->event_dispatcher(), make_rch<PmfNowEvent<RtpsWriter> >(rchandle_from(this), &RtpsWriter::send_nack_responses)))
 , initial_fallback_(link->config()->heartbeat_period())
 , fallback_(initial_fallback_)
{
  send_buff_->bind(link->send_strategy().in());
}

RtpsUdpDataLink::RtpsWriter::~RtpsWriter()
{
  if (!elems_not_acked_.empty()) {
    ACE_DEBUG((LM_WARNING, ACE_TEXT("(%P|%t) WARNING: RtpsWriter::~RtpsWriter - ")
      ACE_TEXT("deleting with %d elements left not fully acknowledged\n"),
      elems_not_acked_.size()));
  }
}

CORBA::Long RtpsUdpDataLink::RtpsWriter::inc_heartbeat_count()
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, mutex_, 0);
  return ++heartbeat_count_;
}

SequenceNumber
RtpsUdpDataLink::RtpsWriter::max_data_seq(const SingleSendBuffer::Proxy& proxy,
                                          const ReaderInfo_rch& ri) const
{
  const SequenceNumber durable_max = ri->durable_data_.empty() ? 0 : ri->durable_data_.rbegin()->first;
  const SequenceNumber pre_max = proxy.pre_empty() ? 0 : proxy.pre_high();
  const SequenceNumber data_max = proxy.empty() ? 0 : proxy.high();
  return std::max(durable_max, std::max(pre_max, data_max));
}

SequenceNumber
RtpsUdpDataLink::RtpsWriter::update_max_sn(const GUID_t& reader, SequenceNumber seq)
{
  ACE_Guard<ACE_Thread_Mutex> g(mutex_);
  SequenceNumber previous_max_sn = max_sn_;
  max_sn_ = std::max(max_sn_, seq);
  make_leader_lagger(reader, previous_max_sn);
  check_leader_lagger();
  return max_sn_;
}

void
RtpsUdpDataLink::RtpsWriter::add_elem_awaiting_ack(TransportQueueElement* element)
{
  ACE_GUARD(ACE_Thread_Mutex, g, elems_not_acked_mutex_);
  elems_not_acked_.insert(SnToTqeMap::value_type(element->sequence(), element));
}

bool
RtpsUdpDataLink::RtpsWriter::is_leading(const GUID_t& reader_id) const
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, mutex_, false);

  ReaderInfoMap::const_iterator iter = remote_readers_.find(reader_id);
  if (iter != remote_readers_.end()) {
    return is_leading(iter->second);
  }

  return false;
}

void
RtpsUdpDataLink::RtpsReader::deliver_held_data(const GUID_t& src)
{
  ACE_GUARD(ACE_Thread_Mutex, g, mutex_);

  if (stopping_) {
    return;
  }

  RtpsUdpDataLink_rch link = link_.lock();

  if (!link) {
    return;
  }

  OPENDDS_VECTOR(ReceivedDataSample) to_deliver;

  const WriterInfoMap::iterator wi = remote_writers_.find(src);
  if (wi != remote_writers_.end()) {
    const SequenceNumber ca = wi->second->recvd_.cumulative_ack();
    const WriterInfo::HeldMap::iterator end = wi->second->held_.upper_bound(ca);
    for (WriterInfo::HeldMap::iterator it = wi->second->held_.begin(); it != end; /*increment in loop body*/) {
      to_deliver.push_back(it->second);
      wi->second->held_.erase(it++);
    }
  }

  const GUID_t dst = id_;

  g.release();

  for (OPENDDS_VECTOR(ReceivedDataSample)::iterator it = to_deliver.begin(); it != to_deliver.end(); ++it) {
    if (Transport_debug_level > 5) {
      LogGuid reader(dst);
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) RtpsUdpDataLink::DeliverHeldData::~DeliverHeldData -")
                 ACE_TEXT(" deliver sequence: %q to %C\n"),
                 it->header_.sequence_.getValue(),
                 reader.c_str()));
    }
    link->data_received(*it, dst);
  }
}

RtpsUdpDataLink::DeliverHeldData::~DeliverHeldData()
{
  if (reader_) {
    reader_->deliver_held_data(writer_id_);
  }
}

RtpsUdpTransport_rch
RtpsUdpDataLink::transport() const
{
  return dynamic_rchandle_cast<RtpsUdpTransport>(impl());
}

RtpsUdpSendStrategy_rch
RtpsUdpDataLink::send_strategy()
{
  return dynamic_rchandle_cast<RtpsUdpSendStrategy>(send_strategy_);
}

RtpsUdpReceiveStrategy_rch
RtpsUdpDataLink::receive_strategy()
{
  return dynamic_rchandle_cast<RtpsUdpReceiveStrategy>(receive_strategy_);
}

NetworkAddressSet
RtpsUdpDataLink::get_addresses(const GUID_t& local, const GUID_t& remote) const
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, locators_lock_, NetworkAddressSet());
  return get_addresses_i(local, remote);
}

NetworkAddressSet
RtpsUdpDataLink::get_addresses(const GUID_t& local) const
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, locators_lock_, NetworkAddressSet());
  return get_addresses_i(local);
}

NetworkAddressSet
RtpsUdpDataLink::get_addresses_i(const GUID_t& local, const GUID_t& remote) const
{
  NetworkAddressSet retval;

  accumulate_addresses(local, remote, retval, true);

  return retval;
}

NetworkAddressSet
RtpsUdpDataLink::get_addresses_i(const GUID_t& local) const
{
  NetworkAddressSet retval;
  bool use_peers = true;

  // For reliable writers, use remote_reader_guids()
  const GuidConverter conv(local);
  if (conv.isWriter()) {
    RtpsWriter_rch writer;
    ACE_Guard<ACE_Thread_Mutex> guard(writers_lock_);
    RtpsWriterMap::const_iterator pos = writers_.find(local);
    if (pos != writers_.end()) {
      writer = pos->second;
    }
    guard.release();
    if (writer) {
      RcHandle<ConstSharedRepoIdSet> addr_guids = writer->get_remote_reader_guids();
      if (addr_guids) {
        for (RepoIdSet::const_iterator it = addr_guids->guids_.begin(),
          limit = addr_guids->guids_.end(); it != limit; ++it) {
          accumulate_addresses(local, *it, retval);

        }
        use_peers = false;
      }
    }
  }

  if (use_peers) {
    const GUIDSeq_var peers = peer_ids(local);
    if (peers.ptr()) {
      for (CORBA::ULong i = 0; i < peers->length(); ++i) {
        accumulate_addresses(local, peers[i], retval);
      }
    }
  }

  return retval;
}

bool RtpsUdpDataLink::RemoteInfo::insert_recv_addr(NetworkAddressSet& aset) const
{
  if (!last_recv_addr_) {
    return false;
  }
  const ACE_INT16 last_addr_type = last_recv_addr_.get_type();
  NetworkAddress limit;
  limit.set_type(last_addr_type);
  NetworkAddressSet::const_iterator it = unicast_addrs_.lower_bound(limit);
  while (it != unicast_addrs_.end() && it->get_type() == last_addr_type) {
    if (it->addr_bytes_equal(last_recv_addr_)) {
      aset.insert(*it);
      return true;
    }
    ++it;
  }
  return false;
}

void
RtpsUdpDataLink::accumulate_addresses(const GUID_t& local, const GUID_t& remote,
                                      NetworkAddressSet& addresses, bool prefer_unicast) const
{
  OPENDDS_ASSERT(local != GUID_UNKNOWN);
  OPENDDS_ASSERT(remote != GUID_UNKNOWN);

  const LocatorCacheKey key(remote, local, prefer_unicast);
  LocatorCache::ScopedAccess entry(locator_cache_, key);
  if (!entry.is_new_) {
    addresses.insert(entry.value().addrs_.begin(), entry.value().addrs_.end());
    return;
  }

  RtpsUdpTransport_rch tport = transport();
  if (!tport) {
    return;
  }

  const NetworkAddress rtps_relay_address = tport->core().rtps_relay_address();

  if (tport->core().rtps_relay_only() && std::memcmp(&local.guidPrefix, &remote.guidPrefix, sizeof(GuidPrefix_t)) != 0) {
    if (NetworkAddress::default_IPV4 != rtps_relay_address) {
      addresses.insert(rtps_relay_address);
      entry.value().addrs_.insert(rtps_relay_address);
    }
    return;
  }

  const TimeDuration receive_address_duration = tport->core().receive_address_duration();
  NetworkAddressSet normal_addrs;
  MonotonicTimePoint normal_addrs_expires = MonotonicTimePoint::max_value;
  NetworkAddress ice_addr;
  bool valid_last_recv_addr = false;
  static const NetworkAddress NO_ADDR;

  const RemoteInfoMap::const_iterator pos = locators_.find(remote);
  if (pos != locators_.end()) {
    if (prefer_unicast && pos->second.insert_recv_addr(normal_addrs)) {
      normal_addrs_expires = pos->second.last_recv_time_ + receive_address_duration;
      valid_last_recv_addr = (MonotonicTimePoint::now() - pos->second.last_recv_time_) <= receive_address_duration;
    } else if (prefer_unicast && !pos->second.unicast_addrs_.empty()) {
      normal_addrs = pos->second.unicast_addrs_;
    } else if (!pos->second.multicast_addrs_.empty()) {
#ifdef ACE_HAS_IPV6
      if (pos->second.last_recv_addr_ != NO_ADDR) {
        const NetworkAddressSet& mc_addrs = pos->second.multicast_addrs_;
        for (NetworkAddressSet::const_iterator it = mc_addrs.begin(); it != mc_addrs.end(); ++it) {
          if (it->get_type() == pos->second.last_recv_addr_.get_type()) {
            normal_addrs.insert(*it);
          }
        }
      } else {
        normal_addrs = pos->second.multicast_addrs_;
      }
#else
      normal_addrs = pos->second.multicast_addrs_;
#endif
    } else if (pos->second.insert_recv_addr(normal_addrs)) {
      normal_addrs_expires = pos->second.last_recv_time_ + receive_address_duration;
      valid_last_recv_addr = (MonotonicTimePoint::now() - pos->second.last_recv_time_) <= receive_address_duration;
    } else {
      normal_addrs = pos->second.unicast_addrs_;
    }
  } else {
    const GuidConverter conv(remote);
    if (conv.isReader()) {
      ACE_GUARD(ACE_Thread_Mutex, g, writers_lock_);
      InterestingRemoteMapType::const_iterator ipos = interesting_readers_.find(remote);
      if (ipos != interesting_readers_.end()) {
        normal_addrs = ipos->second.addresses;
      }
    } else if (conv.isWriter()) {
      ACE_GUARD(ACE_Thread_Mutex, g, readers_lock_);
      InterestingRemoteMapType::const_iterator ipos = interesting_writers_.find(remote);
      if (ipos != interesting_writers_.end()) {
        normal_addrs = ipos->second.addresses;
      }
    }
  }

#ifdef OPENDDS_SECURITY
  WeakRcHandle<ICE::Endpoint> endpoint = get_ice_endpoint();
  if (endpoint) {
    ice_addr = ice_agent_->get_address(endpoint, local, remote);
  }
#endif

  if (ice_addr == NO_ADDR) {
    addresses.insert(normal_addrs.begin(), normal_addrs.end());
    entry.value().addrs_.insert(normal_addrs.begin(), normal_addrs.end());
    entry.value().expires_ = normal_addrs_expires;
    if (!valid_last_recv_addr && rtps_relay_address != NetworkAddress::default_IPV4) {
      addresses.insert(rtps_relay_address);
      entry.value().addrs_.insert(rtps_relay_address);
    }
    return;
  }

  if (normal_addrs.count(ice_addr) == 0) {
    addresses.insert(ice_addr);
    entry.value().addrs_.insert(ice_addr);
    return;
  }

  addresses.insert(normal_addrs.begin(), normal_addrs.end());
  entry.value().addrs_.insert(normal_addrs.begin(), normal_addrs.end());
  entry.value().expires_ = normal_addrs_expires;
}

#ifdef OPENDDS_SECURITY
RcHandle<ICE::Agent>
RtpsUdpDataLink::get_ice_agent() const
{
  return ice_agent_;
}
#endif

WeakRcHandle<ICE::Endpoint>
RtpsUdpDataLink::get_ice_endpoint() const
{
  TransportImpl_rch ti = impl();
  return ti ? ti->get_ice_endpoint() : WeakRcHandle<ICE::Endpoint>();
}

bool RtpsUdpDataLink::is_leading(const GUID_t& writer_id,
                                 const GUID_t& reader_id) const
{
  RtpsWriterMap::mapped_type writer;

  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, writers_lock_, false);
    RtpsWriterMap::const_iterator pos = writers_.find(writer_id);
    if (pos == writers_.end()) {
      return false;
    }
    writer = pos->second;
  }

  return writer->is_leading(reader_id);
}

void RtpsUdpDataLink::RtpsWriter::log_remote_counts(const char* funcname)
{
  if (transport_debug.log_remote_counts) {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) {transport_debug.log_remote_counts} "
      "RtpsUdpDataLink::RtpsWriter::%C: "
      "%C pre: %b assoc: %b\n",
      funcname, LogGuid(id_).c_str(),
      preassociation_readers_.size(), remote_readers_.size()));
  }
}

void RtpsUdpDataLink::RtpsReader::log_remote_counts(const char* funcname)
{
  if (transport_debug.log_remote_counts) {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) {transport_debug.log_remote_counts} "
      "RtpsUdpDataLink::RtpsReader::%C: "
      "%C pre: %b assoc: %b\n",
      funcname, LogGuid(id_).c_str(),
      preassociation_writers_.size(), remote_writers_.size()));
  }
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
