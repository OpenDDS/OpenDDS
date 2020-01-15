#pragma once

#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/transport/framework/TransportRegistry.h"
#include "dds/DCPS/transport/rtps_udp/RtpsUdpInst.h"

class AppConfig {
public:
  AppConfig(int argc, ACE_TCHAR *argv[], bool setLocalAddress = false);
  ~AppConfig();

  const OpenDDS::DCPS::RepoId& getPubWtrId(int i) const { return pubWtrId[i]; }
  const OpenDDS::DCPS::RepoId& getSubRdrId(int i) const { return subRdrId[i]; }

  ACE_INET_Addr getHostAddress() const;
  const OpenDDS::DCPS::TimeDuration& getHeartbeatPeriod() const;
  void setHeartbeatPeriod(const ACE_UINT64& ms);
  void to_cerr(const OpenDDS::DCPS::RepoId& remote, const OpenDDS::DCPS::RepoId& local, const std::string& txt) const;

private:
  OpenDDS::DCPS::RepoId createID(long participantId, long key, CORBA::Octet kind);
  void cleanup();

  DDS::DomainParticipantFactory_var dpf;
  ACE_TString host;
  u_short port;
  OpenDDS::DCPS::TransportInst_rch transportInst;
  OpenDDS::DCPS::RtpsUdpInst* rtpsInst;
  OpenDDS::DCPS::RepoId pubWtrId[3];
  OpenDDS::DCPS::RepoId subRdrId[3];
};
