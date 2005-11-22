// -*- C++ -*-
//
// $Id$
#ifndef TAO_DCPS_TRANSPORTFACTORY_H
#define TAO_DCPS_TRANSPORTFACTORY_H

#include  "dds/DCPS/dcps_export.h"
#include  "TransportImpl_rch.h"
#include  "TransportImplFactory_rch.h"
#include  "TransportReactorTask_rch.h"
#include  "ace/Hash_Map_Manager.h"
#include  "ace/Synch.h"


namespace TAO
{
  namespace DCPS
  {

    class TAO_DdsDcps_Export TransportFactory
    {
      public:

        typedef ACE_UINT32 IdType;

        /// Default Ctor
        TransportFactory();

        /// Dtor
        ~TransportFactory();

        /// Invoked by client application such that the client application
        /// can then use the create() method with the supplied type_id.
        ///
        /// Special Note: Caller is "giving away" the impl_factory to
        ///               this TransportFactory.
        void register_type(IdType                type_id,
                           TransportImplFactory* impl_factory);

        /// Client application calls this to create TransportImpl objects
        /// of the specified type, and with the specified impl_id
        /// (TransportImpl instance id).  Once this has been done, the
        /// client application code can then use the obtain() method
        /// to retrieve a specific TransportImpl object using the
        /// TransportImpl instance id.
        ///
        /// Caller is responsible for the reference returned (ref counted).
        TransportImpl* create(IdType impl_id, IdType type_id);

        /// Client application calls this to retrieve a previously
        /// create()'d TransportImpl object, providing the TransportImpl
        /// instance id to identify the particular TransportImpl object
        /// to be returned.
        ///
        /// Caller is responsible for the reference returned (ref counted).
        TransportImpl* obtain(IdType impl_id);

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
        void release(IdType impl_id);


      private:

        /// The "Impl Type Map"
        ///
        ///   Key   == "Transport Implementation Type Id"
        ///   Value == TransportImpl object
        typedef ACE_Hash_Map_Manager_Ex<IdType,
                                        TransportImplFactory_rch,
                                        ACE_Hash<IdType>,
                                        ACE_Equal_To<IdType>,
                                        ACE_Null_Mutex>        ImplTypeMap;

        /// The "Impl Map"
        ///
        ///   Key   == "Transport Implementation Instance Id"
        ///   Value == TransportImplFactory object
        typedef ACE_Hash_Map_Manager_Ex<IdType,
                                        TransportImpl_rch,
                                        ACE_Hash<IdType>,
                                        ACE_Equal_To<IdType>,
                                        ACE_Null_Mutex>        ImplMap;

        /// Thread Lock type
        typedef ACE_SYNCH_MUTEX     LockType;

        /// Thread Guard type
        typedef ACE_Guard<LockType> GuardType;


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
