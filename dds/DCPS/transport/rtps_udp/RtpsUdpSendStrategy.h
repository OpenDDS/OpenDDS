/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DCPS_RTPSUDPSENDSTRATEGY_H
#define DCPS_RTPSUDPSENDSTRATEGY_H

#include "Rtps_Udp_Export.h"

#if defined(OPENDDS_SECURITY)
#include "dds/DdsSecurityCoreC.h"
#endif

#include "dds/DCPS/transport/framework/TransportSendStrategy.h"

#include "dds/DCPS/RTPS/MessageTypes.h"

#include "ace/INET_Addr.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class RtpsUdpDataLink;
class RtpsUdpInst;
typedef RcHandle<RtpsUdpDataLink> RtpsUdpDataLink_rch;

class OpenDDS_Rtps_Udp_Export RtpsUdpSendStrategy
  : public TransportSendStrategy {
public:
  RtpsUdpSendStrategy(RtpsUdpDataLink* link,
                      const GuidPrefix_t& local_prefix);

  virtual void stop_i();

  struct OverrideToken {
    explicit OverrideToken(RtpsUdpSendStrategy* outer) : outer_(outer) {}
    ~OverrideToken();
    RtpsUdpSendStrategy* outer_;
  };
  friend struct OverrideToken;

  OverrideToken override_destinations(const ACE_INET_Addr& destination);
  OverrideToken override_destinations(
    const OPENDDS_SET(ACE_INET_Addr)& destinations);

  void send_rtps_control(ACE_Message_Block& submessages,
                         const ACE_INET_Addr& destination);
  void send_rtps_control(ACE_Message_Block& submessages,
                         const OPENDDS_SET(ACE_INET_Addr)& destinations);

#if defined(OPENDDS_SECURITY)
  void encode_payload(const RepoId& pub_id, Message_Block_Ptr& payload,
                      RTPS::SubmessageSeq& submessages);
#endif

protected:
  virtual ssize_t send_bytes_i(const iovec iov[], int n);
  ssize_t send_bytes_i_helper(const iovec iov[], int n);

  virtual size_t max_message_size() const
  {
    return UDP_MAX_MESSAGE_SIZE;
  }

  virtual void add_delayed_notification(TransportQueueElement* element);

private:
  bool marshal_transport_header(ACE_Message_Block* mb);
  ssize_t send_multi_i(const iovec iov[], int n,
                       const OPENDDS_SET(ACE_INET_Addr)& addrs);
  ssize_t send_single_i(const iovec iov[], int n,
                        const ACE_INET_Addr& addr);

#ifdef OPENDDS_SECURITY
  ACE_Message_Block* pre_send_packet(const ACE_Message_Block* plain);

  struct Chunk {
    const char* start_;
    unsigned int length_;
    DDS::OctetSeq encoded_;
  };

  bool encode_writer_submessage(const RepoId& receiver,
                                OPENDDS_VECTOR(Chunk)& replacements,
                                DDS::Security::CryptoTransform* crypto,
                                const DDS::OctetSeq& plain,
                                DDS::Security::DatawriterCryptoHandle sender_dwch,
                                const char* submessage_start, CORBA::Octet msgId);

  bool encode_reader_submessage(const RepoId& receiver,
                                OPENDDS_VECTOR(Chunk)& replacements,
                                DDS::Security::CryptoTransform* crypto,
                                const DDS::OctetSeq& plain,
                                DDS::Security::DatareaderCryptoHandle sender_drch,
                                const char* submessage_start, CORBA::Octet msgId);

  ACE_Message_Block* encode_submessages(const ACE_Message_Block* plain,
                                        DDS::Security::CryptoTransform* crypto,
                                        bool& stateless_or_volatile);

  ACE_Message_Block* encode_rtps_message(const ACE_Message_Block* plain,
                                         DDS::Security::CryptoTransform* crypto);

  ACE_Message_Block* replace_chunks(const ACE_Message_Block* plain,
                                    const OPENDDS_VECTOR(Chunk)& replacements);
#endif

  RtpsUdpDataLink* link_;
  const OPENDDS_SET(ACE_INET_Addr)* override_dest_;
  const ACE_INET_Addr* override_single_dest_;

  RTPS::Header rtps_header_;
  char rtps_header_data_[RTPS::RTPSHDR_SZ];
  ACE_Data_Block rtps_header_db_;
  ACE_Message_Block rtps_header_mb_;
  bool network_is_unreachable_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* DCPS_RTPSUDPSENDSTRATEGY_H */
