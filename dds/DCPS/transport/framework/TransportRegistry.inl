/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "EntryExit.h"
#include "TransportInst.h"
#include "TransportType.h"

ACE_INLINE
OpenDDS::DCPS::TransportRegistry::TransportRegistry()
  : global_config_(new TransportConfig(DEFAULT_CONFIG_NAME))
{
  DBG_ENTRY_LVL("TransportRegistry", "TransportRegistry", 6);
  config_map_[DEFAULT_CONFIG_NAME] = global_config_;
  lib_directive_map_["tcp"]       = "dynamic OpenDDS_Tcp Service_Object * OpenDDS_Tcp:_make_TcpLoader()";
  lib_directive_map_["udp"]       = "dynamic OpenDDS_DCPS_Udp_Service Service_Object * OpenDDS_Udp:_make_UdpLoader()";
  lib_directive_map_["multicast"] = "dynamic OpenDDS_DCPS_Multicast_Service Service_Object * OpenDDS_Multicast:_make_MulticastLoader()";
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

