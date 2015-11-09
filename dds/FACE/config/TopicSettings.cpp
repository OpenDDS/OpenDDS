#include "TopicSettings.h"

#include "ace/OS_NS_stdio.h"

#include <cstring>

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
    std::strncpy(type_name_, value, sizeof(type_name_));
  } else {
    // no match
    ACE_DEBUG((LM_ERROR, ACE_TEXT("Don't know of setting %C\n"), name));
    status = 1;
  }

  return status;
}

} } }
