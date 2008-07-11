// $Id$
#include "SimpleTcp_pch.h"
#include "SimpleTcp.h"
#include "SimpleTcp_export.h"
#include "SimpleTcpLoader.h"
#include "tao/debug.h"
#include "ace/Dynamic_Service.h"

DCPS_SimpleTcp_Initializer::DCPS_SimpleTcp_Initializer (void)
{
  ACE_Service_Config::process_directive (ace_svc_desc_DCPS_SimpleTcpLoader);

#if SIMPLETCP_HAS_DLL == 0
  ACE_Service_Config::process_directive (ACE_TEXT ("static "
                                                   "DCPS_SimpleTcpLoader "
                                                   "\"-type SimpleTcp\""));
#endif
}
