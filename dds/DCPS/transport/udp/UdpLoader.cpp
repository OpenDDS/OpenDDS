/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "UdpLoader.h"
#include "UdpGenerator.h"
#include "UdpInst.h"

#include "dds/DCPS/transport/framework/TheTransportFactory.h"
#include "dds/DCPS/transport/framework/TransportType.h"

namespace OpenDDS {
namespace DCPS {

class UdpType : public TransportType {
public:
  const char* name() { return "udp"; }

  TransportInst* new_inst(const std::string& name)
  {
    return new UdpInst(name);
  }

  static TransportInst* new_default()
  {
    return new UdpInst(TransportRegistry::DEFAULT_INST_PREFIX + "0300_UDP");
  }
};

int
UdpLoader::init(int /*argc*/, ACE_TCHAR* /*argv*/[])
{
  static bool initialized(false);

  if (initialized) return 0;  // already initialized

  TransportRegistry* registry = TheTransportRegistry;
  registry->register_type(new UdpType);
  registry->get_config(TransportRegistry::DEFAULT_CONFIG_NAME)
    ->sorted_insert(UdpType::new_default());

  TheTransportFactory->register_generator(ACE_TEXT("udp"), new UdpGenerator);

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
