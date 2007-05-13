// $Id$
#include "SimpleTcp_pch.h"
#include "SimpleTcp.h"
#include "SimpleTcpLoader.h"
#include "tao/debug.h"
#include "ace/Dynamic_Service.h"

DCPS_SimpleTcp_Initializer::DCPS_SimpleTcp_Initializer (void)
{
  ACE_Service_Config::process_directive (ace_svc_desc_DCPS_SimpleTcpLoader);

  //This was added for static builds but is causing problems from non-static
  //builds.  Disabling for now.  The static build may need to use -ORBSvcConf
  //with a static directive to work-around.
  //ACE_Service_Config::process_directive (ACE_TEXT ("static "
  //                                                 "DCPS_SimpleTcpLoader "
  //                                                 "\"-type SimpleTcp\""));
}
