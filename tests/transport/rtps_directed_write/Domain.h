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

class Domain {
public:
  Domain(int argc, ACE_TCHAR* argv[]);
  ~Domain();

  const OpenDDS::DCPS::RepoId& getPubWtrId() const { return pubWtrId; }
  const OpenDDS::DCPS::RepoId& getSubRdrId(int i) const { return subRdrId[i]; }

  ACE_INET_Addr getHostAddress() const;
  void setHostAddress();

  const OpenDDS::DCPS::TimeDuration& getHeartbeatPeriod() const;
  void setHeartbeatPeriod(const ACE_UINT64& ms);

private:
  DDS::DomainParticipantFactory_var dpf;
  ACE_TString host;
  u_short port;
  OpenDDS::DCPS::TransportInst_rch transportInst;
  OpenDDS::DCPS::RtpsUdpInst* rtpsInst;

  OpenDDS::DCPS::RepoId pubWtrId;
  OpenDDS::DCPS::RepoId subRdrId[2];

  OpenDDS::DCPS::RepoId createID(long participantId, long key, CORBA::Octet kind);
  void cleanup();
};
