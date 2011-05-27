// -*- C++ -*-
//
// $Id$
#ifndef TAO_DCPS_THETRANSPORTFACTORY_H
#define TAO_DCPS_THETRANSPORTFACTORY_H

#include "TransportFactory.h"

#if defined(_MSC_VER) && _MSC_VER < 1300 && _MSC_VER >= 1200
# pragma warning( disable : 4231 )
#endif

namespace TAO
{
  namespace DCPS
  {

    #define TheTransportFactory TAO::DCPS::TransportFactory::instance()

  } /* namespace DCPS */

} /* namespace TAO */

#if defined (__ACE_INLINE__)
#include "TheTransportFactory.inl"
#endif /* __ACE_INLINE__ */

#endif  /* TAO_DCPS_THETRANSPORTFACTORY_H */
