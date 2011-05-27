// -*- C++ -*-
//
// $Id$
#ifndef TAO_DCPS_DATALINKSET_RCH_H
#define TAO_DCPS_DATALINKSET_RCH_H

#include  "dds/DCPS/RcHandle_T.h"
#include "ace/Version.h"

// this #if may not work for ACE version < 5.0.0
// but DDS will only build with 5.4.x or greater so that is OK.
#if ( (ACE_MAJOR_VERSION == 5) && (ACE_MINOR_VERSION == 4) && \
      (ACE_BETA_VERSION >= 5) ) \
 || ( ACE_MAJOR_VERSION > 5 || ACE_MINOR_VERSION > 4 )
#include  "dds/DCPS/transport/framework/DataLinkSet.h"
#endif

namespace TAO
{

  namespace DCPS
  {

    /**
     * This file instantiates a smart-pointer type (rch) to a specific
     * underlying "pointed-to" type.
     * 
     * This type definition is in its own header file so that the
     * smart-pointer type can be defined without causing the inclusion
     * of the underlying "pointed-to" type header file.  Instead, the
     * underlying "pointed-to" type is forward-declared.  This is analogous
     * to the inclusion requirements that would be imposed if the
     * "pointed-to" type was being referenced via a raw pointer type.
     * Holding the raw pointer indirectly via a smart pointer doesn't
     * change the inclusion requirements (ie, the underlying type doesn't
     * need to be included in either case).
     */

    // Forward declaration of the underlying type.
    // Somewhere between 1.4a_p6 and 1.4.8 this results in:
    // ..\dds\DCPS\RcHandle_T.h(118) : 
    //         error C2027: use of undefined type 'TAO::DCPS::DataLinkSet'
    // From looking at the change logs, I believe it was with the 
    //       release of ACE 5.4.5.
    // I decided to simplify this #if because DDS will not build
    // ACE version < 5.4.0
#if ACE_MAJOR_VERSION <= 5 && ACE_MINOR_VERSION <= 4 && ACE_BETA_VERSION < 5
    class DataLinkSet;
#endif

    /// The type definition for the smart-pointer to the underlying type.
    typedef RcHandle<DataLinkSet> DataLinkSet_rch;

  }  /* namespace DCPS */

}  /* namespace TAO */

#endif /* TAO_DCPS_DATALINKSET_RCH_H */
