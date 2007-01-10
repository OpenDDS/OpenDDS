// $Id$
#include "SimpleMcast_pch.h"

#include "SimpleMcast.h"
#include "SimpleMcastLoader.h"
#include "tao/debug.h"
#include "ace/Dynamic_Service.h"

TAO_DCPS_SimpleMcast_Initializer::TAO_DCPS_SimpleMcast_Initializer (void)
{
  ACE_Service_Config::process_directive (ace_svc_desc_TAO_DCPS_SimpleMcastLoader);
}

