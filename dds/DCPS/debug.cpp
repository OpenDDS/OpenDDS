// -*- C++ -*-
//
// $Id$
#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include "debug.h"

TAO_DdsDcps_Export unsigned int TAO::DCPS::DCPS_debug_level = 0;

namespace TAO {
    namespace DCPS {

TAO_DdsDcps_Export void set_DCPS_debug_level(unsigned int lvl)
{
  TAO::DCPS::DCPS_debug_level = lvl;
}

    }; // DCPS
}; // TAO
