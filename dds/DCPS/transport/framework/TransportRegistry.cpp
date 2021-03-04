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
#include "dds/DCPS/GuidConverter.h"
#include "dds/DCPS/Util.h"
#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/EntityImpl.h"
#include "dds/DCPS/SafetyProfileStreams.h"
#include <dds/DdsDcpsInfrastructureC.h>


#include "ace/Singleton.h"
#include "ace/OS_NS_strings.h"
#include "ace/Service_Config.h"

#if !defined (__ACE_INLINE__)
#include "TransportRegistry.inl"
#endif /* __ACE_INLINE__ */


namespace {
  /// Used for sorting
  bool predicate(const OpenDDS::DCPS::TransportInst_rch& lhs,
                 const OpenDDS::DCPS::TransportInst_rch& rhs)
  {
    return lhs->name() < rhs->name();
  }
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
  , load_pending_(false)
  , load_condition_(lock_)
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
    if (ACE_OS::strcmp(sect_name.c_str(), TRANSPORT_SECTION_NAME) == 0 ||
        ACE_OS::strcmp(sect_name.c_str(), TRANSPORT_TEMPLATE_SECTION_NAME) == 0) {
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
            const OPENDDS_STRING tid_str = transport_id.c_str() ? ACE_TEXT_ALWAYS_CHAR(transport_id.c_str()) : "";
            TransportInst_rch inst = create_inst(tid_str, transport_type);
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

              // does this config specify a transport_template?
              for (OPENDDS_VECTOR(TransportTemplate)::iterator it = transport_templates_.begin(); it != transport_templates_.end(); ++it) {
                if (it->transport_template_name == value) {
                  it->config_name = config_id;
                  break;
                }
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

int
TransportRegistry::load_transport_templates(ACE_Configuration_Heap& cf)
{
  const ACE_Configuration_Section_Key& root = cf.root_section();
  ACE_Configuration_Section_Key transport_sect;

  if (cf.open_section(root, TRANSPORT_TEMPLATE_SECTION_NAME, false, transport_sect) != 0) {
    if (DCPS_debug_level > 0) {
      // This is not an error if the configuration file does not have
      // any domain range (sub)section.
      ACE_DEBUG((LM_NOTICE,
                 ACE_TEXT("(%P|%t) NOTICE: TransportRegistry::load_transport_templates(): ")
                 ACE_TEXT("config does not have a [%s] section.\n"),
                 TRANSPORT_TEMPLATE_SECTION_NAME));
    }

    return 0;

  } else {
    if (DCPS_debug_level > 0) {
      ACE_DEBUG((LM_NOTICE,
                   ACE_TEXT("(%P|%t) NOTICE: TransportRegistry::load_transport_templates(): ")
                   ACE_TEXT("config has %s sections.\n"),
                   TRANSPORT_TEMPLATE_SECTION_NAME));
    }

    // Ensure there are no properties in this section
    ValueMap vm;
    if (pullValues(cf, transport_sect, vm) > 0) {
      // There are values inside [transport_template]
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) ERROR: TransportRegistry::load_transport_templates(): ")
                        ACE_TEXT("%s sections must have a subsection name\n"),
                        TRANSPORT_TEMPLATE_SECTION_NAME),
                     -1);
    }
    // Process the subsections of this section (the individual domains)
    KeyList keys;
    if (processSections(cf, transport_sect, keys) != 0) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) ERROR: TransportRegistry::load_transport_templates(): ")
                        ACE_TEXT("too many nesting layers in the [%s] section.\n"),
                        TRANSPORT_TEMPLATE_SECTION_NAME),
                       -1);
    }

    // Loop through the [transport_template/*] sections
    for (KeyList::const_iterator it = keys.begin(); it != keys.end(); ++it) {
      TransportTemplate element;
      element.instantiate_per_participant = false;
      element.transport_template_name = it->first;

      if (DCPS_debug_level > 0) {
        ACE_DEBUG((LM_DEBUG,
                   ACE_TEXT("(%P|%t) TransportRegistry::load_transport_templates(): ")
                   ACE_TEXT("processing [%s/%C]\n"),
                   TRANSPORT_TEMPLATE_SECTION_NAME, element.transport_template_name.c_str()));
      }

      ValueMap values;
      pullValues(cf, it->second, values);
      OPENDDS_STRING rule;
      OPENDDS_STRING customization;

      for (ValueMap::const_iterator it = values.begin(); it != values.end(); ++it) {
        OPENDDS_STRING name = it->first;
        if (name == "instantiation_rule") {
          rule = it->second;
          if (rule == "per_participant") {
            element.instantiate_per_participant = true;
          }
          if (DCPS_debug_level > 0) {
            OPENDDS_STRING flag = element.instantiate_per_participant ? "true" : "false";
            ACE_DEBUG((LM_DEBUG,
                       ACE_TEXT("(%P|%t) TransportRegistry::load_transport_templates(): ")
                       ACE_TEXT("[%s/%C]: instantiantion rule == %C\n"),
                       TRANSPORT_TEMPLATE_SECTION_NAME, element.transport_template_name.c_str(), flag.c_str()));
          }
        } else if (name == ACE_TEXT_ALWAYS_CHAR(CUSTOMIZATION_SECTION_NAME)) {
          customization = it->second;
          if (DCPS_debug_level > 0) {
            ACE_DEBUG((LM_DEBUG,
                       ACE_TEXT("(%P|%t) TransportRegistry::load_transport_templates(): ")
                       ACE_TEXT("[%s/%C]: customization == %C\n"),
                       TRANSPORT_TEMPLATE_SECTION_NAME, element.transport_template_name.c_str(), customization.c_str()));
          }

          ACE_Configuration_Section_Key custom_sect;
          if (cf.open_section(root, CUSTOMIZATION_SECTION_NAME, false, custom_sect) == 0) {
            ValueMap vcm;

            if (pullValues(cf, custom_sect, vcm) > 0) {
              ACE_ERROR_RETURN((LM_ERROR,
                                ACE_TEXT("(%P|%t) ERROR: TransportRegistry::load_transport_templates(): ")
                                ACE_TEXT("%s sections must have a subsection name\n"),
                                CUSTOMIZATION_SECTION_NAME),
                                -1);
            }

            // Process the subsections of the custom section
            KeyList keys;
            if (processSections(cf, custom_sect, keys) != 0) {
              ACE_ERROR_RETURN((LM_ERROR,
                                ACE_TEXT("(%P|%t) TransportRegistry::load_transport_templates(): ")
                                ACE_TEXT("too many nesting layers in the [%s] section.\n"),
                                CUSTOMIZATION_SECTION_NAME),
                                -1);
              }

              // add customizations to domain range
              for (KeyList::const_iterator iter = keys.begin(); iter != keys.end(); ++iter) {
                if (customization == iter->first) {
                  ValueMap values;
                  pullValues(cf, iter->second, values);

                  for (ValueMap::const_iterator it = values.begin(); it != values.end(); ++it) {
                    element.customizations[it->first] = it->second;
                  }
                }
              }
            }
        } else {
          element.transport_info[it->first] = it->second;
        }
      }

      transport_templates_.push_back(element);
    }
  }

  return 0;
}

void
TransportRegistry::load_transport_lib(const OPENDDS_STRING& transport_type)
{
  GuardType guard(lock_);
  load_transport_lib_i(transport_type);
}

void
TransportRegistry::load_transport_lib_i(const OPENDDS_STRING& transport_type)
{
#if defined(ACE_AS_STATIC_LIBS)
  ACE_UNUSED_ARG(transport_type);
#else
  LibDirectiveMap::iterator lib_iter = lib_directive_map_.find(transport_type);
  if (lib_iter != lib_directive_map_.end()) {
    ACE_TString directive = ACE_TEXT_CHAR_TO_TCHAR(lib_iter->second.c_str());
    // Release the lock, because loading a transport library will
    // recursively call this function to add its default inst.
    load_pending_ = true;
    ACE_Reverse_Lock<LockType> rev_lock(lock_);
    {
      ACE_Guard<ACE_Reverse_Lock<LockType> > guard(rev_lock);
      ACE_Service_Config::process_directive(directive.c_str());
    }
    load_pending_ = false;
    load_condition_.notify_all();
  }
#endif
}

TransportInst_rch
TransportRegistry::create_inst(const OPENDDS_STRING& name,
                               const OPENDDS_STRING& transport_type,
                               bool wait_for_pending_load)
{
  GuardType guard(lock_);
  TransportType_rch type;

  while (wait_for_pending_load && load_pending_) {
    load_condition_.wait();
  }

  if (find(type_map_, transport_type, type) != 0) {
#if !defined(ACE_AS_STATIC_LIBS)
    // Not present, try to load library
    load_transport_lib_i(transport_type);

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

  DDS::DomainId_t domain_id = ei->get_domain_id();

  // if domain is in a domain range and config is a transport template,
  // get the correct config.
  if (TheServiceParticipant->belongs_to_domain_range(domain_id)) {
    bool found = false;
    for (OPENDDS_VECTOR(TransportTemplate)::const_iterator i = transport_templates_.begin(); i != transport_templates_.end(); ++i) {
      if (cfg->name() == i->config_name) {
        found = true;
        break;
      }
    }

    if (found)
    {
      ACE_TString cfg_name = ACE_TEXT_CHAR_TO_TCHAR(cfg->name().c_str());
      // create if not already created
      int ret = create_transport_template_instance(domain_id, cfg_name);

      if (ret == 0) {
        // get guid and create unique name for transport instance
        GUID_t guid = ei->get_id();
        if (guid == GUID_UNKNOWN) {
          ACE_ERROR((LM_ERROR,
                     ACE_TEXT("(%P|%t) TransportRegistry::bind_config: ")
                     ACE_TEXT("GUID_UNKNOWN. Can not bind entity to a domain template instance.\n")));
          throw Transport::UnableToCreate();
        }
        OPENDDS_STRING transport_inst_name = GuidConverter(guid).uniqueParticipantId();
        OPENDDS_STRING transport_config_name;

        if (cfg_name.c_str() != 0) {
          transport_config_name = ACE_TEXT_ALWAYS_CHAR(cfg_name.c_str());
        } else {
          ACE_ERROR((LM_ERROR,
                     ACE_TEXT("(%P|%t) TransportRegistry::bind_config: ")
                     ACE_TEXT("Config name is null.\n")));
          throw Transport::UnableToCreate();
        }

        bool success = create_new_transport_instance_for_participant(domain_id, transport_config_name, transport_inst_name);

        if (success) {
          // update config
          TransportConfig_rch new_cfg = get_config(transport_config_name);
          update_config_template_instance_info(new_cfg->name(), transport_inst_name);
          ei->transport_config(new_cfg);
          return;
        } else {
          ACE_ERROR((LM_ERROR,
                     ACE_TEXT("(%P|%t) TransportRegistry::bind_config: ")
                     ACE_TEXT("Failed to create new transport template instance.\n")));
          throw Transport::UnableToCreate();
        }
      }
    }
  }

  ei->transport_config(cfg);
}


void
TransportRegistry::remove_transport_template_instance(const OPENDDS_STRING& config_name)
{
  ConfigTemplateToInstanceMap::iterator i = config_template_to_instance_map.find(config_name);
  if (i == config_template_to_instance_map.end()) {
    if (DCPS_debug_level > 4) {
      ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) TransportRegistry::remove_transport_template_instance: ")
               ACE_TEXT("%C is not a transport template instance.\n"),
               config_name.c_str()));
    }
    return;
  }

  const OPENDDS_STRING inst_name = i->second;
  remove_config(config_name);
  remove_inst(inst_name);

  if (DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) DomainParticipantFactoryImpl::delete_participant ")
               ACE_TEXT("deleted TransportRegistry's dynamically created config %C and instance %C\n"),
               config_name.c_str(), inst_name.c_str()));
  }

  config_template_to_instance_map.erase(i);
}


TransportConfig_rch
TransportRegistry::fix_empty_default()
{
  DBG_ENTRY_LVL("TransportRegistry", "fix_empty_default", 6);
  GuardType guard(lock_);
  while (load_pending_) {
    load_condition_.wait();
  }
  if (global_config_.is_nil()
      || !global_config_->instances_.empty()
      || global_config_->name() != DEFAULT_CONFIG_NAME) {
    return global_config_;
  }
  TransportConfig_rch global_config = global_config_;
#if !defined(ACE_AS_STATIC_LIBS)
  load_transport_lib_i("rtps_udp");
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

bool TransportRegistry::has_type(const TransportType_rch& type) const
{
  DBG_ENTRY_LVL("TransportRegistry", "has_type", 6);
  const OPENDDS_STRING name = type->name();
  GuardType guard(lock_);
  return type_map_.count(name);
}

bool
TransportRegistry::create_new_transport_instance_for_participant(DDS::DomainId_t id, OPENDDS_STRING& transport_config_name, OPENDDS_STRING& transport_instance_name)
{
  // check per_participant
  TransportTemplate templ;
  if (get_transport_template_info(ACE_TEXT_CHAR_TO_TCHAR(transport_config_name.c_str()), templ)) {
    if (!templ.instantiate_per_participant) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) ERROR: TransportRegistry::")
                        ACE_TEXT("create_new_transport_instance_for_participant: ")
                        ACE_TEXT("transport_template missing instantiation_rule=per_participant\n")),
                      false);
    }
  }

  TransportConfig_rch cfg = get_config(transport_config_name);

  OPENDDS_STRING inst_name = cfg->instances_[0]->name() + "_" + transport_instance_name;
  OPENDDS_STRING config_name = cfg->name() + "_" + transport_instance_name;

  // assign new config and inst names
  transport_config_name = config_name;
  transport_instance_name = inst_name;

  OpenDDS::DCPS::TransportConfig_rch config = create_config(config_name);
  OpenDDS::DCPS::TransportInst_rch inst = create_inst(inst_name, "rtps_udp");

  ACE_Configuration_Heap ach;
  ACE_Configuration_Section_Key sect_key;

  ach.open();
  ach.open_section(ach.root_section(), ACE_TEXT("the_transport_setup"), true, sect_key);

  if (TheServiceParticipant->belongs_to_domain_range(id) ||
      config_has_transport_template(ACE_TEXT_CHAR_TO_TCHAR(transport_config_name.c_str()))) {
    TransportTemplate tr_inst;

    if (get_transport_template_info(ACE_TEXT_CHAR_TO_TCHAR(cfg->name().c_str()), tr_inst)) {
      ValueMap customs;

      if (!process_customizations(id, tr_inst, customs)) {
        ACE_ERROR_RETURN((LM_ERROR,
                  ACE_TEXT("(%P|%t) ERROR: TransportRegistry::")
                  ACE_TEXT("create_new_transport_instance_for_participant ")
                  ACE_TEXT("could not process_customizations\n")),
                 false);
      }

      // write
      for (ValueMap::const_iterator it = customs.begin(); it != customs.end(); ++it) {
        ach.set_string_value(sect_key, ACE_TEXT_CHAR_TO_TCHAR(it->first.c_str()), ACE_TEXT_CHAR_TO_TCHAR(it->second.c_str()));
      }
    } else {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) ERROR: TransportRegistry::")
                        ACE_TEXT("create_new_transport_instance_for_participant ")
                        ACE_TEXT("could not find transport_template for config %C\n"),
                        cfg->name().c_str()),
                       false);
    }
  } else {
    TransportEntry tr_inst;
    get_transport_info(ACE_TEXT_CHAR_TO_TCHAR(cfg->name().c_str()), tr_inst);

    for (ValueMap::const_iterator it = tr_inst.transport_info.begin();
         it != tr_inst.transport_info.end(); ++it) {
      ach.set_string_value(sect_key, ACE_TEXT_CHAR_TO_TCHAR(it->first.c_str()), ACE_TEXT_CHAR_TO_TCHAR(it->second.c_str()));
      if (DCPS_debug_level > 0) {
        ACE_DEBUG((LM_DEBUG,
                   ACE_TEXT("(%P|%t) TransportRegistry::")
                       ACE_TEXT("create_new_transport_entry_for_participant adding %C=%C\n"),
                   it->first.c_str(), it->second.c_str()));
      }
    }
  }

  inst->load(ach, sect_key);
  config->instances_.push_back(inst);

  return true;
}

void
TransportRegistry::update_config_template_instance_info(const OPENDDS_STRING& config_name, const OPENDDS_STRING& inst_name)
{
  config_template_to_instance_map[config_name] = inst_name;
}

void
TransportRegistry::release()
{
  DBG_ENTRY_LVL("TransportRegistry", "release", 6);
  GuardType guard(lock_);
  released_ = true;

  for (InstMap::iterator iter = inst_map_.begin(); iter != inst_map_.end(); ++iter) {
    iter->second->shutdown();
  }

  config_template_to_instance_map.clear();
  transport_templates_.clear();
  transports_.clear();
  type_map_.clear();
  inst_map_.clear();
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

OPENDDS_STRING
TransportRegistry::get_transport_template_instance_name(const DDS::DomainId_t id)
{
  OpenDDS::DCPS::Discovery::RepoKey configured_name = "transport_template_instance_";
  configured_name += to_dds_string(id);
  return configured_name;
}

OPENDDS_STRING
TransportRegistry::get_config_instance_name(const DDS::DomainId_t id)
{
  OpenDDS::DCPS::Discovery::RepoKey configured_name = "templ_config_";
  configured_name += to_dds_string(id);
  return configured_name;
}

int
TransportRegistry::create_transport_template_instance(DDS::DomainId_t domain, const ACE_TString& config_name)
{
  OPENDDS_STRING transport_inst_name = get_transport_template_instance_name(domain);
  OPENDDS_STRING config_inst_name = get_config_instance_name(domain);

  // has it already been created?
  ConfigMap::const_iterator i = config_map_.find(config_inst_name);
  if (i != config_map_.end()) {
    return 0; // already created
  }

  if (has_transport_templates()) {
    TransportTemplate tr_inst;

    if (get_transport_template_info(config_name, tr_inst)) {
      ACE_Configuration_Heap tcf;
      tcf.open();
      const ACE_Configuration_Section_Key& root = tcf.root_section();

      // create config
      ACE_Configuration_Section_Key csect;
      tcf.open_section(root, ACE_TEXT("config"), true /* create */, csect);
      ACE_Configuration_Section_Key csub_sect;
      tcf.open_section(csect, ACE_TEXT_CHAR_TO_TCHAR(config_inst_name.c_str()), true /* create */, csub_sect);
      tcf.set_string_value(csub_sect, ACE_TEXT("transports"), ACE_TEXT_CHAR_TO_TCHAR(transport_inst_name.c_str()));

      // create matching transport section
      ACE_Configuration_Section_Key tsect;
      tcf.open_section(root, ACE_TEXT("transport"), true /* create */, tsect);
      ACE_Configuration_Section_Key tsub_sect;
      tcf.open_section(tsect, ACE_TEXT_CHAR_TO_TCHAR(transport_inst_name.c_str()), true /* create */, tsub_sect);

      ValueMap customs;

      if (!process_customizations(domain, tr_inst, customs)) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: TransportRegistry::")
                          ACE_TEXT("create_transport_template_instance ")
                          ACE_TEXT("could not process_customizations\n")),
                         false);
      }

      // write
      for (ValueMap::const_iterator it = customs.begin(); it != customs.end(); ++it) {
        tcf.set_string_value(tsub_sect, ACE_TEXT_CHAR_TO_TCHAR(it->first.c_str()), ACE_TEXT_CHAR_TO_TCHAR(it->second.c_str()));

        if (DCPS_debug_level > 0) {
          ACE_DEBUG((LM_DEBUG,
                     ACE_TEXT("(%P|%t) TransportRegistry::")
                     ACE_TEXT("create_transport_template_instance adding %C=%C\n"),
                     it->first.c_str(), it->second.c_str()));
        }
      }

      // load transport
      int status = this->load_transport_configuration("transport_config_" + to_dds_string(domain), tcf);

      if (status != 0) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: TransportRegistry::create_transport_template_instance ")
                          ACE_TEXT("load_transport_configuration() returned %d\n"),
                          status),
                        -1);
      }

    }

  }

  return 0;
}

bool
TransportRegistry::config_has_transport_template(const ACE_TString& config_name) const
{
  for (OPENDDS_VECTOR(TransportTemplate)::const_iterator i = transport_templates_.begin(); i != transport_templates_.end(); ++i) {
    if (!ACE_OS::strcmp(ACE_TEXT_ALWAYS_CHAR(config_name.c_str()), i->config_name.c_str())) {
      return true;
    }
  }

  return false;
}

bool
TransportRegistry::get_transport_template_info(const ACE_TString& config_name, TransportTemplate& inst)
{
  bool ret = false;
  if (has_transport_templates()) {
    for (OPENDDS_VECTOR(TransportTemplate)::const_iterator i = transport_templates_.begin(); i != transport_templates_.end(); ++i) {
      if (!ACE_OS::strcmp(ACE_TEXT_ALWAYS_CHAR(config_name.c_str()), i->config_name.c_str())) {
        inst.transport_template_name = i->transport_template_name;
        inst.config_name = i->config_name;
        inst.instantiate_per_participant = i->instantiate_per_participant;
        inst.customizations = i->customizations;
        inst.transport_info = i->transport_info;

        ret = true;
        break;
      }
    }
  }

  if (DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) TransportRegistry::get_transport_template_info: ")
               ACE_TEXT("%C config %s\n"),
               ret ? "found" : "did not find", config_name.c_str()));
  }

  return ret;
}

bool TransportRegistry::process_customizations(const DDS::DomainId_t id, const TransportTemplate& tr_inst, ValueMap& customs)
{
  for (ValueMap::const_iterator it = tr_inst.transport_info.begin();
       it != tr_inst.transport_info.end(); ++it) {
    // customization.
    ValueMap::const_iterator idx = tr_inst.customizations.find(it->first);
    if (idx != tr_inst.customizations.end()) {
      OPENDDS_STRING addr = it->second;
      OPENDDS_STRING custom = idx->second;

      // check for 'add_domain_id_to_ip_addr' and 'add_domain_id_to_port'
      bool add_to_ip = false;
      bool add_to_port = false;

      OPENDDS_STRING val1;
      OPENDDS_STRING val2;

      size_t comma_pos = custom.find(',');
      if (comma_pos != OPENDDS_STRING::npos) {
        val1 = custom.substr(0, comma_pos);
        // remove spaces
        val1.erase(std::remove(val1.begin(), val1.end(), ' '), val1.end());

        val2 = custom.substr(comma_pos + 1);
        val2.erase(std::remove(val2.begin(), val2.end(), ' '), val2.end());

        add_to_ip = (val1 == CUSTOM_ADD_DOMAIN_TO_IP || val2 == CUSTOM_ADD_DOMAIN_TO_IP);
        add_to_port = (val1 == CUSTOM_ADD_DOMAIN_TO_PORT || val2 == CUSTOM_ADD_DOMAIN_TO_PORT);

      } else {
        custom.erase(std::remove(custom.begin(), custom.end(), ' '), custom.end());

        add_to_ip = (custom == CUSTOM_ADD_DOMAIN_TO_IP);
        add_to_port = (custom == CUSTOM_ADD_DOMAIN_TO_PORT);
      }

      if (!add_to_ip && !add_to_port) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) TransportRegistry::process_customizations: ")
                          ACE_TEXT("%C customization is not supported. Supported values are %C and %C\n"),
                          custom.c_str(), CUSTOM_ADD_DOMAIN_TO_IP.c_str(), CUSTOM_ADD_DOMAIN_TO_PORT.c_str()),
                         false);
      }

      // only add_domain_id_to_ip_addr and add_domain_id_to_port are supported at this time.
      if (add_to_ip) {
        size_t pos = addr.find_last_of(".");
        if (pos != OPENDDS_STRING::npos) {
          OPENDDS_STRING custom = addr.substr(pos + 1);

          OPENDDS_STRING port = "";
          OPENDDS_STRING last_octet = "";

          size_t cpos = custom.find(":");
          if (cpos != OPENDDS_STRING::npos) {
            port = custom.substr(cpos);
            last_octet = custom.substr(0, cpos);
          } else {
            last_octet = custom;
          }
          int val = 0;

          if (!convertToInteger(last_octet, val)) {
            ACE_ERROR_RETURN((LM_ERROR,
                              ACE_TEXT("(%P|%t) ERROR: TransportRegistry::")
                              ACE_TEXT("process_customizations ")
                              ACE_TEXT("could not convert %C to integer\n"),
                              custom.c_str()),
                             false);
          }

          val += id;
          addr = addr.substr(0, pos);
          addr += "." + to_dds_string(val);
          if (port != "") {
            addr += port;
          }
        } else {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) ERROR: Service_Participant::")
                            ACE_TEXT("process_customizations ")
                            ACE_TEXT("could not add_domain_id_to_ip_addr for address %C\n"),
                            idx->second.c_str()),
                           false);
        }

        if (DCPS_debug_level > 0) {
          ACE_DEBUG((LM_DEBUG,
                     ACE_TEXT("(%P|%t) TransportRegistry::")
                     ACE_TEXT("process_customizations processing add_domain_id_to_ip_addr: %C=%C\n"),
                     it->first.c_str(), addr.c_str()));
        }
      }

      if (add_to_port) {
        size_t pos = addr.find_last_of(":");
        if (pos == OPENDDS_STRING::npos) {
          // use default port + domainId. See 9.6.1.3 in the RTPS 2.2 protocol specification.
          const int PB = 7400;
          const int DG = 250;
          const int D2 = 1;
          int rtpsPort = PB + DG * id + D2;
          rtpsPort += id;
          addr += ":" + to_dds_string(rtpsPort);
        } else {
          // address has a port supplied
          int rtpsPort = -1;
          if (convertToInteger(addr.substr(pos + 1), rtpsPort)) {
            addr = addr.substr(0, pos);
            rtpsPort += id;
            addr += ":" + to_dds_string(rtpsPort);
          } else {
            ACE_ERROR_RETURN((LM_ERROR,
                              ACE_TEXT("(%P|%t) ERROR: Service_Participant::")
                              ACE_TEXT("process_customizations ")
                              ACE_TEXT("could not add_domain_id_to_port for %C.\n"),
                              idx->second.c_str()),
                             false);
          }

          if (DCPS_debug_level > 0) {
            ACE_DEBUG((LM_DEBUG,
                     ACE_TEXT("(%P|%t) TransportRegistry::")
                     ACE_TEXT("process_customizations processing add_domain_id_to_port: %C ==> %C\n"),
                     it->first.c_str(), addr.c_str()));
          }

        }

        if (DCPS_debug_level > 0) {
          ACE_DEBUG((LM_DEBUG,
                     ACE_TEXT("(%P|%t) TransportRegistry::")
                     ACE_TEXT("process_customizations processing add_domain_id_to_port: %C=%C\n"),
                     it->first.c_str(), addr.c_str()));
        }
      }

      customs[idx->first] = addr.c_str();
    } else {
      customs[it->first] = it->second.c_str();
      if (DCPS_debug_level > 0) {
        ACE_DEBUG((LM_DEBUG,
                   ACE_TEXT("(%P|%t) TransportRegistry::")
                   ACE_TEXT("process_customizations adding %C=%C\n"),
                   it->first.c_str(), it->second.c_str()));
      }
    }
  }

  return true;
}

bool TransportRegistry::has_transport_templates() const
{
  return !transport_templates_.empty();
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

}
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
