/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TRANSPORT_RTPS_UDP_RTPSUDPDATALINK_H
#define OPENDDS_DCPS_TRANSPORT_RTPS_UDP_RTPSUDPDATALINK_H

#include "Rtps_Udp_Export.h"
#include "BundlingCacheKey.h"
#include "LocatorCacheKey.h"
#include "RtpsCustomizedElement.h"
#include "RtpsUdpDataLink_rch.h"
#include "RtpsUdpReceiveStrategy_rch.h"
#include "RtpsUdpSendStrategy_rch.h"
#include "RtpsUdpTransport_rch.h"
#include "TransactionalRtpsSendQueue.h"

#include <dds/DCPS/transport/framework/DataLink.h>
#include <dds/DCPS/transport/framework/ReceivedDataSample.h>
#include <dds/DCPS/transport/framework/TransportSendBuffer.h>
#include <dds/DCPS/transport/framework/TransportStatistics.h>

#include <dds/DCPS/AddressCache.h>
#include <dds/DCPS/DataBlockLockPool.h>
#include <dds/DCPS/DataSampleElement.h>
#include <dds/DCPS/DiscoveryListener.h>
#include <dds/DCPS/DisjointSequence.h>
#include <dds/DCPS/FibonacciSequence.h>
#include <dds/DCPS/GuidConverter.h>
#include <dds/DCPS/Hash.h>
#include <dds/DCPS/JobQueue.h>
#include <dds/DCPS/MulticastManager.h>
#include <dds/DCPS/PeriodicEvent.h>
#include <dds/DCPS/PoolAllocator.h>
#include <dds/DCPS/RcEventHandler.h>
#include <dds/DCPS/ReactorTask.h>
#include <dds/DCPS/ReactorTask_rch.h>
#include <dds/DCPS/SequenceNumber.h>
#include <dds/DCPS/SporadicEvent.h>

#include <dds/DCPS/RTPS/MessageTypes.h>
#include <dds/DCPS/RTPS/MessageUtils.h>

#include <dds/OpenDDSConfigWrapper.h>

#if OPENDDS_CONFIG_SECURITY
#  include <dds/DCPS/security/framework/SecurityConfig.h>
#  include <dds/DCPS/security/framework/SecurityConfig_rch.h>
#  include <dds/DCPS/RTPS/ICE/Ice.h>
#endif

#if OPENDDS_CONFIG_SECURITY
#  include <dds/DdsSecurityCoreC.h>
#endif

#include <ace/Basic_Types.h>
#include <ace/SOCK_Dgram.h>
#include <ace/SOCK_Dgram_Mcast.h>

#ifdef ACE_HAS_CPP11
#  include <functional>
#endif

class DDS_TEST;

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {

namespace ICE {
  class Endpoint;
}

namespace DCPS {

class RtpsUdpInst;
class ReceivedDataSample;
typedef RcHandle<RtpsUdpInst> RtpsUdpInst_rch;
typedef RcHandle<TransportClient> TransportClient_rch;

struct SeqReaders {
  SequenceNumber seq;
  RepoIdSet readers;
  SeqReaders(const GUID_t& id) : seq(0) { readers.insert(id); }
};

typedef AddressCache<LocatorCacheKey> LocatorCache;
typedef AddressCache<BundlingCacheKey> BundlingCache;

typedef OPENDDS_MAP_CMP(GUID_t, SeqReaders, GUID_tKeyLessThan) WriterToSeqReadersMap;

const size_t initial_bundle_size = 32;

class OpenDDS_Rtps_Udp_Export RtpsUdpDataLink
  : public virtual DataLink
  , public virtual InternalDataReaderListener<NetworkInterfaceAddress>
{
public:
  RtpsUdpDataLink(const RtpsUdpTransport_rch& transport,
                  const GuidPrefix_t& local_prefix,
                  const RtpsUdpInst_rch& config,
                  const ReactorTask_rch& reactor_task);

  ~RtpsUdpDataLink();

  bool add_delayed_notification(TransportQueueElement* element);

  RemoveResult remove_sample(const DataSampleElement* sample);
  void remove_all_msgs(const GUID_t& pub_id);

  RtpsUdpInst_rch config() const;

  ACE_Reactor* get_reactor();
  ReactorTask_rch get_reactor_task() const;

  ACE_SOCK_Dgram& unicast_socket();
  ACE_SOCK_Dgram_Mcast& multicast_socket();
#ifdef ACE_HAS_IPV6
  ACE_SOCK_Dgram& ipv6_unicast_socket();
  ACE_SOCK_Dgram_Mcast& ipv6_multicast_socket();
#endif

  bool open(const ACE_SOCK_Dgram& unicast_socket
#ifdef ACE_HAS_IPV6
            , const ACE_SOCK_Dgram& ipv6_unicast_socket
#endif
            );

  void received(const RTPS::DataSubmessage& data,
                const GuidPrefix_t& src_prefix,
                const NetworkAddress& remote_addr);

  void received(const RTPS::GapSubmessage& gap,
                const GuidPrefix_t& src_prefix,
                bool directed,
                const NetworkAddress& remote_addr);

  void received(const RTPS::HeartBeatSubmessage& heartbeat,
                const GuidPrefix_t& src_prefix,
                bool directed,
                const NetworkAddress& remote_addr);

  void received(const RTPS::HeartBeatFragSubmessage& hb_frag,
                const GuidPrefix_t& src_prefix,
                bool directed,
                const NetworkAddress& remote_addr);

  void received(const RTPS::AckNackSubmessage& acknack,
                const GuidPrefix_t& src_prefix,
                const NetworkAddress& remote_addr);

  void received(const RTPS::NackFragSubmessage& nackfrag,
                const GuidPrefix_t& src_prefix,
                const NetworkAddress& remote_addr);

  const GuidPrefix_t& local_prefix() const { return local_prefix_; }

  void remove_locator_and_bundling_cache(const GUID_t& remote_id);

  NetworkAddress get_last_recv_address(const GUID_t& remote_id);

  void update_locators(const GUID_t& remote_id,
                       NetworkAddressSet& unicast_addresses,
                       NetworkAddressSet& multicast_addresses,
                       bool requires_inline_qos,
                       bool add_ref);

  /// Given a 'local' id and a 'remote' id of a publication or
  /// subscription, return the set of addresses of the remote peers.
  NetworkAddressSet get_addresses(const GUID_t& local, const GUID_t& remote) const;
  /// Given a 'local' id, return the set of address for all remote peers.
  NetworkAddressSet get_addresses(const GUID_t& local) const;

  void filterBestEffortReaders(const ReceivedDataSample& ds, RepoIdSet& selected, RepoIdSet& withheld);

  int make_reservation(const GUID_t& remote_publication_id,
                       const GUID_t& local_subscription_id,
                       const TransportReceiveListener_wrch& receive_listener,
                       bool reliable);

  bool associated(const GUID_t& local, const GUID_t& remote,
                  bool local_reliable, bool remote_reliable,
                  bool local_durable, bool remote_durable,
                  const RTPS::VendorId_t& vendor_id,
                  const MonotonicTime_t& participant_discovered_at,
                  ACE_CDR::ULong participant_flags,
                  SequenceNumber max_sn,
                  const TransportClient_rch& client,
                  NetworkAddressSet& unicast_addresses,
                  NetworkAddressSet& multicast_addresses,
                  const NetworkAddress& last_addr_hint,
                  bool requires_inline_qos);

  void register_for_reader(const GUID_t& writerid,
                           const GUID_t& readerid,
                           const NetworkAddressSet& addresses,
                           DiscoveryListener* listener);

  void unregister_for_reader(const GUID_t& writerid,
                             const GUID_t& readerid);

  void register_for_writer(const GUID_t& readerid,
                           const GUID_t& writerid,
                           const NetworkAddressSet& addresses,
                           DiscoveryListener* listener);

  void unregister_for_writer(const GUID_t& readerid,
                             const GUID_t& writerid);

  void client_stop(const GUID_t& localId);

  virtual void pre_stop_i();

#if OPENDDS_CONFIG_SECURITY
  DCPS::RcHandle<ICE::Agent> get_ice_agent() const;
#endif
  virtual DCPS::WeakRcHandle<ICE::Endpoint> get_ice_endpoint() const;

  virtual bool is_leading(const GUID_t& writer_id,
                          const GUID_t& reader_id) const;

#if OPENDDS_CONFIG_SECURITY
  Security::SecurityConfig_rch security_config() const
  {
    ACE_Guard<ACE_Thread_Mutex> guard(security_mutex_);
    return security_config_;
  }

  Security::HandleRegistry_rch handle_registry() const
  {
    ACE_Guard<ACE_Thread_Mutex> guard(security_mutex_);
    return handle_registry_;
  }

  DDS::Security::ParticipantCryptoHandle local_crypto_handle() const;
  void local_crypto_handle(DDS::Security::ParticipantCryptoHandle pch);

  static bool separate_message(EntityId_t entity);
#endif

  RtpsUdpTransport_rch transport() const;

  void enable_response_queue();
  void disable_response_queue(bool send_immediately);

  bool requires_inline_qos(const GUIDSeq_var& peers);

  EventDispatcher_rch event_dispatcher() { return event_dispatcher_; }
  RcHandle<JobQueue> get_job_queue() const { return job_queue_; }

private:
  void on_data_available(RcHandle<InternalDataReader<NetworkInterfaceAddress> > reader);

  // Internal non-locking versions of the above
  NetworkAddressSet get_addresses_i(const GUID_t& local, const GUID_t& remote) const;
  NetworkAddressSet get_addresses_i(const GUID_t& local) const;

  virtual void stop_i();

  virtual TransportQueueElement* customize_queue_element(
    TransportQueueElement* element);

  virtual void release_reservations_i(const GUID_t& remote_id,
                                      const GUID_t& local_id);

  friend class ::DDS_TEST;
  /// static member used by testing code to force inline qos
  static bool force_inline_qos_;

  ReactorTask_rch reactor_task_;
  RcHandle<JobQueue> job_queue_;
  EventDispatcher_rch event_dispatcher_;

  RtpsUdpSendStrategy_rch send_strategy();
  RtpsUdpReceiveStrategy_rch receive_strategy();

  GuidPrefix_t local_prefix_;

  struct RemoteInfo {
    RemoteInfo() : unicast_addrs_(), multicast_addrs_(), requires_inline_qos_(false), ref_count_(0) {}
    RemoteInfo(const NetworkAddressSet& unicast_addrs, const NetworkAddressSet& multicast_addrs, bool iqos)
      : unicast_addrs_(unicast_addrs), multicast_addrs_(multicast_addrs), requires_inline_qos_(iqos), ref_count_(0) {}
    NetworkAddressSet unicast_addrs_;
    NetworkAddressSet multicast_addrs_;
    bool requires_inline_qos_;
    NetworkAddress last_recv_addr_;
    MonotonicTimePoint last_recv_time_;
    DDS::UInt32 ref_count_;
    bool insert_recv_addr(NetworkAddressSet& aset) const;
  };

#ifdef ACE_HAS_CPP11
  typedef OPENDDS_UNORDERED_MAP(GUID_t, RemoteInfo) RemoteInfoMap;
#else
  typedef OPENDDS_MAP_CMP(GUID_t, RemoteInfo, GUID_tKeyLessThan) RemoteInfoMap;
#endif
  RemoteInfoMap locators_;

  void update_last_recv_addr(const GUID_t& src, const NetworkAddress& addr, const MonotonicTimePoint& now = MonotonicTimePoint::now());

  mutable LocatorCache locator_cache_;
  mutable BundlingCache bundling_cache_;

  ACE_SOCK_Dgram unicast_socket_;
  ACE_SOCK_Dgram_Mcast multicast_socket_;
#ifdef ACE_HAS_IPV6
  ACE_SOCK_Dgram ipv6_unicast_socket_;
  ACE_SOCK_Dgram_Mcast ipv6_multicast_socket_;
#endif

  MessageBlockAllocator mb_allocator_;
  DataBlockAllocator db_allocator_;
  Dynamic_Cached_Allocator_With_Overflow<ACE_Thread_Mutex> custom_allocator_;
  Dynamic_Cached_Allocator_With_Overflow<ACE_Thread_Mutex> bundle_allocator_;
  unique_ptr<DataBlockLockPool> db_lock_pool_;

  ACE_Message_Block* alloc_msgblock(size_t size, ACE_Allocator* data_allocator);
  ACE_Message_Block* submsgs_to_msgblock(const RTPS::SubmessageSeq& subm);

  RcHandle<SingleSendBuffer> get_writer_send_buffer(const GUID_t& pub_id);

  struct MultiSendBuffer : TransportSendBuffer {

    MultiSendBuffer(RtpsUdpDataLink* outer, size_t capacity)
      : TransportSendBuffer(capacity)
      , outer_(outer)
    {}

    void insert(SequenceNumber sequence,
                TransportSendStrategy::QueueType* queue,
                ACE_Message_Block* chain);

    RtpsUdpDataLink* outer_;

  } multi_buff_;

  // RTPS reliability support for local writers:

  typedef CORBA::ULong FragmentNumberValue;
  typedef OPENDDS_MAP(FragmentNumberValue, RTPS::FragmentNumberSet) RequestedFragMap;
  typedef OPENDDS_MAP(SequenceNumber, RequestedFragMap) RequestedFragSeqMap;

  struct ReaderInfo : public virtual RcObject {
    const GUID_t id_;
    const MonotonicTime_t participant_discovered_at_;
    CORBA::Long acknack_recvd_count_, nackfrag_recvd_count_;
    DisjointSequence requests_;
    RequestedFragSeqMap requested_frags_;
    SequenceNumber cur_cumulative_ack_;
    const bool durable_;
    const ACE_CDR::ULong participant_flags_;
    ACE_CDR::Long required_acknack_count_;
    OPENDDS_MAP(SequenceNumber, TransportQueueElement*) durable_data_;
    MonotonicTimePoint durable_timestamp_;
    const SequenceNumber start_sn_;
#if OPENDDS_CONFIG_SECURITY
    SequenceNumber max_pvs_sn_;
    DisjointSequence pvs_outstanding_;
#endif

    ReaderInfo(const GUID_t& id,
               bool durable,
               const MonotonicTime_t& participant_discovered_at,
               ACE_CDR::ULong participant_flags,
               const SequenceNumber& start_sn)
      : id_(id)
      , participant_discovered_at_(participant_discovered_at)
      , acknack_recvd_count_(0)
      , nackfrag_recvd_count_(0)
      , cur_cumulative_ack_(SequenceNumber::ZERO()) // Starting at zero instead of unknown makes the logic cleaner.
      , durable_(durable)
      , participant_flags_(participant_flags)
      , required_acknack_count_(0)
      , start_sn_(start_sn)
#if OPENDDS_CONFIG_SECURITY
      , max_pvs_sn_(SequenceNumber::ZERO())
#endif
    {}
    ~ReaderInfo();
    void swap_durable_data(OPENDDS_MAP(SequenceNumber, TransportQueueElement*)& dd);
    void expunge_durable_data();
    bool expecting_durable_data() const;
    SequenceNumber acked_sn() const { return cur_cumulative_ack_.previous(); }
    bool reflects_heartbeat_count() const;
  };

  typedef RcHandle<ReaderInfo> ReaderInfo_rch;
#ifdef ACE_HAS_CPP11
  typedef OPENDDS_UNORDERED_MAP(GUID_t, ReaderInfo_rch) ReaderInfoMap;
#else
  typedef OPENDDS_MAP_CMP(GUID_t, ReaderInfo_rch, GUID_tKeyLessThan) ReaderInfoMap;
#endif
  typedef OPENDDS_SET(ReaderInfo_rch) ReaderInfoSet;
  struct ReaderInfoSetHolder : RcObject {
    ReaderInfoSet readers;
  };
  typedef RcHandle<ReaderInfoSetHolder> ReaderInfoSetHolder_rch;
  typedef OPENDDS_MAP(SequenceNumber, ReaderInfoSetHolder_rch) SNRIS;

  class ReplayDurableData : public EventBase {
  public:
    ReplayDurableData(WeakRcHandle<RtpsUdpDataLink> link, const GUID_t& local_pub_id, const GUID_t& remote_sub_id)
      : link_(link)
      , local_pub_id_(local_pub_id)
      , remote_sub_id_(remote_sub_id)
    {}

  private:
    WeakRcHandle<RtpsUdpDataLink> link_;
    const GUID_t local_pub_id_;
    const GUID_t remote_sub_id_;

    void handle_event() {
      RtpsUdpDataLink_rch link = link_.lock();

      if (!link) {
        return;
      }

      link->replay_durable_data(local_pub_id_, remote_sub_id_);
    }
  };

  class RtpsWriter : public virtual RcObject {
  private:
    ReaderInfoMap remote_readers_;
    RcHandle<ConstSharedRepoIdSet> remote_reader_guids_;
    /// Preassociation readers require a non-final heartbeat.
    ReaderInfoSet preassociation_readers_;
    typedef OPENDDS_MULTISET(OpenDDS::DCPS::SequenceNumber) SequenceNumberMultiset;
    SequenceNumberMultiset preassociation_reader_start_sns_;
    /// These readers have not acked everything they are supposed to have
    /// acked.
    SNRIS lagging_readers_;
    /// These reader have acked everything they are supposed to have acked.
    SNRIS leading_readers_;
    /// These readers have sent a nack and are expecting data.
    ReaderInfoSet readers_expecting_data_;
    /// These readers have sent a non-final ack are are expecting a heartbeat.
    ReaderInfoSet readers_expecting_heartbeat_;
    RcHandle<SingleSendBuffer> send_buff_;
    SequenceNumber max_sn_;
    typedef OPENDDS_SET_CMP(TransportQueueElement*, TransportQueueElement::OrderBySequenceNumber) TqeSet;
    typedef OPENDDS_MULTIMAP(SequenceNumber, TransportQueueElement*) SnToTqeMap;
    SnToTqeMap elems_not_acked_;
    WeakRcHandle<TransportClient> client_;
    WeakRcHandle<RtpsUdpDataLink> link_;
    const GUID_t id_;
    const bool durable_;
    bool stopping_;
    CORBA::Long heartbeat_count_;
#if OPENDDS_CONFIG_SECURITY
    /// Participant Volatile Secure writer
    const bool is_pvs_writer_;
    /// Partcicipant Secure (Reliable SPDP) writer
    const bool is_ps_writer_;
#endif
    mutable ACE_Thread_Mutex mutex_;
    mutable ACE_Thread_Mutex remote_reader_guids_mutex_;
    mutable ACE_Thread_Mutex elems_not_acked_mutex_;

    RcHandle<SporadicEvent> heartbeat_;
    RcHandle<SporadicEvent> nack_response_;

    const TimeDuration initial_fallback_;
    FibonacciSequence<TimeDuration> fallback_;

    void send_heartbeats(const MonotonicTimePoint& now);
    void send_nack_responses(const MonotonicTimePoint& now);
    void add_gap_submsg_i(RTPS::SubmessageSeq& msg,
                          SequenceNumber gap_start);
    void end_historic_samples_i(const DataSampleHeader& header,
                                ACE_Message_Block* body,
                                MetaSubmessageVec& meta_submessages);
    void request_ack_i(const DataSampleHeader& header,
                       ACE_Message_Block* body,
                       MetaSubmessageVec& meta_submessages);
    void send_heartbeats_manual_i(MetaSubmessageVec& meta_submessages);
    void gather_gaps_i(const ReaderInfo_rch& reader,
                       const DisjointSequence& gaps,
                       MetaSubmessageVec& meta_submessages);
    void acked_by_all_helper_i(TqeSet& to_deliver);
    SequenceNumber expected_max_sn(const ReaderInfo_rch& reader) const;
    static void snris_insert(RtpsUdpDataLink::SNRIS& snris, const ReaderInfo_rch& reader);
    static void snris_erase(RtpsUdpDataLink::SNRIS& snris, const SequenceNumber sn, const ReaderInfo_rch& reader);
    void make_leader_lagger(const GUID_t& reader, SequenceNumber previous_max_sn);
    void make_lagger_leader(const ReaderInfo_rch& reader, const SequenceNumber previous_acked_sn);
    bool is_lagging(const ReaderInfo_rch& reader) const;
    bool is_leading(const ReaderInfo_rch& reader) const;
    void check_leader_lagger() const;
    void record_directed(const GUID_t& reader, SequenceNumber seq);
    void update_remote_guids_cache_i(bool add, const GUID_t& guid);

#if OPENDDS_CONFIG_SECURITY
    bool is_pvs_writer() const { return is_pvs_writer_; }
#else
    bool is_pvs_writer() const { return false; }
#endif

    SequenceNumber non_durable_first_sn(const SingleSendBuffer::Proxy& proxy) const
    {
      if (!proxy.empty()) {
        return proxy.low();
      }
      if (!proxy.pre_empty()) {
        return proxy.pre_low();
      }
      return max_sn_ + 1;
    }

    void remove_preassociation_reader(const ReaderInfo_rch& reader)
    {
      if (preassociation_readers_.erase(reader)) {
        SequenceNumberMultiset::iterator pos = preassociation_reader_start_sns_.find(reader->start_sn_);
        OPENDDS_ASSERT(pos != preassociation_reader_start_sns_.end());
        preassociation_reader_start_sns_.erase(pos);
      }
    }

    void initialize_heartbeat(const SingleSendBuffer::Proxy& proxy,
                              MetaSubmessage& meta_submessage);
    void gather_directed_heartbeat_i(const SingleSendBuffer::Proxy& proxy,
                                     MetaSubmessageVec& meta_submessages,
                                     MetaSubmessage& meta_submessage,
                                     const ReaderInfo_rch& reader);

    void log_remote_counts(const char* funcname);

  public:
    RtpsWriter(const TransportClient_rch& client, const RtpsUdpDataLink_rch& link,
               const GUID_t& id, bool durable,
               SequenceNumber max_sn, CORBA::Long heartbeat_count, size_t capacity);
    virtual ~RtpsWriter();

    SequenceNumber max_data_seq(const SingleSendBuffer::Proxy& proxy,
                                const ReaderInfo_rch&) const;
    SequenceNumber update_max_sn(const GUID_t& reader, SequenceNumber seq);
    void add_elem_awaiting_ack(TransportQueueElement* element);

    RemoveResult remove_sample(const DataSampleElement* sample);
    void remove_all_msgs();

    bool add_reader(const ReaderInfo_rch& reader);
    bool has_reader(const GUID_t& id) const;
    bool is_leading(const GUID_t& id) const;
    bool remove_reader(const GUID_t& id);
    size_t reader_count() const;
    CORBA::Long inc_heartbeat_count();

    void pre_stop_helper(TqeVector& to_drop, bool true_stop);
    TransportQueueElement* customize_queue_element_helper(TransportQueueElement* element,
                                                          bool requires_inline_qos,
                                                          MetaSubmessageVec& meta_submessages,
                                                          bool& deliver_after_send);

    void process_acknack(const RTPS::AckNackSubmessage& acknack,
                         const GUID_t& src,
                         MetaSubmessageVec& meta_submessages);
    void process_nackfrag(const RTPS::NackFragSubmessage& nackfrag,
                          const GUID_t& src,
                          MetaSubmessageVec& meta_submessages);
    void process_acked_by_all();
    void gather_nack_replies_i(MetaSubmessageVec& meta_submessages);
    void gather_heartbeats_i(MetaSubmessageVec& meta_submessages);
    void gather_heartbeats(RcHandle<ConstSharedRepoIdSet> additional_guids,
                           MetaSubmessageVec& meta_submessages);
    void update_required_acknack_count(const GUID_t& id, CORBA::Long current);

    RcHandle<SingleSendBuffer> get_send_buff() { return send_buff_; }
    RcHandle<ConstSharedRepoIdSet> get_remote_reader_guids()
    {
      ACE_Guard<ACE_Thread_Mutex> guard(remote_reader_guids_mutex_);
      return remote_reader_guids_;
    }
  };
  typedef RcHandle<RtpsWriter> RtpsWriter_rch;

#ifdef ACE_HAS_CPP11
  typedef OPENDDS_UNORDERED_MAP(GUID_t, RtpsWriter_rch) RtpsWriterMap;
#else
  typedef OPENDDS_MAP_CMP(GUID_t, RtpsWriter_rch, GUID_tKeyLessThan) RtpsWriterMap;
#endif
  RtpsWriterMap writers_;


  // RTPS reliability support for local readers:

  struct WriterInfo : RcObject {
    const GUID_t id_;
    const RTPS::VendorId_t vendor_id_;
    const MonotonicTime_t participant_discovered_at_;
    DisjointSequence recvd_;
    typedef OPENDDS_MAP(SequenceNumber, ReceivedDataSample) HeldMap;
    HeldMap held_;
    SequenceNumber hb_last_;
    OPENDDS_MAP(SequenceNumber, RTPS::FragmentNumber_t) frags_;
    CORBA::Long heartbeat_recvd_count_, hb_frag_recvd_count_;
    const ACE_CDR::ULong participant_flags_;

    WriterInfo(const GUID_t& id,
               const RTPS::VendorId_t& vendor_id,
               const MonotonicTime_t& participant_discovered_at,
               ACE_CDR::ULong participant_flags)
      : id_(id)
      , vendor_id_(vendor_id)
      , participant_discovered_at_(participant_discovered_at)
      , hb_last_(SequenceNumber::ZERO())
      , heartbeat_recvd_count_(0)
      , hb_frag_recvd_count_(0)
      , participant_flags_(participant_flags)
      , acknack_count_(0)
    { }

    bool should_nack() const;
    bool sends_directed_hb() const;
    SequenceNumber preemptive_acknack_base() const
    {
      // RTI expects 0 while the spec implies it should be 1.
      static const RTPS::VendorId_t rti_vendor_id = {{ 0x01, 0x01 }};
      return vendor_id_ == rti_vendor_id ? 0 : 1;
    }

    CORBA::Long next_acknack_count()
    {
      // Reflect the heartbeat count for OpenDDS.
      return vendor_id_ == RTPS::VENDORID_OPENDDS ? heartbeat_recvd_count_ : ++acknack_count_;
    }

  private:
    CORBA::Long acknack_count_;
  };
  typedef RcHandle<WriterInfo> WriterInfo_rch;
#ifdef ACE_HAS_CPP11
  typedef OPENDDS_UNORDERED_MAP(GUID_t, WriterInfo_rch) WriterInfoMap;
#else
  typedef OPENDDS_MAP_CMP(GUID_t, WriterInfo_rch, GUID_tKeyLessThan) WriterInfoMap;
#endif
  typedef OPENDDS_SET(WriterInfo_rch) WriterInfoSet;

  class RtpsReader : public virtual RcObject {
  public:
    RtpsReader(const RtpsUdpDataLink_rch& link, const GUID_t& id);
    virtual ~RtpsReader();

    bool add_writer(const WriterInfo_rch& info);
    bool has_writer(const GUID_t& id) const;
    bool remove_writer(const GUID_t& id);
    size_t writer_count() const;

    bool should_nack_fragments(const RcHandle<RtpsUdpDataLink>& link,
                               const WriterInfo_rch& info);

    void pre_stop_helper();

    void process_heartbeat_i(const RTPS::HeartBeatSubmessage& heartbeat,
                             const GUID_t& src,
                             bool directed,
                             MetaSubmessageVec& meta_submessages);
    bool process_data_i(const RTPS::DataSubmessage& data, const GUID_t& src, MetaSubmessageVec& meta_submessages);
    void process_gap_i(const RTPS::GapSubmessage& gap,
                       const GUID_t& src,
                       bool directed,
                       MetaSubmessageVec& meta_submessages);
    void process_heartbeat_frag_i(const RTPS::HeartBeatFragSubmessage& hb_frag,
                                  const GUID_t& src,
                                  bool directed,
                                  MetaSubmessageVec& meta_submessages);
    void deliver_held_data(const GUID_t& src);

    const GUID_t& id() const { return id_; }

    void log_remote_counts(const char* funcname);

  private:
    void send_preassociation_acknacks(const MonotonicTimePoint& now);
    void gather_preassociation_acknack_i(MetaSubmessageVec& meta_submessages,
                                         const WriterInfo_rch& writer);

    void gather_ack_nacks_i(const WriterInfo_rch& writer,
                            const RtpsUdpDataLink_rch& link,
                            bool heartbeat_was_non_final,
                            MetaSubmessageVec& meta_submessages,
                            ACE_CDR::ULong& cumulative_bits_added);
    void generate_nack_frags_i(MetaSubmessageVec& meta_submessages,
                               const WriterInfo_rch& wi,
                               EntityId_t reader_id,
                               EntityId_t writer_id,
                               ACE_CDR::ULong& cumulative_bits_added);

    mutable ACE_Thread_Mutex mutex_;
    WeakRcHandle<RtpsUdpDataLink> link_;
    const GUID_t id_;
    WriterInfoMap remote_writers_;
    WriterInfoSet preassociation_writers_;
    bool stopping_;
    CORBA::Long nackfrag_count_;
    RcHandle<SporadicEvent> preassociation_task_;
    const TimeDuration initial_fallback_;
    FibonacciSequence<TimeDuration> fallback_;
  };
  typedef RcHandle<RtpsReader> RtpsReader_rch;

  typedef OPENDDS_VECTOR(MetaSubmessageVec::iterator) MetaSubmessageIterVec;
  typedef OPENDDS_MAP_CMP(GUID_t, MetaSubmessageIterVec, GUID_tKeyLessThan) DestMetaSubmessageMap;
#ifdef ACE_HAS_CPP11
  typedef OPENDDS_UNORDERED_MAP(AddressCacheEntryProxy, DestMetaSubmessageMap) AddrDestMetaSubmessageMap;
#else
  typedef OPENDDS_MAP(AddressCacheEntryProxy, DestMetaSubmessageMap) AddrDestMetaSubmessageMap;
#endif
  typedef OPENDDS_VECTOR(MetaSubmessageIterVec) MetaSubmessageIterVecVec;
  typedef OPENDDS_SET(CORBA::Long) CountSet;
  typedef OPENDDS_MAP_CMP(EntityId_t, CountSet, EntityId_tKeyLessThan) IdCountSet;
  struct CountMapPair {
    CountMapPair() : undirected_(false), is_new_assigned_(false), new_(-1) {}
    bool undirected_;
    bool is_new_assigned_;
    CORBA::Long new_;
  };
  typedef OPENDDS_MAP(CORBA::Long, CountMapPair) CountMap;
  struct CountMapping {
    CountMap map_;
    CountMap::iterator next_directed_unassigned_;
    CountMap::iterator next_undirected_unassigned_;
  };
  typedef OPENDDS_MAP_CMP(EntityId_t, CountMapping, EntityId_tKeyLessThan) IdCountMapping;

  struct CountKeeper {
    IdCountMapping heartbeat_counts_;
    IdCountSet nackfrag_counts_;
  };

public:
  struct Bundle {
    explicit Bundle(const AddressCacheEntryProxy& proxy) : proxy_(proxy), size_(0) { submessages_.reserve(initial_bundle_size); }
    MetaSubmessageIterVec submessages_; // a vectors of iterators pointing to meta_submessages
    AddressCacheEntryProxy proxy_; // a bundle's destination address
    size_t size_; // bundle message size
  };

  typedef OPENDDS_VECTOR(Bundle) BundleVec;

private:
  void build_meta_submessage_map(MetaSubmessageVec& meta_submessages, AddrDestMetaSubmessageMap& addr_map);
  void bundle_mapped_meta_submessages(
    const Encoding& encoding,
    AddrDestMetaSubmessageMap& addr_map,
    BundleVec& bundles,
    CountKeeper& counts);

  void queue_submessages(MetaSubmessageVec& meta_submessages);
  void update_required_acknack_count(const GUID_t& local_id, const GUID_t& remote_id, CORBA::Long current);
  void bundle_and_send_submessages(MetaSubmessageVec& meta_submessages);

  TransactionalRtpsSendQueue sq_;
  mutable ACE_Thread_Mutex fsq_mutex_;
  OPENDDS_VECTOR(MetaSubmessageVec) fsq_vec_;
  size_t fsq_vec_size_;

  void harvest_send_queue(const MonotonicTimePoint& now);
  RcHandle<SporadicEvent> harvest_send_queue_sporadic_;
  void flush_send_queue(const MonotonicTimePoint& now);
  void flush_send_queue_i();
  RcHandle<SporadicEvent> flush_send_queue_sporadic_;

  RepoIdSet pending_reliable_readers_;

#ifdef ACE_HAS_CPP11
  typedef OPENDDS_UNORDERED_MAP(GUID_t, RtpsReader_rch) RtpsReaderMap;
#else
  typedef OPENDDS_MAP_CMP(GUID_t, RtpsReader_rch, GUID_tKeyLessThan) RtpsReaderMap;
#endif
  RtpsReaderMap readers_;

  typedef OPENDDS_MULTIMAP_CMP(GUID_t, RtpsReader_rch, GUID_tKeyLessThan) RtpsReaderMultiMap;
  RtpsReaderMultiMap readers_of_writer_; // keys are remote data writer GUIDs

  WriterToSeqReadersMap writer_to_seq_best_effort_readers_;

  /// What was once a single lock for the whole datalink is now split between three (four including ch_lock_):
  /// - readers_lock_ protects readers_, readers_of_writer_, pending_reliable_readers_, interesting_writers_, and
  ///   writer_to_seq_best_effort_readers_ along with anything else that fits the 'reader side activity' of the datalink
  /// - writers_lock_ protects writers_, heartbeat_counts_ best_effort_heartbeat_count_, and interesting_readers_
  ///   along with anything else that fits the 'writers side activity' of the datalink
  /// - locators_lock_ protects locators_ (and therefore calls to get_addresses_i())
  ///   for both remote writers and remote readers
  /// - send_queues_lock protects thread_send_queues_
  mutable ACE_Thread_Mutex readers_lock_;
  mutable ACE_Thread_Mutex writers_lock_;
  mutable ACE_Thread_Mutex locators_lock_;

  /// Extend the FragmentNumberSet to cover the fragments that are
  /// missing from our last known fragment to the extent
  /// @param fnSet FragmentNumberSet for the message sequence number
  /// in question
  /// @param extent is the highest fragment sequence number for this
  /// FragmentNumberSet
  static void extend_bitmap_range(RTPS::FragmentNumberSet& fnSet,
                                  CORBA::ULong extent,
                                  ACE_CDR::ULong& cumulative_bits_added);

  void durability_resend(TransportQueueElement* element, size_t& cumulative_send_count);
  void durability_resend(TransportQueueElement* element, const RTPS::FragmentNumberSet& fragmentSet, size_t& cumulative_send_count);

  static bool include_fragment(const TransportQueueElement& element,
                               const DisjointSequence& fragments,
                               SequenceNumber& lastFragment);

  template<typename T, typename FN>
  void datawriter_dispatch(const T& submessage, const GuidPrefix_t& src_prefix,
                           const FN& func)
  {
    const GUID_t local = make_id(local_prefix_, submessage.writerId);
    const GUID_t src = make_id(src_prefix, submessage.readerId);

    OPENDDS_VECTOR(RtpsWriter_rch) to_call;
    {
      ACE_GUARD(ACE_Thread_Mutex, g, writers_lock_);
      const RtpsWriterMap::iterator rw = writers_.find(local);
      if (rw == writers_.end()) {
        if (transport_debug.log_dropped_messages) {
          ACE_DEBUG((LM_DEBUG, "(%P|%t) {transport_debug.log_dropped_messages} RtpsUdpDataLink::datawriter_dispatch - %C -> %C unknown local writer\n", LogGuid(local).c_str(), LogGuid(src).c_str()));
        }
        return;
      }
      to_call.push_back(rw->second);
    }
    MetaSubmessageVec meta_submessages;
    for (OPENDDS_VECTOR(RtpsWriter_rch)::const_iterator it = to_call.begin(); it < to_call.end(); ++it) {
      RtpsWriter& writer = **it;
      (writer.*func)(submessage, src, meta_submessages);
    }
    queue_submessages(meta_submessages);
  }

  template<typename T, typename FN>
  void datareader_dispatch(const T& submessage,
                           const GuidPrefix_t& src_prefix,
                           bool directed,
                           const FN& func)
  {
    const GUID_t local = make_id(local_prefix_, submessage.readerId);
    const GUID_t src = make_id(src_prefix, submessage.writerId);

    OPENDDS_VECTOR(RtpsReader_rch) to_call;
    {
      ACE_GUARD(ACE_Thread_Mutex, g, readers_lock_);
      if (local.entityId == ENTITYID_UNKNOWN) {
        typedef std::pair<RtpsReaderMultiMap::iterator, RtpsReaderMultiMap::iterator> RRMM_IterRange;
        for (RRMM_IterRange iters = readers_of_writer_.equal_range(src); iters.first != iters.second; ++iters.first) {
          to_call.push_back(iters.first->second);
        }
        if (to_call.empty()) {
          if (transport_debug.log_dropped_messages) {
            ACE_DEBUG((LM_DEBUG, "(%P|%t) {transport_debug.log_dropped_messages} RtpsUdpDataLink::datawreader_dispatch - %C -> X no local readers\n", LogGuid(src).c_str()));
          }
          return;
        }
      } else {
        const RtpsReaderMap::iterator rr = readers_.find(local);
        if (rr == readers_.end()) {
          if (transport_debug.log_dropped_messages) {
            ACE_DEBUG((LM_DEBUG, "(%P|%t) {transport_debug.log_dropped_messages} RtpsUdpDataLink::datareader_dispatch - %C -> %C unknown local reader\n", LogGuid(src).c_str(), LogGuid(local).c_str()));
          }
          return;
        }
        to_call.push_back(rr->second);
      }
    }
    MetaSubmessageVec meta_submessages;
    for (OPENDDS_VECTOR(RtpsReader_rch)::const_iterator it = to_call.begin(); it < to_call.end(); ++it) {
      RtpsReader& reader = **it;
      (reader.*func)(submessage, src, directed, meta_submessages);
    }
    queue_submessages(meta_submessages);
  }

  void send_heartbeats(const MonotonicTimePoint& now);
  void check_heartbeats(const MonotonicTimePoint& now);

  CORBA::Long best_effort_heartbeat_count_;

  RcHandle<PeriodicEvent> heartbeat_;
  RcHandle<PeriodicEvent> heartbeatchecker_;

  /// Data structure representing an "interesting" remote entity for static discovery.
  struct InterestingRemote {
    /// id of local entity that is interested in this remote.
    GUID_t localid;
    /// addresses of this entity
    NetworkAddressSet addresses;
    /// Callback to invoke.
    DiscoveryListener* listener;
    /**
     * Timestamp indicating the last HeartBeat or AckNack received from the
     * remote entity.
     */
    MonotonicTimePoint last_activity;
    /// Current status of the remote entity.
    enum { DOES_NOT_EXIST, EXISTS } status;

    InterestingRemote() { }
    InterestingRemote(const GUID_t& w, const NetworkAddressSet& a, DiscoveryListener* l)
      : localid(w)
      , addresses(a)
      , listener(l)
      , status(DOES_NOT_EXIST)
    { }
  };
  typedef OPENDDS_MULTIMAP_CMP(GUID_t, InterestingRemote, GUID_tKeyLessThan) InterestingRemoteMapType;
  InterestingRemoteMapType interesting_readers_;
  InterestingRemoteMapType interesting_writers_;

  typedef std::pair<GUID_t, InterestingRemote> CallbackType;

  TransportQueueElement* customize_queue_element_non_reliable_i(TransportQueueElement* element,
                                                                bool requires_inline_qos,
                                                                MetaSubmessageVec& meta_submessages,
                                                                bool& deliver_after_send,
                                                                ACE_Guard<ACE_Thread_Mutex>& guard);

  void send_heartbeats_manual_i(const TransportSendControlElement* tsce,
                                MetaSubmessageVec& meta_submessages);

  typedef OPENDDS_MAP_CMP(EntityId_t, CORBA::Long, EntityId_tKeyLessThan) CountMapType;
  CountMapType heartbeat_counts_;

  const size_t max_bundle_size_;

  class DeliverHeldData {
  public:
    DeliverHeldData()
      : writer_id_(GUID_UNKNOWN)
    {}

    DeliverHeldData(RtpsReader_rch reader,
                    const GUID_t& writer_id)
      : reader_(reader)
      , writer_id_(writer_id)
    {}

    ~DeliverHeldData();

  private:
    RtpsReader_rch reader_;
    GUID_t writer_id_;
  };

#if OPENDDS_CONFIG_SECURITY
  mutable ACE_Thread_Mutex security_mutex_;
  Security::SecurityConfig_rch security_config_;
  Security::HandleRegistry_rch handle_registry_;
  DDS::Security::ParticipantCryptoHandle local_crypto_handle_;
  RcHandle<ICE::Agent> ice_agent_;
#endif

  void accumulate_addresses(const GUID_t& local, const GUID_t& remote, NetworkAddressSet& addresses, bool prefer_unicast = false) const;

  RcHandle<InternalDataReader<NetworkInterfaceAddress> > network_interface_address_reader_;
  MulticastManager multicast_manager_;

  bool uses_end_historic_control_messages() const { return false; }
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#if defined ACE_HAS_CPP11
namespace std
{

  template<> struct OpenDDS_Rtps_Udp_Export hash<OpenDDS::DCPS::AddressCacheEntryProxy>
  {
    std::size_t operator()(const OpenDDS::DCPS::AddressCacheEntryProxy& val) const noexcept
    {
      return val.hash();
    }
  };

} // namespace std
#endif

#ifdef __ACE_INLINE__
# include "RtpsUdpDataLink.inl"
#endif  /* __ACE_INLINE__ */

#endif /* OPENDDS_DCPS_TRANSPORT_RTPS_UDP_RTPSUDPDATALINK_H */
