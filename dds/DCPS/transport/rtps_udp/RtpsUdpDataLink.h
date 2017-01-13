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
#include "dds/DCPS/transport/framework/TransportReactorTask.h"
#include "dds/DCPS/transport/framework/TransportReactorTask_rch.h"
#include "dds/DCPS/transport/framework/TransportSendBuffer.h"

#include "dds/DCPS/DataSampleElement.h"
#include "dds/DCPS/DisjointSequence.h"
#include "dds/DCPS/GuidConverter.h"
#include "dds/DCPS/PoolAllocator.h"
#include "dds/DCPS/DiscoveryListener.h"
#include "dds/DCPS/ReactorInterceptor.h"
#include "dds/DCPS/RcEventHandler.h"


class DDS_TEST;

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class RtpsUdpInst;
class RtpsUdpTransport;
class ReceivedDataSample;
typedef RcHandle<RtpsUdpInst> RtpsUdpInst_rch;
typedef RcHandle<RtpsUdpTransport> RtpsUdpTransport_rch;

class OpenDDS_Rtps_Udp_Export RtpsUdpDataLink : public DataLink {
public:

  RtpsUdpDataLink(const RtpsUdpTransport_rch& transport,
                  const GuidPrefix_t& local_prefix,
                  const RtpsUdpInst_rch& config,
                  const TransportReactorTask_rch& reactor_task);

  bool add_delayed_notification(TransportQueueElement* element);
  void do_remove_sample(const RepoId& pub_id, const TransportQueueElement::MatchCriteria& criteria);
  RtpsUdpInst_rch config() const;

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

  /// Given a 'local_id' of a publication or subscription, populate the set of
  /// 'addrs' with the network addresses of any remote peers (or if 'local_id'
  /// is GUID_UNKNOWN, all known addresses).
  void get_locators(const RepoId& local_id,
                    OPENDDS_SET(ACE_INET_Addr)& addrs) const;

  ACE_INET_Addr get_locator(const RepoId& remote_id) const;

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

  virtual void send_final_acks (const RepoId& readerid);

private:
  virtual void stop_i();
  virtual void send_i(TransportQueueElement* element, bool relink = true);

  virtual TransportQueueElement* customize_queue_element(
    TransportQueueElement* element);

  virtual void release_remote_i(const RepoId& remote_id);
  virtual void release_reservations_i(const RepoId& remote_id,
                                      const RepoId& local_id);

  friend class ::DDS_TEST;
  /// static member used by testing code to force inline qos
  static bool force_inline_qos_;
  bool requires_inline_qos(const PublicationId& pub_id);

  typedef OPENDDS_MAP_CMP(RepoId, OPENDDS_VECTOR(RepoId),GUID_tKeyLessThan) DestToEntityMap;
  void add_gap_submsg(RTPS::SubmessageSeq& msg,
                      const TransportQueueElement& tqe,
                      const DestToEntityMap& dtem);

  RtpsUdpInst_rch config_;
  TransportReactorTask_rch reactor_task_;

  RtpsUdpSendStrategy_rch send_strategy_;
  RtpsUdpReceiveStrategy_rch recv_strategy_;

  GuidPrefix_t local_prefix_;

  struct RemoteInfo {
    RemoteInfo() : addr_(), requires_inline_qos_(false) {}
    RemoteInfo(const ACE_INET_Addr& addr, bool iqos)
      : addr_(addr), requires_inline_qos_(iqos) {}
    ACE_INET_Addr addr_;
    bool requires_inline_qos_;
  };
  OPENDDS_MAP_CMP(RepoId, RemoteInfo, GUID_tKeyLessThan) locators_;

  ACE_SOCK_Dgram unicast_socket_;
  ACE_SOCK_Dgram_Mcast multicast_socket_;

  RtpsCustomizedElementAllocator rtps_customized_element_allocator_;

  struct MultiSendBuffer : TransportSendBuffer {

    MultiSendBuffer(RtpsUdpDataLink* outer, size_t capacity)
      : TransportSendBuffer(capacity)
      , outer_(outer)
    {}

    void retain_all(RepoId pub_id);
    void insert(SequenceNumber sequence,
                TransportSendStrategy::QueueType* queue,
                ACE_Message_Block* chain);

    RtpsUdpDataLink* outer_;

  } multi_buff_;


  // RTPS reliability support for local writers:

  struct ReaderInfo {
    CORBA::Long acknack_recvd_count_, nackfrag_recvd_count_;
    OPENDDS_VECTOR(RTPS::SequenceNumberSet) requested_changes_;
    OPENDDS_MAP(SequenceNumber, RTPS::FragmentNumberSet) requested_frags_;
    SequenceNumber cur_cumulative_ack_;
    bool handshake_done_, durable_;
    OPENDDS_MAP(SequenceNumber, TransportQueueElement*) durable_data_;
    ACE_Time_Value durable_timestamp_;

    ReaderInfo()
      : acknack_recvd_count_(0)
      , nackfrag_recvd_count_(0)
      , handshake_done_(false)
      , durable_(false)
    {}
    ~ReaderInfo();
    void expire_durable_data();
    bool expecting_durable_data() const;
  };

  typedef OPENDDS_MAP_CMP(RepoId, ReaderInfo, GUID_tKeyLessThan) ReaderInfoMap;

  struct RtpsWriter {
    ReaderInfoMap remote_readers_;
    RcHandle<SingleSendBuffer> send_buff_;
    SequenceNumber expected_;
    typedef OPENDDS_MULTIMAP(SequenceNumber, TransportQueueElement*) SnToTqeMap;
    SnToTqeMap elems_not_acked_;
    //Only accessed with RtpsUdpDataLink lock held
    SnToTqeMap to_deliver_;
    bool durable_;

    RtpsWriter() : durable_(false) {}
    ~RtpsWriter();
    SequenceNumber heartbeat_high(const ReaderInfo&) const;
    void add_elem_awaiting_ack(TransportQueueElement* element);
  };

  typedef OPENDDS_MAP_CMP(RepoId, RtpsWriter, GUID_tKeyLessThan) RtpsWriterMap;
  RtpsWriterMap writers_;

  void end_historic_samples(RtpsWriterMap::iterator writer,
                            const DataSampleHeader& header,
                            ACE_Message_Block* body);


  // RTPS reliability support for local readers:

  struct WriterInfo {
    DisjointSequence recvd_;
    OPENDDS_MAP(SequenceNumber, ReceivedDataSample) held_;
    SequenceRange hb_range_;
    OPENDDS_MAP(SequenceNumber, RTPS::FragmentNumber_t) frags_;
    bool ack_pending_, initial_hb_;
    CORBA::Long heartbeat_recvd_count_, hb_frag_recvd_count_,
      acknack_count_, nackfrag_count_;

    WriterInfo()
      : ack_pending_(false), initial_hb_(true), heartbeat_recvd_count_(0),
        hb_frag_recvd_count_(0), acknack_count_(0), nackfrag_count_(0) {}

    bool should_nack() const;
  };

  typedef OPENDDS_MAP_CMP(RepoId, WriterInfo, GUID_tKeyLessThan) WriterInfoMap;

  struct RtpsReader {
    WriterInfoMap remote_writers_;
    bool durable_;
    bool nack_durable(const WriterInfo& info);
  };

  typedef OPENDDS_MAP_CMP(RepoId, RtpsReader, GUID_tKeyLessThan) RtpsReaderMap;
  RtpsReaderMap readers_;

  typedef OPENDDS_MULTIMAP_CMP(RepoId, RtpsReaderMap::iterator, GUID_tKeyLessThan)
    RtpsReaderIndex;
  RtpsReaderIndex reader_index_; // keys are remote data writer GUIDs

  void deliver_held_data(const RepoId& readerId, WriterInfo& info, bool durable);

  /// lock_ protects data structures accessed by both the transport's thread
  /// (TransportReactorTask) and an external thread which is responsible
  /// for adding/removing associations from the DataLink.
  mutable ACE_Thread_Mutex lock_;

  size_t generate_nack_frags(OPENDDS_VECTOR(RTPS::NackFragSubmessage)& nack_frags,
                             WriterInfo& wi, const RepoId& pub_id);

  /// Extend the FragmentNumberSet to cover the fragments that are
  /// missing from our last known fragment to the extent
  /// @param fnSet FragmentNumberSet for the message sequence number
  /// in question
  /// @param extent is the highest fragment sequence number for this
  /// FragmentNumberSet
  static void extend_bitmap_range(RTPS::FragmentNumberSet& fnSet,
                                  CORBA::ULong extent);

  bool process_heartbeat_i(const RTPS::HeartBeatSubmessage& heartbeat,
                           const RepoId& src, RtpsReaderMap::value_type& rr);

  bool process_hb_frag_i(const RTPS::HeartBeatFragSubmessage& hb_frag,
                         const RepoId& src, RtpsReaderMap::value_type& rr);

  bool process_gap_i(const RTPS::GapSubmessage& gap, const RepoId& src,
                     RtpsReaderMap::value_type& rr);

  bool process_data_i(const RTPS::DataSubmessage& data, const RepoId& src,
                      RtpsReaderMap::value_type& rr);

  void durability_resend(TransportQueueElement* element);
  void send_durability_gaps(const RepoId& writer, const RepoId& reader,
                            const DisjointSequence& gaps);
  ACE_Message_Block* marshal_gaps(const RepoId& writer, const RepoId& reader,
                                  const DisjointSequence& gaps,
                                  bool durable = false);

  void send_nackfrag_replies(RtpsWriter& writer, DisjointSequence& gaps,
                             OPENDDS_SET(ACE_INET_Addr)& gap_recipients);

  template<typename T, typename FN>
  void datareader_dispatch(const T& submessage, const GuidPrefix_t& src_prefix,
                           const FN& func)
  {
    using std::pair;
    RepoId local;
    std::memcpy(local.guidPrefix, local_prefix_, sizeof(GuidPrefix_t));
    local.entityId = submessage.readerId;

    RepoId src;
    std::memcpy(src.guidPrefix, src_prefix, sizeof(GuidPrefix_t));
    src.entityId = submessage.writerId;

    bool schedule_timer = false;
    ACE_GUARD(ACE_Thread_Mutex, g, lock_);
    if (local.entityId == ENTITYID_UNKNOWN) {
      for (pair<RtpsReaderIndex::iterator, RtpsReaderIndex::iterator> iters =
             reader_index_.equal_range(src);
           iters.first != iters.second; ++iters.first) {
        schedule_timer |= (this->*func)(submessage, src, *iters.first->second);
      }

    } else {
      const RtpsReaderMap::iterator rr = readers_.find(local);
      if (rr == readers_.end()) {
        return;
      }
      schedule_timer = (this->*func)(submessage, src, *rr);
    }
    g.release();
    if (schedule_timer) {
      heartbeat_reply_.schedule();
    }
  }


  // Timers for reliability:
  void send_nack_replies();
  void process_acked_by_all_i(ACE_Guard<ACE_Thread_Mutex>& g, const RepoId& pub_id);
  void send_heartbeats();
  void check_heartbeats();
  void send_heartbeats_manual(const TransportSendControlElement* tsce);
  void send_heartbeat_replies();

  CORBA::Long best_effort_heartbeat_count_;

  typedef void (RtpsUdpDataLink::*PMF)();

  struct TimedDelay : ACE_Event_Handler {

    TimedDelay(RtpsUdpDataLink* outer, PMF function,
               const ACE_Time_Value& timeout)
      : outer_(outer), function_(function), timeout_(timeout), scheduled_(false)
    {}

    void schedule();
    void cancel();

    int handle_timeout(const ACE_Time_Value&, const void*)
    {
      scheduled_ = false;
      (outer_->*function_)();
      return 0;
    }

    RtpsUdpDataLink* outer_;
    PMF function_;
    ACE_Time_Value timeout_;
    bool scheduled_;

  } nack_reply_, heartbeat_reply_;

  struct HeartBeat : ReactorInterceptor {

    explicit HeartBeat(ACE_Reactor* reactor, ACE_thread_t owner, RtpsUdpDataLink* outer, PMF function)
      : ReactorInterceptor(reactor, owner)
      , outer_(outer)
      , function_(function)
      , enabled_(false) {}

    void schedule_enable()
    {
      ScheduleEnableCommand c(this);
      execute_or_enqueue(c);
    }

    int handle_timeout(const ACE_Time_Value&, const void*)
    {
      (outer_->*function_)();
      return 0;
    }

    bool reactor_is_shut_down() const
    {
      return outer_->reactor_is_shut_down();
    }

    void enable();
    void disable();

    RtpsUdpDataLink* outer_;
    PMF function_;
    bool enabled_;

    struct ScheduleEnableCommand : public Command {
      ScheduleEnableCommand(HeartBeat* hb)
        : heartbeat_(hb)
      { }

      virtual void execute()
      {
        heartbeat_->enable();
      }

      HeartBeat* heartbeat_;
    };

  };

  RcHandle<HeartBeat> heartbeat_, heartbeatchecker_;

  /// Data structure representing an "interesting" remote entity for static discovery.
  struct InterestingRemote {
    /// id of local entity that is interested in this remote.
    RepoId localid;
    /// address of this entity
    ACE_INET_Addr address;
    /// Callback to invoke.
    DiscoveryListener* listener;
    /// Timestamp indicating the last HeartBeat or AckNack received from the remote entity
    ACE_Time_Value last_activity;
    /// Current status of the remote entity.
    enum { DOES_NOT_EXIST, EXISTS } status;

    InterestingRemote() { }
    InterestingRemote(const RepoId& w, const ACE_INET_Addr& a, DiscoveryListener* l)
      : localid(w)
      , address(a)
      , listener(l)
      //, heartbeat_count(0)
      , status(DOES_NOT_EXIST)
    { }
  };
  typedef OPENDDS_MULTIMAP_CMP(RepoId, InterestingRemote, DCPS::GUID_tKeyLessThan) InterestingRemoteMapType;
  InterestingRemoteMapType interesting_readers_;
  InterestingRemoteMapType interesting_writers_;

  mutable ACE_Thread_Mutex writer_no_longer_exists_lock_,
                           reader_no_longer_exists_lock_;

  typedef std::pair<RepoId, InterestingRemote> CallbackType;
  OPENDDS_VECTOR(CallbackType) writerDoesNotExistCallbacks_;
  OPENDDS_VECTOR(CallbackType) readerDoesNotExistCallbacks_;


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

  void send_ack_nacks(RtpsReaderMap::iterator rr, bool finalFlag = false);
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#ifdef __ACE_INLINE__
# include "RtpsUdpDataLink.inl"
#endif  /* __ACE_INLINE__ */

#endif  /* DCPS_RTPSUDPDATALINK_H */
