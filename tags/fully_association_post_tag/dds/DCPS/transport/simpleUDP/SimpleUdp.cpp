// $Id$
#include "SimpleUdp_pch.h"

#include "SimpleUdp.h"
#include "SimpleUdpLoader.h"
#include "tao/debug.h"
#include "ace/Dynamic_Service.h"

TAO_DCPS_SimpleUdp_Initializer::TAO_DCPS_SimpleUdp_Initializer (void)
{
  ACE_Service_Config::process_directive (ace_svc_desc_TAO_DCPS_SimpleUdpLoader);
}

