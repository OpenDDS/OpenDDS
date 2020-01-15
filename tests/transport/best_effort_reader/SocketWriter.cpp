#include "SocketWriter.h"

#include <dds/DCPS/RTPS/RtpsCoreTypeSupportImpl.h>
#include <dds/DCPS/RTPS/BaseMessageTypes.h>
#include <dds/DCPS/RTPS/BaseMessageUtils.h>

#include <dds/DCPS/DataSampleElement.h>
#include <dds/DCPS/GuidConverter.h>
#include <dds/DCPS/Message_Block_Ptr.h>
#include <dds/DCPS/Serializer.h>
#include <dds/DCPS/SendStateDataSampleList.h>
#include <dds/DCPS/transport/framework/NetworkAddress.h>

#include <dds/DdsDcpsGuidC.h>

#include <tao/CORBA_String.h>
#include <ace/Basic_Types.h>
#include <ace/String_Base.h>
#include <ace/OS_NS_sys_time.h>
#include <ace/OS_NS_time.h>
#include <ace/OS_NS_unistd.h>

#include <iostream>
#include <sstream>
#include <cstring>
#include <ctime>

using namespace OpenDDS::RTPS;

const double SocketWriter::NTP_FRACTIONAL = 4294.967296; // NTP fractional (2^-32) sec per microsec

OpenDDS::RTPS::Header SocketWriter::header(OpenDDS::DCPS::RepoId id)
{
  const OpenDDS::RTPS::GuidPrefix_t& px = id.guidPrefix;
  OpenDDS::RTPS::Header hdr = {{'R', 'T', 'P', 'S'}, PROTOCOLVERSION, VENDORID_OPENDDS,
    {px[0], px[1], px[2], px[3], px[4], px[5], px[6], px[7], px[8], px[9], px[10], px[11]}};
  return hdr;
}

SocketWriter::SocketWriter(const OpenDDS::DCPS::RepoId& id, const ACE_INET_Addr& destination)
  : id_(id), hdr_(header(id))
{
  if (!OpenDDS::DCPS::open_appropriate_socket_type(socket_, local_addr_)) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: open_appropriate_socket_type\n")));
    throw;
  }
  addDestination(destination);
}

void SocketWriter::addDestination(const ACE_INET_Addr& dest) {
  dest_addr_.insert(dest);
}

bool SocketWriter::write(const TestMsg& msg, CORBA::ULong seqN) const
{
  const OpenDDS::RTPS::InfoTimestampSubmessage it = timeSubMsg();
  OpenDDS::RTPS::DataSubmessage ds = dataSubMsg(msg, seqN);
  ACE_Message_Block mb(msgSize(it, ds, msg));
  if (serialize(mb, it, ds, msg)) {
    return send(mb);
  }
  return false;
}

bool SocketWriter::write(const TestMsg& msg, CORBA::ULong seqN, const OpenDDS::DCPS::RepoId& directedWrite) const
{
  const OpenDDS::RTPS::InfoTimestampSubmessage it = timeSubMsg();
  CORBA::UShort sz = 20 + 24 + 12 + ACE_OS::strlen(msg.value) + 1;
  DataSubmessage ds = {{DATA, DEQ, sz}, 0, 16, ENTITYID_UNKNOWN, id_.entityId, {0, seqN}, ParameterList()};
  ds.inlineQos.length(1);
  ds.inlineQos[0].guid(directedWrite);
  ds.inlineQos[0]._d(OpenDDS::RTPS::PID_DIRECTED_WRITE);
  ACE_Message_Block mb(msgSize(it, ds, msg));
  if (serialize(mb, it, ds, msg)) {
    return send(mb);
  }
  return false;
}

OpenDDS::RTPS::InfoTimestampSubmessage SocketWriter::timeSubMsg() const
{
  const ACE_Time_Value t = ACE_OS::gettimeofday();
  OpenDDS::RTPS::InfoTimestampSubmessage it = {{INFO_TS, 1, 8},
    {static_cast<ACE_CDR::ULong>(t.sec()), static_cast<ACE_CDR::ULong>(t.usec() * NTP_FRACTIONAL)}};
  return it;
}

OpenDDS::RTPS::DataSubmessage SocketWriter::dataSubMsg(const TestMsg& msg, CORBA::ULong seqN) const
{
  CORBA::UShort sz = 20 + 12 + ACE_OS::strlen(msg.value) + 1;
  DataSubmessage ds = {{DATA, DE, sz}, 0, 16, ENTITYID_UNKNOWN, id_.entityId, {0, seqN}, ParameterList()};
  return ds;
}

size_t SocketWriter::msgSize(const OpenDDS::RTPS::InfoTimestampSubmessage& t,
                             const OpenDDS::RTPS::DataSubmessage& d, const TestMsg& m) const
{
  size_t size = 0, padding = 0;
  gen_find_size(hdr_, size, padding);
  gen_find_size(t, size, padding);
  gen_find_size(d, size, padding);
  find_size_ulong(size, padding);
  gen_find_size(m, size, padding);
  return size + padding;
}

bool SocketWriter::serialize(ACE_Message_Block& mb, const OpenDDS::RTPS::InfoTimestampSubmessage& t,
                             const OpenDDS::RTPS::DataSubmessage& d, const TestMsg& m) const
{
  Serializer s(&mb, hostIsBigEndian, Serializer::ALIGN_CDR);
  bool ok = (s << hdr_) && (s << t) && (s << d) && (s << ENCAP) && (s << m);
  if (!ok) {
    ACE_DEBUG((LM_INFO, ACE_TEXT("ERROR: failed to serialize RTPS message\n")));
  }
  return ok;
}

bool SocketWriter::send(const ACE_Message_Block& mb) const
{
  for (std::set<ACE_INET_Addr>::const_iterator i = dest_addr_.begin(); i != dest_addr_.end(); ++i) {
    ssize_t res = socket_.send(mb.rd_ptr(), mb.length(), *i);
    if (res >= 0) {
      std::ostringstream oss;
      oss << "SocketWriter " << id_ << " sent " << i->get_host_addr() << " (" << res << " bytes)\n";
      ACE_DEBUG((LM_INFO, oss.str().c_str()));
    } else {
      ACE_DEBUG((LM_INFO, ACE_TEXT("ERROR: in socket_.send()\n")));
      return false;
    }
  }
  return true;
}
