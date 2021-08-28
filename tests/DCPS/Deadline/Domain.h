#ifndef DOMAIN_H
#define DOMAIN_H

#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/TimeTypes.h>

class Domain {
public:
  static const int s_id;
  static const char* s_topic;
  static const char* s_topic_type;
  static const int s_key1;
  static const int s_key2;
  static const int n_msg;
  static const int n_instance;
  static const int n_expiration;
  static const DDS::Duration_t w_deadline;
  static const DDS::Duration_t r_deadline;
  static const OpenDDS::DCPS::TimeDuration w_sleep;
  static const OpenDDS::DCPS::TimeDuration r_sleep;
  static const OpenDDS::DCPS::TimeDuration w_interval;

  Domain(int argc, ACE_TCHAR* argv[], const std::string& app_name);
  ~Domain() { cleanup(); }
  DDS::DomainParticipant_var dp() const { return dp_; }
  DDS::Topic_var topic() const { return topic_; }

private:
  void cleanup();
  const std::string app_name_;
  DDS::DomainParticipantFactory_var dpf_;
  DDS::DomainParticipant_var dp_;
  DDS::Topic_var topic_;
};

#endif /* DOMAIN_H */
