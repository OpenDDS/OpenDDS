// $Id$
#include "SimpleUnreliableDgram_pch.h"

#include "SimpleUnreliableDgram.h"
#include "SimpleUnreliableDgramLoader.h"
#include "ace/Dynamic_Service.h"

OPENDDS_DCPS_SimpleUnreliableDgram_Initializer::OPENDDS_DCPS_SimpleUnreliableDgram_Initializer (void)
{
  ACE_Service_Config::process_directive (ace_svc_desc_OPENDDS_DCPS_SimpleUnreliableDgramLoader);

#if SIMPLEUNRELIABLEDGRAM_HAS_DLL == 0
  ACE_Service_Config::process_directive (
    ACE_TEXT ("static OPENDDS_DCPS_SimpleUnreliableDgramLoader \"-type SimpleUdp "
              "-type SimpleMcast\""));
#endif
}

