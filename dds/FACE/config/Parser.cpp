#include "Parser.h"
#include "ace/Configuration_Import_Export.h"
#include <cstring>

namespace OpenDDS { namespace FaceTSS { namespace config {

namespace {
  const char* TOPIC_SECTION = "topic";
  const char* CONNECTION_SECTION = "connection";
} // anon namespace


ConnectionMap Parser::connection_map_;
TopicMap Parser::topic_map_;

int
Parser::parse(const char* filename)
{
  ACE_Configuration_Heap config;
  config.open();
  ACE_Ini_ImpExp import(config);
  int status = import.import_config(filename);
  if (status != 0) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: Initialize() ")
               ACE_TEXT("import_config () returned %d\n"),
               status));
    return status;
  } else {
    status = parse_sections(config, TOPIC_SECTION) ||
             parse_sections(config, CONNECTION_SECTION);
  }
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
                                  value_type))
  {
    if (value_type == ACE_Configuration::STRING) {
      status = config.get_string_value(key, value_name.c_str(), value);
      if (!status) {
        status = status || topic.set(value_name.c_str(), value.c_str());
      }
    } else {
      printf("unexpected value type %d\n", value_type);
      status = -1;
      break;
    }
  }
  if (!status) {
    topic_map_.insert(std::make_pair(topic_name, topic));
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
                                  value_type))
  {
    if (value_type == ACE_Configuration::STRING) {
      status = config.get_string_value(key, value_name.c_str(), value);
      if (!status) {
        status = status || connection.set(value_name.c_str(), value.c_str());
      }
    } else {
      printf("unexpected value type %d\n", value_type);
      status = -1;
      break;
    }
  }
  if (!status) {
    connection_map_.insert(std::make_pair(connection_name, connection));
  }
  return status;
}

int
Parser::parse_sections(ACE_Configuration_Heap& config,
                       const char* section_type)
{
  int status = 1;
  ACE_Configuration_Section_Key key;
  if (config.open_section(config.root_section(),
                          section_type,
                          0, // don't create if missing
                          key) != 0)
  {
    printf("Could not open %s section in config file\n", section_type);
  } else {
    // Open subsections 
    int section_index = 0;
    ACE_TString section_name;
    while (!config.enumerate_sections(key,
                                      section_index++,
                                      section_name))
    {
      ACE_Configuration_Section_Key subkey;
      // Open subsection
      if (config.open_section(key,
                              section_name.c_str(),
                              0, // don't create if missing
                              subkey) != 0)
      {
        printf("Could not open subsections of %s\n", section_name.c_str());
        break;
      }

      if (std::strcmp(section_type, CONNECTION_SECTION) == 0) {
        status = parse_connection(config, subkey, section_name.c_str());
      } else if (std::strcmp(section_type, TOPIC_SECTION) == 0) {
        status = parse_topic(config, subkey, section_name.c_str());
      } else {
        printf("unknown section %s\n", section_type);
      }
    }
  }
  return status;
}

} } }
