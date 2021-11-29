/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_RTPS_BASEMESSAGEUTILS_H
#define OPENDDS_DCPS_RTPS_BASEMESSAGEUTILS_H

#include "RtpsCoreTypeSupportImpl.h"
#include "rtps_export.h"
#include "BaseMessageTypes.h"

#include <dds/DCPS/Hash.h>
#include <dds/DCPS/Util.h>
#include <dds/DCPS/Message_Block_Ptr.h>
#include <dds/DCPS/Serializer.h>
#include <dds/DCPS/TypeSupportImpl.h>
#include <dds/DCPS/SequenceNumber.h>
#include <dds/DCPS/TimeTypes.h>
#include <dds/DCPS/GuidConverter.h>

#include <dds/DdsDcpsInfoUtilsC.h>
#include <dds/DdsDcpsInfoUtilsTypeSupportImpl.h>

#include <ace/INET_Addr.h>
#include <ace/Message_Block.h>

#include <cstring>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace RTPS {

using DCPS::GuidPrefix_t;
using DCPS::GUID_t;
using DCPS::assign;
using DCPS::EntityId_t;
using DCPS::GUID_tKeyLessThan;
using DCPS::LogGuid;
using DCPS::String;
using DCPS::SequenceNumber;
using DCPS::TimeDuration;
using DCPS::MonotonicTimePoint;
using DCPS::SystemTimePoint;
using DCPS::DCPS_debug_level;
using DCPS::RcHandle;

template <typename T>
void marshal_key_hash(const T& msg, KeyHash_t& hash)
{
  using DCPS::Serializer;
  using DCPS::Encoding;
  typedef DCPS::MarshalTraits<T> Traits;

  DCPS::KeyOnly<const T> ko(msg);

  static const size_t hash_limit = 16;
  std::memset(hash.value, 0, hash_limit);

  const Encoding encoding(Encoding::KIND_XCDR1, DCPS::ENDIAN_BIG);
  const OpenDDS::DCPS::SerializedSizeBound bound =
    Traits::key_only_serialized_size_bound(encoding);

  if (bound && bound.get() <= hash_limit) {
    // If it is bounded and can always fit in 16 bytes, we will use the
    // marshaled key
    ACE_Message_Block mb(hash_limit);
    Serializer out_serializer(&mb, encoding);
    out_serializer << ko;
    std::memcpy(hash.value, mb.rd_ptr(), mb.length());

  } else {
    // We will use the hash of the marshaled key
    ACE_Message_Block mb(serialized_size(encoding, ko));
    Serializer out_serializer(&mb, encoding);
    out_serializer << ko;

    DCPS::MD5Hash(hash.value, mb.rd_ptr(), mb.length());
  }
}

OpenDDS_Rtps_Export
const DCPS::Encoding& get_locators_encoding();

OpenDDS_Rtps_Export
DDS::ReturnCode_t blob_to_locators(const DCPS::TransportBLOB& blob,
                                   DCPS::LocatorSeq& locators,
                                   bool* requires_inline_qos = 0,
                                   unsigned int* pBytesRead = 0);

OpenDDS_Rtps_Export
void locators_to_blob(const DCPS::LocatorSeq& locators,
                      DCPS::TransportBLOB& blob);

OpenDDS_Rtps_Export
DCPS::LocatorSeq transport_locator_to_locator_seq(const DCPS::TransportLocator& info);

template <typename T>
void message_block_to_sequence(const ACE_Message_Block& mb_locator, T& out)
{
  out.length (CORBA::ULong(mb_locator.length()));
  std::memcpy (out.get_buffer(), mb_locator.rd_ptr(), mb_locator.length());
}

#ifndef OPENDDS_SAFETY_PROFILE

inline bool operator==(const Duration_t& x, const Duration_t& y)
{
  return x.seconds == y.seconds && x.fraction == y.fraction;
}

inline bool operator==(const VendorId_t& v1, const VendorId_t& v2)
{
  return (v1.vendorId[0] == v2.vendorId[0] && v1.vendorId[1] == v2.vendorId[1]);
}

#endif

inline bool operator<(const ProtocolVersion_t& v1, const ProtocolVersion_t& v2)
{
  return (v1.major < v2.major || (v1.major == v2.major && v1.minor < v2.minor));
}

OpenDDS_Rtps_Export
DCPS::TimeDuration rtps_duration_to_time_duration(const Duration_t& rtps_duration, const ProtocolVersion_t& version, const VendorId_t& vendor);

/// Utility for iterating through a contiguous buffer (either really contiguous
/// or virtually contiguous using message block chaining) of RTPS Submessages
/// optionally prefixed by the RTPS Header
class OpenDDS_Rtps_Export MessageParser {
public:
  explicit MessageParser(const ACE_Message_Block& in);
  explicit MessageParser(const DDS::OctetSeq& in);

  bool parseHeader();
  bool parseSubmessageHeader();
  bool hasNextSubmessage() const;
  bool skipToNextSubmessage();
  bool skipSubmessageContent();

  const Header& header() const { return header_; }
  SubmessageHeader submessageHeader() const { return sub_; }
  size_t remaining() const { return in_ ? in_->total_length() : fromSeq_.length(); }
  const char* current() const { return ser_.pos_rd(); }

  DCPS::Serializer& serializer() { return ser_; }

  template <typename T>
  bool operator>>(T& rhs) { return ser_ >> rhs; }

private:
  ACE_Message_Block fromSeq_;
  DCPS::Message_Block_Ptr in_;
  DCPS::Serializer ser_;
  Header header_;
  SubmessageHeader sub_;
  size_t smContentStart_;
};

OpenDDS_Rtps_Export
bool bitmapNonEmpty(const SequenceNumberSet& snSet);

inline DCPS::SequenceNumber to_opendds_seqnum(const RTPS::SequenceNumber_t& rtps_seqnum)
{
  DCPS::SequenceNumber opendds_seqnum;
  opendds_seqnum.setValue(rtps_seqnum.high, rtps_seqnum.low);
  return opendds_seqnum;
}

inline RTPS::SequenceNumber_t to_rtps_seqnum(const DCPS::SequenceNumber& opendds_seqnum)
{
  RTPS::SequenceNumber_t rtps_seqnum;
  rtps_seqnum.high = opendds_seqnum.getHigh();
  rtps_seqnum.low = opendds_seqnum.getLow();
  return rtps_seqnum;
}

inline void append_submessage(RTPS::Message& message, const RTPS::InfoDestinationSubmessage& submessage)
{
  RTPS::Submessage sm;
  sm.info_dst_sm(submessage);
  DCPS::push_back(message.submessages, sm);
}

inline void append_submessage(RTPS::Message& message, const RTPS::InfoTimestampSubmessage& submessage)
{
  RTPS::Submessage sm;
  sm.info_ts_sm(submessage);
  DCPS::push_back(message.submessages, sm);
}

inline void append_submessage(RTPS::Message& message, const RTPS::DataSubmessage& submessage)
{
  RTPS::Submessage sm;
  sm.data_sm(submessage);
  DCPS::push_back(message.submessages, sm);
}

inline void append_submessage(RTPS::Message& message, const RTPS::DataFragSubmessage& submessage)
{
  RTPS::Submessage sm;
  sm.data_frag_sm(submessage);
  DCPS::push_back(message.submessages, sm);
}

} // namespace RTPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* RTPS_BASEMESSAGETYPES_H */
