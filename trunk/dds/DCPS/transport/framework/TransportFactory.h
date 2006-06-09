// -*- C++ -*-
//
// $Id$
#ifndef TAO_DCPS_TRANSPORTFACTORY_H
#define TAO_DCPS_TRANSPORTFACTORY_H

#include  "dds/DCPS/dcps_export.h"
#include  "TransportDefs.h"
#include  "TransportImpl_rch.h"
#include  "TransportImplFactory_rch.h"
#include  "TransportReactorTask_rch.h"
#include  "TransportGenerator.h"
#include  "TransportGenerator_rch.h"
#include  "TransportConfiguration_rch.h"
#include  "ace/Hash_Map_Manager.h"
#include  "ace/Synch.h"
#include  "ace/Configuration.h"


namespace TAO
{
  namespace DCPS
  {

    class TAO_DdsDcps_Export TransportFactory
    {
      public:

        /// Default Ctor
        TransportFactory();

        /// Dtor
        ~TransportFactory();

        /// Register the SimpleTcp transport since it's part of the DDSDcps library.
        void register_simpletcp ();

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

        /// This function is called as the concrete transport library is loaded. The concrete transport
        /// library creates a concrete transport generator and register with TransportFactory singleton.
        /// Special Note: Caller is "giving away" the generator to
        ///               this TransportFactory.
        void register_generator (const char* name, TransportGenerator* generator);

        /// Transfer the configuration in ACE_Configuration_Heap object to the TransportFactory
        /// object which uses hash map to cache those configuration. This is called by the
        /// Service_Participant at initialization time. This function iterates each sections
        /// in the configuration file, if it's transport section then a TransportConfiguration
        /// object is created and added to the configuration_map_.
        int load_transport_configuration (ACE_Configuration_Heap& cf);

        /// This interface is used when the transport_id is configured via the configuration
        /// file. In this case, the TransportConfiguration object should be already registered
        /// as transport_id in the configuration_map_. If the configuration is not registered
        /// then an exception (Transport::NotConfigured) is raised.
        TransportConfiguration_rch get_configuration (TransportIdType transport_id);

        /// This interface is used when the transport_id is NOT configured in the configuration
        /// file. A new TransportConfiguration object with default/hardcoded configuration is
        /// created and registered as the transport_id in the configuration map. If the
        /// transport_id is already configured then an exception (Transport::duplicate)
        /// is raised.
        TransportConfiguration_rch create_configuration (TransportIdType transport_id,
                                                         ACE_CString transport_type);

        /// This interface is the easiest way for user to get the TransportConfiguration
        /// object without knowing whether the application is configured via a
        /// configuration file or not.
        /// Necessary conflict is checked in this method. If the transport_id is already
        /// configured and the transport type in the TransportConfiguration object is
        /// different from provided transport_type then the an exception
        /// (Transport::ConfigurationConflict) is raised.:
        TransportConfiguration_rch get_or_create_configuration (TransportIdType transport_id,
                                                                ACE_CString transport_type);

        /// This interface is used when the transport_id is configured via the configuration
        /// file. In this case, the TransportConfiguration object should be already registered
        /// as transport_id in the configuration_map_.
        ///
        /// The configuration specifies the type of transport which is used to create the
        /// TransportImpl object. This interface uses the provided transport_id and the
        /// transport_type specified in configuration to create the TransportImpl object.
        /// See the create_transport_impl_i() for implementation.
        ///
        /// The auto_configure flag gives option to configure the TransportImpl object with the
        /// registered TransportConfiguration object via TransportImpl::configure() call.
        TransportImpl_rch create_transport_impl (TransportIdType transport_id,
                                                 bool auto_configure = AUTO_CONFIG);

        /// In general, this interface is used when the transport_id is NOT configured via the
        /// configuration file. In this case, the provided transport_id and transport_type are
        /// passed to the internal implementation. See the create_transport_impl_i() for
        /// implementation.
        ///
        /// This interface can also be used when the TransportConfiguration already exists
        /// which could be created during loading configuration file or created by the user
        /// before this function is called.
        ///
        /// The auto_configure flag gives option to configure the TransportImpl object with the
        /// registered TransportConfiguration object via TransportImpl::configure() call.
        TransportImpl_rch create_transport_impl (TransportIdType transport_id,
                                                 ACE_CString transport_type,
                                                 bool auto_configure = AUTO_CONFIG);

        /// Client application calls this to retrieve a previously
        /// create()'d TransportImpl object, providing the TransportImpl
        /// instance id to identify the particular TransportImpl object
        /// to be returned.
        TransportImpl_rch obtain(TransportIdType impl_id);

     private:

        /// Get the previously created TransportImplFactory object reference or create
        /// a new TransportImplFactory object with factory_id.
        /// Note each type transport supports one instance of TransportImplFactory
        ///      in current implementation, so the factory_id is correspond to the
        ///      transport_type.
        TransportImplFactory_rch get_or_create_factory (FactoryIdType factory_id);

        /// Bind the factory_id->TransportImplFactory_rch to the impl_type_map_.
        void register_factory(FactoryIdType            factory_id,
                              TransportImplFactory_rch impl_factory);

        /// Bind the transport_id->TransportConfigInfo object to the configuration_map_.
        void register_configuration(TransportIdType             transport_id,
                                    TransportConfiguration_rch  config);

        /// This function creates a TransportImpl object using the specified
        /// TransportImplFactory instance. See comments in definition section of this
        /// function.
        TransportImpl_rch create_transport_impl_i (TransportIdType impl_id, FactoryIdType type_id);

        /// The "Transport registration Map"
        ///
        ///  Key  == "Transport type"
        ///  Value == TransportGenerator object
        typedef ACE_Hash_Map_Manager_Ex<ACE_CString,
                                        TransportGenerator_rch,
                                        ACE_Hash<ACE_CString>,
                                        ACE_Equal_To<ACE_CString>,
                                        ACE_Null_Mutex>        GeneratorMap;

        /// The "TransportConfiguration Map"
        ///
        ///  Key  == "transport id"
        ///  Value == TransportConfiguration object
        typedef ACE_Hash_Map_Manager_Ex<TransportIdType,
                                        TransportConfiguration_rch,
                                        ACE_Hash<TransportIdType>,
                                        ACE_Equal_To<TransportIdType>,
                                        ACE_Null_Mutex>        ConfigurationMap;

       /// The "TranportImplFactory instance Map".
       /// Since each transport type has just one TransportImplFactory then we can
       /// use the transport type name as the factory id.
        ///
        ///   Key   == "Transport type"
        ///   Value == TransportImplFactory object
        typedef ACE_Hash_Map_Manager_Ex<FactoryIdType,
                                        TransportImplFactory_rch,
                                        ACE_Hash<FactoryIdType>,
                                        ACE_Equal_To<FactoryIdType>,
                                        ACE_Null_Mutex>        ImplTypeMap;

        /// The "Impl Map"
        ///
        ///   Key   == "TransportImpl instance id"
        ///   Value == TransportImpl object
        typedef ACE_Hash_Map_Manager_Ex<TransportIdType,
                                        TransportImpl_rch,
                                        ACE_Hash<TransportIdType>,
                                        ACE_Equal_To<TransportIdType>,
                                        ACE_Null_Mutex>        ImplMap;

        /// Thread Lock type
        typedef ACE_SYNCH_MUTEX     LockType;

        /// Thread Guard type
        typedef ACE_Guard<LockType> GuardType;

        /// The map of registered TransportGenerator object from the concrete transport library.
        GeneratorMap generator_map_;

        /// The map of the registered configuration information for the transport configured.
        ConfigurationMap configuration_map_;

        /// The map of registered TransportImplFactory objects.
        ImplTypeMap impl_type_map_;

        /// The map of TransportImpl objects that have been create()'d.
        ImplMap     impl_map_;

        /// Thread lock used to protect simultaneous access to the maps.
        LockType    lock_;

        /// This is the reactor task that will be used by those TransportImpl
        /// objects that need it.  It will only be created (and activated)
        /// if there is at least one TransportImpl that needs it.  It is
        /// created (and activated) as the result of the first call to
        /// the TransportFactory::create() method for any TransportImpl that
        /// requires a reactor.  The point is - we delay creation/activation
        /// of the reactor task until its first need arises.
        TransportReactorTask_rch reactor_task_;
//MJM: We may need to provide a mechanism to set this from the client
//MJM: application.
    };

  } /* namespace DCPS */

} /* namespace TAO */

#if defined (__ACE_INLINE__)
#include "TransportFactory.inl"
#endif /* __ACE_INLINE__ */

#endif  /* TAO_DCPS_TRANSPORTFACTORY_H */
