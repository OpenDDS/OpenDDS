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
      ACE_TString ttoken(ACE_TEXT_CHAR_TO_TCHAR(token));
      if (ttoken.c_str()) {
        ACE_Configuration_Section_Key sect_key;
        ach.open_section(keys.back(), ttoken.c_str(), 1, sect_key);
        keys.push_back(sect_key);
        token = std::strtok(NULL, "/");
      }
    }
    for (CORBA::ULong j = 0; j < section.properties.length(); ++j) {
      Log::log()
        << "Adding '" << section.properties[j].name
        <<"' = '" << section.properties[j].value
        << "' to [" << section.name << "] configuration section" << std::endl;
      ACE_TString tname(ACE_TEXT_CHAR_TO_TCHAR(section.properties[j].name.in()));
      ACE_TString tvalue(ACE_TEXT_CHAR_TO_TCHAR(section.properties[j].value.in()));
      ach.set_string_value(keys.back(), tname.c_str(), tvalue.c_str());
    }
  }
  TheServiceParticipant->load_configuration(ach, ACE_TEXT(""));
}

}

