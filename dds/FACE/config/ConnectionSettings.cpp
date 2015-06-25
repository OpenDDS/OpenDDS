#include "ConnectionSettings.h"

#include "ace/OS_NS_stdio.h"

#include <cstring>

namespace OpenDDS { namespace FaceTSS { namespace config {

ConnectionSettings::ConnectionSettings()
: connection_id_(0),
  direction_(FACE::SOURCE),
  domain_id_(0)
{
  std::strcpy(topic_name_, "");
  std::strcpy(datawriter_qos_name_, "");
  std::strcpy(datareader_qos_name_, "");
  std::strcpy(publisher_qos_name_, "");
  std::strcpy(subscriber_qos_name_, "");
  std::strcpy(transport_name_, "");
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
    std::strncpy(topic_name_, value, sizeof(topic_name_));
  } else if (!std::strcmp(name, "datawriterqos")) {
    std::strncpy(datawriter_qos_name_, value, sizeof(datawriter_qos_name_));
  } else if (!std::strcmp(name, "datareaderqos")) {
    std::strncpy(datareader_qos_name_, value, sizeof(datareader_qos_name_));
  } else if (!std::strcmp(name, "publisherqos")) {
    std::strncpy(publisher_qos_name_, value, sizeof(publisher_qos_name_));
  } else if (!std::strcmp(name, "subscriberqos")) {
    std::strncpy(subscriber_qos_name_, value, sizeof(subscriber_qos_name_));
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
      ACE_OS::printf("Direction not supported: %s\n", value);
      status = 1;
    } else {
      ACE_OS::printf("Don't know of direction %s\n", value);
      status = 1;
    }
  } else if (!std::strcmp(name, "transport")) {
    std::strncpy(transport_name_, value, sizeof(transport_name_));
  } else {
    // no match
    ACE_OS::printf("Don't know of setting %s\n", name);
    status = 1;
  }

  return status;
}

const char*
ConnectionSettings::datawriter_qos_name() const
{
  return datawriter_qos_name_;
}

const char*
ConnectionSettings::datareader_qos_name() const
{
  return datareader_qos_name_;
}

const char*
ConnectionSettings::publisher_qos_name() const
{
  return publisher_qos_name_;
}

const char*
ConnectionSettings::subscriber_qos_name() const
{
  return subscriber_qos_name_;
}

const char*
ConnectionSettings::transport_name() const
{
  return transport_name_;
}

bool
ConnectionSettings::datawriter_qos_set() const
{
  return datawriter_qos_name_[0];
}

bool
ConnectionSettings::datareader_qos_set() const
{
  return datareader_qos_name_[0];
}

bool
ConnectionSettings::publisher_qos_set() const
{
  return publisher_qos_name_[0];
}

bool
ConnectionSettings::subscriber_qos_set() const
{
  return subscriber_qos_name_[0];
}

bool
ConnectionSettings::transport_set() const
{
  return transport_name_[0];
}

} } }
