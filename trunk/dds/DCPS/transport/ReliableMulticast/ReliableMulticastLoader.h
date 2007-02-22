// -*- C++ -*-
//

#ifndef TAO_DCPS_RELIABLEMULTICASTLOADER_H
#define TAO_DCPS_RELIABLEMULTICASTLOADER_H

#include /**/ "ace/pre.h"
#include /**/ "ace/config-all.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "ReliableMulticast_Export.h"
#include "tao/orbconf.h"
#include "ace/Service_Object.h"
#include "ace/Service_Config.h"

class ReliableMulticast_Export TAO_DCPS_ReliableMulticastLoader
  : public ACE_Service_Object
{
public:
  TAO_DCPS_ReliableMulticastLoader();
  virtual ~TAO_DCPS_ReliableMulticastLoader();

  virtual int init(int argc, ACE_TCHAR* []);
};

#if defined (__ACE_INLINE__)
#include "ReliableMulticastLoader.inl"
#endif /* __ACE_INLINE__ */

ACE_STATIC_SVC_DECLARE_EXPORT (ReliableMulticast, TAO_DCPS_ReliableMulticastLoader)
ACE_FACTORY_DECLARE (ReliableMulticast, TAO_DCPS_ReliableMulticastLoader)

#include /**/ "ace/post.h"

#endif /* TAO_DCPS_RELIABLEMULTICASTLOADER_H */
