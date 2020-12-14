/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "EntryExit.h"
#include "TransportInst.h"
#include "TransportType.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

ACE_INLINE
TransportRegistry::~TransportRegistry()
{
  DBG_ENTRY_LVL("TransportRegistry", "~TransportRegistry", 6);
}

ACE_INLINE
TransportConfig_rch
TransportRegistry::global_config() const
{
  DBG_ENTRY_LVL("TransportRegistry", "global_config()", 6);
  GuardType guard(this->lock_);
  return this->global_config_;
}

ACE_INLINE
void
TransportRegistry::global_config(const TransportConfig_rch& cfg)
{
  DBG_ENTRY_LVL("TransportRegistry", "global_config(cfg)", 6);
  GuardType guard(this->lock_);
  this->global_config_ = cfg;
}

ACE_INLINE
void
TransportRegistry::domain_default_config(DDS::DomainId_t domain,
                                         const TransportConfig_rch& cfg)
{
  GuardType guard(this->lock_);
  domain_default_config_map_[domain] = cfg;
}

ACE_INLINE
TransportConfig_rch
TransportRegistry::domain_default_config(DDS::DomainId_t domain) const
{
  GuardType guard(this->lock_);
  const DomainConfigMap::const_iterator iter =
    domain_default_config_map_.find(domain);
  return (iter == domain_default_config_map_.end())
    ? TransportConfig_rch() : iter->second;
}

ACE_INLINE
void
TransportRegistry::remove_inst(const TransportInst_rch& inst)
{
  remove_inst(inst->name());
}

ACE_INLINE
void
TransportRegistry::remove_inst(const OPENDDS_STRING& inst_name)
{
  GuardType guard(this->lock_);
  InstMap::iterator iter = inst_map_.find(inst_name);
  if (iter == inst_map_.end()) {
    return;
  }
  if (iter->second) {
    iter->second->shutdown();
  }
  inst_map_.erase(iter);
}

ACE_INLINE
void
TransportRegistry::remove_config(const TransportConfig_rch& cfg)
{
  remove_config(cfg->name());
}

ACE_INLINE
void
TransportRegistry::remove_config(const OPENDDS_STRING& config_name)
{
  GuardType guard(this->lock_);
  config_map_.erase(config_name);
}

ACE_INLINE
void
TransportRegistry::bind_config(const OPENDDS_STRING& name,
                               DDS::Entity_ptr entity)
{
  this->bind_config(this->get_config(name), entity);
}

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
