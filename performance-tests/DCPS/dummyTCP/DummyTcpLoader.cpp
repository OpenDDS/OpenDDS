// $Id$
#include "DummyTcp_pch.h"

#include "DummyTcpLoader.h"
#include "DummyTcpInst.h"

#include "dds/DCPS/transport/framework/EntryExit.h"
#include "dds/DCPS/transport/framework/TransportRegistry.h"

#include "tao/debug.h"
#include "ace/OS_NS_strings.h"

DCPS_DummyTcpLoader::DCPS_DummyTcpLoader (void)
{
  DBG_ENTRY_LVL("DCPS_DummyTcpLoader","DCPS_DummyTcpLoader",1);
}

DCPS_DummyTcpLoader::~DCPS_DummyTcpLoader (void)
{
  DBG_ENTRY_LVL("DCPS_DummyTcpLoader","~DCPS_DummyTcpLoader",1);
}

class DummyTcpType : public OpenDDS::DCPS::TransportType {
public:
  const char* name() { return "dummy_tcp"; }

  OpenDDS::DCPS::TransportInst* new_inst(const std::string& name)
  {
    return new OpenDDS::DCPS::DummyTcpInst(name);
  }
};

int
DCPS_DummyTcpLoader::init (int argc, ACE_TCHAR* argv[])
{
  DBG_ENTRY_LVL("DCPS_DummyTcpLoader","init",1);

  static int initialized = 0;

  // Only allow initialization once.
  if (initialized)
    return 0;

  using namespace OpenDDS::DCPS;

  TransportRegistry* registry = TheTransportRegistry;
  registry->register_type(new DummyTcpType);
  TransportInst_rch default_inst =
    registry->create_inst(TransportRegistry::DEFAULT_INST_PREFIX +
                          "9999_DUMMY_TCP", "dummy_tcp");
  registry->get_config(TransportRegistry::DEFAULT_CONFIG_NAME)
    ->sorted_insert(default_inst);

  initialized = 1;
  return 0;
}

/////////////////////////////////////////////////////////////////////

ACE_FACTORY_DEFINE (DummyTcp, DCPS_DummyTcpLoader)
  ACE_STATIC_SVC_DEFINE (DCPS_DummyTcpLoader,
                         ACE_TEXT ("DCPS_DummyTcpLoader"),
                         ACE_SVC_OBJ_T,
                         &ACE_SVC_NAME (DCPS_DummyTcpLoader),
                         ACE_Service_Type::DELETE_THIS
                         | ACE_Service_Type::DELETE_OBJ,
                         0)
