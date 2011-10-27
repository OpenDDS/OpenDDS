/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "InfoRepoDiscovery.h"
#include "Service_Participant.h"
#include "InfoRepoUtils.h"

namespace OpenDDS {
namespace DCPS {

InfoRepoDiscovery::InfoRepoDiscovery(RepoKey      key,
                                     std::string  ior)
  : Discovery(key),
    ior_(ior),
    bit_transport_port_(0)
{
}

DCPSInfo_ptr InfoRepoDiscovery::get_dcps_info()
{
  if (CORBA::is_nil(this->info_.in())) {
    CORBA::ORB_var orb = TheServiceParticipant->get_ORB();
    try {
      this->info_ = InfoRepoUtils::get_repo(this->ior_.c_str(), orb.in());

      if (CORBA::is_nil(this->info_.in())) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("(%P|%t) ERROR: InfoRepoDiscovery::get_repository: ")
                   ACE_TEXT("unable to narrow DCPSInfo (%C) for key %C. \n"),
                   this->ior_.c_str(),
                   this->key().c_str()));
        return OpenDDS::DCPS::DCPSInfo::_nil();
      }

    } catch (const CORBA::Exception& ex) {
      ex._tao_print_exception(
                              "ERROR: InfoRepoDiscovery::get_repository: failed to resolve ior - ");
      return OpenDDS::DCPS::DCPSInfo::_nil();
    }
  }

  return OpenDDS::DCPS::DCPSInfo::_duplicate(this->info_);
}

} // namespace DCPS
} // namespace OpenDDS
