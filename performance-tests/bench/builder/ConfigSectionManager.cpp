#include "ConfigSectionManager.h"

#include "dds/DCPS/Service_Participant.h"

#include "ace/Configuration.h"

#include <iostream>

namespace Builder {

ConfigSectionManager::ConfigSectionManager(const ConfigSectionSeq& seq) {
  ACE_Configuration_Heap ach;
  ach.open();
  for (CORBA::ULong i = 0; i < seq.length(); ++i) {
    const ConfigSection& section = seq[i];
    std::vector<char> buff(std::strlen(section.name.in()) + 1);
    std::strcpy(&(buff[0]), section.name.in());
    char* token = std::strtok(&(buff[0]), "/");
    std::vector<ACE_Configuration_Section_Key> keys;
    keys.push_back(ach.root_section());
    while (token != NULL) {
      ACE_Configuration_Section_Key sect_key;
      ach.open_section(keys.back(), token, 1, sect_key);
      keys.push_back(sect_key);
      token = std::strtok(NULL, "/");
    }
    for (CORBA::ULong j = 0; j < section.properties.length(); ++j) {
      std::cout << "Adding '" << section.properties[j].name << "' = '" << section.properties[j].value << "' to [" << section.name << "] configuration section" << std::endl;
      ach.set_string_value(keys.back(), section.properties[j].name.in(), section.properties[j].value.in());
    }
  }
  TheServiceParticipant->load_configuration(ach, "");
}

}

