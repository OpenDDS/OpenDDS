 /*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h"

#include "SecurityRegistry.h"

#include "SecurityConfig.h"

#include <dds/DCPS/transport/framework/EntryExit.h>

#include <dds/DCPS/DomainParticipantImpl.h>
#include <dds/DCPS/EntityImpl.h>
#include <dds/DCPS/SafetyProfileStreams.h>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/Util.h>

#include <dds/OpenDDSConfigWrapper.h>

#include <ace/Singleton.h>
#include <ace/OS_NS_strings.h>
#include <ace/Service_Config.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace Security {

const char* SecurityRegistry::DEFAULT_CONFIG_NAME = "_OPENDDS_DEFAULT_CONFIG";
const char* SecurityRegistry::BUILTIN_CONFIG_NAME = "_OPENDDS_BUILTIN_CONFIG";
const char* SecurityRegistry::DEFAULT_INST_PREFIX = "_OPENDDS_";
const char* SecurityRegistry::DEFAULT_PLUGIN_NAME = "BuiltIn";
const char* SecurityRegistry::ACCESS_CTRL_PLUGIN_NAME = "access_ctrl_plugin";
const char* SecurityRegistry::AUTHENTICATION_PLUGIN_NAME = "auth_plugin";
const char* SecurityRegistry::CRYPTO_PLUGIN_NAME = "crypto_plugin";


SecurityRegistry::SecurityConfigEntry::SecurityConfigEntry(const DCPS::String& name)
  : name_(name)
  , config_prefix_(DCPS::ConfigPair::canonicalize("SECURITY_" + name_))
{
}

SecurityRegistry::~SecurityRegistry()
{
  DBG_ENTRY_LVL("SecurityRegistry", "~SecurityRegistry", 6);
  release();
}

SecurityRegistry::SecurityConfigEntry::~SecurityConfigEntry()
{
}

DCPS::String
SecurityRegistry::SecurityConfigEntry::get_auth_name() const
{
  return TheServiceParticipant->config_store()->get(config_key("AUTH_CONFIG").c_str(), DEFAULT_PLUGIN_NAME);
}

DCPS::String
SecurityRegistry::SecurityConfigEntry::get_access_control_name() const
{
  return TheServiceParticipant->config_store()->get(config_key("ACCESS_CTRL_CONFIG").c_str(), DEFAULT_PLUGIN_NAME);
}

DCPS::String
SecurityRegistry::SecurityConfigEntry::get_crypto_name() const
{
  return TheServiceParticipant->config_store()->get(config_key("CRYPTO_CONFIG").c_str(), DEFAULT_PLUGIN_NAME);
}

ConfigPropertyList
SecurityRegistry::SecurityConfigEntry::get_properties() const
{
  const DCPS::ConfigStoreImpl::StringMap sm = TheServiceParticipant->config_store()->get_section_values(config_prefix());
  ConfigPropertyList cpl;
  for (DCPS::ConfigStoreImpl::StringMap::const_iterator pos = sm.begin(), limit = sm.end(); pos != limit; ++pos) {
    cpl.push_back(ConfigProperty(pos->first, pos->second));
  }
  return cpl;
}

SecurityRegistry*
SecurityRegistry::instance()
{
  return ACE_Unmanaged_Singleton<SecurityRegistry, ACE_Recursive_Thread_Mutex>::instance();
}

void
SecurityRegistry::close()
{
  ACE_Unmanaged_Singleton<SecurityRegistry, ACE_Recursive_Thread_Mutex>::close();
}

SecurityRegistry::SecurityRegistry()
{
  DBG_ENTRY_LVL("SecurityRegistry", "SecurityRegistry", 6);
  lib_directive_map_[DEFAULT_PLUGIN_NAME] = "dynamic OpenDDS_Security Service_Object * OpenDDS_Security:_make_BuiltInPluginLoader()";
}

void
SecurityRegistry::release()
{
  DBG_ENTRY_LVL("SecurityRegistry", "release", 6);
  GuardType guard(lock_);

  for (InstMap::iterator iter = registered_plugins_.begin(); iter != registered_plugins_.end(); ++iter) {
    iter->second->shutdown();
  }
  registered_plugins_.clear();
  config_map_.clear();
}

void
SecurityRegistry::register_plugin(const OPENDDS_STRING& plugin_name,
                                  SecurityPluginInst_rch plugin)
{
  GuardType guard(lock_);

  if (registered_plugins_.find(plugin_name) != registered_plugins_.end()) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) SecurityRegistry::register_plugin: ")
               ACE_TEXT("plugin=%C already exists.\n"),
               plugin_name.c_str()));
  } else {
    registered_plugins_.insert(std::make_pair(plugin_name, plugin));
  }
}

SecurityConfig_rch
SecurityRegistry::create_config(const OPENDDS_STRING& config_name)
{
  // If the configuration instance already exists, then it can be reused.
  // Otherwise create a new one and save it for any later needs
  SecurityConfig_rch existing_config;
  if (find_config(config_name, existing_config)) {
    return existing_config;
  }

  // This is making an assumption that the entry map is only written
  // to in single-threaded operation, and all acess from that point on
  // is read-only
  ConfigEntryMap::const_iterator iEntry = config_entries_.find(config_name);
  if (iEntry == config_entries_.end()) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) SecurityRegistry::create_config: ")
               ACE_TEXT("config=%C does not exist.\n"),
               config_name.c_str()));
    return SecurityConfig_rch();
  }

  // This will load any libraries that need to be loaded, and use the
  // resulting plugin instance objects to create the concrete implementations
  const SecurityConfigEntry_rch& entry = iEntry->second;
  SecurityPluginInst_rch auth_plugin_inst = get_plugin_inst(entry->get_auth_name());
  if (auth_plugin_inst.is_nil()) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) SecurityRegistry::create_config: ")
               ACE_TEXT("Failed to load authentication plugin %C\n"),
               entry->get_auth_name().c_str()));
    return SecurityConfig_rch();
  }

  SecurityPluginInst_rch ac_plugin_inst = get_plugin_inst(entry->get_access_control_name());
  if (ac_plugin_inst.is_nil()) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) SecurityRegistry::create_config: ")
               ACE_TEXT("Failed to load access control plugin %C\n"),
               entry->get_access_control_name().c_str()));
    return SecurityConfig_rch();
  }

  SecurityPluginInst_rch crypto_plugin_inst = get_plugin_inst(entry->get_crypto_name());
  if (crypto_plugin_inst.is_nil()) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) SecurityRegistry::create_config: ")
               ACE_TEXT("Failed to load crypto plugin %C\n"),
               entry->get_crypto_name().c_str()));
    return SecurityConfig_rch();
  }

  // Create the new config and try to add it to the container
  // of existing configs.  If this fails for some reason, then
  // release the new config and fail
  SecurityConfig_rch new_config =
    DCPS::make_rch<SecurityConfig>(config_name,
#if OPENDDS_CONFIG_SECURITY
                                   auth_plugin_inst->create_authentication(),
                                   ac_plugin_inst->create_access_control(),
                                   crypto_plugin_inst->create_crypto_key_exchange(),
                                   crypto_plugin_inst->create_crypto_key_factory(),
                                   crypto_plugin_inst->create_crypto_transform(),
                                   DCPS::RcHandle<Utility>(),
#endif
                                   entry->get_properties());

  if (!add_config(config_name, new_config)) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) SecurityRegistry::create_config: ")
               ACE_TEXT("Error storing config instance %C\n"),
               config_name.c_str()));
    return SecurityConfig_rch();
  }

  return new_config;
}

SecurityConfig_rch
SecurityRegistry::create_config(const OPENDDS_STRING& config_name,
                                SecurityPluginInst_rch plugin)
{
#if !OPENDDS_CONFIG_SECURITY
  ACE_UNUSED_ARG(plugin);
#endif

  SecurityConfig_rch existing_config;
  if (find_config(config_name, existing_config)) {
    return existing_config;
  }

  SecurityConfig_rch new_config =
    DCPS::make_rch<SecurityConfig>(config_name,
#if OPENDDS_CONFIG_SECURITY
                                   plugin->create_authentication(),
                                   plugin->create_access_control(),
                                   plugin->create_crypto_key_exchange(),
                                   plugin->create_crypto_key_factory(),
                                   plugin->create_crypto_transform(),
                                   plugin->create_utility(),
#endif
                                   ConfigPropertyList());

  if (!add_config(config_name, new_config)) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) SecurityRegistry::create_config: ")
               ACE_TEXT("Error storing config instance %C\n"),
               config_name.c_str()));
    return SecurityConfig_rch();
  }

  return new_config;
}

SecurityConfig_rch
SecurityRegistry::get_config(const OPENDDS_STRING& config_name) const
{
  GuardType guard(lock_);
  ConfigMap::const_iterator found = config_map_.find(config_name);
  return found == config_map_.end() ? SecurityConfig_rch() : found->second;
}

SecurityConfig_rch
SecurityRegistry::default_config() const
{
#if OPENDDS_CONFIG_SECURITY
  GuardType guard(lock_);
  if (!default_config_ && !TheServiceParticipant->get_security()) {
    Authentication_var a;
    AccessControl_var b;
    CryptoKeyExchange_var c;
    CryptoKeyFactory_var d;
    CryptoTransform_var e;
    default_config_ = DCPS::make_rch<SecurityConfig>("NoPlugins", a, b, c, d, e,
                                                     DCPS::RcHandle<Utility>(),
                                                     ConfigPropertyList());
  }
#endif
  return default_config_;
}

void
SecurityRegistry::default_config(const SecurityConfig_rch& config)
{
  GuardType guard(lock_);
  default_config_ = config;
}

SecurityConfig_rch
SecurityRegistry::builtin_config() const
{
#if OPENDDS_CONFIG_SECURITY
  GuardType g1(default_load_lock_);
  GuardType guard(lock_);
  if (!builtin_config_) {
    LibDirectiveMap::const_iterator lib_iter = lib_directive_map_.find(DEFAULT_PLUGIN_NAME);
    OPENDDS_ASSERT(lib_iter != lib_directive_map_.end());
    ACE_TString directive = ACE_TEXT_CHAR_TO_TCHAR(lib_iter->second.c_str());
    guard.release();
    ACE_Service_Config::process_directive(directive.c_str());
    guard.acquire();
  }
#endif
  return builtin_config_;
}

void
SecurityRegistry::builtin_config(const SecurityConfig_rch& config)
{
  GuardType guard(lock_);
  builtin_config_ = config;
}

int
SecurityRegistry::load_security_configuration()
{
  const DCPS::ConfigStoreImpl::StringList keys =
    TheServiceParticipant->config_store()->get_section_names("SECURITY");

  // Save the properties configured for each security entry
  for (DCPS::ConfigStoreImpl::StringList::const_iterator pos = keys.begin(), limit = keys.end();
       pos != limit; ++pos) {
    // Duplicate entry check
    if (config_entries_.find(*pos) != config_entries_.end()) {
      if (DCPS::log_level >= DCPS::LogLevel::Error) {
        ACE_ERROR((LM_ERROR,
                   "(%P|%t) SecurityRegistry::load_security_configuration: "
                   "duplicate sections named [security/%C].\n",
                   pos->c_str()));
      }
      return -1;
    }

    // Create the SecurityConfigEntry, which will be stored until
    // actual plugin instances are needed for this configuration
    SecurityConfigEntry_rch entry = DCPS::make_rch<SecurityConfigEntry>(*pos);
    config_entries_[*pos] = entry;
  }

  return 0;
}

void
SecurityRegistry::load_security_plugin_lib(const OPENDDS_STRING& security_plugin_type)
{
  GuardType guard(lock_);
  LibDirectiveMap::iterator lib_iter = lib_directive_map_.find(security_plugin_type);
  if (lib_iter != lib_directive_map_.end()) {
    ACE_TString directive = ACE_TEXT_CHAR_TO_TCHAR(lib_iter->second.c_str());
    guard.release();
    ACE_Service_Config::process_directive(directive.c_str());
  }
}

bool
SecurityRegistry::find_config(const OPENDDS_STRING& name, SecurityConfig_rch& config)
{
  GuardType guard(lock_);

  bool found_config = false;
  ConfigMap::iterator iConfig = config_map_.find(name);
  if (iConfig != config_map_.end()) {
    config = iConfig->second;
    found_config = true;
  }

  return found_config;
}

bool
SecurityRegistry::add_config(const OPENDDS_STRING& name, SecurityConfig_rch& config)
{
  GuardType guard(lock_);

  bool added_config = false;
  ConfigMap::iterator iConfig = config_map_.find(name);
  if (iConfig == config_map_.end()) {
    config_map_[name] = config;
    added_config = true;
  } else {
    // Someone else added this already. Use it instead
    config = iConfig->second;
    added_config = true;
  }

  return added_config;
}

SecurityPluginInst_rch SecurityRegistry::get_plugin_inst(
  const OPENDDS_STRING& plugin_name, bool attempt_fix)
{
  GuardType guard(lock_);

  SecurityPluginInst_rch plugin_inst;

  if (find(registered_plugins_, plugin_name, plugin_inst) != 0 && attempt_fix) {
    guard.release();
    // Not present, try to load library
    load_security_plugin_lib(plugin_name);
    guard.acquire();

    // Try to find it again
    find(registered_plugins_, plugin_name, plugin_inst);
  }

  return plugin_inst;
}

bool SecurityRegistry::has_no_configs() const
{
  GuardType guard(lock_);
  return config_map_.empty();
}

} // namespace OpenDDS
} // namespace Security

OPENDDS_END_VERSIONED_NAMESPACE_DECL
