// -*- C++ -*-
//
// $Id$

#ifndef TAO_DCPS_SIMPLEUNRELIABLEDGRAM_H
#define TAO_DCPS_SIMPLEUNRELIABLEDGRAM_H

#include /**/ "ace/pre.h"

#include "SimpleUnreliableDgram_export.h"
#include "ace/Service_Object.h"
#include "ace/Service_Config.h"


#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */


class SimpleUnreliableDgram_Export TAO_DCPS_SimpleUnreliableDgram_Initializer
{
public:
  // Constructor.
  TAO_DCPS_SimpleUnreliableDgram_Initializer (void);
};

static TAO_DCPS_SimpleUnreliableDgram_Initializer TAO_DCPS_SimpleUnreliableDgram_initializer;


#include /**/ "ace/post.h"

#endif /* TAO_DCPS_SIMPLEUNRELIABLEDGRAM_H */

