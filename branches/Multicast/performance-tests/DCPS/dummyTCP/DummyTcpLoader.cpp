// $Id$
#include "DummyTcp_pch.h"

#include "DummyTcpLoader.h"
#include "DummyTcpGenerator.h"
#include "dds/DCPS/transport/framework/TheTransportFactory.h"
#include "dds/DCPS/transport/framework/EntryExit.h"

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

int
DCPS_DummyTcpLoader::init (int argc, ACE_TCHAR* argv[])
{
  DBG_ENTRY_LVL("DCPS_DummyTcpLoader","init",1);

  static int initialized = 0;

  // Only allow initialization once.
  if (initialized)
    return 0;

  initialized = 1;

  // Parse any service configurator parameters.
  for (int curarg = 0; curarg < argc; curarg++)
    if (ACE_OS::strcasecmp (argv[curarg],
                            ACE_TEXT("-type")) == 0)
      {
        curarg++;
        if (curarg < argc)
          {
            ACE_TCHAR* type = argv[curarg];

            if (ACE_OS::strcasecmp (type, ACE_TEXT("DummyTcp")) != 0)
              {
                ACE_ERROR_RETURN ((LM_ERROR,
                  ACE_TEXT("ERROR: DCPS_DummyTcpLoader: Unknown type ")
                  ACE_TEXT("<%s>.\n"), type),
                  -1);
              }

            OpenDDS::DCPS::DummyTcpGenerator* generator;
            ACE_NEW_RETURN (generator,
                            OpenDDS::DCPS::DummyTcpGenerator (),
                            -1);
            TheTransportFactory->register_generator (type,
                                                     generator);
          }
      }
    else
      {
        VDBG_LVL ((LM_ERROR,
                   ACE_TEXT("DCPS_DummyTcpLoader: Unknown option ")
                   ACE_TEXT("<%s>.\n"),
                   argv[curarg]), 1);
      }

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
