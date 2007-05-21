// -*- C++ -*-
//
// $Id$
#ifndef TAO_DDS_DCPS_LOCALOBJECT_H
#define TAO_DDS_DCPS_LOCALOBJECT_H

#include "tao/LocalObject.h"

namespace TAO
{
  namespace DCPS
  {
    /// just in case we want to extend this for DDS specific needs in the future
    typedef TAO_Local_RefCounted_Object LocalObject;

  };
};

#endif /* TAO_DDS_DCPS_LOCALOBJECT_H */