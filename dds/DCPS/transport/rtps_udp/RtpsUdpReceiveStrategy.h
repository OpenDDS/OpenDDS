/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TRANSPORT_RTPS_UDP_RTPSUDPRECEIVESTRATEGY_H
#define OPENDDS_DCPS_TRANSPORT_RTPS_UDP_RTPSUDPRECEIVESTRATEGY_H

#include "Rtps_Udp_Export.h"
#include "RtpsTransportHeader.h"
#include "RtpsSampleHeader.h"

#include "dds/DCPS/transport/framework/TransportReceiveStrategy_T.h"

#include "dds/DCPS/RTPS/RtpsCoreC.h"
#include "dds/DCPS/RTPS/ICE/Ice.h"
#include "dds/DCPS/RcEventHandler.h"
#include "dds/DCPS/NetworkAddress.h"

#include "ace/SOCK_Dgram.h"

#include <cstring>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {

namespace ICE {
  class Endpoint;
}

namespace DCPS {

class RtpsUdpTransport;
class RtpsUdpDataLink;
class ReceivedDataSample;

class OpenDDS_Rtps_Udp_Export RtpsUdpReceiveStrategy
  : public TransportReceiveStrategy<RtpsTransportHeader, RtpsSampleHeader>,
    public RcEventHandler
{
public:
  explicit RtpsUdpReceiveStrategy(RtpsUdpDataLink* link, const GuidPrefix_t& local_prefix);

  virtual int handle_input(ACE_HANDLE fd);

  /// For each "1" bit in the bitmap, change it to a "0" if there are
  /// fragments from publication "pub_id" for the sequence number represented
  /// by that position in the bitmap.
  /// Returns true if the bitmap was changed.
  bool remove_frags_from_bitmap(CORBA::Long bitmap[], CORBA::ULong num_bits,
                                const SequenceNumber& base,
                                const RepoId& pub_id, ACE_CDR::ULong& samples_requested);

  /// Remove any saved fragments.  We do not expect to receive any more
  /// fragments with sequence numbers in "range" from publication "pub_id".
  void remove_fragments(const SequenceRange& range, const RepoId& pub_id);

  typedef std::pair<SequenceNumber, RTPS::FragmentNumberSet> SeqFragPair;
  typedef OPENDDS_VECTOR(SeqFragPair) FragmentInfo;

  void clear_completed_fragments(const RepoId& pub_id);
  bool has_fragments(const SequenceRange& range, const RepoId& pub_id, FragmentInfo* frag_info = 0);

  /// Prevent delivery of the currently in-progress data sample to the
  /// subscription sub_id.  Returns pointer to the in-progress data so
  /// it can be stored for later delivery.
  const ReceivedDataSample* withhold_data_from(const RepoId& sub_id);
  void do_not_withhold_data_from(const RepoId& sub_id);

  static ssize_t receive_bytes_helper(iovec iov[],
                                      int n,
                                      const ACE_SOCK_Dgram& socket,
                                      ACE_INET_Addr& remote_address,
#ifdef OPENDDS_SECURITY
                                      DCPS::RcHandle<ICE::Agent> agent,
                                      DCPS::WeakRcHandle<ICE::Endpoint> endpoint,
#endif
                                      RtpsUdpTransport& tport,
                                      bool& stop);

  virtual void begin_transport_header_processing();
  virtual void end_transport_header_processing();

private:
  bool getDirectedWriteReaders(RepoIdSet& directedWriteReaders, const RTPS::DataSubmessage& ds) const;

  const ACE_SOCK_Dgram& choose_recv_socket(ACE_HANDLE fd) const;

  virtual ssize_t receive_bytes(iovec iov[],
                                int n,
                                ACE_INET_Addr& remote_address,
                                ACE_HANDLE fd,
                                bool& stop);

  virtual void deliver_sample(ReceivedDataSample& sample,
                              const ACE_INET_Addr& remote_address);

  virtual void finish_message();

  void deliver_sample_i(ReceivedDataSample& sample,
                        const RTPS::Submessage& submessage,
                        const NetworkAddress& remote_addr);

  virtual int start_i();
  virtual void stop_i();

  virtual bool check_header(const RtpsTransportHeader& header);

  virtual bool check_header(const RtpsSampleHeader& header);

  virtual bool reassemble(ReceivedDataSample& data);
  virtual bool reassemble_i(ReceivedDataSample& data, RtpsSampleHeader& rsh);

#ifdef OPENDDS_SECURITY
  void sec_submsg_to_octets(DDS::OctetSeq& encoded,
                            const RTPS::Submessage& postfix);

  void deliver_from_secure(const RTPS::Submessage& submessage,
                           const NetworkAddress& remote_addr);

  bool decode_payload(ReceivedDataSample& sample,
                      const RTPS::DataSubmessage& submessage);
#endif

  bool check_encoded(const EntityId_t& sender);

  typedef TransportReceiveStrategy<RtpsTransportHeader, RtpsSampleHeader> BaseReceiveStrategy;

  RtpsUdpDataLink* link_;
  SequenceNumber last_received_;

  const ReceivedDataSample* recvd_sample_;
  RepoIdSet readers_withheld_, readers_selected_;

  SequenceRange frags_;
  ACE_UINT32 total_frags_;
  TransportReassembly reassembly_;

  struct MessageReceiver {

    explicit MessageReceiver(const GuidPrefix_t& local);

    void reset(const ACE_INET_Addr& remote_address, const RTPS::Header& hdr);

    void submsg(const RTPS::Submessage& s);
    void submsg(const RTPS::InfoDestinationSubmessage& id);
    void submsg(const RTPS::InfoReplySubmessage& ir);
    void submsg(const RTPS::InfoReplyIp4Submessage& iri4);
    void submsg(const RTPS::InfoTimestampSubmessage& it);
    void submsg(const RTPS::InfoSourceSubmessage& is);

    void fill_header(DataSampleHeader& header) const;

    GuidPrefix_t local_;
    RTPS::ProtocolVersion_t source_version_;
    RTPS::VendorId_t source_vendor_;
    GuidPrefix_t source_guid_prefix_;
    GuidPrefix_t dest_guid_prefix_;
    bool directed_;
    DCPS::LocatorSeq unicast_reply_locator_list_;
    DCPS::LocatorSeq multicast_reply_locator_list_;
    bool have_timestamp_;
    RTPS::Time_t timestamp_;
  };

  MessageReceiver receiver_;
  ACE_INET_Addr remote_address_;
  RTPS::Message message_;

#ifdef OPENDDS_SECURITY
  RTPS::SecuritySubmessage secure_prefix_;
  OPENDDS_VECTOR(RTPS::Submessage) secure_submessages_;
  ReceivedDataSample secure_sample_;
  bool encoded_rtps_, encoded_submsg_;
#endif
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif  /* DCPS_RTPSUDPRECEIVESTRATEGY_H */
