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
  const std::string Discovery::DEFAULT_RTPS = "DEFAULT_RTPS";

  std::string Discovery::get_stringified_dcps_info_ior()
  {
    // Get the actual reference and generate the stringified IOR
    CORBA::ORB_var orb = TheServiceParticipant->get_ORB();
    DCPSInfo_var repo = this->get_dcps_info();

    // TODO: Remove this check when the RtpsDiscovery has a real
    // DCPSInfo object reference to return
    if (CORBA::is_nil(repo)) {
      return "";
    }

    CORBA::String_var retval = orb->object_to_string(repo);
    return std::string(retval.in());
  }

} // namespace DCPS
} // namespace OpenDDS
