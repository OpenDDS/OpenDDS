/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TRANSPORTREGISTRY_H
#define OPENDDS_DCPS_TRANSPORTREGISTRY_H

#include "dds/DCPS/dcps_export.h"
#include "TransportDefs.h"
#include "TransportImpl_rch.h"
#include "TransportImplFactory_rch.h"
#include "TransportReactorTask_rch.h"
#include "TransportType_rch.h"
#include "TransportInst_rch.h"
#include "TransportConfig_rch.h"
#include "TransportConfig.h"
#include "ace/Synch.h"

#include <string>
#include <map>

class ACE_Configuration_Heap;

namespace DDS {
  class Entity;
  typedef Entity* Entity_ptr;
}

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

  /// This will shutdown all TransportImpl objects.
  ///
  /// Client Application calls this method to tear down the transport
  /// framework.
  void release();

  TransportInst_rch create_inst(const std::string& name,
                                const std::string& transport_type);
  TransportInst_rch get_inst(const std::string& name) const;
  void remove_inst(const TransportInst_rch& inst);

  static const std::string DEFAULT_CONFIG_NAME;

  TransportConfig_rch create_config(const std::string& name);
  TransportConfig_rch get_config(const std::string& name) const;
  void remove_config(const TransportConfig_rch& cfg);

  TransportConfig_rch global_config() const;
  void global_config(const TransportConfig_rch& cfg);

  void bind_config(const std::string& name, DDS::Entity_ptr entity);
  void bind_config(const TransportConfig_rch& cfg, DDS::Entity_ptr entity);

  /// SPI (Service Provider Interface) for specific transport types:
  /// This function is called as the concrete transport library is loaded.
  /// The concrete transport library creates a concrete transport type object
  /// and registers it with TransportRegistry singleton.
  void register_type(const TransportType_rch& type);

  /// For internal use by OpenDDS DCPS layer:
  /// Transfer the configuration in ACE_Configuration_Heap object to the
  /// TransportRegistry.
  /// This is called by the Service_Participant at initialization time.
  int load_transport_configuration(ACE_Configuration_Heap& cf);

private:
  friend class ACE_Singleton<TransportRegistry, ACE_Recursive_Thread_Mutex>;

  TransportRegistry();
  ~TransportRegistry();

  typedef std::map<std::string, TransportType_rch> TypeMap;
  typedef std::map<std::string, TransportConfig_rch> ConfigMap;
  typedef std::map<std::string, TransportInst_rch> InstMap;

  typedef ACE_SYNCH_MUTEX LockType;
  typedef ACE_Guard<LockType> GuardType;

  TypeMap type_map_;
  ConfigMap config_map_;
  InstMap inst_map_;

  TransportConfig_rch global_config_;

  mutable LockType lock_;
};

} // namespace DCPS
} // namespace OpenDDS

#if defined (__ACE_INLINE__)
#include "TransportRegistry.inl"
#endif /* __ACE_INLINE__ */

#endif  /* OPENDDS_DCPS_TRANSPORTREGISTRY_H */
