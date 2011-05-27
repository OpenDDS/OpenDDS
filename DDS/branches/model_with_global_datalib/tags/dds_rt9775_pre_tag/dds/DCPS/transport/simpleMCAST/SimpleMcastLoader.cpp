// $Id$
#include "SimpleMcast_pch.h"
#include "SimpleMcastLoader.h"
#include "SimpleMcastGenerator.h"
#include "dds/DCPS/transport/framework/TheTransportFactory.h"

#include "tao/debug.h"
#include "ace/OS_NS_strings.h"

TAO_DCPS_SimpleMcastLoader::TAO_DCPS_SimpleMcastLoader (void)
{
}

TAO_DCPS_SimpleMcastLoader::~TAO_DCPS_SimpleMcastLoader (void)
{
}

int
TAO_DCPS_SimpleMcastLoader::init (int argc,
                       ACE_TCHAR* argv[])
{
  ACE_TRACE ("TAO_DCPS_SimpleMcastLoader::init");

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
            
            TAO::DCPS::SimpleMcastGenerator* generator;
            ACE_NEW_RETURN (generator,
                            TAO::DCPS::SimpleMcastGenerator (),
                            -1);
            TheTransportFactory->register_generator (TAO::DCPS::DEFAULT_SIMPLE_MCAST_ID,
                                                     type,
                                                     generator);
          }
      }
    else
      {
        if (TAO_debug_level > 0)
          {
            ACE_DEBUG ((LM_ERROR,
                        ACE_TEXT("TAO_DCPS_SimpleMcastLoader: Unknown option ")
                        ACE_TEXT("<%s>.\n"),
                        argv[curarg]));
          }
      }

  return 0;
}

/////////////////////////////////////////////////////////////////////

ACE_FACTORY_DEFINE (SimpleMcast, TAO_DCPS_SimpleMcastLoader)
ACE_STATIC_SVC_DEFINE (TAO_DCPS_SimpleMcastLoader,
                       ACE_TEXT ("DCPS_SimpleMcastLoader"),
                       ACE_SVC_OBJ_T,
                       &ACE_SVC_NAME (TAO_DCPS_SimpleMcastLoader),
                       ACE_Service_Type::DELETE_THIS
                       | ACE_Service_Type::DELETE_OBJ,
                       0)

