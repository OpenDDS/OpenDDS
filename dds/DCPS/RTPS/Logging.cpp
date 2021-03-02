/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "Logging.h"

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
#ifdef OPENDDS_HAVE_JSON_VALUE_WRITER
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
  ACE_DEBUG((LM_DEBUG, format, "ERROR: OpenDDS lacks JSON serialization support"));
#endif
}

} // namespace RTPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
