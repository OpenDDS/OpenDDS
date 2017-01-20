#include "ConnectionSettings.h"

#include "ace/OS_NS_stdio.h"
#include "ace/Log_Priority.h"
#include "ace/Log_Msg.h"

#include <cstring>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS { namespace FaceTSS { namespace config {

ConnectionSettings::ConnectionSettings()
: connection_id_(0),
  direction_(FACE::SOURCE),
  domain_id_(0),
  participant_id_(0)
{
  std::strcpy(topic_name_, "");
  std::strcpy(datawriter_qos_name_, "");
  std::strcpy(datareader_qos_name_, "");
  std::strcpy(publisher_qos_name_, "");
  std::strcpy(subscriber_qos_name_, "");
  std::strcpy(config_name_, "");
}

int
ConnectionSettings::set(const char* name, const char* value)
{
  int status = 0;
  if (!std::strcmp(name, "id")) {
    connection_id_ = atoi(value);
  } else if (!std::strcmp(name, "participantid")) {
    participant_id_ = atoi(value);
  } else if (!std::strcmp(name, "domain")) {
    domain_id_ = atoi(value);
  } else if (!std::strcmp(name, "topic")) {
    if (std::strlen(value) >= ALLOWABLE_NAME_LEN) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("topic name %C exceeds allowable length,"
        "must be < %B \n"), value, ALLOWABLE_NAME_LEN));
      status = 1;
    } else {
      std::strncpy(topic_name_, value, sizeof(topic_name_));
    }
  } else if (!std::strcmp(name, "datawriterqos")) {
    if (std::strlen(value) >= ALLOWABLE_NAME_LEN) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("datawriterqos name %C exceeds allowable length,"
        "must be < %B \n"), value, ALLOWABLE_NAME_LEN));
      status = 1;
    } else {
      std::strncpy(datawriter_qos_name_, value, sizeof(datawriter_qos_name_));
    }
  } else if (!std::strcmp(name, "datareaderqos")) {
    if (std::strlen(value) >= ALLOWABLE_NAME_LEN) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("datareaderqos name %C exceeds allowable length,"
        "must be < %B \n"), value, ALLOWABLE_NAME_LEN));
      status = 1;
    } else {
      std::strncpy(datareader_qos_name_, value, sizeof(datareader_qos_name_));
    }
  } else if (!std::strcmp(name, "publisherqos")) {
    if (std::strlen(value) >= ALLOWABLE_NAME_LEN) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("publisherqos name %C exceeds allowable length,"
        "must be < %B \n"), value, ALLOWABLE_NAME_LEN));
      status = 1;
    } else {
      std::strncpy(publisher_qos_name_, value, sizeof(publisher_qos_name_));
    }
  } else if (!std::strcmp(name, "subscriberqos")) {
    if (std::strlen(value) >= ALLOWABLE_NAME_LEN) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("subscriberqos name %C exceeds allowable length,"
        "must be < %B \n"), value, ALLOWABLE_NAME_LEN));
      status = 1;
    } else {
      std::strncpy(subscriber_qos_name_, value, sizeof(subscriber_qos_name_));
    }
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
      ACE_ERROR((LM_ERROR, ACE_TEXT("Direction not supported: %C\n"), value));
      status = 1;
    } else {
      ACE_ERROR((LM_ERROR, ACE_TEXT("Don't know of direction %C\n"), value));
      status = 1;
    }
  } else if (!std::strcmp(name, "config")) {
    // Guarantee that value will fit in config_name_ and still be null terminated
    // config_name_ is sized to ALLOWABLE_NAME_LEN
    if (std::strlen(value) >= sizeof(config_name_)) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("config name %C exceeds allowable length,"
        "must be < %B \n"), value, ALLOWABLE_NAME_LEN));
      status = 1;
    } else {
      std::strncpy(config_name_, value, sizeof(config_name_));
    }
  } else {
    // no match
    ACE_ERROR((LM_ERROR, ACE_TEXT("Don't know of setting %C\n"), name));
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
ConnectionSettings::config_name() const
{
  return config_name_;
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
ConnectionSettings::config_set() const
{
  return config_name_[0];
}

} } }


OPENDDS_END_VERSIONED_NAMESPACE_DECL
