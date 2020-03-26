#include "TestMsg.h"
#include "AppConfig.h"

#include "dds/DCPS/transport/framework/TransportSendListener.h"
#include "dds/DCPS/transport/rtps_udp/RtpsUdpDataLink.h"

#include "dds/DCPS/RTPS/RtpsCoreTypeSupportImpl.h"
#include "dds/DCPS/RTPS/BaseMessageTypes.h"
#include "dds/DCPS/RTPS/BaseMessageUtils.h"
#include "dds/DCPS/RTPS/RtpsCoreC.h"
#include "dds/DCPS/Serializer.h"
#include "dds/DCPS/DataSampleElement.h"
#include <dds/DCPS/transport/framework/NetworkAddress.h>

#include <ace/OS_main.h>
#include <ace/Basic_Types.h>
#include <ace/CDR_Base.h>
#include <ace/String_Base.h>
#include <ace/SOCK_Dgram.h>
#include <ace/Message_Block.h>

#include <ace/OS_NS_sys_time.h>
#include <ace/OS_NS_time.h>
#include <ctime>

class DDS_TEST {
public:
  DDS_TEST(int argc, ACE_TCHAR* argv[]) : config(argc, argv), msgSeqN(0), heartbeat_count_(0) {
    config.setHeartbeatPeriod(100);

    ACE_INET_Addr remoteAddr = config.getHostAddress();
    locators.length(1);
    locators[0].kind = OpenDDS::RTPS::address_to_kind(remoteAddr);
    locators[0].port = remoteAddr.get_port_number();
    OpenDDS::RTPS::address_to_bytes(locators[0].address, remoteAddr);
  }

  int run() {
    bool ret = writeHeartbeat() &&
               writeToSocket(TestMsg(10, "Msg1")) &&
               writeToSocket(TestMsg(10, "Msg2 DirectedWrite"), DEQ) &&
               writeToSocket(TestMsg(10, "Msg3")) &&
               writeToSocket(TestMsg(99, "key=99 end")) &&
               writeHeartbeat();

    // Allow enough time for a HEARTBEAT message to be generated
    ACE_OS::sleep((config.getHeartbeatPeriod() * 2.0).value());
    return (ret ? 0 : 1);
  }

private:
  static const bool hostIsBigEndian = !ACE_CDR_BYTE_ORDER;
  static const ACE_CDR::ULong encap = 0x00000100; // {CDR_LE, options} in BE format
  static const CORBA::Octet DE  = OpenDDS::RTPS::FLAG_D | OpenDDS::RTPS::FLAG_E;
  static const CORBA::Octet DEQ = OpenDDS::RTPS::FLAG_D | OpenDDS::RTPS::FLAG_E | OpenDDS::RTPS::FLAG_Q;

  void force_inline_qos(bool val) {
    OpenDDS::DCPS::RtpsUdpDataLink::force_inline_qos_ = val;
  }
  void log_time(const ACE_Time_Value& t) const {
    ACE_TCHAR buffer[32];
    const std::time_t seconds = t.sec();
    std::string ts(ACE_TEXT_ALWAYS_CHAR(ACE_OS::ctime_r(&seconds, buffer, 32)));
    ts.erase(ts.size() - 1); // remove \n from ctime()
    ACE_DEBUG((LM_INFO, ACE_TEXT("Sending with timestamp %C %q usec\n"), ts.c_str(), ACE_INT64(t.usec())));
  }

  bool writeHeartbeat() const;
  bool writeToSocket(const TestMsg& msg, const CORBA::Octet flags = DE) const;

  AppConfig config;
  LocatorSeq locators;
  mutable CORBA::ULong msgSeqN;
  mutable CORBA::Long heartbeat_count_;
};

using namespace OpenDDS::RTPS;

bool DDS_TEST::writeHeartbeat() const
{
  const OpenDDS::RTPS::GuidPrefix_t& local_prefix = config.getPubWtrId().guidPrefix;
  const Header hdr = { {'R', 'T', 'P', 'S'}, PROTOCOLVERSION, VENDORID_OPENDDS,
    {local_prefix[0], local_prefix[1], local_prefix[2], local_prefix[3],
     local_prefix[4], local_prefix[5], local_prefix[6], local_prefix[7],
     local_prefix[8], local_prefix[9], local_prefix[10], local_prefix[11]} };
  const HeartBeatSubmessage hb = {
    {HEARTBEAT, 0, HEARTBEAT_SZ},
    ENTITYID_UNKNOWN, // any matched reader
    config.getPubWtrId().entityId,
    {0, 1},
    {0, msgSeqN},
    {++heartbeat_count_}
  };

  size_t size = 0, padding = 0;
  gen_find_size(hdr, size, padding);
  gen_find_size(hb, size, padding);

  ACE_Message_Block mb(size + padding);
  Serializer ser(&mb, hostIsBigEndian, Serializer::ALIGN_CDR);
  bool ok = (ser << hdr) && (ser << hb);
  if (!ok) {
    ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: failed to serialize RTPS heartbeat:%m\n")), false);
  }

  ACE_INET_Addr local_addr;
  ACE_SOCK_Dgram sock;
  if (!open_appropriate_socket_type(sock, local_addr)) {
    ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: open_appropriate_socket_type:%m\n")), false);
  }

  ACE_INET_Addr dest;
  locator_to_address(dest, locators[0], local_addr.get_type() != AF_INET);
  ssize_t res = sock.send(mb.rd_ptr(), mb.length(), dest);
  if (res < 0) {
    ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: in sock.send()%m\n")), false);
  } else {
    ACE_DEBUG((LM_INFO, "Sent %d bytes.\n", res));
  }

  return true;
}

bool DDS_TEST::writeToSocket(const TestMsg& msg, const CORBA::Octet flags) const
{
  const OpenDDS::RTPS::GuidPrefix_t& local_prefix = config.getPubWtrId().guidPrefix;
  const Header hdr = { {'R', 'T', 'P', 'S'}, PROTOCOLVERSION, VENDORID_OPENDDS,
    {local_prefix[0], local_prefix[1], local_prefix[2], local_prefix[3],
     local_prefix[4], local_prefix[5], local_prefix[6], local_prefix[7],
     local_prefix[8], local_prefix[9], local_prefix[10], local_prefix[11]} };

  const ACE_Time_Value now = ACE_OS::gettimeofday();
  log_time(now);
  const double conv = 4294.967296; // NTP fractional (2^-32) sec per microsec
  const InfoTimestampSubmessage it = { {INFO_TS, 1, 8},
    {static_cast<ACE_CDR::ULong>(now.sec()),
     static_cast<ACE_CDR::ULong>(now.usec() * conv)} };
  CORBA::UShort sz = static_cast<CORBA::UShort>(
    20 + (flags == DEQ ? 24 : 0) + 12 + ACE_OS::strlen(msg.value) + 1);
  DataSubmessage ds = { {DATA, flags, sz}, 0, 16, ENTITYID_UNKNOWN,
    config.getPubWtrId().entityId, {0, ++msgSeqN}, ParameterList() };
  if (flags == DEQ) {
    ds.inlineQos.length(1);
    ds.inlineQos[0].guid(config.getSubRdrId(1));
    ds.inlineQos[0]._d(OpenDDS::RTPS::PID_DIRECTED_WRITE);
  }

  size_t size = 0, padding = 0;
  gen_find_size(hdr, size, padding);
  gen_find_size(it, size, padding);
  gen_find_size(ds, size, padding);
  find_size_ulong(size, padding);
  gen_find_size(msg, size, padding);

  ACE_Message_Block mb(size + padding);
  Serializer ser(&mb, hostIsBigEndian, Serializer::ALIGN_CDR);
  bool ok = (ser << hdr) && (ser << it) && (ser << ds) && (ser << encap) && (ser << msg);
  if (!ok) {
    ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: failed to serialize RTPS message:%m\n")), false);
  }

  ACE_INET_Addr local_addr;
  ACE_SOCK_Dgram sock;
  if (!open_appropriate_socket_type(sock, local_addr)) {
    ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: open_appropriate_socket_type:%m\n")), false);
  }

  ACE_INET_Addr dest;
  locator_to_address(dest, locators[0], local_addr.get_type() != AF_INET);
  ssize_t res = sock.send(mb.rd_ptr(), mb.length(), dest);
  if (res < 0) {
    ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: in sock.send()%m\n")), false);
  } else {
    ACE_DEBUG((LM_INFO, "Sent %d bytes.\n", res));
  }

  return true;
}

int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
  try {
    DDS_TEST test(argc, argv);
    return test.run();
  } catch (...) {
    return 1;
  }
}
