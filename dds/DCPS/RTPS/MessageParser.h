/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_RTPS_MESSAGEPARSER_H
#define OPENDDS_DCPS_RTPS_MESSAGEPARSER_H

#include "RtpsCoreTypeSupportImpl.h"


OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace RTPS {

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

} // namespace RTPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_RTPS_MESSAGEPARSER_H */
