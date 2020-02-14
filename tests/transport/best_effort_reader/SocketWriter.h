#ifndef test_transport_SocketWriter_H
#define test_transport_SocketWriter_H

#include "TestMsg.h"

#include <dds/DCPS/GuidUtils.h>
#include <dds/DCPS/RTPS/RtpsCoreC.h>
#include <dds/DdsDcpsInfoUtilsC.h>

#include <ace/CDR_Base.h>
#include <ace/INET_Addr.h>
#include <ace/SOCK_Dgram.h>

#include <set>

class SocketWriter {
public:
  SocketWriter(const OpenDDS::DCPS::RepoId& id, const ACE_INET_Addr& destination);

  void addDestination(const ACE_INET_Addr& dest);
  bool write(CORBA::ULong seqN, const TestMsg& msg) const;
  bool write(CORBA::ULong seqN, const TestMsg& msg, const OpenDDS::DCPS::RepoId& directedWrite) const;
  bool writeHeartbeat(CORBA::ULong seqN, CORBA::Long heartbeatCount) const;

private:
  static const bool hostIsBigEndian = !ACE_CDR_BYTE_ORDER;
  static const double NTP_FRACTIONAL; // NTP fractional (2^-32) sec per microsec
  static const ACE_CDR::ULong ENCAP = 0x00000100; // {CDR_LE, options} in BE format
  static const CORBA::Octet DE  = OpenDDS::RTPS::FLAG_D | OpenDDS::RTPS::FLAG_E;
  static const CORBA::Octet DEQ = OpenDDS::RTPS::FLAG_D | OpenDDS::RTPS::FLAG_E | OpenDDS::RTPS::FLAG_Q;
  static OpenDDS::RTPS::Header header(OpenDDS::DCPS::RepoId id);

  OpenDDS::RTPS::InfoTimestampSubmessage timeSubMsg() const;
  OpenDDS::RTPS::DataSubmessage dataSubMsg(CORBA::ULong seqN, const TestMsg& msg) const;
  OpenDDS::RTPS::HeartBeatSubmessage heartBeatSubMsg(CORBA::ULong seqN, CORBA::Long heartbeatCount) const;

  size_t msgSize(const OpenDDS::RTPS::InfoTimestampSubmessage& t,
                 const OpenDDS::RTPS::DataSubmessage& d, const TestMsg& m) const;

  size_t hbSize(const OpenDDS::RTPS::HeartBeatSubmessage& hb) const;

  bool serialize(ACE_Message_Block& mb, const OpenDDS::RTPS::InfoTimestampSubmessage& t,
                 const OpenDDS::RTPS::DataSubmessage& d, const TestMsg& m) const;

  bool serialize(ACE_Message_Block& mb, const OpenDDS::RTPS::HeartBeatSubmessage& hb) const;

  bool send(const ACE_Message_Block& mb) const;

  const OpenDDS::DCPS::RepoId id_;
  const OpenDDS::RTPS::Header hdr_;
  ACE_INET_Addr local_addr_;
  ACE_SOCK_Dgram socket_;
  std::set<ACE_INET_Addr> dest_addr_;
};

#endif /* test_transport_SocketWriter_H */
