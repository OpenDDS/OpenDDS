/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "SimpleUnreliableDgram_pch.h"
#include "SimpleUnreliableDgramLoader.h"
#include "SimpleUdpGenerator.h"
#include "SimpleMcastGenerator.h"
#include "dds/DCPS/transport/framework/TheTransportFactory.h"

#include "tao/debug.h"
#include "ace/OS_NS_strings.h"

OPENDDS_DCPS_SimpleUnreliableDgramLoader::OPENDDS_DCPS_SimpleUnreliableDgramLoader()
{
}

OPENDDS_DCPS_SimpleUnreliableDgramLoader::~OPENDDS_DCPS_SimpleUnreliableDgramLoader()
{
}

int
OPENDDS_DCPS_SimpleUnreliableDgramLoader::init(int argc,
                                               ACE_TCHAR* argv[])
{
  ACE_TRACE("OPENDDS_DCPS_SimpleUnreliableDgramLoader::init");

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
        OpenDDS::DCPS::TransportGenerator* generator = 0;

        if (ACE_OS::strcasecmp(type, ACE_TEXT("SimpleUdp")) == 0) {
          ACE_NEW_RETURN(generator,
                         OpenDDS::DCPS::SimpleUdpGenerator(),
                         -1);

        } else if (ACE_OS::strcasecmp(type, ACE_TEXT("SimpleMcast")) == 0) {
          ACE_NEW_RETURN(generator,
                         OpenDDS::DCPS::SimpleMcastGenerator(),
                         -1);

        } else {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("ERROR: OPENDDS_DCPS_SimpleUnreliableDgramLoader: Unknown type ")
                            ACE_TEXT("<%s>.\n"), type),
                           -1);
        }

        TheTransportFactory->register_generator(type,
                                                generator);
      }

    } else {
      if (TAO_debug_level > 0) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("OPENDDS_DCPS_SimpleUnreliableDgramLoader: Unknown option ")
                   ACE_TEXT("<%s>.\n"),
                   argv[curarg]));
      }
    }

  return 0;
}

/////////////////////////////////////////////////////////////////////

ACE_FACTORY_DEFINE(SimpleUnreliableDgram, OPENDDS_DCPS_SimpleUnreliableDgramLoader)
ACE_STATIC_SVC_DEFINE(OPENDDS_DCPS_SimpleUnreliableDgramLoader,
                      ACE_TEXT("OPENDDS_DCPS_SimpleUnreliableDgramLoader"),
                      ACE_SVC_OBJ_T,
                      &ACE_SVC_NAME(OPENDDS_DCPS_SimpleUnreliableDgramLoader),
                      ACE_Service_Type::DELETE_THIS
                      | ACE_Service_Type::DELETE_OBJ,
                      0)
