/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "Discovery.h"
#include "Service_Participant.h"

namespace OpenDDS {
namespace DCPS {

  const std::string Discovery::DEFAULT_REPO = "DEFAULT_REPO";

  std::string Discovery::get_stringified_dcps_info_ior()
  {
    // Get the actual reference and generate the stringified IOR
    CORBA::ORB_var orb = TheServiceParticipant->get_ORB();
    DCPSInfo_var repo = this->get_dcps_info();
    CORBA::String_var callback = orb->object_to_string(repo);
    return std::string(callback.in());
  }

} // namespace DCPS
} // namespace OpenDDS
