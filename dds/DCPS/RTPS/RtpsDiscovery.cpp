/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "RtpsDiscovery.h"
#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/InfoRepoUtils.h"
#include "dds/DCPS/ConfigUtils.h"

namespace OpenDDS {
namespace RTPS {

RtpsDiscovery::RtpsDiscovery(const RepoKey& key)
  : DCPS::Discovery(key)
{
}

DCPS::DCPSInfo_ptr RtpsDiscovery::get_dcps_info()
{
  // TODO: Write an RTPS DCPSInfo servant, incarnate a collocated
  // instance of it, and return an object reference to it.
  return DCPS::DCPSInfo::_nil();
}

static const ACE_TCHAR RTPS_SECTION_NAME[]   = ACE_TEXT("rtps_discovery");

int RtpsDiscovery::load_rtps_discovery_configuration(ACE_Configuration_Heap& cf)
{
  const ACE_Configuration_Section_Key &root = cf.root_section();
  ACE_Configuration_Section_Key rtps_sect;

  if (cf.open_section(root, RTPS_SECTION_NAME, 0, rtps_sect) == 0) {

    // Ensure there are no properties in this section
    DCPS::ValueMap vm;
    if (DCPS::pullValues(cf, rtps_sect, vm) > 0) {
      // There are values inside [rtps_discovery]
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) RtpsDiscovery::load_rtps_discovery_configuration(): ")
                        ACE_TEXT("rtps_discovery sections must have a subsection name\n")),
                       -1);
    }
    // Process the subsections of this section (the individual rtps_discovery/*)
    DCPS::KeyList keys;
    if (DCPS::processSections( cf, rtps_sect, keys ) != 0) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) RtpsDiscovery::load_rtps_discovery_configuration(): ")
                        ACE_TEXT("too many nesting layers in the [rtps] section.\n")),
                       -1);
    }

    // Loop through the [rtps/*] sections
    for (DCPS::KeyList::const_iterator it=keys.begin(); it != keys.end(); ++it) {
      std::string rtps_name = it->first;

      DCPS::ValueMap values;
      DCPS::pullValues( cf, it->second, values );
      for (DCPS::ValueMap::const_iterator it=values.begin(); it != values.end(); ++it) {
        std::string name = it->first;
        if (name == "???") {
          // TODO: Implement other RTPS Discovery config parameters (including those
          // required by the spec.
          std::string value = it->second;

        } else {
          ACE_ERROR_RETURN((LM_ERROR,
                            ACE_TEXT("(%P|%t) RtpsDiscovery::load_rtps_discovery_configuration(): ")
                            ACE_TEXT("Unexpected entry (%s) in [rtps_discovery/%s] section.\n"),
                            name.c_str(), rtps_name.c_str()),
                           -1);
        }
      }

      RtpsDiscovery_rch discovery = new RtpsDiscovery(rtps_name);
      TheServiceParticipant->add_discovery(
        DCPS::dynamic_rchandle_cast<Discovery>(discovery));
    }
  }

  // If the default RTPS discovery object has not been configured,
  // instantiate it now.
  const DCPS::Service_Participant::RepoKeyDiscoveryMap& discoveryMap = TheServiceParticipant->discoveryMap();
  if (discoveryMap.find(Discovery::DEFAULT_RTPS) == discoveryMap.end()) {
    RtpsDiscovery_rch discovery = new RtpsDiscovery(Discovery::DEFAULT_RTPS);
    TheServiceParticipant->add_discovery(
      DCPS::dynamic_rchandle_cast<Discovery>(discovery));
  }

  return 0;
}

RtpsDiscovery::StaticInitializer RtpsDiscovery::dummy_;


} // namespace DCPS
} // namespace OpenDDS
