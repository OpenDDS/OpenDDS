#pragma once

#include "dds/DCPS/RepoIdBuilder.h"
#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/transport/framework/TransportRegistry.h"
#include "dds/DCPS/transport/rtps_udp/RtpsUdpInst.h"
#include "dds/DCPS/RTPS/BaseMessageUtils.h"
#ifdef ACE_AS_STATIC_LIBS
#include "dds/DCPS/transport/rtps_udp/RtpsUdp.h"
#endif
#include <ace/Basic_Types.h>

class AppConfig {
public:
  AppConfig(int argc, ACE_TCHAR *argv[], bool setLocalAddress = false);
  ~AppConfig();

  const OpenDDS::DCPS::RepoId& getPubWtrId() const { return pubWtrId; }
  const OpenDDS::DCPS::RepoId& getSubRdrId(int i) const { return subRdrId[i]; }

  ACE_INET_Addr getHostAddress() const;
  const OpenDDS::DCPS::TimeDuration& getHeartbeatPeriod() const;
  void setHeartbeatPeriod(const ACE_UINT64& ms);
  bool readersReliable() const { return reliableReaders; }

private:
  OpenDDS::DCPS::RepoId createID(long participantId, long key, CORBA::Octet kind);
  void cleanup();

  DDS::DomainParticipantFactory_var dpf;
  ACE_TString host;
  u_short port;
  bool reliableReaders;
  OpenDDS::DCPS::TransportInst_rch transportInst;
  OpenDDS::DCPS::RtpsUdpInst* rtpsInst;
  OpenDDS::DCPS::RepoId pubWtrId;
  OpenDDS::DCPS::RepoId subRdrId[2];
};
