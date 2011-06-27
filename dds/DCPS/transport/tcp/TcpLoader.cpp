/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "Tcp_pch.h"

#include "TcpLoader.h"
#include "TcpGenerator.h"
#include "dds/DCPS/transport/framework/TheTransportFactory.h"
#include "dds/DCPS/transport/framework/TransportRegistry.h"
#include "dds/DCPS/transport/framework/EntryExit.h"

#include "tao/debug.h"
#include "ace/OS_NS_strings.h"

namespace OpenDDS {
namespace DCPS {

TcpLoader::TcpLoader()
{
  DBG_ENTRY_LVL("TcpLoader","TcpLoader",6);
}

TcpLoader::~TcpLoader()
{
  DBG_ENTRY_LVL("TcpLoader","~TcpLoader",6);
}

const ACE_TCHAR TCP_TRANSPORT_TYPE[] = ACE_TEXT("tcp");

int
TcpLoader::init(int argc, ACE_TCHAR* argv[])
{
  DBG_ENTRY_LVL("TcpLoader","init",6);

  static bool initialized = false;

  // Only allow initialization once.
  if (initialized)
    return 0;

  TransportGenerator_rch generator = new TcpGenerator;

  TheTransportRegistry->register_generator(TCP_TRANSPORT_TYPE,
                                           generator);

  TheTransportFactory->register_generator(TCP_TRANSPORT_TYPE,
                                          generator._retn());
  initialized = true;
  return 0;
}

/////////////////////////////////////////////////////////////////////

ACE_FACTORY_DEFINE(OpenDDS_Tcp, TcpLoader)
ACE_STATIC_SVC_DEFINE(TcpLoader,
                      ACE_TEXT("OpenDDS_Tcp"),
                      ACE_SVC_OBJ_T,
                      &ACE_SVC_NAME(TcpLoader),
                      ACE_Service_Type::DELETE_THIS
                      | ACE_Service_Type::DELETE_OBJ,
                      0)
}
}
