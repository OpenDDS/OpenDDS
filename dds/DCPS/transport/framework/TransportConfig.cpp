/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "TransportConfig.h"
#include "TransportInst.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

#if !defined _MSC_VER || _MSC_VER >= 1900
const unsigned long TransportConfig::DEFAULT_PASSIVE_CONNECT_DURATION;
#endif

TransportConfig::TransportConfig(const OPENDDS_STRING& name)
  : swap_bytes_(*this, &TransportConfig::swap_bytes, &TransportConfig::swap_bytes)
  , passive_connect_duration_(*this, &TransportConfig::passive_connect_duration, &TransportConfig::passive_connect_duration)
  , transports_(*this, &TransportConfig::transports, &TransportConfig::transports)
  , name_(name)
  , config_prefix_(ConfigPair::canonicalize("CONFIG_" + name_))
{}

TransportConfig::~TransportConfig()
{}

void
TransportConfig::swap_bytes(bool flag)
{
  TheServiceParticipant->config_store()->set_boolean(config_key("SWAP_BYTES").c_str(), flag);
}

bool
TransportConfig::swap_bytes() const
{
  return TheServiceParticipant->config_store()->get_boolean(config_key("SWAP_BYTES").c_str(), false);
}

void
TransportConfig::passive_connect_duration(const TimeDuration& pcd)
{
  TheServiceParticipant->config_store()->set(config_key("PASSIVE_CONNECT_DURATION").c_str(),
                                             pcd,
                                             ConfigStoreImpl::Format_IntegerMilliseconds);

}

TimeDuration
TransportConfig::passive_connect_duration() const
{
  return TheServiceParticipant->config_store()->get(config_key("PASSIVE_CONNECT_DURATION").c_str(),
                                                    TimeDuration::from_msec(DEFAULT_PASSIVE_CONNECT_DURATION),
                                                    ConfigStoreImpl::Format_IntegerMilliseconds);
}

void
TransportConfig::transports(const ConfigStoreImpl::StringList& t)
{
  TheServiceParticipant->config_store()->set(config_key("TRANSPORTS").c_str(), t);
}

ConfigStoreImpl::StringList
TransportConfig::transports() const
{
  return TheServiceParticipant->config_store()->get(config_key("TRANSPORTS").c_str(), ConfigStoreImpl::StringList());
}

void
TransportConfig::sorted_insert(const TransportInst_rch& inst)
{
  const OPENDDS_STRING name = inst->name();
  InstancesType::iterator it = instances_.begin();
  while (it != instances_.end() && (*it)->name() < name) {
    ++it;
  }
  instances_.insert(it, inst);
}

void
TransportConfig::populate_locators(TransportLocatorSeq& trans_info,
                                   DDS::DomainId_t domain) const
{
  for (InstancesType::const_iterator pos = instances_.begin(), limit = instances_.end();
       pos != limit;
       ++pos) {
    const CORBA::ULong idx = DCPS::grow(trans_info) - 1;
    if ((*pos)->populate_locator(trans_info[idx], CONNINFO_ALL, domain) == 0) {
      trans_info.length(idx);
    }
  }
}

bool
TransportConfig::uses_template() const
{
  for (InstancesType::const_iterator pos = instances_.begin(), limit = instances_.end();
       pos != limit;
       ++pos) {
    if ((*pos)->is_template()) {
      return true;
    }
  }
  return false;
}

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
