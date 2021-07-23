#include "SocketWriter.h"

#include "../RtpsUtils.h"

#include <dds/DCPS/RTPS/BaseMessageTypes.h>
#include <dds/DCPS/RTPS/BaseMessageUtils.h>
#include <dds/DCPS/RTPS/MessageTypes.h>
#include <dds/DCPS/RTPS/RtpsCoreC.h>
#include <dds/DCPS/RTPS/RtpsCoreTypeSupportImpl.h>

#include <dds/DCPS/DataSampleElement.h>
#include <dds/DCPS/GuidConverter.h>
#include <dds/DCPS/LogAddr.h>
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
#include <cstring>
#include <ctime>
#include <exception>

using namespace OpenDDS::DCPS;
using namespace OpenDDS::RTPS;

const Encoding SocketWriter::encoding(Encoding::KIND_XCDR1, ENDIAN_LITTLE);
const EncapsulationHeader SocketWriter::encap(SocketWriter::encoding, FINAL);
const double SocketWriter::NTP_FRACTIONAL = 4294.967296; // NTP fractional (2^-32) sec per microsec

Header SocketWriter::header(RepoId id)
{
  const GuidPrefix_t& px = id.guidPrefix;
  Header hdr = {{'R', 'T', 'P', 'S'}, PROTOCOLVERSION, VENDORID_OPENDDS,
    {px[0], px[1], px[2], px[3], px[4], px[5], px[6], px[7], px[8], px[9], px[10], px[11]}};
  return hdr;
}

SocketWriter::SocketWriter(const RepoId& id, const ACE_INET_Addr& destination)
  : id_(id), hdr_(header(id))
{
  if (!open_appropriate_socket_type(socket_, local_addr_)) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: open_appropriate_socket_type\n")));
    throw std::exception();
  }
  addDestination(destination);
}

void SocketWriter::addDestination(const ACE_INET_Addr& dest) {
  dest_addr_.insert(dest);
}

bool SocketWriter::write(CORBA::ULong seqN, const TestMsg& msg) const
{
  const InfoTimestampSubmessage it = timeSubMsg();
  DataSubmessage ds = dataSubMsg(seqN, msg);
  ACE_Message_Block mb(msgSize(it, ds, msg));
  if (serialize(mb, it, ds, msg)) {
    return send(mb);
  }
  return false;
}

bool SocketWriter::write(CORBA::ULong seqN, const TestMsg& msg, const RepoId& directedWrite) const
{
  const InfoTimestampSubmessage it = timeSubMsg();
  CORBA::UShort sz = static_cast<CORBA::UShort>(20 + 24 + 12 + ACE_OS::strlen(msg.value) + 1);
  DataSubmessage ds = {{DATA, DEQ, sz}, 0, 16, ENTITYID_UNKNOWN, id_.entityId, {0, seqN}, ParameterList()};
  ds.inlineQos.length(1);
  ds.inlineQos[0].guid(directedWrite);
  ds.inlineQos[0]._d(PID_DIRECTED_WRITE);
  ACE_Message_Block mb(msgSize(it, ds, msg));
  if (serialize(mb, it, ds, msg)) {
    return send(mb);
  }
  return false;
}

bool SocketWriter::writeHeartbeat(CORBA::ULong seqN, CORBA::Long heartbeatCount,
                                  const OpenDDS::DCPS::RepoId& reader) const
{
  static const unsigned int STARTING_SEQ = 2;
  OpenDDS::DCPS::Message_Block_Ptr mb(buildHeartbeat(id_.entityId, hdr_,
                                                     std::make_pair(toSN(STARTING_SEQ), toSN(seqN)),
                                                     heartbeatCount, reader));
  return mb ? send(*mb) : false;
}

InfoTimestampSubmessage SocketWriter::timeSubMsg() const
{
  const ACE_Time_Value t = ACE_OS::gettimeofday();
  InfoTimestampSubmessage it = {{INFO_TS, 1, 8},
    {static_cast<ACE_CDR::ULong>(t.sec()), static_cast<ACE_CDR::ULong>(t.usec() * NTP_FRACTIONAL)}};
  return it;
}

DataSubmessage SocketWriter::dataSubMsg(CORBA::ULong seqN, const TestMsg& msg) const
{
  CORBA::UShort sz = static_cast<CORBA::UShort>(20 + 12 + ACE_OS::strlen(msg.value) + 1);
  DataSubmessage ds = {{DATA, DE, sz}, 0, 16, ENTITYID_UNKNOWN, id_.entityId, {0, seqN}, ParameterList()};
  return ds;
}

size_t SocketWriter::msgSize(const InfoTimestampSubmessage& t,
                             const DataSubmessage& d, const TestMsg& m) const
{
  size_t size = serialized_size(encoding, hdr_);
  serialized_size(encoding, size, t);
  serialized_size(encoding, size, d);
  primitive_serialized_size_ulong(encoding, size);
  serialized_size(encoding, size, m);
  return size;
}

size_t SocketWriter::hbSize(const HeartBeatSubmessage& hb) const
{
  size_t size = serialized_size(encoding, hdr_);
  serialized_size(encoding, size, hb);
  return size;
}

bool SocketWriter::serialize(ACE_Message_Block& mb, const InfoTimestampSubmessage& t,
                             const DataSubmessage& d, const TestMsg& m) const
{
  Serializer s(&mb, encoding);
  if (!((s << hdr_) && (s << t) && (s << d) && (s << encap) && (s << m))) {
    ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: failed to serialize RTPS message:%m\n")), false);
  }
  return true;
}

bool SocketWriter::serialize(ACE_Message_Block& mb, const HeartBeatSubmessage& hb) const
{
  Serializer s(&mb, encoding);
  if (!((s << hdr_) && (s << hb))) {
    ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: failed to serialize RTPS heartbeat:%m\n")), false);
  }
  return true;
}

bool SocketWriter::send(const ACE_Message_Block& mb) const
{
  for (std::set<ACE_INET_Addr>::const_iterator i = dest_addr_.begin(); i != dest_addr_.end(); ++i) {
    Locator_t locator;
    locator.kind = address_to_kind(*i);
    locator.port = i->get_port_number();
    address_to_bytes(locator.address, *i);
    ACE_INET_Addr dest;
    locator_to_address(dest, locator, local_addr_.get_type() != AF_INET);

    ssize_t res = socket_.send(mb.rd_ptr(), mb.length(), dest);
    if (res >= 0) {
      ACE_DEBUG((LM_INFO, "SocketWriter %C sent %C (%d bytes)\n",
                 OPENDDS_STRING(GuidConverter(id_)).c_str(), LogAddr::ip(*i).c_str(), res));
    } else {
      ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: in socket_.send()%m\n")), false);
    }
  }
  return true;
}
