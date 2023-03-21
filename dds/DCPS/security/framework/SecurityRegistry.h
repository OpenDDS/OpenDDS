/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_SECURITY_FRAMEWORK_SECURITYREGISTRY_H
#define OPENDDS_DCPS_SECURITY_FRAMEWORK_SECURITYREGISTRY_H

#include "SecurityPluginInst_rch.h"
#include "SecurityConfig_rch.h"
#include "SecurityConfigPropertyList.h"

#include <dds/DCPS/dcps_export.h>
#include <dds/DCPS/PoolAllocator.h>
#include <dds/DdsDcpsDomainC.h>

ACE_BEGIN_VERSIONED_NAMESPACE_DECL
class ACE_Configuration_Heap;
ACE_END_VERSIONED_NAMESPACE_DECL

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

#define TheSecurityRegistry OpenDDS::Security::SecurityRegistry::instance()

namespace OpenDDS {
namespace Security {

/**
 * The TheSecurityRegistry is a singleton object which provides a mechanism to
 * the application code to configure OpenDDS's security plugins.
 */
class OpenDDS_Dcps_Export SecurityRegistry {
public:

  /// Return a singleton instance of this class.
  static SecurityRegistry* instance();

  /// Close the singleton instance of this class.
  static void close();

  static const char* DEFAULT_CONFIG_NAME;
  static const char* BUILTIN_CONFIG_NAME;

  /// This will shutdown all Security plugin objects.
  ///
  /// Client Application calls this method to tear down the security framework.
  void release();

  /**
   * If the plugin is registered then return it. If it's not and attempt_fix is
   * true, then try to load and return the plugin, otherwise return a nil rch.
   */
  SecurityPluginInst_rch get_plugin_inst(
    const OPENDDS_STRING& plugin_name, bool attempt_fix = true);

  // Called by plugins to register their factory interface
  void register_plugin(const OPENDDS_STRING& plugin_name,
                       SecurityPluginInst_rch plugin);

  // Create or get an existing SecurityConfig object based off the name
  // in the configuration file
  SecurityConfig_rch create_config(const OPENDDS_STRING& config_name);

  SecurityConfig_rch create_config(const OPENDDS_STRING& config_name,
                                   SecurityPluginInst_rch plugin);

  bool has_no_configs() const;

  SecurityConfig_rch get_config(const OPENDDS_STRING& config_name) const;

  SecurityConfig_rch default_config() const;
  void default_config(const SecurityConfig_rch& cfg);

  SecurityConfig_rch builtin_config() const;
  void builtin_config(const SecurityConfig_rch& cfg);

  /// For internal use by OpenDDS DCPS layer:
  /// Transfer the configuration in ACE_Configuration_Heap object to
  /// the SecurityRegistry.  This is called by the Service_Participant
  /// at initialization time. This function iterates each section in
  /// the configuration file, and creates SecurityConfigEntry
  /// objects and adds them to the registry.
  int load_security_configuration(ACE_Configuration_Heap& cf);

private:
  friend class ACE_Singleton<SecurityRegistry, ACE_Recursive_Thread_Mutex>;

  static const char* DEFAULT_INST_PREFIX;
  static const char* DEFAULT_PLUGIN_NAME;
  static const char* SECURITY_SECTION_NAME;
  static const char* ACCESS_CTRL_PLUGIN_NAME;
  static const char* AUTHENTICATION_PLUGIN_NAME;
  static const char* CRYPTO_PLUGIN_NAME;

  // Internal class used to store configuration information
  class SecurityConfigEntry : public DCPS::RcObject
  {
  public:

    SecurityConfigEntry(const OPENDDS_STRING& entryNamee);
    ~SecurityConfigEntry();

    void add_property(const OPENDDS_STRING& name, const OPENDDS_STRING& value);

    const OPENDDS_STRING& get_entry_name() const { return entry_name_; }
    const OPENDDS_STRING& get_auth_name() const { return auth_name_; }
    const OPENDDS_STRING& get_access_control_name() const { return access_ctrl_name_; }
    const OPENDDS_STRING& get_crypto_name() const { return crypto_name_; }

    const ConfigPropertyList& get_properties() const { return properties_; }

  private:
    const OPENDDS_STRING entry_name_;

    OPENDDS_STRING auth_name_;
    OPENDDS_STRING access_ctrl_name_;
    OPENDDS_STRING crypto_name_;
    ConfigPropertyList properties_;
  };

  typedef DCPS::RcHandle<SecurityConfigEntry> SecurityConfigEntry_rch;
  typedef OPENDDS_MAP(OPENDDS_STRING, SecurityConfig_rch) ConfigMap;
  typedef OPENDDS_MAP(OPENDDS_STRING, SecurityPluginInst_rch) InstMap;
  typedef OPENDDS_MAP(OPENDDS_STRING, OPENDDS_STRING) LibDirectiveMap;
  typedef OPENDDS_MAP(OPENDDS_STRING, SecurityConfigEntry_rch) ConfigEntryMap;

  SecurityRegistry();
  ~SecurityRegistry();

  int load_security_sections(ACE_Configuration_Heap& cf, ConfigEntryMap& entries);

  /// For internal use by OpenDDS DCPS layer:
  /// Dynamically load the library for the supplied security plugin type.
  void load_security_plugin_lib(const OPENDDS_STRING& security_plugin_type);

  bool find_config(const OPENDDS_STRING& name, SecurityConfig_rch& config);
  bool add_config(const OPENDDS_STRING& name, SecurityConfig_rch& config);

  typedef ACE_SYNCH_MUTEX LockType;
  typedef ACE_Guard<LockType> GuardType;

  ConfigEntryMap config_entries_;
  ConfigMap config_map_;
  InstMap registered_plugins_;
  LibDirectiveMap lib_directive_map_;
  mutable SecurityConfig_rch default_config_;
  mutable SecurityConfig_rch builtin_config_;

  mutable LockType lock_;
  mutable LockType default_load_lock_;
};

} // namespace Security
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif  /* OPENDDS_DCPS_SECURITYREGISTRY_H */
