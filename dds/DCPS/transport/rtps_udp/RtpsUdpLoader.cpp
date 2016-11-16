/*
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

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class RtpsUdpType : public TransportType {
public:
  const char* name() { return RTPS_UDP_NAME; }

  TransportInst* new_inst(const OPENDDS_STRING& name)
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
  // Don't create a default for RTPS.  At least for the initial implementation,
  // the user needs to explicitly configure it...
#ifdef OPENDDS_SAFETY_PROFILE
  // ...except for Safety Profile where RTPS is the only option.
  TransportInst_rch default_inst =
    registry->create_inst(TransportRegistry::DEFAULT_INST_PREFIX +
                          OPENDDS_STRING("0600_RTPS_UDP"),
                          RTPS_UDP_NAME);
  registry->get_config(TransportRegistry::DEFAULT_CONFIG_NAME)
    ->sorted_insert(default_inst);
#endif
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

OPENDDS_END_VERSIONED_NAMESPACE_DECL
