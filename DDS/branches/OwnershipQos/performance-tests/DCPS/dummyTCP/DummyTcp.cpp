// $Id$
#include "DummyTcp_pch.h"
#include "DummyTcp.h"
#include "DummyTcp_export.h"
#include "DummyTcpLoader.h"
#include "tao/debug.h"
#include "ace/Dynamic_Service.h"

DCPS_DummyTcp_Initializer::DCPS_DummyTcp_Initializer (void)
{
  ACE_Service_Config::process_directive (ace_svc_desc_DCPS_DummyTcpLoader);

#if DUMMYTCP_HAS_DLL == 0
  ACE_Service_Config::process_directive (ACE_TEXT ("static "
                                                   "DCPS_DummyTcpLoader "
                                                   "\"-type DummyTcp\""));
#endif
}
