/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TRANSPORT_RTPS_UDP_RTPSUDPSENDSTRATEGY_H
#define OPENDDS_DCPS_TRANSPORT_RTPS_UDP_RTPSUDPSENDSTRATEGY_H

#include "Rtps_Udp_Export.h"

#if defined(OPENDDS_SECURITY)
#include "dds/DdsSecurityCoreC.h"
#endif

#include "dds/DCPS/NetworkAddress.h"
#include "dds/DCPS/transport/framework/TransportSendStrategy.h"

#include "dds/DCPS/RTPS/MessageTypes.h"

#include "ace/SOCK_Dgram.h"

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

  OverrideToken override_destinations(const NetworkAddress& destination);
  OverrideToken override_destinations(
    const AddrSet& destinations);

  void send_rtps_control(RTPS::Message& message,
                         ACE_Message_Block& submessages,
                         const NetworkAddress& destination);
  void send_rtps_control(RTPS::Message& message,
                         ACE_Message_Block& submessages,
                         const AddrSet& destinations);
  void append_submessages(const RTPS::SubmessageSeq& submessages);

#if defined(OPENDDS_SECURITY)
  void encode_payload(const RepoId& pub_id, Message_Block_Ptr& payload,
                      RTPS::SubmessageSeq& submessages);
#endif

  // NOTE: The header and footer sizes are dependent on the built-in crypto plugin.
  static const size_t MaxCryptoHeaderSize = 20;
  static const size_t MaxCryptoFooterSize = 20;
  static const size_t MaxSecurePrefixSize = RTPS::SMHDR_SZ + MaxCryptoHeaderSize;
  static const size_t MaxSubmessagePadding = RTPS::SM_ALIGN - 1;
  static const size_t MaxSecureSuffixSize = RTPS::SMHDR_SZ + MaxCryptoFooterSize;
  static const size_t MaxSecureSubmessageLeadingSize = MaxSecurePrefixSize;
  static const size_t MaxSecureSubmessageFollowingSize =
    RTPS::SMHDR_SZ /* SEC_BODY */ + MaxSecureSuffixSize;
  static const size_t MaxSecureSubmessageAdditionalSize =
    MaxSecureSubmessageLeadingSize + MaxSubmessagePadding + MaxSecureSubmessageFollowingSize;
  static const size_t MaxSecureFullMessageLeadingSize =
    RTPS::SMHDR_SZ + RTPS::INFO_SRC_SZ + MaxSecurePrefixSize;
  static const size_t MaxSecureFullMessageFollowingSize = MaxSecureSuffixSize;
  static const size_t MaxSecureFullMessageAdditionalSize =
    MaxSecureFullMessageLeadingSize + MaxSubmessagePadding + MaxSecureFullMessageFollowingSize;

#ifdef OPENDDS_SECURITY
  virtual Security::SecurityConfig_rch security_config() const;
#endif

protected:
  virtual ssize_t send_bytes_i(const iovec iov[], int n);
  ssize_t send_bytes_i_helper(const iovec iov[], int n);

  virtual size_t max_message_size() const;

  virtual void add_delayed_notification(TransportQueueElement* element);

private:
  bool marshal_transport_header(ACE_Message_Block* mb);
  ssize_t send_multi_i(const iovec iov[], int n,
                       const AddrSet& addrs);
  const ACE_SOCK_Dgram& choose_send_socket(const NetworkAddress& addr) const;
  ssize_t send_single_i(const iovec iov[], int n,
                        const NetworkAddress& addr);

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
  const AddrSet* override_dest_;
  const NetworkAddress* override_single_dest_;

  const size_t max_message_size_;
  RTPS::Message rtps_message_;
  ACE_Thread_Mutex rtps_message_mutex_;
  char rtps_header_data_[RTPS::RTPSHDR_SZ];
  ACE_Data_Block rtps_header_db_;
  ACE_Message_Block rtps_header_mb_;
  ACE_Thread_Mutex rtps_header_mb_lock_;
  ACE_Atomic_Op<ACE_Thread_Mutex, bool> network_is_unreachable_;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* DCPS_RTPSUDPSENDSTRATEGY_H */
