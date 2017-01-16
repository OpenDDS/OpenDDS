/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DCPS_RTPSUDPRECEIVESTRATEGY_H
#define DCPS_RTPSUDPRECEIVESTRATEGY_H

#include "Rtps_Udp_Export.h"
#include "RtpsTransportHeader.h"
#include "RtpsSampleHeader.h"

#include "dds/DCPS/transport/framework/TransportReceiveStrategy_T.h"

#include "dds/DCPS/RTPS/RtpsCoreC.h"

#include "ace/Event_Handler.h"
#include "ace/INET_Addr.h"

#include <cstring>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class RtpsUdpDataLink;
class ReceivedDataSample;

class OpenDDS_Rtps_Udp_Export RtpsUdpReceiveStrategy
  : public TransportReceiveStrategy<RtpsTransportHeader, RtpsSampleHeader>,
    public ACE_Event_Handler {
public:
  explicit RtpsUdpReceiveStrategy(RtpsUdpDataLink* link, const GuidPrefix_t& local_prefix);

  virtual int handle_input(ACE_HANDLE fd);

  /// For each "1" bit in the bitmap, change it to a "0" if there are
  /// fragments from publication "pub_id" for the sequence number represented
  /// by that position in the bitmap.
  /// Returns true if the bitmap was changed.
  bool remove_frags_from_bitmap(CORBA::Long bitmap[], CORBA::ULong num_bits,
                                const SequenceNumber& base,
                                const RepoId& pub_id);

  /// Remove any saved fragments.  We do not expect to receive any more
  /// fragments with sequence numbers in "range" from publication "pub_id".
  void remove_fragments(const SequenceRange& range, const RepoId& pub_id);

  typedef std::pair<SequenceNumber, RTPS::FragmentNumberSet> SeqFragPair;
  typedef OPENDDS_VECTOR(SeqFragPair) FragmentInfo;

  bool has_fragments(const SequenceRange& range, const RepoId& pub_id,
                     FragmentInfo* frag_info = 0);

  /// Prevent delivery of the currently in-progress data sample to the
  /// subscription sub_id.  Returns pointer to the in-progress data so
  /// it can be stored for later delivery.
  const ReceivedDataSample* withhold_data_from(const RepoId& sub_id);
  void do_not_withhold_data_from(const RepoId& sub_id);

private:
  virtual ssize_t receive_bytes(iovec iov[],
                                int n,
                                ACE_INET_Addr& remote_address,
                                ACE_HANDLE fd);

  virtual void deliver_sample(ReceivedDataSample& sample,
                              const ACE_INET_Addr& remote_address);

  virtual int start_i();
  virtual void stop_i();

  virtual bool check_header(const RtpsTransportHeader& header);

  virtual bool check_header(const RtpsSampleHeader& header);

  virtual bool reassemble(ReceivedDataSample& data);


  RtpsUdpDataLink* link_;
  SequenceNumber last_received_;

  const ReceivedDataSample* recvd_sample_;
  RepoIdSet readers_withheld_, readers_selected_;

  SequenceRange frags_;
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
    DCPS::LocatorSeq unicast_reply_locator_list_;
    DCPS::LocatorSeq multicast_reply_locator_list_;
    bool have_timestamp_;
    RTPS::Time_t timestamp_;
  };

  MessageReceiver receiver_;
  ACE_INET_Addr remote_address_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif  /* DCPS_RTPSUDPRECEIVESTRATEGY_H */
