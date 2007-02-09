// $Id$
#include  "DCPS/DdsDcps_pch.h"

#include "SimpleTcp.h"
#include "SimpleTcpLoader.h"
#include "tao/debug.h"
#include "ace/Dynamic_Service.h"

DCPS_SimpleTcp_Initializer::DCPS_SimpleTcp_Initializer (void)
{
  ACE_Service_Config::process_directive (ace_svc_desc_DCPS_SimpleTcpLoader);
  //ACE_Service_Config::open (ace_svc_desc_DCPS_SimpleTcpLoader);
}
