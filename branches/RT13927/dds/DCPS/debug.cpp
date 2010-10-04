/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include "debug.h"

OpenDDS_Dcps_Export unsigned int OpenDDS::DCPS::DCPS_debug_level = 0;

namespace OpenDDS {
namespace DCPS {

OpenDDS_Dcps_Export void set_DCPS_debug_level(unsigned int lvl)
{
  OpenDDS::DCPS::DCPS_debug_level = lvl;
}

}
}
