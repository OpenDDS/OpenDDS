/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "ReliableMulticast_pch.h"
#include "ReliableMulticastLoader.h"
#include "ReliableMulticastTransportGenerator.h"
#include "dds/DCPS/transport/framework/TheTransportFactory.h"

#if !defined (__ACE_INLINE__)
#include "ReliableMulticastLoader.inl"
#endif /* __ACE_INLINE__ */

int
OPENDDS_DCPS_ReliableMulticastLoader::init(int argc, ACE_TCHAR* argv[])
{
  ACE_UNUSED_ARG(argc);
  ACE_UNUSED_ARG(argv);

  static int initialized = 0;

  if (initialized) return 0;

  initialized = 1;

  OpenDDS::DCPS::TransportGenerator* generator = 0;

  ACE_NEW_RETURN(
    generator,
    OpenDDS::DCPS::ReliableMulticastTransportGenerator,
    -1);

  TheTransportFactory->register_generator(ACE_TEXT("ReliableMulticast")
                                          , generator);

  return 0;
}

ACE_FACTORY_DEFINE(
  ReliableMulticast,
  OPENDDS_DCPS_ReliableMulticastLoader)
ACE_STATIC_SVC_DEFINE(
  OPENDDS_DCPS_ReliableMulticastLoader,
  ACE_TEXT("OPENDDS_DCPS_ReliableMulticastLoader"),
  ACE_SVC_OBJ_T,
  &ACE_SVC_NAME(OPENDDS_DCPS_ReliableMulticastLoader),
  ACE_Service_Type::DELETE_THIS | ACE_Service_Type::DELETE_OBJ,
  0)
