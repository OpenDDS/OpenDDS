#include "ConnectionSettings.h"
#include <cstring>

namespace OpenDDS { namespace FaceTSS { namespace config {

ConnectionSettings::ConnectionSettings()
: connection_id_(0),
  direction_(FACE::SOURCE),
  domain_id_(0)
{
  strcpy(topic_name_, "");
  strcpy(qos_name_, "");
}

int
ConnectionSettings::set(const char* name, const char* value)
{
  int status = 0;
  if (!std::strcmp(name, "id")) {
    connection_id_ = atoi(value);
  } else if (!std::strcmp(name, "domain")) {
    domain_id_ = atoi(value);
  } else if (!std::strcmp(name, "topic")) {
    strncpy(topic_name_, value, sizeof(topic_name_));
  } else if (!std::strcmp(name, "qos")) {
    strncpy(qos_name_, value, sizeof(qos_name_));
  } else if (!std::strcmp(name, "direction")) {
    if (!std::strcmp(value, "source") ||
        !std::strcmp(value, "one_way_request_source") ||
        !std::strcmp(value, "two_way_request_synchronous_source") ||
        !std::strcmp(value, "two_way_request_reply_asynchronous_source")) {
      direction_ = FACE::SOURCE;
    } else if (!std::strcmp(value, "destination") ||
               !std::strcmp(value, "one_way_request_destination") ||
               !std::strcmp(value, "two_way_request_synchronous_destination") ||
               !std::strcmp(value, "two_way_request_reply_asynchronous_destination")) {
      direction_ = FACE::DESTINATION;
    } else if (!std::strcmp(value, "bi_directional") ||
               !std::strcmp(value, "not_defined_connection_direction_type")) {
      printf("Direction not supported: %s\n", value);
      status = 1;
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
