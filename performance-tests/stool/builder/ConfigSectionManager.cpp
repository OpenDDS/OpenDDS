#include "ConfigSectionManager.h"

#include "dds/DCPS/Service_Participant.h"

#include "ace/Configuration.h"

#include <iostream>

namespace Builder {

ConfigSectionManager::ConfigSectionManager(const ConfigSectionSeq& seq) {
  ACE_Configuration_Heap ach;
  ACE_Configuration_Section_Key sect_key;
  ach.open();
  for (CORBA::ULong i = 0; i < seq.length(); ++i) {
    const ConfigSection& section = seq[i];
    ach.open_section(ach.root_section(), section.name.in(), 1, sect_key);
    for (CORBA::ULong j = 0; j < section.properties.length(); ++j) {
      std::cout << "Adding '" << section.properties[j].name << "' = '" << section.properties[j].value << "' to [" << section.name << "] configuration section" << std::endl;
      ach.set_string_value(sect_key, section.properties[j].name.in(), section.properties[j].value.in());
    }
  }
  TheServiceParticipant->load_configuration(ach, "");
}

}

