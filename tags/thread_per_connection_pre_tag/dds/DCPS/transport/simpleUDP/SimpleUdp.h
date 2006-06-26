// -*- C++ -*-
//
// $Id$

#ifndef TAO_DCPS_SIMPLEUDP_H
#define TAO_DCPS_SIMPLEUDP_H

#include /**/ "ace/pre.h"

#include "SimpleUdp_export.h"
#include "ace/Service_Object.h"
#include "ace/Service_Config.h"


#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */


class SimpleUdp_Export TAO_DCPS_SimpleUdp_Initializer
{
public:
  // Constructor.
  TAO_DCPS_SimpleUdp_Initializer (void);
};

static TAO_DCPS_SimpleUdp_Initializer TAO_DCPS_SimpleUdp_initializer;


#include /**/ "ace/post.h"

#endif /* TAO_DCPS_SIMPLEUDP_H */

