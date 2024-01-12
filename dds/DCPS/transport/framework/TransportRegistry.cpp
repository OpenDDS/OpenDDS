/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <DCPS/DdsDcps_pch.h> //Only the _pch include should start with DCPS/

#include "TransportRegistry.h"
#include "TransportDebug.h"
#include "TransportInst.h"
#include "TransportExceptions.h"
#include "TransportType.h"

#include <dds/DCPS/debug.h>
#include <dds/DCPS/GuidConverter.h>
#include <dds/DCPS/Util.h>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/EntityImpl.h>
#include <dds/DCPS/SafetyProfileStreams.h>

#include <dds/DdsDcpsInfrastructureC.h>

#include <ace/Singleton.h>
#include <ace/OS_NS_strings.h>
#include <ace/Service_Config.h>

#ifndef __ACE_INLINE__
#  include "TransportRegistry.inl"
#endif

namespace {
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
  return ACE_Unmanaged_Singleton<TransportRegistry, ACE_Recursive_Thread_Mutex>::instance();
}

void
TransportRegistry::close()
{
  ACE_Unmanaged_Singleton<TransportRegistry, ACE_Recursive_Thread_Mutex>::close();
}

const char TransportRegistry::DEFAULT_CONFIG_NAME[] = "_OPENDDS_DEFAULT_CONFIG";
const char TransportRegistry::DEFAULT_INST_PREFIX[] = "_OPENDDS_";

// transport template customizations
const OPENDDS_STRING TransportRegistry::CUSTOM_ADD_DOMAIN_TO_IP = "add_domain_id_to_ip_addr";
const OPENDDS_STRING TransportRegistry::CUSTOM_ADD_DOMAIN_TO_PORT = "add_domain_id_to_port";

TransportRegistry::TransportRegistry()
  : global_config_(make_rch<TransportConfig>(DEFAULT_CONFIG_NAME))
  , released_(false)
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
    const bool is_template = ACE_OS::strcmp(sect_name.c_str(), TRANSPORT_TEMPLATE_SECTION_NAME) == 0;
    if (ACE_OS::strcmp(sect_name.c_str(), TRANSPORT_SECTION_NAME) == 0 ||
        is_template) {
      // found the [transport/*] or [transport_template/*] section,
      // now iterate through subsections...
      ACE_Configuration_Section_Key sect;
      if (cf.open_section(root, sect_name.c_str(), false, sect) != 0) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) TransportRegistry::load_transport_configuration: ")
                          ACE_TEXT("failed to open section %C\n"),
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
                            ACE_TEXT("too many nesting layers in [%C] section.\n"),
                            sect_name.c_str()),
                           -1);
        }
        for (KeyList::const_iterator it = keys.begin(); it != keys.end(); ++it) {
          ACE_TString transport_id = ACE_TEXT_CHAR_TO_TCHAR(it->first.c_str());
          ACE_Configuration_Section_Key inst_sect = it->second;

          ValueMap values;
          if (pullValues(cf, it->second, values) != 0) {
            // Get the factory_id for the transport.
            OPENDDS_STRING transport_type;
            ValueMap::const_iterator vm_it = values.find("TRANSPORT_TYPE");
            if (vm_it == values.end()) {
              vm_it = values.find("transport_type");
            }
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
            const OPENDDS_STRING tid_str = transport_id.c_str() ? ACE_TEXT_ALWAYS_CHAR(transport_id.c_str()) : "";
            TransportInst_rch inst = create_inst(tid_str, transport_type, is_template);
            if (!inst) {
              ACE_ERROR_RETURN((LM_ERROR,
                                ACE_TEXT("(%P|%t) TransportRegistry::load_transport_configuration: ")
                                ACE_TEXT("Unable to create transport instance in [transport/%C] section.\n"),
                                transport_id.c_str()),
                               -1);
            }

            instances.push_back(inst);
            inst->load(cf, inst_sect);

            // store the transport info
            TransportEntry entry;
            entry.transport_name = transport_id;
            entry.transport_info = values;
            transports_.push_back(entry);
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
      if (cf.open_section(root, sect_name.c_str(), false, sect) != 0) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) TransportRegistry::load_transport_configuration: ")
                          ACE_TEXT("failed to open section [%C]\n"),
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
                            ACE_TEXT("too many nesting layers in [%C] section.\n"),
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

              // store the config name for the transport entry
              for (OPENDDS_VECTOR(TransportEntry)::iterator it = transports_.begin(); it != transports_.end(); ++it) {
                if (!ACE_OS::strcmp(ACE_TEXT_ALWAYS_CHAR(it->transport_name.c_str()), value.c_str())) {
                  it->config_name = ACE_TEXT_CHAR_TO_TCHAR(config_id.c_str());
                  break;
                }
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
                                  ACE_TEXT("Illegal integer value for passive_connect_duration (%C) in [config/%C] section.\n"),
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
  GuardType guard(lock_);
  if (!load_transport_lib_i(transport_type)) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) TransportRegistry::load_transport_lib: ")
               ACE_TEXT("could not load transport_type=%C.\n"),
               transport_type.c_str()));
  }
}

TransportType_rch
TransportRegistry::load_transport_lib_i(const OPENDDS_STRING& transport_type)
{
  TransportType_rch type;
  if (find(type_map_, transport_type, type) == 0) {
    return type;
  }

#if !defined(ACE_AS_STATIC_LIBS)
  // Attempt to load it.
  LibDirectiveMap::iterator lib_iter = lib_directive_map_.find(transport_type);
  if (lib_iter == lib_directive_map_.end()) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) TransportRegistry::load_transport_lib_i: ")
               ACE_TEXT("no directive for transport_type=%C.\n"),
               transport_type.c_str()));
    return type;
  }

  ACE_TString directive = ACE_TEXT_CHAR_TO_TCHAR(lib_iter->second.c_str());
  // Release the lock because the transport will call back into the registry.
  ACE_Reverse_Lock<LockType> rev_lock(lock_);
  {
    ACE_Guard<ACE_Reverse_Lock<LockType> > guard(rev_lock);
    if (0 != ACE_Service_Config::process_directive(directive.c_str())) {
      if (log_level >= LogLevel::Error) {
        ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: TransportRegistry::load_transport_lib_i: "
          "process_directive failed for transport_type=%C\n",
          transport_type.c_str()));
      }
      return TransportType_rch();
    }
  }
#endif

  find(type_map_, transport_type, type);
  return type;
}

TransportInst_rch
TransportRegistry::create_inst(const OPENDDS_STRING& name,
                               const OPENDDS_STRING& transport_type,
                               bool is_template)
{
  GuardType guard(lock_);

  TransportType_rch type = load_transport_lib_i(transport_type);
  if (!type) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) TransportRegistry::create_inst: ")
               ACE_TEXT("transport_type=%C is not registered.\n"),
               transport_type.c_str()));
    return TransportInst_rch();
  }

  if (inst_map_.count(name)) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) TransportRegistry::create_inst: ")
               ACE_TEXT("name=%C is already in use.\n"),
               name.c_str()));
    return TransportInst_rch();
  }
  TransportInst_rch inst (type->new_inst(name, is_template));
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
  load_transport_lib_i(FALLBACK_TYPE);
  return global_config;
}


bool
TransportRegistry::register_type(const TransportType_rch& type)
{
  DBG_ENTRY_LVL("TransportRegistry", "register_type", 6);
  const OPENDDS_STRING name = type->name();

  GuardType guard(this->lock_);
  if (type_map_.count(name)) {
    return false;
  }

  type_map_[name] = type;

  if (name == "rtps_udp") {
    type_map_["rtps_discovery"] = type;
  }

  return true;
}

void
TransportRegistry::release()
{
  DBG_ENTRY_LVL("TransportRegistry", "release", 6);
  GuardType guard(lock_);
  released_ = true;
  InstMap inst_map_copy_;
  std::swap(inst_map_copy_, inst_map_);


  {
    ACE_Reverse_Lock<LockType> rev_lock(lock_);
    ACE_Guard<ACE_Reverse_Lock<LockType> > guard(rev_lock);
    for (InstMap::iterator iter = inst_map_copy_.begin(); iter != inst_map_copy_.end(); ++iter) {
      iter->second->shutdown();
    }
  }

  transports_.clear();
  type_map_.clear();
  config_map_.clear();
  domain_default_config_map_.clear();
  global_config_.reset();
}

bool
TransportRegistry::released() const
{
  GuardType guard(lock_);
  return released_;
}

bool
TransportRegistry::get_transport_info(const ACE_TString& config_name, TransportEntry& inst)
{
  bool ret = false;
  if (has_transports()) {
    for (OPENDDS_VECTOR(TransportEntry)::const_iterator i = transports_.begin(); i != transports_.end(); ++i) {
      if (!ACE_OS::strcmp(ACE_TEXT_ALWAYS_CHAR(config_name.c_str()), ACE_TEXT_ALWAYS_CHAR(i->config_name.c_str()))) {
        inst.transport_name = i->transport_name;
        inst.config_name = i->config_name;
        inst.transport_info = i->transport_info;

        ret = true;
        break;
      }
    }
  }

  if (DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) TransportRegistry::get_transport_info: ")
               ACE_TEXT("%C config %s\n"),
               ret ? "found" : "did not find", config_name.c_str()));
  }

  return ret;
}

bool TransportRegistry::has_transports() const
{
  return !transports_.empty();
}

void
TransportRegistry::remove_participant(DDS::DomainId_t domain,
                                      DomainParticipantImpl* participant)
{
  GuardType guard(lock_);
  for (InstMap::const_iterator pos = inst_map_.begin(), limit = inst_map_.end(); pos != limit; ++pos) {
    pos->second->remove_participant(domain, participant);
  }
}

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
