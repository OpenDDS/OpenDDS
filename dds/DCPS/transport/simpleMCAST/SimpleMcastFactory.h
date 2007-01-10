// -*- C++ -*-
//
// $Id$
#ifndef TAO_DCPS_SIMPLEMCASTFACTORY_H
#define TAO_DCPS_SIMPLEMCASTFACTORY_H

#include  "SimpleMcast_export.h"
#include  "dds/DCPS/transport/framework/TransportImplFactory.h"

class SimpleMcastTransport;


namespace TAO
{

  namespace DCPS
  {

    class SimpleMcast_Export SimpleMcastFactory : public TransportImplFactory
    {
      public:

        SimpleMcastFactory();
        virtual ~SimpleMcastFactory();

        virtual int requires_reactor() const;


      protected:

        virtual TransportImpl* create();
    };

  }  /* namespace DCPS */

}  /* namespace TAO */

#if defined (__ACE_INLINE__)
#include "SimpleMcastFactory.inl"
#endif /* __ACE_INLINE__ */

#endif  /* TAO_DDS_SIMPLEMCASTFACTORY_H */
