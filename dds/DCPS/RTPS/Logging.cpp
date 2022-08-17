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
#if OPENDDS_HAS_JSON_VALUE_WRITER
  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  DCPS::JsonValueWriter<rapidjson::Writer<rapidjson::StringBuffer> > jvw(writer);
  jvw.begin_struct();
  jvw.begin_struct_member(XTypes::MemberDescriptorImpl("guidPrefix", false));
  vwrite(jvw, prefix);
  jvw.end_struct_member();
  jvw.begin_struct_member(XTypes::MemberDescriptorImpl("send", false));
  jvw.write_boolean(send);
  jvw.end_struct_member();
  jvw.begin_struct_member(XTypes::MemberDescriptorImpl("message", false));
  vwrite(jvw, message);
  jvw.end_struct_member();
  jvw.end_struct();
  writer.Flush();
  ACE_DEBUG((LM_DEBUG, format, buffer.GetString()));
#else
  ACE_UNUSED_ARG(format);
  ACE_UNUSED_ARG(prefix);
  ACE_UNUSED_ARG(send);
  ACE_UNUSED_ARG(message);
  ACE_DEBUG((LM_DEBUG, "ERROR: OpenDDS lacks JSON serialization support, can't log RTPS messages\n"));
#endif
}

void parse_submessages(Message& message,
                       const ACE_Message_Block& mb)
{
  MessageParser mp(mb);
  DCPS::Serializer& ser = mp.serializer();

  while (mp.parseSubmessageHeader()) {
    switch (mp.submessageHeader().submessageId) {
    case RTPS::INFO_DST: {
      RTPS::InfoDestinationSubmessage sm;
      ser >> sm;
      append_submessage(message, sm);
      break;
    }
    case RTPS::INFO_TS: {
      RTPS::InfoTimestampSubmessage sm;
      ser >> sm;
      append_submessage(message, sm);
      break;
    }
    case RTPS::DATA: {
      RTPS::DataSubmessage sm;
      ser >> sm;
      append_submessage(message, sm);
      break;
    }
    case RTPS::DATA_FRAG: {
      RTPS::DataFragSubmessage sm;
      ser >> sm;
      append_submessage(message, sm);
      break;
    }
    default:
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: parse_submessages() - ")
                 ACE_TEXT("unhandle submessageId %d\n"), mp.submessageHeader().submessageId));
      break;
    }
    mp.skipToNextSubmessage();
  }
}

} // namespace RTPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
