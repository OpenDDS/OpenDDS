/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TRANSPORT_FRAMEWORK_TRANSPORTSTATISTICS_H
#define OPENDDS_DCPS_TRANSPORT_FRAMEWORK_TRANSPORTSTATISTICS_H

#include "dds/DCPS/NetworkAddress.h"
#include "dds/DCPS/NetworkResource.h"

#include <dds/OpenddsDcpsExtC.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

struct InternalMessageCountKey {
  NetworkAddress address;
  MessageCountKind kind;
  bool relay;

  InternalMessageCountKey(const NetworkAddress& a_address,
                          MessageCountKind a_kind,
                          bool a_relay)
    : address(a_address)
    , kind(a_kind)
    , relay(a_relay)
  {}

  bool operator<(const InternalMessageCountKey& other) const
  {
    if (address != other.address) {
      return address < other.address;
    }
    return kind < other.kind;
  }
};

class InternalMessageCountValue {
public:
  InternalMessageCountValue()
    : send_count_(0)
    , send_bytes_(0)
    , send_fail_count_(0)
    , send_fail_bytes_(0)
    , recv_count_(0)
    , recv_bytes_(0)
  {}

  void send(ssize_t bytes)
  {
    ++send_count_;
    send_bytes_ += bytes;
  }
  size_t send_count() const { return send_count_; }
  ssize_t send_bytes() const { return send_bytes_; }

  void send_fail(ssize_t bytes)
  {
    ++send_fail_count_;
    send_fail_bytes_ += bytes;
  }
  size_t send_fail_count() const { return send_fail_count_; }
  ssize_t send_fail_bytes() const { return send_fail_bytes_; }

  void recv(ssize_t bytes)
  {
    ++recv_count_;
    recv_bytes_ += bytes;
  }
  size_t recv_count() const { return recv_count_; }
  ssize_t recv_bytes() const { return recv_bytes_; }

private:
  size_t send_count_;
  ssize_t send_bytes_;
  size_t send_fail_count_;
  ssize_t send_fail_bytes_;
  size_t recv_count_;
  ssize_t recv_bytes_;
};

struct InternalTransportStatistics {
  const OPENDDS_STRING transport;
  typedef OPENDDS_MAP(InternalMessageCountKey, InternalMessageCountValue) MessageCountMap;
  MessageCountMap message_count;
  typedef OPENDDS_MAP(GUID_t, CORBA::ULong) GuidCountMap;
  GuidCountMap writer_resend_count;
  GuidCountMap reader_nack_count;

  explicit InternalTransportStatistics(const OPENDDS_STRING& a_transport)
    : transport(a_transport)
  {}

  void clear()
  {
    message_count.clear();
    writer_resend_count.clear();
    reader_nack_count.clear();
  }
};

inline void append(TransportStatisticsSequence& seq, const InternalTransportStatistics& istats)
{
  const ACE_CDR::ULong idx = grow(seq) - 1;
  TransportStatistics& stats = seq[idx];
  stats.transport = istats.transport.c_str();
  for (InternalTransportStatistics::MessageCountMap::const_iterator pos = istats.message_count.begin(),
         limit = istats.message_count.end(); pos != limit; ++pos) {
    MessageCount mc;
    address_to_locator(mc.locator, pos->first.address.to_addr());
    mc.kind = pos->first.kind;
    mc.relay = pos->first.relay;
    mc.send_count = static_cast<ACE_CDR::ULong>(pos->second.send_count());
    mc.send_bytes = static_cast<ACE_CDR::ULong>(pos->second.send_bytes());
    mc.send_fail_count = static_cast<ACE_CDR::ULong>(pos->second.send_fail_count());
    mc.send_fail_bytes = static_cast<ACE_CDR::ULong>(pos->second.send_fail_bytes());
    mc.recv_count = static_cast<ACE_CDR::ULong>(pos->second.recv_count());
    mc.recv_bytes = static_cast<ACE_CDR::ULong>(pos->second.recv_bytes());
    push_back(stats.message_count, mc);
  }
  for (InternalTransportStatistics::GuidCountMap::const_iterator pos = istats.writer_resend_count.begin(),
         limit = istats.writer_resend_count.end(); pos != limit; ++pos) {
    const GuidCount gc = { pos->first, pos->second };
    push_back(stats.writer_resend_count, gc);
  }
  for (InternalTransportStatistics::GuidCountMap::const_iterator pos = istats.reader_nack_count.begin(),
         limit = istats.reader_nack_count.end(); pos != limit; ++pos) {
    const GuidCount gc = { pos->first, pos->second };
    push_back(stats.reader_nack_count, gc);
  }
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif  /* OPENDDS_DCPS_TRANSPORTSTATISTICS_H */
