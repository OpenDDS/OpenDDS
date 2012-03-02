/*
 * $Id$
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

#include "dds/DCPS/RTPS/RtpsBaseMessageTypesC.h"
#include "dds/DCPS/RTPS/RtpsMessageTypesC.h"

#include "ace/Event_Handler.h"
#include "ace/INET_Addr.h"

#include <cstring>

namespace OpenDDS {
namespace DCPS {

class RtpsUdpDataLink;

class OpenDDS_Rtps_Udp_Export RtpsUdpReceiveStrategy
  : public TransportReceiveStrategy<RtpsTransportHeader, RtpsSampleHeader>,
    public ACE_Event_Handler {
public:
  explicit RtpsUdpReceiveStrategy(RtpsUdpDataLink* link);

  virtual int handle_input(ACE_HANDLE fd);

  /// For each "1" bit in the bitmap, change it to a "0" if there are
  /// fragments from publication "pub_id" for the sequence number represented
  /// by that position in the bitmap.
  void remove_frags_from_bitmap(CORBA::Long bitmap[], CORBA::ULong num_bits,
                                const SequenceNumber& base,
                                const RepoId& pub_id);

  /// Remove any saved fragments.  We do not expect to receive any more
  /// fragments with sequence numbers in "range" from publication "pub_id".
  void remove_fragments(const SequenceRange& range, const RepoId& pub_id);

  typedef std::vector<std::pair<SequenceNumber, RTPS::FragmentNumberSet> >
    FragmentInfo;

  bool has_fragments(const SequenceRange& range, const RepoId& pub_id,
                     FragmentInfo* frag_info = 0);

protected:
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

private:
  RtpsUdpDataLink* link_;
  SequenceNumber last_received_;

  SequenceRange frags_;
  TransportReassembly reassembly_;

  struct MessageReceiver {

    explicit MessageReceiver(const OpenDDS::RTPS::GuidPrefix_t& local);

    void reset(const ACE_INET_Addr& remote_address,
               const OpenDDS::RTPS::Header& hdr);

    void submsg(const OpenDDS::RTPS::Submessage& s);
    void submsg(const OpenDDS::RTPS::InfoDestinationSubmessage& id);
    void submsg(const OpenDDS::RTPS::InfoReplySubmessage& ir);
    void submsg(const OpenDDS::RTPS::InfoReplyIp4Submessage& iri4);
    void submsg(const OpenDDS::RTPS::InfoTimestampSubmessage& it);
    void submsg(const OpenDDS::RTPS::InfoSourceSubmessage& is);

    void fill_header(DataSampleHeader& header) const;

    OpenDDS::RTPS::GuidPrefix_t local_;
    OpenDDS::RTPS::ProtocolVersion_t source_version_;
    OpenDDS::RTPS::VendorId_t source_vendor_;
    OpenDDS::RTPS::GuidPrefix_t source_guid_prefix_;
    OpenDDS::RTPS::GuidPrefix_t dest_guid_prefix_;
    OpenDDS::RTPS::LocatorSeq unicast_reply_locator_list_;
    OpenDDS::RTPS::LocatorSeq multicast_reply_locator_list_;
    bool have_timestamp_;
    OpenDDS::RTPS::Time_t timestamp_;
  };

  MessageReceiver receiver_;
  ACE_INET_Addr remote_address_;
};

} // namespace DCPS
} // namespace OpenDDS

#endif  /* DCPS_RTPSUDPRECEIVESTRATEGY_H */
