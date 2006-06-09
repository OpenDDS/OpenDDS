// -*- C++ -*-
//
// $Id$
#ifndef TAO_DCPS_TRANSPORTIMPLFACTORY_H
#define TAO_DCPS_TRANSPORTIMPLFACTORY_H

#include  "dds/DCPS/dcps_export.h"
#include  "dds/DCPS/RcObject_T.h"
#include  "TransportImpl.h"
#include  "TransportImpl_rch.h"
#include  "ace/Synch.h"


namespace TAO
{
  namespace DCPS
  {

    class TransportImpl;
    class TransportReactorTask;


    class TAO_DdsDcps_Export TransportImplFactory : public RcObject<ACE_SYNCH_MUTEX>
    {
      public:

        virtual ~TransportImplFactory();

        TransportImpl* create_impl();
        TransportImpl* create_impl(TransportReactorTask* reactor_task);
//MJM: Why not just have this method with a default null value?

        // Subclass should override if it requires the reactor because
        // the default implementation is that the reactor is not required.
        virtual int requires_reactor() const;

      protected:

        TransportImplFactory();

        /// This should return 0 (nil) if the create() cannot be done
        /// for any reason.
        virtual TransportImpl* create() = 0;

    };

  } /* namespace DCPS */

} /* namespace TAO */

#if defined (__ACE_INLINE__)
#include "TransportImplFactory.inl"
#endif /* __ACE_INLINE__ */

#endif  /* TAO_DCPS_TRANSPORTIMPLFACTORY_H */
