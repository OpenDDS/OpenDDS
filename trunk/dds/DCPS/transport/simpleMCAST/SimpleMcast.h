// -*- C++ -*-
//
// $Id$

#ifndef TAO_DCPS_SIMPLEMCAST_H
#define TAO_DCPS_SIMPLEMCAST_H

#include /**/ "ace/pre.h"

#include "SimpleMcast_export.h"
#include "ace/Service_Object.h"
#include "ace/Service_Config.h"


#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */


class SimpleMcast_Export TAO_DCPS_SimpleMcast_Initializer
{
public:
  // Constructor.
  TAO_DCPS_SimpleMcast_Initializer (void);
};

static TAO_DCPS_SimpleMcast_Initializer TAO_DCPS_SimpleMcast_initializer;


#include /**/ "ace/post.h"

#endif /* TAO_DCPS_SIMPLEMCAST_H */

