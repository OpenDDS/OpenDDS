// $Id$
#include "SimpleUnreliableDgram_pch.h"

#include "SimpleUnreliableDgram.h"
#include "SimpleUnreliableDgramLoader.h"
#include "ace/Dynamic_Service.h"

TAO_DCPS_SimpleUnreliableDgram_Initializer::TAO_DCPS_SimpleUnreliableDgram_Initializer (void)
{
  ACE_Service_Config::process_directive (ace_svc_desc_TAO_DCPS_SimpleUnreliableDgramLoader);
}

