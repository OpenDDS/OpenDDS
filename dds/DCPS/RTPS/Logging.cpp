/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "Logging.h"

#include "BaseMessageUtils.h"
#include "MessageTypes.h"

#include <dds/DCPS/JsonValueWriter.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {

namespace DCPS {

void vwrite(ValueWriter& vw, const GuidPrefix_t& prefix)
{
  vw.begin_array();
  for (size_t idx = 0; idx != sizeof(prefix); ++idx) {
    vw.write_byte(prefix[idx]);
  }
  vw.end_array();
}

}

namespace RTPS {

void log_message(const char* format,
                 const DCPS::GuidPrefix_t& prefix,
                 bool send,
                 const Message& message)
{
#ifdef OPENDDS_HAS_JSON_VALUE_WRITER
  DCPS::JsonValueWriter<> jvw;
  jvw.begin_struct();
  jvw.begin_struct_member("guidPrefix");
  vwrite(jvw, prefix);
  jvw.end_struct_member();
  jvw.begin_struct_member("send");
  jvw.write_boolean(send);
  jvw.end_struct_member();
  jvw.begin_struct_member("message");
  vwrite(jvw, message);
  jvw.end_struct_member();
  jvw.end_struct();
  ACE_DEBUG((LM_DEBUG, format, jvw.buffer().GetString()));
#else
  ACE_UNUSED_ARG(format);
  ACE_UNUSED_ARG(prefix);
  ACE_UNUSED_ARG(send);
  ACE_UNUSED_ARG(message);
  ACE_DEBUG((LM_DEBUG, "ERROR: OpenDDS lacks JSON serialization support, can't log RTPS messages\n"));
#endif
}

void parse_submessages(Message& message,
                       ACE_Message_Block* mb)
{
  DCPS::Message_Block_Ptr buff(mb);
  const DCPS::Encoding encoding_plain_native(DCPS::Encoding::KIND_XCDR1);
  DCPS::Serializer ser(buff.get(), encoding_plain_native);
  while (buff->length() > 3) {
    const char subm = buff->rd_ptr()[0], flags = buff->rd_ptr()[1];
    ser.swap_bytes((flags & RTPS::FLAG_E) != ACE_CDR_BYTE_ORDER);
    const size_t start = buff->length();
    CORBA::UShort submessageLength = 0;
    switch (subm) {
    case RTPS::INFO_DST: {
      RTPS::InfoDestinationSubmessage sm;
      ser >> sm;
      submessageLength = sm.smHeader.submessageLength;
      append_submessage(message, sm);
      break;
    }
    case RTPS::INFO_TS: {
      RTPS::InfoTimestampSubmessage sm;
      ser >> sm;
      submessageLength = sm.smHeader.submessageLength;
      append_submessage(message, sm);
      break;
    }
    case RTPS::DATA: {
      RTPS::DataSubmessage sm;
      ser >> sm;
      submessageLength = sm.smHeader.submessageLength;
      append_submessage(message, sm);
      break;
    }
    case RTPS::DATA_FRAG: {
      RTPS::DataFragSubmessage sm;
      ser >> sm;
      submessageLength = sm.smHeader.submessageLength;
      append_submessage(message, sm);
      break;
    }
    default:
      RTPS::SubmessageHeader smHeader;
      if (!(ser >> smHeader)) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("(%P|%t) ERROR: RtpsUdpDataLink::durability_resend() - ")
                   ACE_TEXT("failed to deserialize SubmessageHeader\n")));
      }
      submessageLength = smHeader.submessageLength;
      break;
    }
    if (submessageLength && buff->length()) {
      const size_t read = start - buff->length();
      if (read < static_cast<size_t>(submessageLength + RTPS::SMHDR_SZ)) {
        if (!ser.skip(static_cast<CORBA::UShort>(submessageLength + RTPS::SMHDR_SZ
                                                 - read))) {
          ACE_ERROR((LM_ERROR,
                     ACE_TEXT("(%P|%t) ERROR: RtpsUdpDataLink::durability_resend() - ")
                     ACE_TEXT("failed to skip sub message length\n")));
        }
      }
    } else if (!submessageLength) {
      break; // submessageLength of 0 indicates the last submessage
    }

  }
}

} // namespace RTPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
