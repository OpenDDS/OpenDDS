// -*- C++ -*-
//
// $Id$

#include "ReliableMulticast_pch.h"
#include "ReliableMulticastLoader.h"
#include "ReliableMulticastTransportGenerator.h"
#include "dds/DCPS/transport/framework/TheTransportFactory.h"

#if !defined (__ACE_INLINE__)
#include "ReliableMulticastLoader.inl"
#endif /* __ACE_INLINE__ */

int
TAO_DCPS_ReliableMulticastLoader::init(int argc, ACE_TCHAR* argv[])
{
  ACE_UNUSED_ARG(argc);
  ACE_UNUSED_ARG(argv);

  TAO::DCPS::TransportGenerator* generator = 0;

  ACE_NEW_RETURN(
    generator,
    TAO::DCPS::ReliableMulticastTransportGenerator,
    -1
    );

  TheTransportFactory->register_generator(
    "ReliableMulticast", 
    generator
    );

  return 0;
}

ACE_FACTORY_DEFINE(
  ReliableMulticast,
  TAO_DCPS_ReliableMulticastLoader
  )
ACE_STATIC_SVC_DEFINE (
  TAO_DCPS_ReliableMulticastLoader,
  ACE_TEXT ("TAO_DCPS_ReliableMulticastLoader"),
  ACE_SVC_OBJ_T,
  &ACE_SVC_NAME (TAO_DCPS_ReliableMulticastLoader),
  ACE_Service_Type::DELETE_THIS | ACE_Service_Type::DELETE_OBJ,
  0
  )
