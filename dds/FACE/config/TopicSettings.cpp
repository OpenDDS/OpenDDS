#include "TopicSettings.h"

#include "ace/OS_NS_stdio.h"
#include "ace/Log_Priority.h"
#include "ace/Log_Msg.h"

#include <cstring>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS { namespace FaceTSS { namespace config {

int
TopicSettings::set(const char* name, const char* value)
{
  int status = 0;
  if (!std::strcmp(name, "platform_view_guid")) {
    platform_view_guid_ = atoi(value);
  } else if (!std::strcmp(name, "max_message_size")) {
    max_message_size_ = atoi(value);
  } else if (!std::strcmp(name, "type_name")) {
    // Guarantee that value will fit in type_name_ and still be null terminated
    // type_name_ is sized to TYPE_NAME_LEN
    if (std::strlen(value) >= sizeof(type_name_)) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("Type name %C exceeds allowable length,"
        "must be < %d \n"), value, TYPE_NAME_LEN));
      status = 1;
    } else {
      std::strncpy(type_name_, value, sizeof(type_name_));
    }
  } else {
    // no match
    ACE_ERROR((LM_ERROR, ACE_TEXT("Don't know of setting %C\n"), name));
    status = 1;
  }

  return status;
}

} } }

OPENDDS_END_VERSIONED_NAMESPACE_DECL
