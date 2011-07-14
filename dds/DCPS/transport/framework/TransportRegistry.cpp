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
#include "ace/Service_Config.h"

#if !defined (__ACE_INLINE__)
#include "TransportRegistry.inl"
#endif /* __ACE_INLINE__ */

#include <sstream>

namespace {
  /// Helper types and functions for config file parsing
  typedef std::map<std::string, std::string> ValueMap;
  typedef std::pair<std::string, ACE_Configuration_Section_Key> SubsectionPair;
  typedef std::list<SubsectionPair> KeyList;

  /// Used for sorting
  bool predicate( const OpenDDS::DCPS::TransportInst_rch& lhs,
                  const OpenDDS::DCPS::TransportInst_rch& rhs )
  {
    return lhs->name() < rhs->name();
  }

  ///     Function that pulls all the values from the
  ///     specified ACE Configuration Section and places them in a
  ///     value map based on the field name.  Returns the number of
  ///     values found in the section (and added to the value map).
  ///
  ///     cf     ACE_Configuration_Heap object being processed
  ///     key    ACE_Configuration_Section_Key object that specifies
  ///            the section of the .ini file to process
  ///     values Map of field names to values (both std::strings)
  ///            that this function will add to.
  ///
  int pullValues( ACE_Configuration_Heap& cf,
                  const ACE_Configuration_Section_Key& key,
                  ValueMap& values ) {
    int index = 0;
    ACE_TString name;
    ACE_Configuration::VALUETYPE type;

    while (cf.enumerate_values( key, index, name, type ) == 0) {
      ACE_TString value;
      if (type == ACE_Configuration::STRING) {
        cf.get_string_value( key, name.c_str(), value );
        values[name.c_str()] = value.c_str();
      } else {
        ACE_DEBUG((LM_WARNING, "Unexpected value type in config file (ignored): "
                   "name=%s, type=%d\n", name.c_str(), type));
      }
      index++;
    }
    return index;
  }


  ///     Function that processes the specified ACE Configuration Section
  ///     for subsections.  If multiple levels of subsections are found,
  ///     a non-zero value is returned to indicate the error.
  ///     All valid subsection will be placed into the supplied
  ///     KeyList (std::pair<> of the subsection number and
  ///     ACE_Configuration_Section_Key).  A return value of zero indicates
  ///     error-free success.
  ///
  ///
  ///     cf              ACE_Configuration_Heap object being processed
  ///     key             ACE_Configuration_Section_Key object that
  ///                     specifies the section of the .ini file to process
  ///     subsections     List of subsections found (list contains a
  ///                     std::pair<> of the subsection number and
  ///                     ACE_Configuration_Section_Key).
  ///
  int processSections( ACE_Configuration_Heap& cf,
                       const ACE_Configuration_Section_Key& key,
                       KeyList& subsections ) {
    int index = 0;
    ACE_TString name;
    while (cf.enumerate_sections( key, index, name ) == 0) {
      ACE_Configuration_Section_Key subkey;
      cf.open_section( key, name.c_str(), 0, subkey );
      subsections.push_back( SubsectionPair( name.c_str(), subkey ) );

      int subindex = 0;
      ACE_TString subname;
      if (cf.enumerate_sections( subkey, subindex, subname ) == 0) {
        // Found additional nesting of subsections that we don't care
        // to allow (e.g. [transport/my/yours]), so return an error.
        return 1;
      }
      index++;
    }
    return 0;
  }
}

namespace OpenDDS {
namespace DCPS {

TransportRegistry*
TransportRegistry::instance()
{
  return ACE_Singleton<TransportRegistry, ACE_Recursive_Thread_Mutex>::instance();
}

const std::string TransportRegistry::DEFAULT_CONFIG_NAME = "_OPENDDS_DEFAULT_CONFIG";
const std::string TransportRegistry::DEFAULT_INST_PREFIX = "_OPENDDS_";

OpenDDS::DCPS::TransportRegistry::TransportRegistry()
  : global_config_(new TransportConfig(DEFAULT_CONFIG_NAME))
{
  DBG_ENTRY_LVL("TransportRegistry", "TransportRegistry", 6);
  config_map_[DEFAULT_CONFIG_NAME] = global_config_;
  lib_directive_map_["tcp"]       = "dynamic OpenDDS_Tcp Service_Object * OpenDDS_Tcp:_make_TcpLoader()";
  lib_directive_map_["udp"]       = "dynamic OpenDDS_Udp Service_Object * OpenDDS_Udp:_make_UdpLoader()";
  lib_directive_map_["multicast"] = "dynamic OpenDDS_Multicast Service_Object * OpenDDS_Multicast:_make_MulticastLoader()";
}

int
TransportRegistry::load_transport_configuration(const std::string& file_name,
                                                ACE_Configuration_Heap& cf)
{
  int status = 0;
  const ACE_Configuration_Section_Key &root = cf.root_section();

  // Create a vector to hold configuration information so we can populate
  // them after the transports instances are created.
  typedef std::pair<TransportConfig_rch, std::vector<std::string> > ConfigInfo;
  std::vector<ConfigInfo> configInfoVec;

  // Record the transport instances created, so we can place them
  // in the implicit transport configuration for this file.
  std::list<TransportInst_rch> instances;

  ACE_TString sect_name;

  for (int index = 0;
       (status = cf.enumerate_sections(root, index, sect_name)) == 0;
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
        if (processSections( cf, sect, keys ) != 0) {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) TransportRegistry::load_transport_configuration: ")
                            ACE_TEXT("too many nesting layers in [%s] section.\n"),
                            sect_name.c_str()),
                           -1);
        }
        for (KeyList::const_iterator it=keys.begin(); it != keys.end(); ++it) {
          std::string transport_id = (*it).first;
          ACE_Configuration_Section_Key inst_sect = (*it).second;

          ValueMap values;
          if (pullValues( cf, (*it).second, values ) != 0) {
            // Get the factory_id for the transport.
            std::string transport_type;
            ValueMap::const_iterator vm_it = values.find("transport_type");
            if (vm_it != values.end()) {
              transport_type = (*vm_it).second;
            } else {
              ACE_ERROR_RETURN((LM_ERROR,
                                ACE_TEXT("(%P|%t) TransportRegistry::load_transport_configuration: ")
                                ACE_TEXT("missing transport_type in [transport/%s] section.\n"),
                                transport_id.c_str()),
                               -1);
            }
            // Create the TransportInst object and load the transport
            // configuration in ACE_Configuration_Heap to the TransportInst
            // object.
            TransportInst_rch inst = this->create_inst(transport_id,
                                                       transport_type);
            if (inst == 0) {
              ACE_ERROR_RETURN((LM_ERROR,
                                ACE_TEXT("(%P|%t) TransportRegistry::load_transport_configuration: ")
                                ACE_TEXT("Unable to create transport instance in [transport/%s] section.\n"),
                                transport_id.c_str()),
                               -1);
            }
            instances.push_back(inst);
            inst->load(cf, inst_sect);
          } else {
            ACE_ERROR_RETURN((LM_ERROR,
                              ACE_TEXT("(%P|%t) TransportRegistry::load_transport_configuration: ")
                              ACE_TEXT("missing transport_type in [transport/%s] section.\n"),
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
          // There are values inside [transport]
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) TransportRegistry::load_transport_configuration: ")
                            ACE_TEXT("config sections must have a section name\n"),
                            sect_name.c_str()),
                           -1);
        }
        // Process the subsections of this section (the individual config
        // impls).
        KeyList keys;
        if (processSections( cf, sect, keys ) != 0) {
          // Don't allow multiple layers of nesting ([config/x/y]).
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) TransportRegistry::load_transport_configuration: ")
                            ACE_TEXT("too many nesting layers in [%s] section.\n"),
                            sect_name.c_str()),
                           -1);
        }
        for (KeyList::const_iterator it=keys.begin(); it != keys.end(); ++it) {
          std::string config_id = (*it).first;

          ValueMap values;
          if (pullValues( cf, (*it).second, values ) == 0) {
            ACE_ERROR_RETURN((LM_ERROR,
                              ACE_TEXT("(%P|%t) TransportRegistry::load_transport_configuration: ")
                              ACE_TEXT("no instance defined in [config/%s] section.\n"),
                              config_id.c_str()),
                             -1);
          } else {
            // Create a TransportConfig object.
            TransportConfig_rch config = this->create_config(config_id);
            if (config == 0) {
              ACE_ERROR_RETURN((LM_ERROR,
                                ACE_TEXT("(%P|%t) TransportRegistry::load_transport_configuration: ")
                                ACE_TEXT("Unable to create transport config in [config/%s] section.\n"),
                                config_id.c_str()),
                               -1);
            }
            ConfigInfo configInfo;
            configInfo.first = config;
            for (ValueMap::const_iterator it=values.begin();
                 it != values.end(); ++it) {
              std::string name = (*it).first;
              if (name == "transports") {
                std::string value = (*it).second;
                std::stringstream ss(value);
                std::string item;
                while(std::getline(ss, item, ',')) {
                  configInfo.second.push_back(item);
                }
                configInfoVec.push_back(configInfo);
              } else if (name == "swap_bytes") {
                std::string value = (*it).second;
                if ((value == "1") || (value == "true")) {
                  config->swap_bytes_ = true;
                } else if ((value != "0") && (value != "false")) {
                  ACE_ERROR_RETURN((LM_ERROR,
                                    ACE_TEXT("(%P|%t) TransportRegistry::load_transport_configuration: ")
                                    ACE_TEXT("Illegal value for swap_bytes (%s) in [config/%s] section.\n"),
                                    value.c_str(), config_id.c_str()),
                                   -1);
                }
              } else {
                ACE_ERROR_RETURN((LM_ERROR,
                                  ACE_TEXT("(%P|%t) TransportRegistry::load_transport_configuration: ")
                                  ACE_TEXT("Unexpected entry (%s) in [config/%s] section.\n"),
                                  name.c_str(), config_id.c_str()),
                                 -1);
              }
            }
          }
        }
      }
    }
  }

  // Populate the configurations with instances
  for (unsigned int i = 0; i < configInfoVec.size(); ++i) {
    TransportConfig_rch config = configInfoVec[i].first;
    std::vector<std::string>& insts = configInfoVec[i].second;
    for (unsigned int j = 0; j < insts.size(); ++j) {
      TransportInst_rch inst = this->get_inst(insts[j]);
      if (inst == 0) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) TransportRegistry::load_transport_configuration: ")
                          ACE_TEXT("The inst (%s) in [config/%s] section is undefined.\n"),
                          insts[j].c_str(), config->name().c_str()),
                         -1);
      }
      config->instances_.push_back(inst);
    }
  }

  // Create and populate the default configuration for this
  // file with all the instances from this file.
  if (instances.size() > 0) {
    TransportConfig_rch config = this->create_config(file_name);
    if (config == 0) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) TransportRegistry::load_transport_configuration: ")
                        ACE_TEXT("Unable to create default transport config.\n"),
                        file_name.c_str()),
                       -1);
    }
    instances.sort( predicate );
    for (std::list<TransportInst_rch>::const_iterator it=instances.begin();
         it != instances.end(); ++it) {
      config->instances_.push_back(*it);
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
#if !defined(ACE_AS_STATIC_LIBS)
    // Not present, try to load library
    LibDirectiveMap::iterator lib_iter =
      this->lib_directive_map_.find(transport_type);
    if (lib_iter != this->lib_directive_map_.end()) {
      ACE_TString directive = ACE_TEXT((*lib_iter).second.c_str());
      // Release the lock, because loading a transport library will
      // recursively call this function to add its default inst.
      guard.release();
      ACE_Service_Config::process_directive(directive.c_str());
      guard.acquire();
    }

    // Try to find it again
    if (find(this->type_map_, transport_type, type) != 0) {
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
  DBG_ENTRY_LVL("TransportRegistry", "register_type", 6);
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
