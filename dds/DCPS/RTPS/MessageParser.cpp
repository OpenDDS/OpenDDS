/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "MessageParser.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace RTPS {

using DCPS::Encoding;

namespace {
  const Encoding encoding_plain_native(Encoding::KIND_XCDR1);
}

MessageParser::MessageParser(const ACE_Message_Block& in)
  : in_(in.duplicate())
  , ser_(in_.get(), encoding_plain_native)
  , header_()
  , sub_()
  , smContentStart_(0)
{}

MessageParser::MessageParser(const DDS::OctetSeq& in)
  : fromSeq_(reinterpret_cast<const char*>(in.get_buffer()), in.length())
  , ser_(&fromSeq_, encoding_plain_native)
  , header_()
  , sub_()
  , smContentStart_(0)
{
  fromSeq_.wr_ptr(fromSeq_.size());
}

bool MessageParser::parseHeader()
{
  return ser_ >> header_;
}

bool MessageParser::parseSubmessageHeader()
{
  if(!(ser_ >> ACE_InputCDR::to_octet(sub_.submessageId)) ||
    !(ser_ >> ACE_InputCDR::to_octet(sub_.flags))) {
    return false;
  }

  ser_.swap_bytes(ACE_CDR_BYTE_ORDER != (sub_.flags & FLAG_E));
  if(!(ser_ >> sub_.submessageLength)) {
    return false;
  }
  smContentStart_ = ser_.length();
  return sub_.submessageLength <= ser_.length();
}

bool MessageParser::hasNextSubmessage() const
{
  if(sub_.submessageLength == 0) {
    if(sub_.submessageId != PAD && sub_.submessageId != INFO_TS) {
      return false;
    }
    return ser_.length();
  }
  return smContentStart_ > sub_.submessageLength;
}

bool MessageParser::skipToNextSubmessage()
{
  const size_t read = smContentStart_ - ser_.length();
  return ser_.skip(static_cast<unsigned short>(sub_.submessageLength - read));
}

bool MessageParser::skipSubmessageContent()
{
  if(sub_.submessageLength) {
    const size_t read = smContentStart_ - ser_.length();
    return ser_.skip(static_cast<unsigned short>(sub_.submessageLength - read));
  } else if (sub_.submessageId == PAD || sub_.submessageId == INFO_TS) {
    return true;
  } else {
    return ser_.skip(static_cast<unsigned short>(ser_.length()));
  }
}

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
