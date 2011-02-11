/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "SimpleTcp_pch.h"

#include "SimpleTcpLoader.h"
#include "SimpleTcpGenerator.h"
#include "dds/DCPS/transport/framework/TheTransportFactory.h"
#include "dds/DCPS/transport/framework/EntryExit.h"

#include "tao/debug.h"
#include "ace/OS_NS_strings.h"

DCPS_SimpleTcpLoader::DCPS_SimpleTcpLoader()
{
  DBG_ENTRY_LVL("DCPS_SimpleTcpLoader","DCPS_SimpleTcpLoader",6);
}

DCPS_SimpleTcpLoader::~DCPS_SimpleTcpLoader()
{
  DBG_ENTRY_LVL("DCPS_SimpleTcpLoader","~DCPS_SimpleTcpLoader",6);
}

int
DCPS_SimpleTcpLoader::init(int argc, ACE_TCHAR* argv[])
{
  DBG_ENTRY_LVL("DCPS_SimpleTcpLoader","init",6);

  static int initialized = 0;

  // Only allow initialization once.
  if (initialized)
    return 0;

  initialized = 1;

  // Parse any service configurator parameters.
  for (int curarg = 0; curarg < argc; curarg++)
    if (ACE_OS::strcasecmp(argv[curarg],
                           ACE_TEXT("-type")) == 0) {
      curarg++;

      if (curarg < argc) {
        ACE_TCHAR* type = argv[curarg];

        if (ACE_OS::strcasecmp(type, ACE_TEXT("SimpleTcp")) != 0) {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("ERROR: DCPS_SimpleTcpLoader: Unknown type ")
                            ACE_TEXT("<%s>.\n"), type),
                           -1);
        }

        OpenDDS::DCPS::SimpleTcpGenerator* generator;
        ACE_NEW_RETURN(generator,
                       OpenDDS::DCPS::SimpleTcpGenerator(),
                       -1);
        TheTransportFactory->register_generator(type,
                                                generator);
      }

    } else {
      VDBG_LVL((LM_ERROR,
                ACE_TEXT("DCPS_SimpleTcpLoader: Unknown option ")
                ACE_TEXT("<%s>.\n"),
                argv[curarg]), 1);
    }

  return 0;
}

/////////////////////////////////////////////////////////////////////

ACE_FACTORY_DEFINE(SimpleTcp, DCPS_SimpleTcpLoader)
ACE_STATIC_SVC_DEFINE(DCPS_SimpleTcpLoader,
                      ACE_TEXT("DCPS_SimpleTcpLoader"),
                      ACE_SVC_OBJ_T,
                      &ACE_SVC_NAME(DCPS_SimpleTcpLoader),
                      ACE_Service_Type::DELETE_THIS
                      | ACE_Service_Type::DELETE_OBJ,
                      0)
