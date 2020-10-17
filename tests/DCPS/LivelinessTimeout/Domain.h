// -*- C++ -*-
//
#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include <dds/DCPS/Service_Participant.h>

#include <ace/SString.h>

class Domain {
  const std::string appName;
  DDS::DomainParticipantFactory_var dpf_;
public:
  static const int ID = 111;
  static const char* TOPIC;
  static const char* TOPIC_TYPE;
  int lease_duration_sec_;
  ACE_Time_Value test_duration_sec_;
  unsigned int threshold_liveliness_lost_; // default to using TCP
  DDS::DomainParticipant_var participant_;
  DDS::Topic_var topic_;

  Domain(int argc, ACE_TCHAR* argv[], const std::string& app_mame);
  ~Domain() { cleanup(); }
  void parse_args(int argc, ACE_TCHAR* argv[]);
private:
  void cleanup();
};
