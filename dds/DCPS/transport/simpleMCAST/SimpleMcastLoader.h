// -*- C++ -*-
//
// $Id$


#ifndef TAO_DCPS_SIMPLEMCAST_LOADER_H
#define TAO_DCPS_SIMPLEMCAST_LOADER_H
#include /**/ "ace/pre.h"

#include "SimpleMcast_export.h"

#include "tao/orbconf.h"

#include "ace/Service_Object.h"
#include "ace/Service_Config.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */


class SimpleMcast_Export TAO_DCPS_SimpleMcastLoader : public ACE_Service_Object
{
public:
  /// Constructor.
  TAO_DCPS_SimpleMcastLoader (void);

  /// Destructor.
  virtual ~TAO_DCPS_SimpleMcastLoader (void);

  /// Initialize the loader hooks.
  virtual int init (int argc,
                    ACE_TCHAR* []);
};

ACE_STATIC_SVC_DECLARE_EXPORT (SimpleMcast, TAO_DCPS_SimpleMcastLoader)
ACE_FACTORY_DECLARE (SimpleMcast, TAO_DCPS_SimpleMcastLoader)


#include /**/ "ace/post.h"
#endif /* TAO_DCPS_SIMPLEMCAST_LOADER_H */
