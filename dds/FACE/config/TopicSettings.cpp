#include "TopicSettings.h"

#include <cstring>

namespace OpenDDS { namespace FaceTSS { namespace config {

int 
TopicSettings::set(const char* name, const char* value)
{
  int status = 0;
  if (!std::strcmp(name, "max_message_size")) {
    max_message_size_ = atoi(value);
  } else if (!std::strcmp(name, "type_name")) {
    strncpy(type_name_, value, sizeof(type_name_));
  } else {
    // no match
    printf("Don't know of setting %s\n", name);
    status = 1;
  }

  return status;
}

} } }
