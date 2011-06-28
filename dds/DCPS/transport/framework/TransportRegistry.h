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
#include "TransportGenerator.h"
#include "TransportGenerator_rch.h"
#include "TransportInst_rch.h"
#include "ace/Synch.h"
#include "ace/Configuration.h"

#include <map>

namespace OpenDDS {
namespace DCPS {

/**
 * The TransportRegistry is a singleton object which provides a mechanism to
 * the application code to configure OpenDDS's use of the transport layer.
 * 1) Own the transport configuration objects, transport implementation objects
 *    and TransportReactorTask object.
 */
class OpenDDS_Dcps_Export TransportRegistry {
public:

  /// Return a singleton instance of this class.
  static TransportRegistry * instance();

  /// This will shutdown all TransportImpl objects.
  ///
  /// Client Application calls this method to tear down the transport
  /// framework.
  void release();

  /// This will shutdown just one TransportImpl object.
  ///
  /// Client Application can call this method to tear down just one
  /// TransportImpl object.  Note that this is effectively an
  /// "uncreate()" method.
  void release(TransportIdType impl_id);

  /// Transfer the configuration in ACE_Configuration_Heap object to the
  /// TransportRegistry.
  /// This is called by the Service_Participant at initialization time.
  int load_transport_configuration(ACE_Configuration_Heap& cf);

  TransportInst_rch create_configuration(ACE_TString transport_type);

  TransportImpl_rch create_transport_impl(TransportIdType id,
                                          TransportInst_rch config);

  /// Client application calls this to retrieve a previously
  /// create()'d TransportImpl object, providing the TransportImpl
  /// instance id to identify the particular TransportImpl object
  /// to be returned.
  TransportImpl_rch obtain(TransportIdType impl_id);

  /// SPI (Service Provider Interface) for specific transport types:
  /// This function is called as the concrete transport library is loaded. The concrete transport
  /// library creates a concrete transport generator and register with TransportRegistry singleton.
  void register_generator(const ACE_TCHAR* name,
                          TransportGenerator_rch generator);

private:
  friend class ACE_Singleton<TransportRegistry, ACE_Recursive_Thread_Mutex>;

  TransportRegistry();
  ~TransportRegistry();

  /// Get the previously created TransportImplFactory object reference or create
  /// a new TransportImplFactory object with factory_id.
  /// Note each type transport supports one instance of TransportImplFactory
  ///      in current implementation, so the factory_id is correspond to the
  ///      transport_type.
  TransportImplFactory_rch get_or_create_factory(FactoryIdType factory_id);

  /// Bind the factory_id->TransportImplFactory_rch to the impl_type_map_.
  void register_factory(FactoryIdType            factory_id,
                        TransportImplFactory_rch impl_factory);

  /// This function creates a TransportImpl object using the specified
  /// TransportImplFactory instance. See comments in definition section of this
  /// function.
  TransportImpl_rch create_transport_impl_i(TransportIdType impl_id, FactoryIdType type_id);

  /// The "Impl Map"
  ///
  ///   Key   == "TransportImpl instance id"
  ///   Value == TransportImpl object
  typedef std::map<TransportIdType, TransportImpl_rch> ImplMap;

  /// The "Transport registration Map"
  ///
  ///  Key  == "Transport type"
  ///  Value == TransportGenerator object
  typedef std::map<ACE_TString, TransportGenerator_rch> GeneratorMap;

  /// The "TranportImplFactory instance Map".
  /// Since each transport type has just one TransportImplFactory then we can
  /// use the transport type name as the factory id.
  ///
  ///   Key   == "Transport type"
  ///   Value == TransportImplFactory object
  typedef std::map<FactoryIdType, TransportImplFactory_rch> ImplTypeMap;

  /// Thread Lock type
  typedef ACE_SYNCH_MUTEX     LockType;

  /// Thread Guard type
  typedef ACE_Guard<LockType> GuardType;

  /// The map of registered TransportGenerator object from the concrete transport library.
  GeneratorMap generator_map_;

  /// The map of registered TransportImplFactory objects.
  ImplTypeMap impl_type_map_;

  /// The map of TransportImpl objects that have been create()'d.
  ImplMap     impl_map_;

  /// Thread lock used to protect simultaneous access to the maps.
  LockType    lock_;
};

} // namespace DCPS
} // namespace OpenDDS

#if defined (__ACE_INLINE__)
#include "TransportRegistry.inl"
#endif /* __ACE_INLINE__ */

#endif  /* OPENDDS_DCPS_TRANSPORTREGISTRY_H */
