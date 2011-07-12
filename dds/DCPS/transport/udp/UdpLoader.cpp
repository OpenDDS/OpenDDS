/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "UdpLoader.h"
#include "UdpInst.h"

#include "dds/DCPS/transport/framework/TheTransportFactory.h"
#include "dds/DCPS/transport/framework/TransportType.h"

namespace {
  const char UDP_NAME[] = "udp";
}

namespace OpenDDS {
namespace DCPS {

class UdpType : public TransportType {
public:
  const char* name() { return UDP_NAME; }

  TransportInst* new_inst(const std::string& name)
  {
    return new UdpInst(name);
  }
};

int
UdpLoader::init(int /*argc*/, ACE_TCHAR* /*argv*/[])
{
  static bool initialized(false);

  if (initialized) return 0;  // already initialized

  TransportRegistry* registry = TheTransportRegistry;
  registry->register_type(new UdpType);
  TransportInst_rch default_inst =
    registry->create_inst(TransportRegistry::DEFAULT_INST_PREFIX + "0300_UDP",
                          UDP_NAME);
  registry->get_config(TransportRegistry::DEFAULT_CONFIG_NAME)
    ->sorted_insert(default_inst);

  initialized = true;

  return 0;
}

ACE_FACTORY_DEFINE(OpenDDS_Udp, UdpLoader);
ACE_STATIC_SVC_DEFINE(
  UdpLoader,
  ACE_TEXT("OpenDDS_Udp"),
  ACE_SVC_OBJ_T,
  &ACE_SVC_NAME(UdpLoader),
  ACE_Service_Type::DELETE_THIS | ACE_Service_Type::DELETE_OBJ,
  0)

} // namespace DCPS
} // namespace OpenDDS
