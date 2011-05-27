// -*- C++ -*-
//
// $Id$

#ifndef DCPS_DUMMYTCP_H
#define DCPS_DUMMYTCP_H

#include /**/ "ace/pre.h"

#include "DummyTcp_export.h"
#include "ace/Service_Object.h"
#include "ace/Service_Config.h"


#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */


class DummyTcp_Export DCPS_DummyTcp_Initializer
{
public:
  // Constructor.
  DCPS_DummyTcp_Initializer (void);
};

static DCPS_DummyTcp_Initializer DCPS_DummyTcp_initializer;


#include /**/ "ace/post.h"

#endif /* DCPS_DUMMYTCP_H */
