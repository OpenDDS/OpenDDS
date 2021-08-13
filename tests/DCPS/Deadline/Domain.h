#ifndef DOMAIN_H
#define DOMAIN_H

#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/TimeTypes.h>

class Domain {
public:
  static const int sID = 11;
  static const char* sTopic;
  static const char* sTopicType;
  static const int sKey1 = 11;
  static const int sKey2 = 13;
  static const int N_Msg = 10;
  static const int N_Instance = 2;
  static const int N_Expiration = 2;
  static const DDS::Duration_t W_Deadline;
  static const DDS::Duration_t R_Deadline;
  static const OpenDDS::DCPS::TimeDuration W_Sleep;
  static const OpenDDS::DCPS::TimeDuration R_Sleep;

  Domain(int argc, ACE_TCHAR* argv[], const std::string& app_mame);
  ~Domain() { cleanup(); }
  DDS::DomainParticipant_var dp() const { return dp_; }
  DDS::Topic_var topic() const { return topic_; }

private:
  void cleanup();
  const std::string app_mame_;
  DDS::DomainParticipantFactory_var dpf_;
  DDS::DomainParticipant_var dp_;
  DDS::Topic_var topic_;
};

#endif /* DOMAIN_H */
