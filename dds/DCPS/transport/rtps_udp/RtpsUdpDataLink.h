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

#include "ace/Basic_Types.h"
#include "ace/SOCK_Dgram.h"
#include "ace/SOCK_Dgram_Mcast.h"

#include "dds/DCPS/transport/framework/DataLink.h"
#include "dds/DCPS/transport/framework/TransportReactorTask.h"
#include "dds/DCPS/transport/framework/TransportReactorTask_rch.h"
#include "dds/DCPS/transport/framework/TransportSendBuffer.h"

#include "dds/DCPS/DataSampleList.h"
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

  RtpsUdpInst* config();

  ACE_Reactor* get_reactor();

  ACE_SOCK_Dgram& unicast_socket();
  ACE_SOCK_Dgram_Mcast& multicast_socket();

  bool open();

  void received(const OpenDDS::RTPS::DataSubmessage& data,
                const GuidPrefix_t& src_prefix,
                const GuidPrefix_t& dst_prefix);

  void received(const OpenDDS::RTPS::GapSubmessage& gap,
                const GuidPrefix_t& src_prefix,
                const GuidPrefix_t& dst_prefix);

  void received(const OpenDDS::RTPS::HeartBeatSubmessage& heartbeat,
                const GuidPrefix_t& src_prefix,
                const GuidPrefix_t& dst_prefix);

  void received(const OpenDDS::RTPS::AckNackSubmessage& acknack,
                const GuidPrefix_t& src_prefix,
                const GuidPrefix_t& dst_prefix);

  const GuidPrefix_t& local_prefix() const { return local_prefix_; }

  void add_locator(const RepoId& remote_id, const ACE_INET_Addr& address);

  /// Given a 'local_id' of a publication or subscription, populate the set of
  /// 'addrs' with the network addresses of any remote peers (or if 'local_id'
  /// is GUID_UNKNOWN, all known addresses).
  void get_locators(const RepoId& local_id,
                    std::set<ACE_INET_Addr>& addrs) const;

  void associated(const RepoId& local, const RepoId& remote, bool reliable);

private:
  virtual void stop_i();

  virtual TransportQueueElement* customize_queue_element(
    TransportQueueElement* element);

  virtual void release_remote_i(const RepoId& remote_id);
  virtual void release_reservations_i(const RepoId& remote_id,
                                      const RepoId& local_id);

  friend class ::DDS_TEST;
  /// static member used by testing code to force inline qos
  static bool force_inline_qos_;
  bool requires_inline_qos(const PublicationId& pub_id);

  void add_gap_submsg(OpenDDS::RTPS::SubmessageSeq& msg,
                      const TransportQueueElement& tqe);

  RtpsUdpInst* config_;
  TransportReactorTask_rch reactor_task_;

  RtpsUdpSendStrategy_rch send_strategy_;
  RtpsUdpReceiveStrategy_rch recv_strategy_;

  GuidPrefix_t local_prefix_;
  std::map<RepoId, ACE_INET_Addr, GUID_tKeyLessThan> locators_;

  ACE_SOCK_Dgram unicast_socket_;
  ACE_SOCK_Dgram_Mcast multicast_socket_;

  TransportCustomizedElementAllocator transport_customized_element_allocator_;

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
    CORBA::Long acknack_recvd_count_;
    std::vector<OpenDDS::RTPS::SequenceNumberSet> requested_changes_;

    ReaderInfo() : acknack_recvd_count_(0) {}
  };

  typedef std::map<RepoId, ReaderInfo, GUID_tKeyLessThan> ReaderInfoMap;

  struct RtpsWriter {
    ReaderInfoMap remote_readers_;
    RcHandle<SingleSendBuffer> send_buff_;
    SequenceNumber last_sent_;
    CORBA::Long heartbeat_count_;

    RtpsWriter() : heartbeat_count_(0) {}
  };

  typedef std::map<RepoId, RtpsWriter, GUID_tKeyLessThan> RtpsWriterMap;
  RtpsWriterMap writers_;


  // RTPS reliability support for local readers:

  struct WriterInfo {
    DisjointSequence recvd_;
    bool ack_pending_;
    CORBA::Long heartbeat_recvd_count_, acknack_count_;

    WriterInfo()
      : ack_pending_(false), heartbeat_recvd_count_(0), acknack_count_(0) {}
  };

  typedef std::map<RepoId, WriterInfo, GUID_tKeyLessThan> WriterInfoMap;

  struct RtpsReader {
    WriterInfoMap remote_writers_;
  };

  typedef std::map<RepoId, RtpsReader, GUID_tKeyLessThan> RtpsReaderMap;
  RtpsReaderMap readers_;

  typedef std::multimap<RepoId, RtpsReaderMap::iterator, GUID_tKeyLessThan>
    RtpsReaderIndex;
  RtpsReaderIndex reader_index_; // keys are remote data writer GUIDs

  void process_heartbeat_i(const OpenDDS::RTPS::HeartBeatSubmessage& heartbeat,
                           const RepoId& src, RtpsReaderMap::value_type& rr);

  void process_gap_i(const OpenDDS::RTPS::GapSubmessage& gap,
                     const RepoId& src, RtpsReaderMap::value_type& rr);

  void process_data_i(const OpenDDS::RTPS::DataSubmessage& data,
                      const RepoId& src, RtpsReaderMap::value_type& rr);


  template<typename T, typename FN>
  void datareader_dispatch(const T& submessage, const GuidPrefix_t& src_prefix,
                           const GuidPrefix_t& dst_prefix, const FN& func)
  {
    using std::pair;
    RepoId local;
    std::memcpy(local.guidPrefix, dst_prefix, sizeof(GuidPrefix_t));
    local.entityId = submessage.readerId; 

    RepoId src;
    std::memcpy(src.guidPrefix, src_prefix, sizeof(GuidPrefix_t));
    src.entityId = submessage.writerId;

    if (local.entityId == ENTITYID_UNKNOWN) {
      for (pair<RtpsReaderIndex::iterator, RtpsReaderIndex::iterator> iters =
             reader_index_.equal_range(src);
           iters.first != iters.second; ++iters.first) {
        (this->*func)(submessage, src, *iters.first->second);
      }

    } else {
      const RtpsReaderMap::iterator rr = readers_.find(local);
      if (rr == readers_.end()) {
        return;
      }
      (this->*func)(submessage, src, *rr);
    }
  }

  // Timers for reliability:

  void send_nack_replies();
  void send_heartbeats();
  void send_heartbeat_replies();

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

  } nack_reply_, heartbeat_reply_;


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

  OpenDDS::RTPS::InfoDestinationSubmessage info_dst_;
  OpenDDS::RTPS::InfoReplySubmessage info_reply_;
};

} // namespace DCPS
} // namespace OpenDDS

#ifdef __ACE_INLINE__
# include "RtpsUdpDataLink.inl"
#endif  /* __ACE_INLINE__ */

#endif  /* DCPS_RTPSUDPDATALINK_H */
