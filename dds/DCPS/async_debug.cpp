/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/

#include "async_debug.h"

OpenDDS_Dcps_Export unsigned int OpenDDS::DCPS::ASYNC_debug = 0;

namespace OpenDDS {
namespace DCPS {

OpenDDS_Dcps_Export void set_ASYNC_debug(unsigned int lvl)
{
  OpenDDS::DCPS::ASYNC_debug = lvl;
}

} // namespace OpenDDS
} // namespace DCPS
