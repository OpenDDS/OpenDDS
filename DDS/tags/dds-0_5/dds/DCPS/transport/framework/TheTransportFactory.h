// -*- C++ -*-
//
// $Id$
#ifndef TAO_DCPS_THETRANSPORTFACTORY_H
#define TAO_DCPS_THETRANSPORTFACTORY_H

#include  "TransportFactory.h"
#include  "tao/TAO_Singleton.h"


namespace TAO
{
  namespace DCPS
  {

    typedef TAO_Singleton<TransportFactory, TAO_SYNCH_MUTEX>
                                                 TRANSPORT_FACTORY_SINGLETON;

    TAO_DDSDCPS_SINGLETON_DECLARE(::TAO_Singleton,
                                  TransportFactory,
                                  TAO_SYNCH_MUTEX)  

    #define TheTransportFactory TAO::DCPS::TRANSPORT_FACTORY_SINGLETON::instance()

  } /* namespace DCPS */

} /* namespace TAO */

#if defined (__ACE_INLINE__)
#include "TheTransportFactory.inl"
#endif /* __ACE_INLINE__ */

#endif  /* TAO_DCPS_THETRANSPORTFACTORY_H */
