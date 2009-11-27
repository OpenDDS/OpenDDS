/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "MulticastLoader.h"
#include "MulticastGenerator.h"

#include "dds/DCPS/transport/framework/TheTransportFactory.h"

namespace OpenDDS {
namespace DCPS {

int
MulticastLoader::init(int /*argc*/, ACE_TCHAR* /*argv*/[])
{
  static bool initialized(false);
  
  if (initialized) return 0;  // already initialized

  TransportGenerator *generator;
  ACE_NEW_RETURN(generator, MulticastGenerator, -1);

  TheTransportFactory->register_generator(ACE_TEXT("multicast"),
                                          generator);
  initialized = true;

  return 0;
}

ACE_FACTORY_DEFINE(OpenDDS_Multicast, MulticastLoader);
ACE_STATIC_SVC_DEFINE(
  MulticastLoader,
  ACE_TEXT("MulticastLoader"),
  ACE_SVC_OBJ_T,
  &ACE_SVC_NAME(MulticastLoader),
  ACE_Service_Type::DELETE_THIS | ACE_Service_Type::DELETE_OBJ,
  0)

} // namespace DCPS
} // namespace OpenDDS
