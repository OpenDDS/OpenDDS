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

bool TransportRegistry::process_transport(const String& transport_id,
                                          bool is_template,
                                          OPENDDS_LIST(TransportInst_rch)& instances)
{
  // Get the factory_id for the transport.
  const String transport_type =
    TheServiceParticipant->config_store()->get(String((is_template ? "TRANSPORT_TEMPLATE_" : "TRANSPORT_") + transport_id + "_TRANSPORT_TYPE").c_str(), "");

  if (transport_type.empty()) {
    if (log_level >= LogLevel::Error) {
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) TransportRegistry::process_transport: "
                 "missing transport_type in [transport/%C] section.\n",
                 transport_id.c_str()));
    }
    return false;
  }

  // Create the TransportInst object and load the transport
  // configuration.
  TransportInst_rch inst = create_inst(transport_id, transport_type, is_template);
  if (!inst) {
    if (log_level >= LogLevel::Error) {
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) TransportRegistry::process_transport: "
                 "Unable to create transport instance in [transport/%C] section.\n",
                 transport_id.c_str()));
    }
    return false;
  }

  instances.push_back(inst);

  return true;
}

bool TransportRegistry::process_config(const String& config_id)
{
  // Create a TransportConfig object.
  TransportConfig_rch config = create_config(config_id);
  if (!config) {
    if (log_level >= LogLevel::Error) {
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) TransportRegistry::process_config: "
                 "Unable to create transport config in [config/%C] section.\n",
                 config_id.c_str()));
    }
    return false;
  }

  if (config->transports().empty()) {
    if (log_level >= LogLevel::Error) {
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) TransportRegistry::process_config: "
                 "No transport instances listed in [config/%C] section.\n",
                 config_id.c_str()));
    }
    return false;
  }

  const ConfigStoreImpl::StringList transports = config->transports();
  for (ConfigStoreImpl::StringList::const_iterator pos = transports.begin(), limit = transports.end();
       pos != limit; ++pos) {
    TransportInst_rch inst = get_inst(*pos);
    if (!inst) {
      if (log_level >= LogLevel::Error) {
        ACE_ERROR((LM_ERROR,
                   "(%P|%t) TransportRegistry::load_transport_configuration: "
                   "The inst (%C) in [config/%C] section is undefined.\n",
                   pos->c_str(), config->name().c_str()));
      }
      return false;
    }
    config->instances_.push_back(inst);
  }

  return true;
}

int
TransportRegistry::load_transport_configuration()
{
  // Record the transport instances created, so we can place them
  // in the implicit transport configuration for this file.
  OPENDDS_LIST(TransportInst_rch) instances;

  {
    const ConfigStoreImpl::StringList transports =
      TheServiceParticipant->config_store()->get_section_names("TRANSPORT");
    for (ConfigStoreImpl::StringList::const_iterator pos = transports.begin(), limit = transports.end();
         pos != limit; ++pos) {
      if (!process_transport(*pos, false, instances)) {
        return -1;
      }
    }
  }

  {
    const ConfigStoreImpl::StringList transports =
      TheServiceParticipant->config_store()->get_section_names("TRANSPORT_TEMPLATE");
    for (ConfigStoreImpl::StringList::const_iterator pos = transports.begin(), limit = transports.end();
         pos != limit; ++pos) {
      if (!process_transport(*pos, true, instances)) {
        return -1;
      }
    }
  }

  {
    const ConfigStoreImpl::StringList configs =
      TheServiceParticipant->config_store()->get_section_names("CONFIG");
    for (ConfigStoreImpl::StringList::const_iterator pos = configs.begin(), limit = configs.end();
         pos != limit; ++pos) {
      if (!process_config(*pos)) {
        return -1;
      }
    }
  }

  // Create and populate the default configuration for this
  // file with all the instances from this file.
  if (!instances.empty()) {
    TransportConfig_rch config = create_config("$file");
    if (!config) {
      if (log_level >= LogLevel::Error) {
        ACE_ERROR((LM_ERROR,
                   "(%P|%t) ERROR: TransportRegistry::load_transport_configuration: "
                   "Unable to create default transport config.\n"));
      }
      return -1;
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

  String real_name = name;

  AliasMap::const_iterator pos = alias_map_.find(real_name);
  if (pos != alias_map_.end()) {
    real_name = pos->second;
  }

  ConfigMap::const_iterator found = config_map_.find(real_name);
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

void
TransportRegistry::add_config_alias(const String& key,
                                    const String& value)
{
  GuardType guard(lock_);
  alias_map_[key] = value;
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
