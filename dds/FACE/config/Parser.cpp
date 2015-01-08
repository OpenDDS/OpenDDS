#include "Parser.h"
#include "ace/Configuration_Import_Export.h"

namespace OpenDDS { namespace FaceTSS { namespace config {

namespace {

enum SectionType {
  ignored,
  connection,
  topic
};

int
parse_section(ACE_Configuration_Heap& config,
              ACE_Configuration_Section_Key& key,
              SectionType section_type)
{
  int status = 0;

  int value_index = 0;
  ACE_TString value_name, value;
  ACE_Configuration::VALUETYPE value_type;
printf("parsing section\n");

  while (!config.enumerate_values(key,
                                  value_index++,
                                  value_name,
                                  value_type))
  {
    if (value_type == ACE_Configuration::STRING) {
      status = config.get_string_value(key, value_name.c_str(), value);
      if (!status) {
        printf("  %s: %s\n", value_name.c_str(), value.c_str());
      }
    } else {
      status = -1;
      break;
    }
  }
                          
  return status;
}

} // anon namespace

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
    int section_index = 0;
    ACE_TString section_name;
    while (!config.enumerate_sections(config.root_section(),
                                      section_index++,
                                      section_name)) {
      SectionType section_type = ignored;
printf("found section %s\n", section_name.c_str());

      if (0 == strcmp(section_name.c_str(), "topic")) {
        section_type = topic;
      } else if (0 == strcmp(section_name.c_str(), "connection")) {
        section_type = connection;
      }

      if (section_type != ignored) {
        ACE_Configuration_Section_Key subkey;
        if (config.open_section(config.root_section(),
                                section_name.c_str(),
                                0,
                                subkey) != 0) {
          return 1;
        } else {
          status = parse_section(config, subkey, section_type);
        }
      }
    }
  }
  return status;
}

} } }
