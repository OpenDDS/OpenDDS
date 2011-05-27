// -*- C++ -*-
//
// $Id$
#ifndef TAO_DCPS_ASSOCIATIONDATA_H
#define TAO_DCPS_ASSOCIATIONDATA_H

#include "dds/DdsDcpsInfoUtilsC.h"


namespace TAO
{

  namespace DCPS
  {

    struct AssociationData
    {
      RepoId                 remote_id_;
      // TBD - May change to a pointer (a smart pointer?)
      TransportInterfaceInfo remote_data_;
    };

  }  /* namespace DCPS */

}  /* namespace TAO */

#endif  /* TAO_DCPS_ASSOCIATIONDATA_H */
