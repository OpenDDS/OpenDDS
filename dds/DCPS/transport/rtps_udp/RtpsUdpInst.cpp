/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "RtpsUdpInst.h"
#include "RtpsUdpLoader.h"
#include "RtpsUdpTransport.h"

#include "dds/DCPS/transport/framework/TransportDefs.h"
#include "dds/DCPS/SafetyProfileStreams.h"
#include "ace/Configuration.h"


namespace OpenDDS {
namespace DCPS {

RtpsUdpInst::RtpsUdpInst(const OPENDDS_STRING& name)
  : TransportInst("rtps_udp", name)
  , use_multicast_(true)
  , ttl_(1)
  , multicast_group_address_(7401, "239.255.0.2")
  , nak_depth_(32) // default nak_depth in OpenDDS_Multicast
  , nak_response_delay_(0, 200*1000 /*microseconds*/) // default from RTPS
  , heartbeat_period_(1) // no default in RTPS spec
  , heartbeat_response_delay_(0, 500*1000 /*microseconds*/) // default from RTPS
  , handshake_timeout_(30) // default syn_timeout in OpenDDS_Multicast
  , durable_data_timeout_(60)
  , opendds_discovery_default_listener_(0)
  , opendds_discovery_guid_(GUID_UNKNOWN)
{
}

TransportImpl*
RtpsUdpInst::new_impl(const TransportInst_rch& inst)
{
  return new RtpsUdpTransport(inst);
}

int
RtpsUdpInst::load(ACE_Configuration_Heap& cf,
                  ACE_Configuration_Section_Key& sect)
{
  TransportInst::load(cf, sect); // delegate to parent

  ACE_TString local_address_s;
  GET_CONFIG_TSTRING_VALUE(cf, sect, ACE_TEXT("local_address"),
                           local_address_s);
  if (!local_address_s.is_empty()) {
    local_address_.set(local_address_s.c_str());
  }

  GET_CONFIG_VALUE(cf, sect, ACE_TEXT("use_multicast"), use_multicast_, bool);

  ACE_TString group_address_s;
  GET_CONFIG_TSTRING_VALUE(cf, sect, ACE_TEXT("multicast_group_address"),
                           group_address_s);
  if (!group_address_s.is_empty()) {
    if (group_address_s.rfind(':') == group_address_s.npos) {
      // Concatenate a port number if the user does not supply one.
      group_address_s += ACE_TEXT(":7401");
    }
    multicast_group_address_.set(group_address_s.c_str());
  }

  GET_CONFIG_STRING_VALUE(cf, sect, ACE_TEXT("multicast_interface"),
                          multicast_interface_);

  GET_CONFIG_VALUE(cf, sect, ACE_TEXT("nak_depth"), nak_depth_, size_t);

  GET_CONFIG_VALUE(cf, sect, ACE_TEXT("ttl"), ttl_, short);

  GET_CONFIG_TIME_VALUE(cf, sect, ACE_TEXT("nak_response_delay"),
                        nak_response_delay_);
  GET_CONFIG_TIME_VALUE(cf, sect, ACE_TEXT("heartbeat_period"),
                        heartbeat_period_);
  GET_CONFIG_TIME_VALUE(cf, sect, ACE_TEXT("heartbeat_response_delay"),
                        heartbeat_response_delay_);
  GET_CONFIG_TIME_VALUE(cf, sect, ACE_TEXT("handshake_timeout"),
                        handshake_timeout_);
  return 0;
}

#ifndef OPENDDS_SAFETY_PROFILE
void
RtpsUdpInst::dump(std::ostream& os)
{
  // ACE_INET_Addr uses a static buffer for get_host_addr() so we can't
  // directly call it on both local_address_ and multicast_group_address_,
  // since the second call could overwrite the result of the first before the
  // ostream gets a chance to see it.
  const OPENDDS_STRING local = local_address_.get_host_addr(),
    multi = multicast_group_address_.get_host_addr();
  TransportInst::dump(os);
#ifndef ACE_LYNXOS_MAJOR
  const std::ios::fmtflags flags = os.setf(ios::boolalpha);
#endif
  os << formatNameForDump("local_address") << local
     << ':' << local_address_.get_port_number() << '\n'
     << formatNameForDump("use_multicast") << use_multicast_ << '\n'
     << formatNameForDump("multicast_group_address") << multi
     << ':' << multicast_group_address_.get_port_number() << '\n'
     << formatNameForDump("multicast_interface") << multicast_interface_ << '\n'
     << formatNameForDump("nak_depth") << nak_depth_ << '\n'
     << formatNameForDump("nak_response_delay") << nak_response_delay_.msec()
     << '\n'
     << formatNameForDump("heartbeat_period") << heartbeat_period_.msec()
     << '\n'
     << formatNameForDump("heartbeat_response_delay")
     << heartbeat_response_delay_.msec() << '\n'
     << formatNameForDump("handshake_timeout") << handshake_timeout_.msec()
     << std::endl;
#ifndef ACE_LYNXOS_MAJOR
  os.flags(flags);
#endif
}
#endif

OPENDDS_STRING
RtpsUdpInst::dump_to_str()
{
  OPENDDS_STRING ret;
  // ACE_INET_Addr uses a static buffer for get_host_addr() so we can't
  // directly call it on both local_address_ and multicast_group_address_,
  // since the second call could overwrite the result of the first before the
  // OPENDDS_STRING gets a chance to see it.
  OPENDDS_STRING local;
  OPENDDS_STRING multi;
  const char* loc = local_address_.get_host_addr();
  if (loc) {
    local = loc;
  } else {
    local = "NOT_SUPPORTED";
  }
  const char* mul = multicast_group_address_.get_host_addr();
  if (mul) {
    multi = mul;
  } else {
    multi = "NOT_SUPPORTED";
  }
  ret += TransportInst::dump_to_str();
  ret += formatNameForDump("local_address") + local;
  int sz = ACE_OS::snprintf(NULL, 0, "%hu", local_address_.get_port_number());
  int buff_size = sz + 1;
  char buf1[buff_size]; // note +1 for null terminator
  ACE_OS::snprintf(&buf1[0], buff_size, "%hu", local_address_.get_port_number());
  ret += ':' + buf1 + '\n';
  ret += formatNameForDump("use_multicast") + (use_multicast_ ? "true" : "false") + '\n';
  sz = ACE_OS::snprintf(NULL, 0, "%hu", multicast_group_address_.get_port_number());
  buff_size = sz + 1;
  char buf2[buff_size]; // note +1 for null terminator
  ACE_OS::snprintf(&buf2[0], buff_size, "%hu", multicast_group_address_.get_port_number());
  ret += formatNameForDump("multicast_group_address") + multi
      + ':' + buf2 + '\n';
  ret += formatNameForDump("multicast_interface") + multicast_interface_ + '\n';
  sz = ACE_OS::snprintf(NULL, 0, "%u", unsigned(nak_depth_));
  buff_size = sz + 1;
  char buf3[buff_size]; // note +1 for null terminator
  ACE_OS::snprintf(&buf3[0], buff_size, "%u", unsigned(nak_depth_));
  ret += formatNameForDump("nak_depth") + buf3 + '\n';
  sz = ACE_OS::snprintf(NULL, 0, "%lu", nak_response_delay_.msec());
  buff_size = sz + 1;
  char buf4[buff_size]; // note +1 for null terminator
  ACE_OS::snprintf(&buf4[0], buff_size, "%lu", nak_response_delay_.msec());
  ret += formatNameForDump("nak_response_delay") + buf4 + '\n';
  sz = ACE_OS::snprintf(NULL, 0, "%lu", heartbeat_period_.msec());
  buff_size = sz + 1;
  char buf5[buff_size]; // note +1 for null terminator
  ACE_OS::snprintf(&buf5[0], buff_size, "%lu", heartbeat_period_.msec());
  ret += formatNameForDump("heartbeat_period") + buf5 + '\n';
  sz = ACE_OS::snprintf(NULL, 0, "%lu", heartbeat_response_delay_.msec());
  buff_size = sz + 1;
  char buf6[buff_size]; // note +1 for null terminator
  ACE_OS::snprintf(&buf6[0], buff_size, "%lu", heartbeat_response_delay_.msec());
  ret += formatNameForDump("heartbeat_response_delay") + buf6 + '\n';
  sz = ACE_OS::snprintf(NULL, 0, "%lu", handshake_timeout_.msec());
  buff_size = sz + 1;
  char buf7[buff_size]; // note +1 for null terminator
  ACE_OS::snprintf(&buf7[0], buff_size, "%lu", handshake_timeout_.msec());
  ret += formatNameForDump("handshake_timeout") + buf7 + '\n';
  return ret;
}

} // namespace DCPS
} // namespace OpenDDS
