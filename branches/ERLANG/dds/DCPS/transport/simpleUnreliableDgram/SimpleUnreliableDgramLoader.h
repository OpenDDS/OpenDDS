// -*- C++ -*-
//
// $Id$


#ifndef OPENDDS_DCPS_SIMPLEUNRELIABLEDGRAM_LOADER_H
#define OPENDDS_DCPS_SIMPLEUNRELIABLEDGRAM_LOADER_H
#include /**/ "ace/pre.h"

#include "SimpleUnreliableDgram_export.h"

#include "tao/orbconf.h"

#include "ace/Service_Object.h"
#include "ace/Service_Config.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */


class SimpleUnreliableDgram_Export OPENDDS_DCPS_SimpleUnreliableDgramLoader : public ACE_Service_Object
{
public:
  /// Constructor.
  OPENDDS_DCPS_SimpleUnreliableDgramLoader (void);

  /// Destructor.
  virtual ~OPENDDS_DCPS_SimpleUnreliableDgramLoader (void);

  /// Initialize the loader hooks.
  virtual int init (int argc,
                    ACE_TCHAR* []);
};

ACE_STATIC_SVC_DECLARE_EXPORT (SimpleUnreliableDgram, OPENDDS_DCPS_SimpleUnreliableDgramLoader)
ACE_FACTORY_DECLARE (SimpleUnreliableDgram, OPENDDS_DCPS_SimpleUnreliableDgramLoader)


#include /**/ "ace/post.h"
#endif /* OPENDDS_DCPS_SIMPLEUNRELIABLEDGRAM_LOADER_H */
