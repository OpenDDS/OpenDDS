/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DCPS_SIMPLETCP_LOADER_H
#define DCPS_SIMPLETCP_LOADER_H
#include /**/ "ace/pre.h"

#include "SimpleTcp_export.h"

#include "tao/orbconf.h"

#include "ace/Service_Object.h"
#include "ace/Service_Config.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

class SimpleTcp_Export DCPS_SimpleTcpLoader : public ACE_Service_Object {
public:
  /// Constructor.
  DCPS_SimpleTcpLoader();

  /// Destructor.
  virtual ~DCPS_SimpleTcpLoader();

  /// Initialize the loader hooks.
  virtual int init(int argc,
                   ACE_TCHAR* []);
};

ACE_STATIC_SVC_DECLARE_EXPORT(SimpleTcp, DCPS_SimpleTcpLoader)
ACE_FACTORY_DECLARE(SimpleTcp, DCPS_SimpleTcpLoader)

#include /**/ "ace/post.h"
#endif /* DCPS_SIMPLETCP_LOADER_H */
