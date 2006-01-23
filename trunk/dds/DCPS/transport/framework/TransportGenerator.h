// -*- C++ -*-
//
// $Id$
#ifndef TAO_DCPS_TRANSPORT_GENERATOR_H
#define TAO_DCPS_TRANSPORT_GENERATOR_H

#include  "dds/DCPS/dcps_export.h"
#include  "dds/DCPS/RcObject_T.h"
#include  "dds/DCPS/transport/framework/TransportImplFactory.h"
#include  "dds/DCPS/transport/framework/TransportConfiguration.h"

#include  "ace/Synch.h"

namespace TAO
{

  namespace DCPS
  {

    /**
     * @class TransportGenerator
     *
     * @brief Base class for concrete transports to provide new objects.
     *
     * Each transport implementation will need to define a concrete
     * subclass of the TransportGenerator class.  The base
     * class (TransportGenerator) contains the pure virtual functions to 
     * provide new objects. The concrete transport implements these methods
     * to provide the new concrete transport object.
     *
     * The TransportGenerator object is registered with the Transport6supplied to the 
     * TransportImpl::configure() method.
     */
    class TAO_DdsDcps_Export TransportGenerator : public RcObject<ACE_SYNCH_MUTEX>
    {
      public:

        /// Dtor
        virtual ~TransportGenerator();

        virtual TransportImplFactory* new_factory() = 0;

        virtual TransportConfiguration* new_configuration() = 0;
        

      protected:

        /// Default ctor.
        TransportGenerator();
    };

  }

}

#endif
