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
#include "TcpInst.h"
#include "dds/DCPS/transport/framework/TheTransportFactory.h"
#include "dds/DCPS/transport/framework/TransportRegistry.h"
#include "dds/DCPS/transport/framework/TransportType.h"
#include "dds/DCPS/transport/framework/EntryExit.h"

#include "tao/debug.h"
#include "ace/OS_NS_strings.h"

namespace OpenDDS {
namespace DCPS {

TcpLoader::TcpLoader()
{
  DBG_ENTRY_LVL("TcpLoader", "TcpLoader", 6);
}

TcpLoader::~TcpLoader()
{
  DBG_ENTRY_LVL("TcpLoader", "~TcpLoader", 6);
}

class TcpType : public TransportType {
public:
  const char* name() { return "tcp"; }
  bool requires_reactor() { return true; }

  TransportInst* new_inst(const std::string& name)
  {
    return new TcpInst(name);
  }
};

int
TcpLoader::init(int, ACE_TCHAR*[])
{
  DBG_ENTRY_LVL("TcpLoader", "init", 6);

  static bool initialized = false;

  // Only allow initialization once.
  if (initialized)
    return 0;

  TheTransportRegistry->register_type(new TcpType);

  TheTransportFactory->register_generator(ACE_TEXT("tcp"), new TcpGenerator);

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
