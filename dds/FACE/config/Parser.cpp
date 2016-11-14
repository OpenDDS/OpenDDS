#include "Parser.h"
#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/StaticDiscovery.h"
#include "dds/DCPS/transport/framework/TransportRegistry.h"

#include "ace/Configuration_Import_Export.h"
#include "ace/OS_NS_stdio.h"
#include "ace/Log_Priority.h"
#include "ace/Log_Msg.h"

#include <cstring>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS { namespace FaceTSS { namespace config {

namespace {
  const char* TOPIC_SECTION          = "topic";
  const char* CONNECTION_SECTION     = "connection";
  const char* DATAWRITER_QOS_SECTION = "datawriterqos";
  const char* DATAREADER_QOS_SECTION = "datareaderqos";
  const char* PUBLISHER_QOS_SECTION  = "publisherqos";
  const char* SUBSCRIBER_QOS_SECTION = "subscriberqos";

  OpenDDS::DCPS::RepoId build_id(const ConnectionSettings& conn)
  {
    unsigned char participant_key[6];
    participant_key[0] = (conn.participant_id_ >> 0) & 0xFF;
    participant_key[1] = (conn.participant_id_ >> 8) & 0xFF;
    participant_key[2] = (conn.participant_id_ >> 16) & 0xFF;
    participant_key[3] = (conn.participant_id_ >> 24) & 0xFF;
    participant_key[4] = 0; //(conn.domain_id_ >> 32) & 0xFF;
    participant_key[5] = 0; //(conn.domain_id_ >> 40) & 0xFF;

    unsigned char entity_key[3];
    entity_key[0] = (conn.connection_id_ >> 0) & 0xFF;
    entity_key[1] = (conn.connection_id_ >> 8) & 0xFF;
    entity_key[2] = (conn.connection_id_ >> 16) & 0xFF;

    unsigned char entity_kind = 0;
    switch (conn.direction_) {
    case FACE::SOURCE:
      entity_kind = DCPS::ENTITYKIND_USER_WRITER_WITH_KEY;
      break;
    case FACE::DESTINATION:
      entity_kind = DCPS::ENTITYKIND_USER_READER_WITH_KEY;
      break;
    case FACE::BI_DIRECTIONAL:
    case FACE::ONE_WAY_REQUEST_SOURCE:
    case FACE::ONE_WAY_REQUEST_DESTINATION:
    case FACE::TWO_WAY_REQUEST_SYNCHRONOUS_SOURCE:
    case FACE::TWO_WAY_REQUEST_SYNCHRONOUS_DESTINATION:
    case FACE::TWO_WAY_REQUEST_REPLY_ASYNCHRONOUS_SOURCE:
    case FACE::TWO_WAY_REQUEST_REPLY_ASYNCHRONOUS_DESTINATION:
    case FACE::NOT_DEFINED_CONNECTION_DIRECTION_TYPE:
      break;
    }
    return OpenDDS::DCPS::EndpointRegistry::build_id(conn.domain_id_,
                                                     participant_key,
                                                     OpenDDS::DCPS::EndpointRegistry::build_id(entity_key,
                                                                                               entity_kind));
  }

} // anon namespace


ConnectionMap Parser::connection_map_;
QosMap Parser::qos_map_;
TopicMap Parser::topic_map_;

int
Parser::parse(const char* filename)
{
  ACE_Configuration_Heap config;
  config.open();
  ACE_Ini_ImpExp import(config);
  int status = import.import_config(filename);
  if (status) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: Initialize() ")
               ACE_TEXT("import_config () returned %d\n"),
               status));
    return status;
  }

  status = parse_sections(config, DATAWRITER_QOS_SECTION, false) ||
    parse_sections(config, DATAREADER_QOS_SECTION, false) ||
    parse_sections(config, PUBLISHER_QOS_SECTION, false) ||
    parse_sections(config, SUBSCRIBER_QOS_SECTION, false) ||
    parse_sections(config, TOPIC_SECTION, true) ||
    parse_sections(config, CONNECTION_SECTION, true);

  if (status)
    return status;

  status = TheServiceParticipant->load_configuration(config,
                                                     filename);

  if (status)
    return status;

  for (ConnectionMap::const_iterator pos = connection_map_.begin(), limit = connection_map_.end();
       pos != limit;
       ++pos) {
    const ConnectionSettings& conn = pos->second;
    OpenDDS::DCPS::RepoId id = build_id(conn);

    switch (conn.direction_) {
    case FACE::SOURCE:
      {
        OPENDDS_STRING topic_name = conn.topic_name_;

        DDS::DataWriterQos qos(TheServiceParticipant->initial_DataWriterQos());
        if (conn.datawriter_qos_set()) {
          QosMap::const_iterator p = qos_map_.find(conn.datawriter_qos_name());
          if (p != qos_map_.end()) {
            p->second.apply_to(qos);
          } else {
            ACE_ERROR((LM_ERROR,
                       ACE_TEXT("(%P|%t) ERROR: Could not find datawriterqos/%s\n"),
                       conn.datawriter_qos_name()));
            return -1;
          }
        }

        DDS::PublisherQos publisher_qos(TheServiceParticipant->initial_PublisherQos());
        if (conn.publisher_qos_set()) {
          QosMap::const_iterator p = qos_map_.find(conn.publisher_qos_name());
          if (p != qos_map_.end()) {
            p->second.apply_to(publisher_qos);
          } else {
            ACE_ERROR((LM_ERROR,
                       ACE_TEXT("(%P|%t) ERROR: Could not find publisherqos/%s\n"),
                       conn.publisher_qos_name()));
            return -1;
          }
        }

        DCPS::TransportLocatorSeq trans_info;

        OpenDDS::DCPS::TransportConfig_rch config;

        if (conn.config_set()) {
          config = TheTransportRegistry->get_config(conn.config_name());
          if (config.is_nil()) {
            ACE_ERROR((LM_ERROR,
                       ACE_TEXT("(%P|%t) ERROR: Could not find config/%s\n"),
                       conn.config_name()));
            return -1;
          }
        }

        if (config.is_nil()) {
          config = TheTransportRegistry->domain_default_config(conn.domain_id_);
        }

        if (config.is_nil()) {
          config = TheTransportRegistry->global_config();
        }

        config->populate_locators(trans_info);

        // Typically, we would ensure that trans_info is not empty.
        // However, when using RTPS, trans_info will be empty so don't check.

        // Populate the userdata.
        qos.user_data.value.length(3);
        qos.user_data.value[0] = (conn.connection_id_ >> 0) & 0xFF;
        qos.user_data.value[1] = (conn.connection_id_ >> 8) & 0xFF;
        qos.user_data.value[2] = (conn.connection_id_ >> 16) & 0xFF;

        OpenDDS::DCPS::EndpointRegistry::Writer w(topic_name, qos, publisher_qos, conn.config_name(), trans_info);
        OpenDDS::DCPS::StaticDiscovery::instance()->registry.writer_map.insert(std::make_pair(id, w));
      }
      break;
    case FACE::DESTINATION:
      {
        OPENDDS_STRING topic_name = conn.topic_name_;

        DDS::DataReaderQos qos(TheServiceParticipant->initial_DataReaderQos());
        if (conn.datareader_qos_set()) {
          QosMap::const_iterator p = qos_map_.find(conn.datareader_qos_name());
          if (p != qos_map_.end()) {
            p->second.apply_to(qos);
          } else {
            ACE_ERROR((LM_ERROR,
                       ACE_TEXT("(%P|%t) ERROR: Could not find datareaderqos/%s\n"),
                       conn.datawriter_qos_name()));
            return -1;
          }
        }

        DDS::SubscriberQos subscriber_qos(TheServiceParticipant->initial_SubscriberQos());
        if (conn.subscriber_qos_set()) {
          QosMap::const_iterator p = qos_map_.find(conn.subscriber_qos_name());
          if (p != qos_map_.end()) {
            p->second.apply_to(subscriber_qos);
          } else {
            ACE_ERROR((LM_ERROR,
                       ACE_TEXT("(%P|%t) ERROR: Could not find subscriberqos/%s\n"),
                       conn.subscriber_qos_name()));
            return -1;
          }
        }

        DCPS::TransportLocatorSeq trans_info;

        OpenDDS::DCPS::TransportConfig_rch config;

        if (conn.config_set()) {
          config = TheTransportRegistry->get_config(conn.config_name());
          if (config.is_nil()) {
            ACE_ERROR((LM_ERROR,
                       ACE_TEXT("(%P|%t) ERROR: Could not find transport/%s\n"),
                       conn.config_name()));
            return -1;
          }
        }

        if (config.is_nil()) {
          config = TheTransportRegistry->domain_default_config(conn.domain_id_);
        }

        if (config.is_nil()) {
          config = TheTransportRegistry->global_config();
        }

        config->populate_locators(trans_info);

        // Typically, we would ensure that trans_info is not empty.
        // However, when using RTPS, trans_info will be empty so don't check.

        // Populate the userdata.
        qos.user_data.value.length(3);
        qos.user_data.value[0] = (conn.connection_id_ >> 0) & 0xFF;
        qos.user_data.value[1] = (conn.connection_id_ >> 8) & 0xFF;
        qos.user_data.value[2] = (conn.connection_id_ >> 16) & 0xFF;

        OpenDDS::DCPS::EndpointRegistry::Reader r(topic_name, qos, subscriber_qos, conn.config_name(), trans_info);
        OpenDDS::DCPS::StaticDiscovery::instance()->registry.reader_map.insert(std::make_pair(id, r));
      }
      break;
    case FACE::BI_DIRECTIONAL:
    case FACE::ONE_WAY_REQUEST_SOURCE:
    case FACE::ONE_WAY_REQUEST_DESTINATION:
    case FACE::TWO_WAY_REQUEST_SYNCHRONOUS_SOURCE:
    case FACE::TWO_WAY_REQUEST_SYNCHRONOUS_DESTINATION:
    case FACE::TWO_WAY_REQUEST_REPLY_ASYNCHRONOUS_SOURCE:
    case FACE::TWO_WAY_REQUEST_REPLY_ASYNCHRONOUS_DESTINATION:
    case FACE::NOT_DEFINED_CONNECTION_DIRECTION_TYPE:
      break;
    }
  }

  DCPS::StaticDiscovery::instance()->registry.match();

  return status;
}

int
Parser::find_connection(const char* name,
                        ConnectionSettings& target)
{
  int status = 1;
  ConnectionMap::iterator result;
  if ((result = connection_map_.find(name)) != connection_map_.end()) {
    status = 0;
    target = result->second;
  }
  return status;
}

int
Parser::find_topic(const char* name,
                   TopicSettings& target)
{
  int status = 1;
  TopicMap::iterator result;
  if ((result = topic_map_.find(name)) != topic_map_.end()) {
    status = 0;
    target = result->second;
  }
  return status;
}

int
Parser::find_qos(const ConnectionSettings& conn, QosSettings& target)
{
  int status = 0;
  QosMap::iterator result;
  // If thie is a SOURCE
  if (conn.direction_ == FACE::SOURCE) {
    if (conn.datawriter_qos_set()) {
      // Name specified, must be in map
      result = qos_map_.find(conn.datawriter_qos_name());
      if (result != qos_map_.end()) {
        result->second.apply_to(target.datawriter_qos());
      } else {
        // Failed
        status = 1;
      }
    }
    if (status == 0 && (conn.publisher_qos_set())) {
      // Name specified, must be in map
      result = qos_map_.find(conn.publisher_qos_name());
      if (result != qos_map_.end()) {
        result->second.apply_to(target.publisher_qos());
      } else {
        // Failed
        status = 1;
      }
    }
  // Else DESTINATION
  } else {
    if (conn.datareader_qos_set()) {
      // Name specified, must be in map
      result = qos_map_.find(conn.datareader_qos_name());
      if (result != qos_map_.end()) {
        result->second.apply_to(target.datareader_qos());
      } else {
        // Failed
        status = 1;
      }
    }
    if (status == 0 && (conn.subscriber_qos_set())) {
      // Name specified, must be in map
      result = qos_map_.find(conn.subscriber_qos_name());
      if (result != qos_map_.end()) {
        result->second.apply_to(target.subscriber_qos());
      } else {
        // Failed
        status = 1;
      }
    }
  }

  return status;
}

int
Parser::parse_topic(ACE_Configuration_Heap& config,
                    ACE_Configuration_Section_Key& key,
                    const char* topic_name)
{
  int status = 0;
  int value_index = 0;
  ACE_TString value_name, value;
  ACE_Configuration::VALUETYPE value_type;

  TopicSettings topic;

  while (!config.enumerate_values(key,
                                  value_index++,
                                  value_name,
                                  value_type)) {
    if (value_type == ACE_Configuration::STRING) {
      status = config.get_string_value(key, value_name.c_str(), value);
      if (!status) {
        status = status || topic.set(value_name.c_str(), value.c_str());
      }
    } else {
      ACE_ERROR((LM_ERROR, ACE_TEXT("unexpected value type %d\n"), value_type));
      status = -1;
      break;
    }
  }
  if (!status) {
    topic_map_[topic_name] = topic;
  }
  return status;
}

int
Parser::parse_connection(ACE_Configuration_Heap& config,
                         ACE_Configuration_Section_Key& key,
                         const char* connection_name)
{
  int status = 0;
  int value_index = 0;
  ACE_TString value_name, value;
  ACE_Configuration::VALUETYPE value_type;

  ConnectionSettings connection;

  while (!config.enumerate_values(key,
                                  value_index++,
                                  value_name,
                                  value_type)) {
    if (value_type == ACE_Configuration::STRING) {
      status = config.get_string_value(key, value_name.c_str(), value);
      if (!status) {
        status = status || connection.set(value_name.c_str(), value.c_str());
      }
    } else {
      ACE_ERROR((LM_ERROR, ACE_TEXT("unexpected value type %d\n"), value_type));
      status = -1;
      break;
    }
  }
  if (!status) {
    connection_map_[connection_name] = connection;
  }
  return status;
}

int
Parser::parse_qos(ACE_Configuration_Heap& config,
                  ACE_Configuration_Section_Key& key,
                  const char* qos_name,
                  QosSettings::QosLevel level)
{
  int status = 0;
  int value_index = 0;
  ACE_TString value_name, value;
  ACE_Configuration::VALUETYPE value_type;

  // Find existing or create new settings
  QosSettings& qos = qos_map_[qos_name];

  while (!config.enumerate_values(key,
                                  value_index++,
                                  value_name,
                                  value_type)) {
    if (value_type == ACE_Configuration::STRING) {
      status = config.get_string_value(key, value_name.c_str(), value);
      if (!status) {
        status = status ||
                 qos.set_qos(level, value_name.c_str(), value.c_str());
      }
    } else {
      ACE_ERROR((LM_ERROR, ACE_TEXT("unexpected value type %d\n"), value_type));
      status = -1;
      break;
    }
  }
  return status;
}

int
Parser::parse_sections(ACE_Configuration_Heap& config,
                       const char* section_type,
                       bool required)
{
  int status = 0;
  ACE_Configuration_Section_Key key;
  // If we can't open this section
  if (config.open_section(config.root_section(),
                          section_type,
                          0, // don't create if missing
                          key) != 0) {
    if (required) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("Could not open %C section in config file, status %d\n"), section_type, status));
      status = -1;
    }
  // Else, we can open this section
  } else {
    // Open subsections
    int section_index = 0;
    ACE_TString section_name;

    while (!config.enumerate_sections(key,
                                      section_index++,
                                      section_name)) {
      ACE_Configuration_Section_Key subkey;
      // Open subsection
      if (config.open_section(key,
                              section_name.c_str(),
                              0, // don't create if missing
                              subkey) != 0) {
        ACE_ERROR((LM_ERROR, ACE_TEXT("Could not open subsections of %C\n"), section_name.c_str()));
        break;
      }

      if (std::strcmp(section_type, CONNECTION_SECTION) == 0) {
        status = parse_connection(config, subkey, section_name.c_str());
      } else if (std::strcmp(section_type, TOPIC_SECTION) == 0) {
        status = parse_topic(config, subkey, section_name.c_str());
      } else if (std::strcmp(section_type, DATAWRITER_QOS_SECTION) == 0) {
        status = parse_qos(
            config, subkey, section_name.c_str(), QosSettings::datawriter);
      } else if (std::strcmp(section_type, DATAREADER_QOS_SECTION) == 0) {
        status = parse_qos(
            config, subkey, section_name.c_str(), QosSettings::datareader);
      } else if (std::strcmp(section_type, PUBLISHER_QOS_SECTION) == 0) {
        status = parse_qos(
            config, subkey, section_name.c_str(), QosSettings::publisher);
      } else if (std::strcmp(section_type, SUBSCRIBER_QOS_SECTION) == 0) {
        status = parse_qos(
            config, subkey, section_name.c_str(), QosSettings::subscriber);
      } else {
        ACE_ERROR((LM_ERROR, ACE_TEXT("unknown section %C\n"), section_type));
      }
    }
  }
  return status;
}

} } }

OPENDDS_END_VERSIONED_NAMESPACE_DECL
