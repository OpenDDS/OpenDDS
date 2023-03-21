#ifndef tests_transport_best_effort_reader_AppConfig_h
#define tests_transport_best_effort_reader_AppConfig_h

#include <dds/DCPS/Service_Participant.h>

class AppConfig {
public:
  static const OpenDDS::DCPS::GUID_t writerId[3];
  static const OpenDDS::DCPS::GUID_t readerId[3];

  AppConfig(int argc, ACE_TCHAR* argv[]);
  ~AppConfig();

  ACE_INET_Addr getHostAddress() const;
  bool configureTransport();
  void to_cerr(const OpenDDS::DCPS::GUID_t& remote, const OpenDDS::DCPS::GUID_t& local, const std::string& txt) const;

private:
  static OpenDDS::DCPS::GUID_t createID(long participantId, long key, CORBA::Octet kind);
  void cleanup();

  DDS::DomainParticipantFactory_var dpf;
  ACE_TString host;
  u_short port;
};

#endif
