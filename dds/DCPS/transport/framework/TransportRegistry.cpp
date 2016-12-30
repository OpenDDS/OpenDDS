/*
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
#include "dds/DCPS/ConfigUtils.h"
#include "dds/DCPS/SafetyProfileStreams.h"

#include "ace/Singleton.h"
#include "ace/OS_NS_strings.h"
#include "ace/Service_Config.h"

#if !defined (__ACE_INLINE__)
#include "TransportRegistry.inl"
#endif /* __ACE_INLINE__ */


namespace {
  const ACE_TString OLD_TRANSPORT_PREFIX = ACE_TEXT("transport_");

  /// Used for sorting
  bool predicate(const OpenDDS::DCPS::TransportInst_rch& lhs,
                 const OpenDDS::DCPS::TransportInst_rch& rhs)
  {
    return lhs->name() < rhs->name();
  }

  // transport type to try loading if none are loaded when DCPS attempts to use
#ifdef OPENDDS_SAFETY_PROFILE
  const char FALLBACK_TYPE[] = "rtps_udp";
#else
  const char FALLBACK_TYPE[] = "tcp";
#endif
}

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

TransportRegistry*
TransportRegistry::instance()
{
  return ACE_Singleton<TransportRegistry, ACE_Recursive_Thread_Mutex>::instance();
}

const char TransportRegistry::DEFAULT_CONFIG_NAME[] = "_OPENDDS_DEFAULT_CONFIG";
const char TransportRegistry::DEFAULT_INST_PREFIX[] = "_OPENDDS_";

TransportRegistry::TransportRegistry()
  : global_config_(make_rch<TransportConfig>(DEFAULT_CONFIG_NAME))
{
  DBG_ENTRY_LVL("TransportRegistry", "TransportRegistry", 6);
  config_map_[DEFAULT_CONFIG_NAME] = global_config_;

  lib_directive_map_["tcp"]       = "dynamic OpenDDS_Tcp Service_Object * OpenDDS_Tcp:_make_TcpLoader()";
  lib_directive_map_["udp"]       = "dynamic OpenDDS_Udp Service_Object * OpenDDS_Udp:_make_UdpLoader()";
  lib_directive_map_["multicast"] = "dynamic OpenDDS_Multicast Service_Object * OpenDDS_Multicast:_make_MulticastLoader()";
  lib_directive_map_["rtps_udp"]  = "dynamic OpenDDS_Rtps_Udp Service_Object * OpenDDS_Rtps_Udp:_make_RtpsUdpLoader()";
  lib_directive_map_["shmem"]     = "dynamic OpenDDS_Shmem Service_Object * OpenDDS_Shmem:_make_ShmemLoader()";

  // load_transport_lib() is used for discovery as well:
  lib_directive_map_["rtps_discovery"] = lib_directive_map_["rtps_udp"];
  lib_directive_map_["repository"] = "dynamic OpenDDS_InfoRepoDiscovery Service_Object * OpenDDS_InfoRepoDiscovery:_make_IRDiscoveryLoader()";
}

int
TransportRegistry::load_transport_configuration(const OPENDDS_STRING& file_name,
                                                ACE_Configuration_Heap& cf)
{
  const ACE_Configuration_Section_Key& root = cf.root_section();

  // Create a vector to hold configuration information so we can populate
  // them after the transports instances are created.
  typedef std::pair<TransportConfig_rch, OPENDDS_VECTOR(OPENDDS_STRING) > ConfigInfo;
  OPENDDS_VECTOR(ConfigInfo) configInfoVec;

  // Record the transport instances created, so we can place them
  // in the implicit transport configuration for this file.
  OPENDDS_LIST(TransportInst_rch) instances;

  ACE_TString sect_name;

  for (int index = 0;
       cf.enumerate_sections(root, index, sect_name) == 0;
       ++index) {
    if (ACE_OS::strcmp(sect_name.c_str(), TRANSPORT_SECTION_NAME) == 0) {
      // found the [transport/*] section, now iterate through subsections...
      ACE_Configuration_Section_Key sect;
      if (cf.open_section(root, sect_name.c_str(), 0, sect) != 0) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) TransportRegistry::load_transport_configuration: ")
                          ACE_TEXT("failed to open section %s\n"),
                          sect_name.c_str()),
                         -1);
      } else {
        // Ensure there are no properties in this section
        ValueMap vm;
        if (pullValues(cf, sect, vm) > 0) {
          // There are values inside [transport]
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) TransportRegistry::load_transport_configuration: ")
                            ACE_TEXT("transport sections must have a section name\n"),
                            sect_name.c_str()),
                           -1);
        }
        // Process the subsections of this section (the individual transport
        // impls).
        KeyList keys;
        if (processSections(cf, sect, keys) != 0) {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) TransportRegistry::load_transport_configuration: ")
                            ACE_TEXT("too many nesting layers in [%s] section.\n"),
                            sect_name.c_str()),
                           -1);
        }
        for (KeyList::const_iterator it = keys.begin(); it != keys.end(); ++it) {
          OPENDDS_STRING transport_id = it->first;
          ACE_Configuration_Section_Key inst_sect = it->second;

          ValueMap values;
          if (pullValues(cf, it->second, values) != 0) {
            // Get the factory_id for the transport.
            OPENDDS_STRING transport_type;
            ValueMap::const_iterator vm_it = values.find("transport_type");
            if (vm_it != values.end()) {
              transport_type = vm_it->second;
            } else {
              ACE_ERROR_RETURN((LM_ERROR,
                                ACE_TEXT("(%P|%t) TransportRegistry::load_transport_configuration: ")
                                ACE_TEXT("missing transport_type in [transport/%C] section.\n"),
                                transport_id.c_str()),
                               -1);
            }
            // Create the TransportInst object and load the transport
            // configuration in ACE_Configuration_Heap to the TransportInst
            // object.
            TransportInst_rch inst = create_inst(transport_id, transport_type);
            if (!inst) {
              ACE_ERROR_RETURN((LM_ERROR,
                                ACE_TEXT("(%P|%t) TransportRegistry::load_transport_configuration: ")
                                ACE_TEXT("Unable to create transport instance in [transport/%C] section.\n"),
                                transport_id.c_str()),
                               -1);
            }
            instances.push_back(inst);
            inst->load(cf, inst_sect);
          } else {
            ACE_ERROR_RETURN((LM_ERROR,
                              ACE_TEXT("(%P|%t) TransportRegistry::load_transport_configuration: ")
                              ACE_TEXT("missing transport_type in [transport/%C] section.\n"),
                              transport_id.c_str()),
                             -1);
          }
        }
      }
    } else if (ACE_OS::strcmp(sect_name.c_str(), CONFIG_SECTION_NAME) == 0) {
      // found the [config/*] section, now iterate through subsections...
      ACE_Configuration_Section_Key sect;
      if (cf.open_section(root, sect_name.c_str(), 0, sect) != 0) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) TransportRegistry::load_transport_configuration: ")
                          ACE_TEXT("failed to open section [%s]\n"),
                          sect_name.c_str()),
                         -1);
      } else {
        // Ensure there are no properties in this section
        ValueMap vm;
        if (pullValues(cf, sect, vm) > 0) {
          // There are values inside [config]
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) TransportRegistry::load_transport_configuration: ")
                            ACE_TEXT("config sections must have a section name\n"),
                            sect_name.c_str()),
                           -1);
        }
        // Process the subsections of this section (the individual config
        // impls).
        KeyList keys;
        if (processSections(cf, sect, keys) != 0) {
          // Don't allow multiple layers of nesting ([config/x/y]).
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) TransportRegistry::load_transport_configuration: ")
                            ACE_TEXT("too many nesting layers in [%s] section.\n"),
                            sect_name.c_str()),
                           -1);
        }
        for (KeyList::const_iterator it = keys.begin(); it != keys.end(); ++it) {
          OPENDDS_STRING config_id = it->first;

          // Create a TransportConfig object.
          TransportConfig_rch config = create_config(config_id);
          if (!config) {
            ACE_ERROR_RETURN((LM_ERROR,
                              ACE_TEXT("(%P|%t) TransportRegistry::load_transport_configuration: ")
                              ACE_TEXT("Unable to create transport config in [config/%C] section.\n"),
                              config_id.c_str()),
                             -1);
          }

          ValueMap values;
          pullValues(cf, it->second, values);

          ConfigInfo configInfo;
          configInfo.first = config;
          for (ValueMap::const_iterator it = values.begin(); it != values.end(); ++it) {
            OPENDDS_STRING name = it->first;
            OPENDDS_STRING value = it->second;
            if (name == "transports") {
              char delim = ',';
              size_t pos = 0;
              OPENDDS_STRING token;
              while ((pos = value.find(delim)) != OPENDDS_STRING::npos) {
                token = value.substr(0, pos);
                configInfo.second.push_back(token);
                value.erase(0, pos + 1);
              }
              configInfo.second.push_back(value);

              configInfoVec.push_back(configInfo);
            } else if (name == "swap_bytes") {
              if ((value == "1") || (value == "true")) {
                config->swap_bytes_ = true;
              } else if ((value != "0") && (value != "false")) {
                ACE_ERROR_RETURN((LM_ERROR,
                                  ACE_TEXT("(%P|%t) TransportRegistry::load_transport_configuration: ")
                                  ACE_TEXT("Illegal value for swap_bytes (%C) in [config/%C] section.\n"),
                                  value.c_str(), config_id.c_str()),
                                 -1);
              }
            } else if (name == "passive_connect_duration") {
              if (!convertToInteger(value,
                                    config->passive_connect_duration_)) {
                ACE_ERROR_RETURN((LM_ERROR,
                                  ACE_TEXT("(%P|%t) TransportRegistry::load_transport_configuration: ")
                                  ACE_TEXT("Illegal integer value for passive_connect_duration (%s) in [config/%C] section.\n"),
                                  value.c_str(), config_id.c_str()),
                                 -1);
              }
            } else {
              ACE_ERROR_RETURN((LM_ERROR,
                                ACE_TEXT("(%P|%t) TransportRegistry::load_transport_configuration: ")
                                ACE_TEXT("Unexpected entry (%C) in [config/%C] section.\n"),
                                name.c_str(), config_id.c_str()),
                               -1);
            }
          }
          if (configInfo.second.empty()) {
            ACE_ERROR_RETURN((LM_ERROR,
                              ACE_TEXT("(%P|%t) TransportRegistry::load_transport_configuration: ")
                              ACE_TEXT("No transport instances listed in [config/%C] section.\n"),
                              config_id.c_str()),
                             -1);
          }
        }
      }
    } else if (ACE_OS::strncmp(sect_name.c_str(), OLD_TRANSPORT_PREFIX.c_str(),
                               OLD_TRANSPORT_PREFIX.length()) == 0) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) ERROR: ")
                        ACE_TEXT("Obsolete transport configuration found (%s).\n"),
                        sect_name.c_str()),
                       -1);
    }
  }

  // Populate the configurations with instances
  for (unsigned int i = 0; i < configInfoVec.size(); ++i) {
    TransportConfig_rch config = configInfoVec[i].first;
    OPENDDS_VECTOR(OPENDDS_STRING)& insts = configInfoVec[i].second;
    for (unsigned int j = 0; j < insts.size(); ++j) {
      TransportInst_rch inst = get_inst(insts[j]);
      if (!inst) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) TransportRegistry::load_transport_configuration: ")
                          ACE_TEXT("The inst (%C) in [config/%C] section is undefined.\n"),
                          insts[j].c_str(), config->name().c_str()),
                         -1);
      }
      config->instances_.push_back(inst);
    }
  }

  // Create and populate the default configuration for this
  // file with all the instances from this file.
  if (!instances.empty()) {
    TransportConfig_rch config = create_config(file_name);
    if (!config) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) TransportRegistry::load_transport_configuration: ")
                        ACE_TEXT("Unable to create default transport config.\n"),
                        file_name.c_str()),
                       -1);
    }
    instances.sort(predicate);
    for (OPENDDS_LIST(TransportInst_rch)::const_iterator it = instances.begin();
         it != instances.end(); ++it) {
      config->instances_.push_back(*it);
    }
  }

  return 0;
}

void
TransportRegistry::load_transport_lib(const OPENDDS_STRING& transport_type)
{
  ACE_UNUSED_ARG(transport_type);
#if !defined(ACE_AS_STATIC_LIBS)
  GuardType guard(lock_);
  LibDirectiveMap::iterator lib_iter = lib_directive_map_.find(transport_type);
  if (lib_iter != lib_directive_map_.end()) {
    ACE_TString directive = ACE_TEXT_CHAR_TO_TCHAR(lib_iter->second.c_str());
    // Release the lock, because loading a transport library will
    // recursively call this function to add its default inst.
    guard.release();
    ACE_Service_Config::process_directive(directive.c_str());
  }
#endif
}

TransportInst_rch
TransportRegistry::create_inst(const OPENDDS_STRING& name,
                               const OPENDDS_STRING& transport_type)
{
  GuardType guard(lock_);
  TransportType_rch type;

  if (find(type_map_, transport_type, type) != 0) {
#if !defined(ACE_AS_STATIC_LIBS)
    guard.release();
    // Not present, try to load library
    load_transport_lib(transport_type);
    guard.acquire();

    // Try to find it again
    if (find(type_map_, transport_type, type) != 0) {
#endif
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) TransportRegistry::create_inst: ")
                 ACE_TEXT("transport_type=%C is not registered.\n"),
                 transport_type.c_str()));
      return TransportInst_rch();
#if !defined(ACE_AS_STATIC_LIBS)
    }
#endif
  }

  if (inst_map_.count(name)) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) TransportRegistry::create_inst: ")
               ACE_TEXT("name=%C is already in use.\n"),
               name.c_str()));
    return TransportInst_rch();
  }
  TransportInst_rch inst (type->new_inst(name));
  inst_map_[name] = inst;
  return inst;
}


TransportInst_rch
TransportRegistry::get_inst(const OPENDDS_STRING& name) const
{
  GuardType guard(lock_);
  InstMap::const_iterator found = inst_map_.find(name);
  if (found != inst_map_.end()) {
    return found->second;
  }
  return TransportInst_rch();
}


TransportConfig_rch
TransportRegistry::create_config(const OPENDDS_STRING& name)
{
  GuardType guard(lock_);

  if (config_map_.count(name)) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) TransportRegistry::create_config: ")
               ACE_TEXT("name=%C is already in use.\n"),
               name.c_str()));
    return TransportConfig_rch();
  }

  TransportConfig_rch inst  (make_rch<TransportConfig>(name));
  config_map_[name] = inst;
  return inst;
}


TransportConfig_rch
TransportRegistry::get_config(const OPENDDS_STRING& name) const
{
  GuardType guard(lock_);
  ConfigMap::const_iterator found = config_map_.find(name);
  if (found != config_map_.end()) {
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


TransportConfig_rch
TransportRegistry::fix_empty_default()
{
  DBG_ENTRY_LVL("TransportRegistry", "fix_empty_default", 6);
  GuardType guard(lock_);
  if (global_config_.is_nil()
      || !global_config_->instances_.empty()
      || global_config_->name() != DEFAULT_CONFIG_NAME) {
    return global_config_;
  }
  TransportConfig_rch global_config = global_config_;
#if !defined(ACE_AS_STATIC_LIBS)
  guard.release();
  load_transport_lib(FALLBACK_TYPE);
#endif
  return global_config;
}


void
TransportRegistry::register_type(const TransportType_rch& type)
{
  DBG_ENTRY_LVL("TransportRegistry", "register_type", 6);
  int result;
  const OPENDDS_STRING name = type->name();

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
  GuardType guard(lock_);

  for (InstMap::iterator iter = inst_map_.begin(); iter != inst_map_.end(); ++iter) {
    iter->second->shutdown();
  }

  type_map_.clear();
  inst_map_.clear();
  config_map_.clear();
  domain_default_config_map_.clear();
  global_config_.reset();
}


}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
