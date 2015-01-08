#include "ConnectionSettings.h"

namespace OpenDDS { namespace FaceTSS { namespace config {

int 
ConnectionSettings::set(const char* name, const char* value)
{
  int status = 0;
  if (!strcmp(name, "id")) {
    connection_id_ = atoi(value);
  } else if (!strcmp(name, "domain")) {
    domain_id_ = atoi(value);
  } else if (!strcmp(name, "topic")) {
    strncpy(topic_name_, value, sizeof(topic_name_));
  } else if (!strcmp(name, "direction")) {
    if (!strcmp(value, "source")) {
      direction_ = FACE::SOURCE;
    } else if (!strcmp(value, "destination")) {
      direction_ = FACE::DESTINATION;
    } else {
      printf("Don't know of direction %s\n", value);
      status = 1;
    }
  } else {
    // no match
    printf("Don't know of setting %s\n", name);
    status = 1;
  }

  return status;
}

} } }
