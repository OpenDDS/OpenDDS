/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "MessageUtils.h"

#include "MessageTypes.h"

#include <dds/DCPS/Time_Helper.h>
#include <dds/DCPS/debug.h>

#include <dds/OpenddsDcpsExtTypeSupportImpl.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace RTPS {

using DCPS::Encoding;
using DCPS::LogLevel;
using DCPS::log_level;

const DCPS::Encoding& get_locators_encoding()
{
  static const Encoding encoding(Encoding::KIND_XCDR1, DCPS::ENDIAN_BIG);
  return encoding;
}

DDS::ReturnCode_t blob_to_locators(const DCPS::TransportBLOB& blob,
                                   DCPS::LocatorSeq& locators,
                                   VendorId_t& vendor_id,
                                   bool* requires_inline_qos,
                                   unsigned int* pBytesRead)
{
  ACE_Data_Block db(blob.length(), ACE_Message_Block::MB_DATA,
                    reinterpret_cast<const char*>(blob.get_buffer()),
                    0 /*alloc*/, 0 /*lock*/, ACE_Message_Block::DONT_DELETE, 0 /*db_alloc*/);
  ACE_Message_Block mb(&db, ACE_Message_Block::DONT_DELETE, 0 /*mb_alloc*/);
  mb.wr_ptr(mb.space());

  DCPS::Serializer ser(&mb, get_locators_encoding());
  if (!(ser >> locators)) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) blob_to_locators: ")
                      ACE_TEXT("Failed to deserialize blob's locators\n")),
                     DDS::RETCODE_ERROR);
  }

  if (!(ser >> vendor_id)) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) blob_to_locators: ")
                      ACE_TEXT("Failed to deserialize blob's vendor id\n")),
                     DDS::RETCODE_ERROR);
  }

  if (requires_inline_qos) {
    if (!(ser >> ACE_InputCDR::to_boolean(*requires_inline_qos))) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) blob_to_locators: ")
                        ACE_TEXT("Failed to deserialize blob inline QoS flag\n")),
                       DDS::RETCODE_ERROR);
    }
  } else {
    if (!ser.skip(1)) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) blob_to_locators: ")
                        ACE_TEXT("Failed to skip blob inline QoS flag\n")),
                       DDS::RETCODE_ERROR);
    }
  }

  if (pBytesRead) {
    *pBytesRead = blob.length() - static_cast<unsigned int>(mb.length());
  }
  return DDS::RETCODE_OK;
}

void locators_to_blob(const DCPS::LocatorSeq& locators,
                      const VendorId_t& vendor_id,
                      DCPS::TransportBLOB& blob)
{
  using namespace OpenDDS::DCPS;
  const Encoding& encoding = get_locators_encoding();
  size_t size = 0;
  serialized_size(encoding, size, locators);
  serialized_size(encoding, size, vendor_id);
  DCPS::primitive_serialized_size_boolean(encoding, size);
  ACE_Message_Block mb_locator(size);
  Serializer ser(&mb_locator, encoding);
  if (!(ser << locators)) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) locators_to_blob: ")
      ACE_TEXT("Failed to serialize locators to blob\n")));
  }
  if (!(ser << vendor_id)) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) locators_to_blob: ")
               ACE_TEXT("Failed to serialize vendor id to blob\n")));
  }
  // Add a bool for 'requires inline qos', see Sedp::set_inline_qos():
  // if the bool is no longer the last octet of the sequence then that function
  // must be changed as well.
  if (!(ser << ACE_OutputCDR::from_boolean(false))) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) locators_to_blob: ")
      ACE_TEXT("Failed to serialize boolean for blob\n")));
  }
  message_block_to_sequence(mb_locator, blob);
}

OpenDDS_Rtps_Export
DCPS::LocatorSeq transport_locator_to_locator_seq(const DCPS::TransportLocator& info)
{
  DCPS::LocatorSeq locators;
  VendorId_t vendor_id;
  blob_to_locators(info.data, locators, vendor_id);
  return locators;
}

DCPS::TimeDuration rtps_duration_to_time_duration(const Duration_t& rtps_duration, const ProtocolVersion_t& version, const VendorId_t& vendor)
{
  if (rtps_duration == DURATION_INFINITE) {
    return DCPS::TimeDuration::max_value;
  }

  if (version < PROTOCOLVERSION_2_4 && vendor == VENDORID_OPENDDS) {
    return OpenDDS::DCPS::TimeDuration(
      rtps_duration.seconds,
      static_cast<ACE_UINT32>(rtps_duration.fraction / 1000));
  } else {
    return OpenDDS::DCPS::TimeDuration(
      rtps_duration.seconds,
      DCPS::uint32_fractional_seconds_to_microseconds(rtps_duration.fraction));
  }
}

bool bitmapNonEmpty(const SequenceNumberSet& snSet)
{
  const CORBA::ULong num_ulongs = (snSet.numBits + 31) / 32;

  OPENDDS_ASSERT(num_ulongs <= snSet.bitmap.length());

  if (num_ulongs == 0) {
    return false;
  }

  const CORBA::ULong last_index = num_ulongs - 1;
  for (CORBA::ULong i = 0; i < last_index; ++i) {
    if (snSet.bitmap[i]) {
      return true;
    }
  }

  const CORBA::ULong mod = snSet.numBits % 32;
  const CORBA::ULong mask = mod ? (1 + ~(1u << (32 - mod))) : 0xFFFFFFFF;
  return (bool)(snSet.bitmap[last_index] & mask);
}

bool get_rtps_port(DDS::UInt16& port_result, const char* what,
  DDS::UInt16 port_base, DDS::UInt16 offset,
  DDS::UInt16 domain, DDS::UInt16 domain_gain,
  DDS::UInt16 part, DDS::UInt16 part_gain)
{
  const DDS::UInt32 port = static_cast<DDS::UInt32>(port_base) +
    domain * domain_gain + part * part_gain + offset;
  port_result = static_cast<DDS::UInt16>(port);
  if (port > 65535) {
    if (log_level >= LogLevel::Warning) {
      ACE_ERROR((LM_WARNING, "(%P|%t) WARNING: rtps_port: "
        "%C port %u is going to be truncated to %u. This behavior is deprecated, please reduce "
        "domain ID or other RTPS port parameters.\n",
        what, port, port_result));
    }
  }
  return true;
}

namespace {
  const DCPS::EnumList<PortMode> port_modes[] = {
    {PortMode_System, "system"},
    {PortMode_Probe, "probe"}
  };
};

PortMode get_port_mode(const String& key, PortMode default_value)
{
  return TheServiceParticipant->config_store()->get(key.c_str(), default_value, port_modes);
}

void set_port_mode(const String& key, PortMode value)
{
  TheServiceParticipant->config_store()->set(key.c_str(), value, port_modes);
}

bool set_rtps_multicast_port(
  DCPS::NetworkAddress& addr, const char* what,
  DDS::UInt16 port_base, DDS::UInt16 offset,
  DDS::UInt16 domain, DDS::UInt16 domain_gain)
{
  if (addr.get_port_number() == 0) {
    DDS::UInt16 port;
    if (!get_rtps_port(port, what, port_base, offset, domain, domain_gain)) {
      return false;
    }
    addr.set_port_number(port);
  }
  return true;
}

bool set_rtps_unicast_port(
  DCPS::NetworkAddress& addr, bool& fixed_port,
  const char* what, PortMode port_mode,
  DDS::UInt16 port_base, DDS::UInt16 offset,
  DDS::UInt16 domain, DDS::UInt16 domain_gain,
  DDS::UInt16 part, DDS::UInt16 part_gain)
{
  fixed_port = addr.get_port_number() > 0 || port_mode != PortMode_Probe;
  if (!fixed_port) {
    DDS::UInt16 port = 0;
    if (!get_rtps_port(port, what, port_base, offset, domain, domain_gain, part, part_gain)) {
      return false;
    }
    addr.set_port_number(port);
  }
  return true;
}

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
