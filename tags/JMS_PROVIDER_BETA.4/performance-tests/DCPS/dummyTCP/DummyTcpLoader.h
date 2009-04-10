// -*- C++ -*-
//
// $Id$


#ifndef DCPS_DUMMYTCP_LOADER_H
#define DCPS_DUMMYTCP_LOADER_H
#include /**/ "ace/pre.h"

#include "DummyTcp_export.h"

#include "tao/orbconf.h"

#include "ace/Service_Object.h"
#include "ace/Service_Config.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */


class DummyTcp_Export DCPS_DummyTcpLoader : public ACE_Service_Object
{
public:
  /// Constructor.
  DCPS_DummyTcpLoader (void);

  /// Destructor.
  virtual ~DCPS_DummyTcpLoader (void);

  /// Initialize the loader hooks.
  virtual int init (int argc,
                    ACE_TCHAR* []);
};

ACE_STATIC_SVC_DECLARE_EXPORT (DummyTcp, DCPS_DummyTcpLoader)
ACE_FACTORY_DECLARE (DummyTcp, DCPS_DummyTcpLoader)


#include /**/ "ace/post.h"
#endif /* DCPS_DUMMYTCP_LOADER_H */
