// -*- C++ -*-
//
// $Id$


#ifndef TAO_DCPS_SIMPLEUDP_LOADER_H
#define TAO_DCPS_SIMPLEUDP_LOADER_H
#include /**/ "ace/pre.h"

#include "SimpleUdp_export.h"

#include "tao/orbconf.h"

#include "ace/Service_Object.h"
#include "ace/Service_Config.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */


class SimpleUdp_Export TAO_DCPS_SimpleUdpLoader : public ACE_Service_Object
{
public:
  /// Constructor.
  TAO_DCPS_SimpleUdpLoader (void);

  /// Destructor.
  virtual ~TAO_DCPS_SimpleUdpLoader (void);

  /// Initialize the loader hooks.
  virtual int init (int argc,
                    ACE_TCHAR* []);
};

ACE_STATIC_SVC_DECLARE_EXPORT (SimpleUdp, TAO_DCPS_SimpleUdpLoader)
ACE_FACTORY_DECLARE (SimpleUdp, TAO_DCPS_SimpleUdpLoader)


#include /**/ "ace/post.h"
#endif /* TAO_DCPS_SIMPLEUDP_LOADER_H */
