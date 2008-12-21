// -*- C++ -*-
//
// $Id$
#ifndef OPENDDS_DCPS_THETRANSPORTFACTORY_H
#define OPENDDS_DCPS_THETRANSPORTFACTORY_H

#include "TransportFactory.h"

#if defined(_MSC_VER) && _MSC_VER < 1300 && _MSC_VER >= 1200
# pragma warning( disable : 4231 )
#endif

namespace OpenDDS
{
  namespace DCPS
  {

    #define TheTransportFactory OpenDDS::DCPS::TransportFactory::instance()

  } /* namespace DCPS */

} /* namespace OpenDDS */


#endif  /* OPENDDS_DCPS_THETRANSPORTFACTORY_H */
