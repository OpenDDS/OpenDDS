#pragma once

#include "TestObserver.h"
#include <dds/DCPS/Service_Participant.h>

class Domain {
public:
  static const int ID = 1066;
  static const char* TEST_TOPIC;
  static const char* TEST_TOPIC_TYPE;
  static const CORBA::Long N_READER = 2;

  template<typename Qos>
  static void change_qos(Qos& qos, const std::string& data) {
    qos.user_data.value.length(data.length());
    for (std::string::size_type i = 0; i < data.length(); ++i) {
      qos.user_data.value[i] = data[i];
    }
  }

  Domain(int argc, ACE_TCHAR* argv[], const std::string& app_mame);
  ~Domain();

  DDS::DomainParticipant_var participant;
  DDS::Topic_var topic;

private:
  void cleanup();
  DDS::DomainParticipantFactory_var dpf;
  const std::string appName;
};
