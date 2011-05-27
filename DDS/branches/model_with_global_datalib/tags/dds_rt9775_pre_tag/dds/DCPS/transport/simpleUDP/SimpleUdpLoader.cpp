// $Id$
#include "SimpleUdp_pch.h"
#include "SimpleUdpLoader.h"
#include "SimpleUdpGenerator.h"
#include "dds/DCPS/transport/framework/TheTransportFactory.h"

#include "tao/debug.h"
#include "ace/OS_NS_strings.h"

TAO_DCPS_SimpleUdpLoader::TAO_DCPS_SimpleUdpLoader (void)
{
}

TAO_DCPS_SimpleUdpLoader::~TAO_DCPS_SimpleUdpLoader (void)
{
}

int
TAO_DCPS_SimpleUdpLoader::init (int argc,
                       ACE_TCHAR* argv[])
{
  ACE_TRACE ("TAO_DCPS_SimpleUdpLoader::init");

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
            
            TAO::DCPS::SimpleUdpGenerator* generator;
            ACE_NEW_RETURN (generator,
                            TAO::DCPS::SimpleUdpGenerator (),
                            -1);
            TheTransportFactory->register_generator (TAO::DCPS::DEFAULT_SIMPLE_UDP_ID,
                                                     type, 
                                                     generator); 
          }
      }
    else
      {
        if (TAO_debug_level > 0)
          {
            ACE_DEBUG ((LM_ERROR,
                        ACE_TEXT("TAO_DCPS_SimpleUdpLoader: Unknown option ")
                        ACE_TEXT("<%s>.\n"),
                        argv[curarg]));
          }
      }

  return 0;
}

/////////////////////////////////////////////////////////////////////

ACE_FACTORY_DEFINE (SimpleUdp, TAO_DCPS_SimpleUdpLoader)
ACE_STATIC_SVC_DEFINE (TAO_DCPS_SimpleUdpLoader,
                       ACE_TEXT ("DCPS_SimpleUdpLoader"),
                       ACE_SVC_OBJ_T,
                       &ACE_SVC_NAME (TAO_DCPS_SimpleUdpLoader),
                       ACE_Service_Type::DELETE_THIS
                       | ACE_Service_Type::DELETE_OBJ,
                       0)

