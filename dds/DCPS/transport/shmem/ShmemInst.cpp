/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "ShmemInst.h"
#include "ShmemLoader.h"

#include <dds/DCPS/NetworkResource.h>

#include <ace/Configuration.h>
#include <ace/OS_NS_unistd.h>

#include <iostream>
#include <sstream>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

const TimeDuration ShmemInst::default_association_resend_period(0, 250000);

ShmemInst::ShmemInst(const std::string& name)
  : TransportInst("shmem", name)
  , pool_size_(*this, &ShmemInst::pool_size, &ShmemInst::pool_size)
  , datalink_control_size_(*this, &ShmemInst::datalink_control_size, &ShmemInst::datalink_control_size)
{
  std::ostringstream pool;
  pool << "OpenDDS-" << ACE_OS::getpid() << '-' << this->name();
  poolname_ = pool.str();
}

TransportImpl_rch
ShmemInst::new_impl(DDS::DomainId_t domain)
{
  return make_rch<ShmemTransport>(rchandle_from(this), domain);
}

OPENDDS_STRING
ShmemInst::dump_to_str(DDS::DomainId_t domain) const
{
  std::ostringstream os;
  os << TransportInst::dump_to_str(domain);
  os << formatNameForDump("pool_size") << pool_size() << "\n"
     << formatNameForDump("datalink_control_size") << datalink_control_size() << "\n"
     << formatNameForDump("pool_name") << this->poolname_ << "\n"
     << formatNameForDump("host_name") << this->hostname() << "\n"
     << formatNameForDump("association_resend_period") << association_resend_period().str() << "\n";
  return OPENDDS_STRING(os.str());
}

size_t
ShmemInst::populate_locator(OpenDDS::DCPS::TransportLocator& info,
                            ConnectionInfoFlags,
                            DDS::DomainId_t) const
{
  info.transport_type = "shmem";

  const String host = hostname();
  const size_t len = host.size() + 1 /* null */ + poolname_.size();
  info.data.length(static_cast<CORBA::ULong>(len));

  CORBA::Octet* buff = info.data.get_buffer();
  std::memcpy(buff, host.c_str(), host.size());
  buff += host.size();

  *(buff++) = 0;
  std::memcpy(buff, poolname_.c_str(), poolname_.size());

  return 1;
}

void
ShmemInst::pool_size(size_t ps)
{
  TheServiceParticipant->config_store()->set_uint32(config_key("POOL_SIZE").c_str(), static_cast<DDS::UInt32>(ps));
}

size_t
ShmemInst::pool_size() const
{
  return TheServiceParticipant->config_store()->get_uint32(config_key("POOL_SIZE").c_str(), 16 * 1024 * 1024);
}

void
ShmemInst::datalink_control_size(size_t dcs)
{
  TheServiceParticipant->config_store()->set_uint32(config_key("DATALINK_CONTROL_SIZE").c_str(),
                                                    static_cast<DDS::UInt32>(dcs));
}

size_t
ShmemInst::datalink_control_size() const
{
  return TheServiceParticipant->config_store()->get_uint32(config_key("DATALINK_CONTROL_SIZE").c_str(), 4 * 1024);
}

void
ShmemInst::hostname(const String& h)
{
  TheServiceParticipant->config_store()->set(config_key("HOSTNAME").c_str(), h);
}

String
ShmemInst::hostname() const
{
  return TheServiceParticipant->config_store()->get(config_key("HOSTNAME").c_str(), get_fully_qualified_hostname(), false);
}

void
ShmemInst::association_resend_period(const TimeDuration& arp)
{
  TheServiceParticipant->config_store()->set(config_key("ASSOCIATION_RESEND_PERIOD").c_str(),
                                             arp,
                                             ConfigStoreImpl::Format_IntegerMilliseconds);
}

TimeDuration
ShmemInst::association_resend_period() const
{
  return TheServiceParticipant->config_store()->get(config_key("ASSOCIATION_RESEND_PERIOD").c_str(),
                                                    default_association_resend_period,
                                                    ConfigStoreImpl::Format_IntegerMilliseconds);
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
