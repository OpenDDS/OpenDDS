/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "MulticastLoader.h"
#include "MulticastInst.h"

#include "dds/DCPS/transport/framework/TransportRegistry.h"
#include "dds/DCPS/transport/framework/TransportType.h"

namespace {
  const char MULTICAST_NAME[] = "multicast";
}

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class MulticastType : public TransportType {
public:
  const char* name() { return MULTICAST_NAME; }

  TransportInst_rch new_inst(const std::string& name)
  {
    return make_rch<MulticastInst>(name);
  }
};

int
MulticastLoader::init(int /*argc*/, ACE_TCHAR* /*argv*/[])
{
  static bool initialized(false);

  if (initialized) return 0;  // already initialized

  TransportRegistry* registry = TheTransportRegistry;
  registry->register_type(make_rch<MulticastType>());
  TransportConfig_rch cfg =
    registry->get_config(TransportRegistry::DEFAULT_CONFIG_NAME);

  TransportInst_rch default_unrel =
    registry->create_inst(TransportRegistry::DEFAULT_INST_PREFIX
                          + std::string("0410_MCAST_UNRELIABLE"),
                          MULTICAST_NAME);
  MulticastInst* mi = dynamic_cast<MulticastInst*>(default_unrel.in());
  mi->reliable_ = false;
  cfg->sorted_insert(default_unrel);

  TransportInst_rch default_rel =
    registry->create_inst(TransportRegistry::DEFAULT_INST_PREFIX
                          + std::string("0420_MCAST_RELIABLE"), MULTICAST_NAME);
  cfg->sorted_insert(default_rel);

  initialized = true;

  return 0;
}

ACE_FACTORY_DEFINE(OpenDDS_Multicast, MulticastLoader)
ACE_STATIC_SVC_DEFINE(
  MulticastLoader,
  ACE_TEXT("OpenDDS_Multicast"),
  ACE_SVC_OBJ_T,
  &ACE_SVC_NAME(MulticastLoader),
  ACE_Service_Type::DELETE_THIS | ACE_Service_Type::DELETE_OBJ,
  0)

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
