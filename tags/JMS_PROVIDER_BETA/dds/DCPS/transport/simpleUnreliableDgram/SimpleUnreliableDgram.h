// -*- C++ -*-
//
// $Id$

#ifndef OPENDDS_DCPS_SIMPLEUNRELIABLEDGRAM_H
#define OPENDDS_DCPS_SIMPLEUNRELIABLEDGRAM_H

#include /**/ "ace/pre.h"

#include "SimpleUnreliableDgram_export.h"
#include "ace/Service_Object.h"
#include "ace/Service_Config.h"


#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */


class SimpleUnreliableDgram_Export OPENDDS_DCPS_SimpleUnreliableDgram_Initializer
{
public:
  // Constructor.
  OPENDDS_DCPS_SimpleUnreliableDgram_Initializer (void);
};

static OPENDDS_DCPS_SimpleUnreliableDgram_Initializer OPENDDS_DCPS_SimpleUnreliableDgram_initializer;


#include /**/ "ace/post.h"

#endif /* OPENDDS_DCPS_SIMPLEUNRELIABLEDGRAM_H */

