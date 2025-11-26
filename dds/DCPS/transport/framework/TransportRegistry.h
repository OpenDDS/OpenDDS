/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TRANSPORT_FRAMEWORK_TRANSPORTREGISTRY_H
#define OPENDDS_DCPS_TRANSPORT_FRAMEWORK_TRANSPORTREGISTRY_H

#include "dds/DCPS/dcps_export.h"
#include "TransportDefs.h"
#include "TransportImpl_rch.h"
#include "dds/DCPS/ReactorTask_rch.h"
#include "TransportType.h"
#include "TransportType_rch.h"
#include "TransportInst_rch.h"
#include "TransportConfig_rch.h"
#include "TransportConfig.h"
#include "dds/DCPS/PoolAllocator.h"
#include "dds/DdsDcpsInfrastructureC.h"
#include "ace/Synch_Traits.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace DDS {
  class Entity;
  typedef Entity* Entity_ptr;
}

#define TheTransportRegistry OpenDDS::DCPS::TransportRegistry::instance()

namespace OpenDDS {
namespace DCPS {

/**
 * The TransportRegistry is a singleton object which provides a mechanism to
 * the application code to configure OpenDDS's use of the transport layer.
 */
class OpenDDS_Dcps_Export TransportRegistry {
public:

  /// Return a singleton instance of this class.
  static TransportRegistry* instance();

  /// Close the singleton instance of this class.
  static void close();

  /// This will shutdown all TransportImpl objects.
  ///
  /// Client Application calls this method to tear down the transport
  /// framework.
  void release();

  TransportInst_rch create_inst(const OPENDDS_STRING& name,
                                const OPENDDS_STRING& transport_type,
                                bool is_template = false);
  TransportInst_rch get_inst(const OPENDDS_STRING& name) const;

  /// Removing a TransportInst from the registry shuts down the underlying
  /// transport implementation (if one was started) and therefore should only
  /// be attempted after DDS Entities using this transport have been deleted.
  void remove_inst(const TransportInst_rch& inst);
  void remove_inst(const OPENDDS_STRING& inst_name);

  static const char DEFAULT_CONFIG_NAME[];
  static const char DEFAULT_INST_PREFIX[];

  TransportConfig_rch create_config(const OPENDDS_STRING& name);
  TransportConfig_rch get_config(const OPENDDS_STRING& name) const;
  void remove_config(const TransportConfig_rch& cfg);
  void remove_config(const OPENDDS_STRING& config_name);

  TransportConfig_rch global_config() const;
  void global_config(const TransportConfig_rch& cfg);

  void domain_default_config(DDS::DomainId_t domain,
                             const TransportConfig_rch& cfg);
  TransportConfig_rch domain_default_config(DDS::DomainId_t domain) const;

  void bind_config(const OPENDDS_STRING& name, DDS::Entity_ptr entity);
  void bind_config(const TransportConfig_rch& cfg, DDS::Entity_ptr entity);

  void add_config_alias(const String& key,
                        const String& value);

  /// SPI (Service Provider Interface) for specific transport types:
  /// This function is called as the concrete transport library is
  /// loaded.  The concrete transport library creates a concrete
  /// transport type object and registers it with TransportRegistry
  /// singleton.  Returns true for success.
  bool register_type(const TransportType_rch& type);

  /// For internal use by OpenDDS DCPS layer: Transfer the
  /// configuration in the ConfigStore to the TransportRegistry. This
  /// is called by the Service_Participant at initialization
  /// time. This function iterates each section in the configuration
  /// file, and creates TransportInst and TransportConfig objects and
  /// adds them to the registry.
  int load_transport_configuration();

  /// For internal use by OpenDDS DCPS layer:
  /// If the default config is empty when it's about to be used, allow the
  /// TransportRegistry to attempt to load a fallback option.
  TransportConfig_rch fix_empty_default();

  /// For internal use by OpenDDS DCPS layer:
  /// Dynamically load the library for the supplied transport type.
  void load_transport_lib(const OPENDDS_STRING& transport_type);

  bool released() const;

  void remove_participant(DDS::DomainId_t domain,
                          DomainParticipantImpl* participant);

private:
  friend class ACE_Singleton<TransportRegistry, ACE_Recursive_Thread_Mutex>;

  TransportRegistry();
  ~TransportRegistry();

  typedef OPENDDS_MAP(OPENDDS_STRING, TransportType_rch) TypeMap;
  typedef OPENDDS_MAP(OPENDDS_STRING, OPENDDS_STRING) AliasMap;
  typedef OPENDDS_MAP(OPENDDS_STRING, TransportConfig_rch) ConfigMap;
  typedef OPENDDS_MAP(OPENDDS_STRING, TransportInst_rch) InstMap;
  typedef OPENDDS_MAP(OPENDDS_STRING, OPENDDS_STRING) LibDirectiveMap;
  typedef OPENDDS_MAP(DDS::DomainId_t, TransportConfig_rch) DomainConfigMap;

  typedef ACE_SYNCH_MUTEX LockType;
  typedef ACE_Guard<LockType> GuardType;

  TypeMap type_map_;
  AliasMap alias_map_;
  ConfigMap config_map_;
  InstMap inst_map_;
  LibDirectiveMap lib_directive_map_;
  DomainConfigMap domain_default_config_map_;

  TransportConfig_rch global_config_;
  bool released_;

  mutable LockType lock_;

  // transport template support
  static const OPENDDS_STRING CUSTOM_ADD_DOMAIN_TO_IP;
  static const OPENDDS_STRING CUSTOM_ADD_DOMAIN_TO_PORT;

  TransportType_rch load_transport_lib_i(const OPENDDS_STRING& transport_type);

  bool process_transport(const String& transport_id,
                         bool is_template,
                         OPENDDS_LIST(TransportInst_rch)& instances);

  bool process_config(const String& config_id);
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#if defined (__ACE_INLINE__)
#include "TransportRegistry.inl"
#endif /* __ACE_INLINE__ */

#endif  /* OPENDDS_DCPS_TRANSPORTREGISTRY_H */
