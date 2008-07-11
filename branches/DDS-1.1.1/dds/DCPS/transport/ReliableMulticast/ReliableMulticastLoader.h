// -*- C++ -*-
//

#ifndef OPENDDS_DCPS_RELIABLEMULTICASTLOADER_H
#define OPENDDS_DCPS_RELIABLEMULTICASTLOADER_H

#include /**/ "ace/pre.h"
#include /**/ "ace/config-all.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "ReliableMulticast_Export.h"
#include "tao/orbconf.h"
#include "ace/Service_Object.h"
#include "ace/Service_Config.h"

class ReliableMulticast_Export OPENDDS_DCPS_ReliableMulticastLoader
  : public ACE_Service_Object
{
public:
  OPENDDS_DCPS_ReliableMulticastLoader();
  virtual ~OPENDDS_DCPS_ReliableMulticastLoader();

  virtual int init(int argc, ACE_TCHAR* []);
};

#if defined (__ACE_INLINE__)
#include "ReliableMulticastLoader.inl"
#endif /* __ACE_INLINE__ */

ACE_STATIC_SVC_DECLARE_EXPORT (ReliableMulticast, OPENDDS_DCPS_ReliableMulticastLoader)
ACE_FACTORY_DECLARE (ReliableMulticast, OPENDDS_DCPS_ReliableMulticastLoader)

#include /**/ "ace/post.h"

#endif /* OPENDDS_DCPS_RELIABLEMULTICASTLOADER_H */
