/*
 * $Id$
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

#include <map>
#include <set>

class DDS_TEST;

namespace OpenDDS {
namespace DCPS {

class RtpsUdpInst;
class RtpsUdpTransport;
class ReceivedDataSample;

class OpenDDS_Rtps_Udp_Export RtpsUdpDataLink : public DataLink {
public:

  RtpsUdpDataLink(RtpsUdpTransport* transport,
                  const GuidPrefix_t& local_prefix,
                  RtpsUdpInst* config,
                  TransportReactorTask* reactor_task);

  void send_strategy(RtpsUdpSendStrategy* send_strategy);
  void receive_strategy(RtpsUdpReceiveStrategy* recv_strategy);
  bool add_delayed_notification(TransportQueueElement* element);
  void do_remove_sample(const RepoId& pub_id, const TransportQueueElement::MatchCriteria& criteria);
  RtpsUdpInst* config();

  ACE_Reactor* get_reactor();

  ACE_SOCK_Dgram& unicast_socket();
  ACE_SOCK_Dgram_Mcast& multicast_socket();

#ifdef ACE_HAS_IPV6
  ACE_SOCK_Dgram& socket_for(int address_type);
#endif

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
                    std::set<ACE_INET_Addr>& addrs) const;

  ACE_INET_Addr get_locator(const RepoId& remote_id) const;

  void associated(const RepoId& local, const RepoId& remote,
                  bool local_reliable, bool remote_reliable,
                  bool local_durable, bool remote_durable);

  bool check_handshake_complete(const RepoId& local, const RepoId& remote);

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

  void add_gap_submsg(RTPS::SubmessageSeq& msg,
                      const TransportQueueElement& tqe);

#ifdef ACE_LYNXOS_MAJOR
public:
#endif

  RtpsUdpInst* config_;
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
#ifdef ACE_HAS_IPV6
  ACE_SOCK_Dgram ipv6_alternate_socket_;
  int unicast_socket_type_;
#endif

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
    std::vector<RTPS::SequenceNumberSet> requested_changes_;
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
    OPENDDS_MAP(SequenceNumber, TransportQueueElement*) elems_not_acked_;
    CORBA::Long heartbeat_count_;
    bool durable_;

    RtpsWriter() : heartbeat_count_(0), durable_(false) {}
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
  };

  typedef OPENDDS_MAP_CMP(RepoId, RtpsReader, GUID_tKeyLessThan) RtpsReaderMap;
  RtpsReaderMap readers_;

  typedef std::multimap<RepoId, RtpsReaderMap::iterator, GUID_tKeyLessThan>
    RtpsReaderIndex;
  RtpsReaderIndex reader_index_; // keys are remote data writer GUIDs

  void deliver_held_data(const RepoId& readerId, WriterInfo& info, bool durable);

  /// lock_ protects data structures accessed by both the transport's thread
  /// (TransportReactorTask) and an external thread which is responsible
  /// for adding/removing associations from the DataLink.
  mutable ACE_Thread_Mutex lock_;

  size_t generate_nack_frags(std::vector<RTPS::NackFragSubmessage>& nack_frags,
                             WriterInfo& wi, const RepoId& pub_id);

  /// Extend the FragmentNumberSet to cover the fragments that are
  /// missing from our last known fragment to the extent
  /// @param fnSet FragmentNumberSet for the message sequence number
  /// in question
  /// @param extent is the highest fragement sequence number for this
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
                             std::set<ACE_INET_Addr>& gap_recipients);

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

#ifdef ACE_LYNXOS_MAJOR
public:
#endif
  void send_nack_replies();
  void process_acked_by_all();
  void send_heartbeats();
  void send_heartbeats_manual(const TransportSendControlElement* tsce);
  void send_heartbeat_replies();
#ifdef ACE_LYNXOS_MAJOR
private:
#endif

  CORBA::Long best_effort_heartbeat_count_;

  struct TimedDelay : ACE_Event_Handler {

    typedef void (RtpsUdpDataLink::*PMF)();

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

  } nack_reply_, heartbeat_reply_, acked_by_all_check_;


  struct HeartBeat : ACE_Event_Handler {

    explicit HeartBeat(RtpsUdpDataLink* outer)
      : outer_(outer), enabled_(false) {}

    int handle_timeout(const ACE_Time_Value&, const void*)
    {
      outer_->send_heartbeats();
      return 0;
    }

    void enable();
    void disable();

    RtpsUdpDataLink* outer_;
    bool enabled_;

  } heartbeat_;

  RTPS::InfoReplySubmessage info_reply_;
};

} // namespace DCPS
} // namespace OpenDDS

#ifdef __ACE_INLINE__
# include "RtpsUdpDataLink.inl"
#endif  /* __ACE_INLINE__ */

#endif  /* DCPS_RTPSUDPDATALINK_H */
