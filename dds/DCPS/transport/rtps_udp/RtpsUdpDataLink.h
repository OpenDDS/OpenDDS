/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DCPS_RTPSUDPDATALINK_H
#define DCPS_RTPSUDPDATALINK_H

#include "Rtps_Udp_Export.h"

#include "RtpsUdpSendStrategy.h"
#include "RtpsUdpSendStrategy_rch.h"
#include "RtpsUdpReceiveStrategy.h"
#include "RtpsUdpReceiveStrategy_rch.h"
#include "RtpsCustomizedElement.h"

#include "ace/Basic_Types.h"
#include "ace/SOCK_Dgram.h"
#include "ace/SOCK_Dgram_Mcast.h"

#include "dds/DCPS/transport/framework/DataLink.h"
#include "dds/DCPS/ReactorTask.h"
#include "dds/DCPS/ReactorTask_rch.h"
#include "dds/DCPS/PeriodicTask.h"
#include "dds/DCPS/transport/framework/TransportSendBuffer.h"
#include "dds/DCPS/NetworkConfigMonitor.h"

#include "dds/DCPS/DataSampleElement.h"
#include "dds/DCPS/DisjointSequence.h"
#include "dds/DCPS/GuidConverter.h"
#include "dds/DCPS/PoolAllocator.h"
#include "dds/DCPS/DiscoveryListener.h"
#include "dds/DCPS/ReactorInterceptor.h"
#include "dds/DCPS/RcEventHandler.h"
#include "dds/DCPS/JobQueue.h"
#include "dds/DCPS/SequenceNumber.h"

#ifdef OPENDDS_SECURITY
#include "dds/DdsSecurityCoreC.h"
#include "dds/DCPS/security/framework/SecurityConfig.h"
#include "dds/DCPS/security/framework/SecurityConfig_rch.h"
#endif

class DDS_TEST;

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {

namespace ICE {
  class Endpoint;
}

namespace DCPS {

class RtpsUdpInst;
class RtpsUdpTransport;
class ReceivedDataSample;
typedef RcHandle<RtpsUdpInst> RtpsUdpInst_rch;
typedef RcHandle<RtpsUdpTransport> RtpsUdpTransport_rch;

struct SeqReaders {
  SequenceNumber seq;
  RepoIdSet readers;
  SeqReaders(const RepoId& id) : seq(0) { readers.insert(id); }
};

typedef OPENDDS_MAP_CMP(RepoId, SeqReaders, GUID_tKeyLessThan) WriterToSeqReadersMap;

class OpenDDS_Rtps_Udp_Export RtpsUdpDataLink : public DataLink, public virtual NetworkConfigListener {
public:

  RtpsUdpDataLink(RtpsUdpTransport& transport,
                  const GuidPrefix_t& local_prefix,
                  const RtpsUdpInst& config,
                  const ReactorTask_rch& reactor_task);

  bool add_delayed_notification(TransportQueueElement* element);

  RemoveResult remove_sample(const DataSampleElement* sample);
  void remove_all_msgs(const RepoId& pub_id);

  RtpsUdpInst& config() const;

  ACE_Reactor* get_reactor();
  bool reactor_is_shut_down();

  ACE_SOCK_Dgram& unicast_socket();
  ACE_SOCK_Dgram_Mcast& multicast_socket();

  bool open(const ACE_SOCK_Dgram& unicast_socket);

  void received(const RTPS::DataSubmessage& data,
                const GuidPrefix_t& src_prefix);

  void received(const RTPS::GapSubmessage& gap, const GuidPrefix_t& src_prefix);

  void received(const RTPS::HeartBeatSubmessage& heartbeat,
                const GuidPrefix_t& src_prefix);

  void received(const RTPS::HeartBeatFragSubmessage& hb_frag,
                const GuidPrefix_t& src_prefix);

  void received(const RTPS::AckNackSubmessage& acknack,
                const GuidPrefix_t& src_prefix);

  void received(const RTPS::NackFragSubmessage& nackfrag,
                const GuidPrefix_t& src_prefix);

  const GuidPrefix_t& local_prefix() const { return local_prefix_; }

  void add_locator(const RepoId& remote_id, const ACE_INET_Addr& address,
                   bool requires_inline_qos);

  typedef OPENDDS_SET(ACE_INET_Addr) AddrSet;

  /// Given a 'local' id and a 'remote' id of a publication or
  /// subscription, return the set of addresses of the remote peers.
  AddrSet get_addresses(const RepoId& local, const RepoId& remote) const;
  /// Given a 'local' id, return the set of address for all remote peers.
  AddrSet get_addresses(const RepoId& local) const;

  void filterBestEffortReaders(const ReceivedDataSample& ds, RepoIdSet& selected, RepoIdSet& withheld);

  int make_reservation(const RepoId& remote_publication_id,
                       const RepoId& local_subscription_id,
                       const TransportReceiveListener_wrch& receive_listener,
                       bool reliable);

  void associated(const RepoId& local, const RepoId& remote,
                  bool local_reliable, bool remote_reliable,
                  bool local_durable, bool remote_durable);

  bool check_handshake_complete(const RepoId& local, const RepoId& remote);

  void register_for_reader(const RepoId& writerid,
                           const RepoId& readerid,
                           const ACE_INET_Addr& address,
                           OpenDDS::DCPS::DiscoveryListener* listener);

  void unregister_for_reader(const RepoId& writerid,
                             const RepoId& readerid);

  void register_for_writer(const RepoId& readerid,
                           const RepoId& writerid,
                           const ACE_INET_Addr& address,
                           OpenDDS::DCPS::DiscoveryListener* listener);

  void unregister_for_writer(const RepoId& readerid,
                             const RepoId& writerid);

  virtual void pre_stop_i();

  virtual void send_final_acks(const RepoId& readerid);

  virtual ICE::Endpoint* get_ice_endpoint() const;

#ifdef OPENDDS_SECURITY
  Security::SecurityConfig_rch security_config() const
  { return security_config_; }

  DDS::Security::ParticipantCryptoHandle local_crypto_handle() const;
  void local_crypto_handle(DDS::Security::ParticipantCryptoHandle pch);

  DDS::Security::ParticipantCryptoHandle peer_crypto_handle(const RepoId& peer) const;
  DDS::Security::DatawriterCryptoHandle writer_crypto_handle(const RepoId& writer) const;
  DDS::Security::DatareaderCryptoHandle reader_crypto_handle(const RepoId& reader) const;

  void populate_security_handles(const RepoId& local_id, const RepoId& remote_id,
                                 const unsigned char* buffer,
                                 unsigned int buffer_size);

  DDS::Security::EndpointSecurityAttributesMask security_attributes(const RepoId& endpoint) const;

  static bool separate_message(EntityId_t entity);
#endif

private:
  void join_multicast_group(const DCPS::NetworkInterface& nic,
                            bool all_interfaces = false);
  void leave_multicast_group(const DCPS::NetworkInterface& nic);
  void add_address(const DCPS::NetworkInterface& interface,
                   const ACE_INET_Addr& address);
  void remove_address(const DCPS::NetworkInterface& interface,
                      const ACE_INET_Addr& address);

  // Internal non-locking versions of the above
  AddrSet get_addresses_i(const RepoId& local, const RepoId& remote) const;
  AddrSet get_addresses_i(const RepoId& local) const;

  void get_locators_i(const RepoId& local_id,
                      AddrSet& addrs) const;

  bool get_locator_i(const RepoId& remote_id, ACE_INET_Addr& addr) const;

  virtual void stop_i();

  virtual TransportQueueElement* customize_queue_element(
    TransportQueueElement* element);

  virtual void release_remote_i(const RepoId& remote_id);
  virtual void release_reservations_i(const RepoId& remote_id,
                                      const RepoId& local_id);

  friend class ::DDS_TEST;
  /// static member used by testing code to force inline qos
  static bool force_inline_qos_;
  bool requires_inline_qos(const GUIDSeq_var & peers);

  typedef OPENDDS_MAP_CMP(RepoId, OPENDDS_VECTOR(RepoId),GUID_tKeyLessThan) DestToEntityMap;

  ReactorTask_rch reactor_task_;
  RcHandle<DCPS::JobQueue> job_queue_;

  RtpsUdpSendStrategy* send_strategy();
  RtpsUdpReceiveStrategy* receive_strategy();

  GuidPrefix_t local_prefix_;

  struct RemoteInfo {
    RemoteInfo() : addr_(), requires_inline_qos_(false) {}
    RemoteInfo(const ACE_INET_Addr& addr, bool iqos)
      : addr_(addr), requires_inline_qos_(iqos) {}
    ACE_INET_Addr addr_;
    bool requires_inline_qos_;
  };

  typedef OPENDDS_MAP_CMP(RepoId, RemoteInfo, GUID_tKeyLessThan) RemoteInfoMap;
  RemoteInfoMap locators_;

  ACE_SOCK_Dgram unicast_socket_;
  ACE_SOCK_Dgram_Mcast multicast_socket_;
  OPENDDS_SET(OPENDDS_STRING) joined_interfaces_;

  RcHandle<SingleSendBuffer> get_writer_send_buffer(const RepoId& pub_id);

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

  struct MetaSubmessage {
    MetaSubmessage(const RepoId& from, const RepoId& dst)
      : from_guid_(from), dst_guid_(dst) {}
    MetaSubmessage(const RepoId& from, const RepoId& dst, const RepoIdSet& to)
      : from_guid_(from), dst_guid_(dst), to_guids_(to) {}
    RepoId from_guid_;
    RepoId dst_guid_;
    RepoIdSet to_guids_;
    RTPS::Submessage sm_;
  };
  typedef OPENDDS_VECTOR(MetaSubmessage) MetaSubmessageVec;

  // RTPS reliability support for local writers:

  struct ReaderInfo {
    CORBA::Long acknack_recvd_count_, nackfrag_recvd_count_;
    OPENDDS_VECTOR(RTPS::SequenceNumberSet) requested_changes_;
    OPENDDS_MAP(SequenceNumber, RTPS::FragmentNumberSet) requested_frags_;
    SequenceNumber cur_cumulative_ack_;
    bool handshake_done_, durable_;
    OPENDDS_MAP(SequenceNumber, TransportQueueElement*) durable_data_;
    MonotonicTimePoint durable_timestamp_;

    explicit ReaderInfo(bool durable)
      : acknack_recvd_count_(0)
      , nackfrag_recvd_count_(0)
      , handshake_done_(false)
      , durable_(durable)
    {}
    ~ReaderInfo();
    void swap_durable_data(OPENDDS_MAP(SequenceNumber, TransportQueueElement*)& dd);
    void expire_durable_data();
    bool expecting_durable_data() const;
  };

  typedef OPENDDS_MAP_CMP(RepoId, ReaderInfo, GUID_tKeyLessThan) ReaderInfoMap;

  class RtpsWriter : public RcObject {
  protected:
    ReaderInfoMap remote_readers_;
    RcHandle<SingleSendBuffer> send_buff_;
    SequenceNumber expected_;
    typedef OPENDDS_SET(TransportQueueElement*) TqeSet;
    typedef OPENDDS_MULTIMAP(SequenceNumber, TransportQueueElement*) SnToTqeMap;
    SnToTqeMap elems_not_acked_;
    WeakRcHandle<RtpsUdpDataLink> link_;
    RepoId id_;
    bool durable_;
    CORBA::Long heartbeat_count_;
    mutable ACE_Thread_Mutex mutex_;
    mutable ACE_Thread_Mutex elems_not_acked_mutex_;

    void add_gap_submsg_i(RTPS::SubmessageSeq& msg,
                          const TransportQueueElement& tqe,
                          const DestToEntityMap& dtem);
    void end_historic_samples_i(const DataSampleHeader& header,
                                ACE_Message_Block* body);
    void send_heartbeats_manual_i(MetaSubmessageVec& meta_submessages);

    void gather_gaps_i(const RepoId& reader,
                       const DisjointSequence& gaps,
                       MetaSubmessageVec& meta_submessages);
    void acked_by_all_helper_i(TqeSet& to_deliver);
    void send_directed_nack_replies_i(const RepoId& readerId, ReaderInfo& reader, MetaSubmessageVec& meta_submessages);
    void process_requested_changes_i(DisjointSequence& requests, const ReaderInfo& reader);
    void send_nackfrag_replies_i(DisjointSequence& gaps, AddrSet& gap_recipients);

  public:
    RtpsWriter(RcHandle<RtpsUdpDataLink> link, const RepoId& id, bool durable, CORBA::Long hbc, size_t capacity);
    ~RtpsWriter();
    SequenceNumber heartbeat_high(const ReaderInfo&) const;
    void add_elem_awaiting_ack(TransportQueueElement* element);

    void send_delayed_notifications(const TransportQueueElement::MatchCriteria& criteria);
    RemoveResult remove_sample(const DataSampleElement* sample);
    void remove_all_msgs();

    bool add_reader(const RepoId& id, const ReaderInfo& info);
    bool has_reader(const RepoId& id) const;
    bool remove_reader(const RepoId& id);
    size_t reader_count() const;
    CORBA::Long get_heartbeat_count() const { return heartbeat_count_; }

    bool is_reader_handshake_done(const RepoId& id) const;
    void pre_stop_helper(OPENDDS_VECTOR(TransportQueueElement*)& to_drop);
    TransportQueueElement* customize_queue_element_helper(TransportQueueElement* element,
                                                          bool requires_inline_qos,
                                                          MetaSubmessageVec& meta_submessages,
                                                          bool& deliver_after_send);

    void process_acknack(const RTPS::AckNackSubmessage& acknack,
                         const RepoId& src,
                         MetaSubmessageVec& meta_submessages);
    void process_nackfrag(const RTPS::NackFragSubmessage& nackfrag,
                          const RepoId& src,
                          MetaSubmessageVec& meta_submessages);
    void process_acked_by_all();
    void send_and_gather_nack_replies(MetaSubmessageVec& meta_submessages);
    bool gather_heartbeats(OPENDDS_VECTOR(TransportQueueElement*)& pendingCallbacks,
                           const RepoIdSet& additional_guids,
                           bool allow_final,
                           MetaSubmessageVec& meta_submessages);
    RcHandle<SingleSendBuffer> get_send_buff() { return send_buff_; }
  };
  typedef RcHandle<RtpsWriter> RtpsWriter_rch;

  typedef OPENDDS_MAP_CMP(RepoId, RtpsWriter_rch, GUID_tKeyLessThan) RtpsWriterMap;
  RtpsWriterMap writers_;


  // RTPS reliability support for local readers:

  struct WriterInfo {
    DisjointSequence recvd_;
    OPENDDS_MAP(SequenceNumber, ReceivedDataSample) held_;
    SequenceRange hb_range_;
    OPENDDS_MAP(SequenceNumber, RTPS::FragmentNumber_t) frags_;
    bool ack_pending_, first_activity_, first_valid_hb_, first_delivered_data_;
    CORBA::Long heartbeat_recvd_count_, hb_frag_recvd_count_,
      acknack_count_, nackfrag_count_;

    WriterInfo()
      : ack_pending_(false), first_activity_(true), first_valid_hb_(true), first_delivered_data_(true)
      , heartbeat_recvd_count_(0), hb_frag_recvd_count_(0), acknack_count_(0), nackfrag_count_(0)
    { hb_range_.second = SequenceNumber::ZERO(); }

    bool should_nack() const;
  };

  typedef OPENDDS_MAP_CMP(RepoId, WriterInfo, GUID_tKeyLessThan) WriterInfoMap;

  typedef OPENDDS_VECTOR(RTPS::NackFragSubmessage) NackFragSubmessageVec;

  class RtpsReader : public RcObject {
  public:
    RtpsReader(RcHandle<RtpsUdpDataLink> link, const RepoId& id, bool durable) : link_(link), id_(id), durable_(durable), stopping_(false) {}

    bool add_writer(const RepoId& id, const WriterInfo& info);
    bool has_writer(const RepoId& id) const;
    bool remove_writer(const RepoId& id);
    size_t writer_count() const;

    bool is_writer_handshake_done(const RepoId& id) const;
    bool should_nack_durable(const WriterInfo& info);

    void pre_stop_helper();

    bool process_heartbeat_i(const RTPS::HeartBeatSubmessage& heartbeat, const RepoId& src, MetaSubmessageVec& meta_submessages);
    bool process_data_i(const RTPS::DataSubmessage& data, const RepoId& src, MetaSubmessageVec& meta_submessages);
    bool process_gap_i(const RTPS::GapSubmessage& gap, const RepoId& src, MetaSubmessageVec& meta_submessages);
    bool process_hb_frag_i(const RTPS::HeartBeatFragSubmessage& hb_frag, const RepoId& src, MetaSubmessageVec& meta_submessages);

    void gather_ack_nacks(MetaSubmessageVec& meta_submessages, bool finalFlag = false);

  protected:
    void gather_ack_nacks_i(MetaSubmessageVec& meta_submessages, bool finalFlag = false);
    void generate_nack_frags_i(NackFragSubmessageVec& nack_frags,
                               WriterInfo& wi, const RepoId& pub_id);

    mutable ACE_Thread_Mutex mutex_;
    WeakRcHandle<RtpsUdpDataLink> link_;
    RepoId id_;
    bool durable_;
    WriterInfoMap remote_writers_;
    bool stopping_;
  };
  typedef RcHandle<RtpsReader> RtpsReader_rch;

  typedef OPENDDS_VECTOR(MetaSubmessageVec::iterator) MetaSubmessageIterVec;
  typedef OPENDDS_MAP_CMP(RepoId, MetaSubmessageIterVec, GUID_tKeyLessThan) DestMetaSubmessageMap;
  typedef OPENDDS_MAP(AddrSet, DestMetaSubmessageMap) AddrDestMetaSubmessageMap;
  typedef OPENDDS_VECTOR(MetaSubmessageIterVec) MetaSubmessageIterVecVec;

  void build_meta_submessage_map(MetaSubmessageVec& meta_submessages, AddrDestMetaSubmessageMap& adr_map);
  void bundle_mapped_meta_submessages(AddrDestMetaSubmessageMap& adr_map,
                               MetaSubmessageIterVecVec& meta_submessage_bundles,
                               OPENDDS_VECTOR(AddrSet)& meta_submessage_bundle_addrs,
                               OPENDDS_VECTOR(size_t)& meta_submessage_bundle_sizes);
  void send_bundled_submessages(MetaSubmessageVec& meta_submessages);

  RepoIdSet pending_reliable_readers_;

  typedef OPENDDS_MAP_CMP(RepoId, RtpsReader_rch, GUID_tKeyLessThan) RtpsReaderMap;
  RtpsReaderMap readers_;

  typedef OPENDDS_MULTIMAP_CMP(RepoId, RtpsReader_rch, GUID_tKeyLessThan) RtpsReaderMultiMap;
  RtpsReaderMultiMap readers_of_writer_; // keys are remote data writer GUIDs

  WriterToSeqReadersMap writer_to_seq_best_effort_readers_;

  void deliver_held_data(const RepoId& readerId, WriterInfo& info, bool durable);

  /// What was once a single lock for the whole datalink is now split between three (four including ch_lock_):
  /// - readers_lock_ protects readers_, readers_of_writer_, pending_reliable_readers_, interesting_writers_, and
  ///   writer_to_seq_best_effort_readers_ along with anything else that fits the 'reader side activity' of the datalink
  /// - writers_lock_ protects writers_, heartbeat_counts_, best_effort_heartbeat_count_, and interesting_readers_
  ///   along with anything else that fits the 'writers side activity' of the datalink
  /// - locators_lock_ protects locators_ (and therefore calls to get_addresses_i())
  ///   for both remote writers and remote readers
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
                                  CORBA::ULong extent);

  void durability_resend(TransportQueueElement* element);

  template<typename T, typename FN>
  void datawriter_dispatch(const T& submessage, const GuidPrefix_t& src_prefix,
                           const FN& func)
  {
    RepoId local;
    std::memcpy(local.guidPrefix, local_prefix_, sizeof(GuidPrefix_t));
    local.entityId = submessage.writerId;

    RepoId src;
    std::memcpy(src.guidPrefix, src_prefix, sizeof(GuidPrefix_t));
    src.entityId = submessage.readerId;

    OPENDDS_VECTOR(RtpsWriter_rch) to_call;
    {
      ACE_GUARD(ACE_Thread_Mutex, g, writers_lock_);
      const RtpsWriterMap::iterator rw = writers_.find(local);
      if (rw == writers_.end()) {
        return;
      }
      to_call.push_back(rw->second);
    }
    MetaSubmessageVec meta_submessages;
    for (OPENDDS_VECTOR(RtpsWriter_rch)::const_iterator it = to_call.begin(); it < to_call.end(); ++it) {
      RtpsWriter& writer = **it;
      (writer.*func)(submessage, src, meta_submessages);
    }
    send_bundled_submessages(meta_submessages);
  }

  template<typename T, typename FN>
  void datareader_dispatch(const T& submessage, const GuidPrefix_t& src_prefix,
                           const FN& func)
  {
    RepoId local;
    std::memcpy(local.guidPrefix, local_prefix_, sizeof(GuidPrefix_t));
    local.entityId = submessage.readerId;

    RepoId src;
    std::memcpy(src.guidPrefix, src_prefix, sizeof(GuidPrefix_t));
    src.entityId = submessage.writerId;

    bool schedule_timer = false;
    OPENDDS_VECTOR(RtpsReader_rch) to_call;
    {
      ACE_GUARD(ACE_Thread_Mutex, g, readers_lock_);
      if (local.entityId == ENTITYID_UNKNOWN) {
        typedef std::pair<RtpsReaderMultiMap::iterator, RtpsReaderMultiMap::iterator> RRMM_IterRange;
        for (RRMM_IterRange iters = readers_of_writer_.equal_range(src); iters.first != iters.second; ++iters.first) {
          to_call.push_back(iters.first->second);
        }
      } else {
        const RtpsReaderMap::iterator rr = readers_.find(local);
        if (rr == readers_.end()) {
          return;
        }
        to_call.push_back(rr->second);
      }
    }
    MetaSubmessageVec meta_submessages;
    for (OPENDDS_VECTOR(RtpsReader_rch)::const_iterator it = to_call.begin(); it < to_call.end(); ++it) {
      RtpsReader& reader = **it;
      schedule_timer |= (reader.*func)(submessage, src, meta_submessages);
    }
    send_bundled_submessages(meta_submessages);
    if (schedule_timer) {
      heartbeat_reply_.schedule();
    }
  }

  void send_nack_replies();
  void send_heartbeats(const DCPS::MonotonicTimePoint& now);
  void send_directed_heartbeats(OPENDDS_VECTOR(RTPS::HeartBeatSubmessage)& hbs);
  void check_heartbeats(const DCPS::MonotonicTimePoint& now);
  void send_heartbeat_replies();
  void send_relay_beacon(const DCPS::MonotonicTimePoint& now);

  CORBA::Long best_effort_heartbeat_count_;

  typedef void (RtpsUdpDataLink::*PMF)();

  struct TimedDelay : ACE_Event_Handler {

    TimedDelay(RtpsUdpDataLink* outer, PMF function,
               const TimeDuration& timeout)
      : outer_(outer)
      , function_(function)
      , timeout_(timeout)
    {}

    void schedule(const TimeDuration& timeout = TimeDuration::zero_value);
    void cancel();

    int handle_timeout(const ACE_Time_Value&, const void*)
    {
      scheduled_ = MonotonicTimePoint::zero_value;
      (outer_->*function_)();
      return 0;
    }

    RtpsUdpDataLink* outer_;
    PMF function_;
    TimeDuration timeout_;
    MonotonicTimePoint scheduled_;

  } nack_reply_, heartbeat_reply_;

  typedef PmfPeriodicTask<RtpsUdpDataLink> Periodic;
  Periodic heartbeat_, heartbeatchecker_, relay_beacon_;

  /// Data structure representing an "interesting" remote entity for static discovery.
  struct InterestingRemote {
    /// id of local entity that is interested in this remote.
    RepoId localid;
    /// address of this entity
    ACE_INET_Addr address;
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
    InterestingRemote(const RepoId& w, const ACE_INET_Addr& a, DiscoveryListener* l)
      : localid(w)
      , address(a)
      , listener(l)
      , status(DOES_NOT_EXIST)
    { }
  };
  typedef OPENDDS_MULTIMAP_CMP(RepoId, InterestingRemote, DCPS::GUID_tKeyLessThan) InterestingRemoteMapType;
  InterestingRemoteMapType interesting_readers_;
  InterestingRemoteMapType interesting_writers_;

  typedef std::pair<RepoId, InterestingRemote> CallbackType;

  TransportQueueElement* customize_queue_element_non_reliable_i(TransportQueueElement* element,
                                                                bool requires_inline_qos,
                                                                MetaSubmessageVec& meta_submessages,
                                                                bool& deliver_after_send,
                                                                ACE_Guard<ACE_Thread_Mutex>& guard);

  void send_heartbeats_manual_i(const TransportSendControlElement* tsce,
                                MetaSubmessageVec& meta_submessages);

  typedef OPENDDS_MAP_CMP(RepoId, CORBA::Long, DCPS::GUID_tKeyLessThan) HeartBeatCountMapType;
  HeartBeatCountMapType heartbeat_counts_;

  struct InterestingAckNack {
    RepoId writerid;
    RepoId readerid;
    ACE_INET_Addr writer_address;

    InterestingAckNack() { }
    InterestingAckNack(const RepoId& w, const RepoId& r, const ACE_INET_Addr& wa)
      : writerid(w)
      , readerid(r)
      , writer_address(wa)
    { }

    bool operator<(const InterestingAckNack& other) const {
      if (writerid != other.writerid) {
        return DCPS::GUID_tKeyLessThan() (writerid, other.writerid);
      }
      return DCPS::GUID_tKeyLessThan() (readerid, other.readerid);
    }
  };

  typedef OPENDDS_SET(InterestingAckNack) InterestingAckNackSetType;
  InterestingAckNackSetType interesting_ack_nacks_;

  class HeldDataDeliveryHandler : public RcEventHandler {
  public:
    HeldDataDeliveryHandler(RtpsUdpDataLink* link)
      : link_(link) {
      }

    /// Reactor invokes this after being notified in schedule_stop or cancel_release
    int handle_exception(ACE_HANDLE /* fd */);

    void notify_delivery(const RepoId& readerId, WriterInfo& info);

    virtual ACE_Event_Handler::Reference_Count add_reference();
    virtual ACE_Event_Handler::Reference_Count remove_reference();
  private:
    RtpsUdpDataLink* link_;
    typedef std::pair<ReceivedDataSample, RepoId> HeldDataEntry;
    typedef OPENDDS_VECTOR(HeldDataEntry) HeldData;
    HeldData held_data_;
  };
  HeldDataDeliveryHandler held_data_delivery_handler_;
  const size_t max_bundle_size_;
  TimeDuration quick_reply_delay_;

#ifdef OPENDDS_SECURITY
  mutable ACE_Thread_Mutex ch_lock_;
  Security::SecurityConfig_rch security_config_;
  DDS::Security::ParticipantCryptoHandle local_crypto_handle_;

  typedef OPENDDS_MAP_CMP(RepoId, DDS::Security::NativeCryptoHandle,
                          GUID_tKeyLessThan) PeerHandlesMap;
  typedef OPENDDS_MAP_CMP(RepoId, DDS::Security::NativeCryptoHandle,
                          GUID_tKeyLessThan)::const_iterator PeerHandlesCIter;
  PeerHandlesMap peer_crypto_handles_;

  typedef OPENDDS_MAP_CMP(RepoId, DDS::Security::EndpointSecurityAttributesMask,
                          GUID_tKeyLessThan) EndpointSecurityAttributesMap;
  EndpointSecurityAttributesMap endpoint_security_attributes_;
#endif

  void accumulate_addresses(const RepoId& local, const RepoId& remote, AddrSet& addresses) const;

  struct ChangeMulticastGroup : public JobQueue::Job {
    enum CmgAction {CMG_JOIN, CMG_LEAVE};

    ChangeMulticastGroup(RcHandle<RtpsUdpDataLink> link,
                         const NetworkInterface& nic, CmgAction action)
      : link_(link)
      , nic_(nic)
      , action_(action)
    {}

    void execute()
    {
      if (action_ == CMG_JOIN) {
        link_->join_multicast_group(nic_);
      } else {
        link_->leave_multicast_group(nic_);
      }
    }

    RcHandle<RtpsUdpDataLink> link_;
    NetworkInterface nic_;
    CmgAction action_;
  };
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#ifdef __ACE_INLINE__
# include "RtpsUdpDataLink.inl"
#endif  /* __ACE_INLINE__ */

#endif  /* DCPS_RTPSUDPDATALINK_H */
