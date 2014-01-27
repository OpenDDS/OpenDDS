/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "RtpsUdpLoader.h"
#include "RtpsUdpInst.h"

#include "dds/DCPS/transport/framework/TransportRegistry.h"
#include "dds/DCPS/transport/framework/TransportType.h"

namespace {
  const char RTPS_UDP_NAME[] = "rtps_udp";
}

namespace OpenDDS {
namespace DCPS {

class RtpsUdpType : public TransportType {
public:
  const char* name() { return RTPS_UDP_NAME; }

  TransportInst* new_inst(const std::string& name)
  {
    return new RtpsUdpInst(name);
  }
};

int
RtpsUdpLoader::init(int /*argc*/, ACE_TCHAR* /*argv*/[])
{
  static bool initialized(false);

  if (initialized) return 0;  // already initialized

  TransportRegistry* registry = TheTransportRegistry;
  registry->register_type(new RtpsUdpType);
  /*  Don't create a default for RTPS.  At least for the initial implementation,
      the user needs to explicitly configure it.
  TransportInst_rch default_inst =
    registry->create_inst(TransportRegistry::DEFAULT_INST_PREFIX +
                          "0600_RTPS_UDP",
                          RTPS_UDP_NAME);
  registry->get_config(TransportRegistry::DEFAULT_CONFIG_NAME)
    ->sorted_insert(default_inst);
  */
  initialized = true;

  return 0;
}

ACE_FACTORY_DEFINE(OpenDDS_Rtps_Udp, RtpsUdpLoader);
ACE_STATIC_SVC_DEFINE(
  RtpsUdpLoader,
  ACE_TEXT("OpenDDS_Rtps_Udp"),
  ACE_SVC_OBJ_T,
  &ACE_SVC_NAME(RtpsUdpLoader),
  ACE_Service_Type::DELETE_THIS | ACE_Service_Type::DELETE_OBJ,
  0)

} // namespace DCPS
} // namespace OpenDDS
