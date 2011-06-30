/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "EntryExit.h"

ACE_INLINE
OpenDDS::DCPS::TransportRegistry::TransportRegistry()
{
  DBG_ENTRY_LVL("TransportRegistry", "TransportRegistry", 6);
}

ACE_INLINE
OpenDDS::DCPS::TransportRegistry::~TransportRegistry()
{
  DBG_ENTRY_LVL("TransportRegistry", "~TransportRegistry", 6);
}

ACE_INLINE
OpenDDS::DCPS::TransportConfig_rch
OpenDDS::DCPS::TransportRegistry::global_config() const
{
  DBG_ENTRY_LVL("TransportRegistry", "global_config()", 6);
  GuardType guard(this->lock_);
  return this->global_config_;
}

ACE_INLINE
void
OpenDDS::DCPS::TransportRegistry::global_config(const TransportConfig_rch& cfg)
{
  DBG_ENTRY_LVL("TransportRegistry", "global_config(cfg)", 6);
  GuardType guard(this->lock_);
  this->global_config_ = cfg;
}

ACE_INLINE
void
OpenDDS::DCPS::TransportRegistry::remove_inst(const TransportInst_rch& inst)
{
  GuardType guard(this->lock_);
  this->inst_map_.erase(inst->name());
}

ACE_INLINE
void
OpenDDS::DCPS::TransportRegistry::remove_config(const TransportConfig_rch& cfg)
{
  GuardType guard(this->lock_);
  this->config_map_.erase(cfg->name());
}


ACE_INLINE
void
OpenDDS::DCPS::TransportRegistry::bind_config(const std::string& name,
                                              DDS::Entity_ptr entity)
{
  this->bind_config(this->get_config(name), entity);
}

