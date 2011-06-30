/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "TransportRegistry.h"
#include "TransportDebug.h"
#include "TransportInst.h"
#include "TransportExceptions.h"
#include "TransportType.h"
#include "dds/DCPS/Util.h"
#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/EntityImpl.h"

#include "ace/Singleton.h"
#include "ace/OS_NS_strings.h"

#if !defined (__ACE_INLINE__)
#include "TransportRegistry.inl"
#endif /* __ACE_INLINE__ */

namespace OpenDDS {
namespace DCPS {

TransportRegistry*
TransportRegistry::instance()
{
  return ACE_Singleton<TransportRegistry, ACE_Recursive_Thread_Mutex>::instance();
}

const std::string TransportRegistry::DEFAULT_CONFIG_NAME = "_OPENDDS_DEFAULT_CONFIG";

int
TransportRegistry::load_transport_configuration(ACE_Configuration_Heap& cf)
{
  int status = 0;
  const ACE_Configuration_Section_Key &root = cf.root_section();
  ACE_Configuration_Section_Key trans_sect;

  ACE_TString sect_name;

  for (int index = 0;
       (status = cf.enumerate_sections(root, index, sect_name)) == 0;
       ++index) {
    if (ACE_OS::strncasecmp(sect_name.c_str(),
                            TRANSPORT_SECTION_NAME_PREFIX,
                            TRANSPORT_SECTION_NAME_PREFIX_LEN) == 0) { // found a [transport_impl_<id>] section
      TransportIdType transport_id
      = static_cast <TransportIdType>(ACE_OS::atoi(sect_name.substr(TRANSPORT_SECTION_NAME_PREFIX_LEN).c_str()));
      ACE_Configuration_Section_Key sect;

      if (cf.open_section(root, sect_name.c_str(), 0, sect) != 0)
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) TransportRegistry::load_transport_configuration: ")
                          ACE_TEXT("failed to open section %s\n"),
                          sect_name.c_str()),
                         -1);

      else {
        ACE_TString transport_type;
        // Get the factory_id for the transport.
        GET_CONFIG_STRING_VALUE(cf, sect, ACE_TEXT("transport_type"), transport_type)

        if (transport_type == ACE_TEXT("")) {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) TransportRegistry::load_transport_configuration: ")
                            ACE_TEXT("missing transport_type in \"%s\" section.\n"),
                            sect_name.c_str()),
                           -1);

        } else {
          // Create a TransportInst object and load the transport configuration in
          // ACE_Configuration_Heap to the TransportInst object.
#if 0
          TransportInst_rch config = this->create_configuration(transport_type);

          if (!config.is_nil() && config->load(transport_id, cf) == -1)
            return -1;
#endif
        }
      }
    }
  }

  return 0;
}


TransportInst_rch
TransportRegistry::create_inst(const std::string& name,
                               const std::string& transport_type)
{
  GuardType guard(this->lock_);
  TransportType_rch type;

  if (find(this->type_map_, transport_type, type) != 0) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) TransportRegistry::create_inst: ")
               ACE_TEXT("transport_type=%C is not registered.\n"),
               transport_type.c_str()));
    return TransportInst_rch();
  }

  if (this->inst_map_.count(name)) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) TransportRegistry::create_inst: ")
               ACE_TEXT("name=%C is already in use.\n"),
               name.c_str()));
    return TransportInst_rch();
  }

  TransportInst_rch inst = type->new_inst(name);
  this->inst_map_[name] = inst;
  return inst;
}


TransportInst_rch
TransportRegistry::get_inst(const std::string& name) const
{
  GuardType guard(this->lock_);
  InstMap::const_iterator found = this->inst_map_.find(name);
  if (found != this->inst_map_.end()) {
    return found->second;
  }
  return TransportInst_rch();
}


TransportConfig_rch
TransportRegistry::create_config(const std::string& name)
{
  GuardType guard(this->lock_);

  if (this->config_map_.count(name)) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) TransportRegistry::create_config: ")
               ACE_TEXT("name=%C is already in use.\n"),
               name.c_str()));
    return TransportConfig_rch();
  }

  TransportConfig_rch inst = new TransportConfig(name);
  this->config_map_[name] = inst;
  return inst;
}


TransportConfig_rch
TransportRegistry::get_config(const std::string& name) const
{
  GuardType guard(this->lock_);
  ConfigMap::const_iterator found = this->config_map_.find(name);
  if (found != this->config_map_.end()) {
    return found->second;
  }
  return TransportConfig_rch();
}


void
TransportRegistry::bind_config(const TransportConfig_rch& cfg,
                               DDS::Entity_ptr entity)
{
  if (cfg.is_nil()) {
    throw Transport::NotFound();
  }
  EntityImpl* ei = dynamic_cast<EntityImpl*>(entity);
  if (!ei) {
    throw Transport::MiscProblem();
  }
  ei->transport_config(cfg);
}


void
TransportRegistry::register_type(const TransportType_rch& type)
{
  DBG_ENTRY_LVL("TransportRegistry", "register_generator", 6);
  int result;
  const std::string name = type->name();

  {
    GuardType guard(this->lock_);
    result = OpenDDS::DCPS::bind(type_map_, name, type);
  }

  // Check to see if it worked.
  //
  // 0 means it worked, 1 means it is a duplicate (and didn't work), and
  // -1 means something bad happened.
  if (result == 1) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: transport type=%C already registered ")
               ACE_TEXT("with TransportRegistry.\n"), name.c_str()));
    throw Transport::Duplicate();

  } else if (result == -1) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: Failed to bind transport type=%C to ")
               ACE_TEXT("type_map_.\n"),
               name.c_str()));
    throw Transport::MiscProblem();
  }
}


void
TransportRegistry::release()
{
  DBG_ENTRY_LVL("TransportRegistry", "release", 6);
  GuardType guard(this->lock_);

  for (InstMap::iterator iter = inst_map_.begin();
       iter != inst_map_.end();
       ++iter) {
    iter->second->shutdown();
  }

  type_map_.clear();
  inst_map_.clear();
  config_map_.clear();
}


}
}
