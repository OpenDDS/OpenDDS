/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "UdpLoader.h"
#include "UdpGenerator.h"

#include "dds/DCPS/transport/framework/TheTransportFactory.h"

namespace OpenDDS {
namespace DCPS {

const ACE_TCHAR* UDP_TRANSPORT_TYPE(ACE_TEXT("udp"));

int
UdpLoader::init(int /*argc*/, ACE_TCHAR* /*argv*/[])
{
  static bool initialized(false);

  if (initialized) return 0;  // already initialized

  TransportGenerator *generator;
  ACE_NEW_RETURN(generator, UdpGenerator, -1);

  TheTransportFactory->register_generator(UDP_TRANSPORT_TYPE,
                                          generator);
  initialized = true;

  return 0;
}

ACE_FACTORY_DEFINE(OpenDDS_Udp, UdpLoader);
ACE_STATIC_SVC_DEFINE(
  UdpLoader,
  ACE_TEXT("UdpLoader"),
  ACE_SVC_OBJ_T,
  &ACE_SVC_NAME(UdpLoader),
  ACE_Service_Type::DELETE_THIS | ACE_Service_Type::DELETE_OBJ,
  0)

} // namespace DCPS
} // namespace OpenDDS
