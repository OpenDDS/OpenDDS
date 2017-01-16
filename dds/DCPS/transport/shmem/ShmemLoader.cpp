/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "ShmemLoader.h"
#include "ShmemInst.h"

#include "dds/DCPS/transport/framework/TransportRegistry.h"
#include "dds/DCPS/transport/framework/TransportType.h"

namespace {
  const char SHMEM_NAME[] = "shmem";
}

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

class ShmemType : public TransportType {
public:
  const char* name() { return SHMEM_NAME; }

  TransportInst_rch new_inst(const std::string& name)
  {
    return make_rch<ShmemInst>(name);
  }
};

int
ShmemLoader::init(int /*argc*/, ACE_TCHAR* /*argv*/[])
{
  static bool initialized(false);

  if (initialized) return 0;  // already initialized

  TransportRegistry* registry = TheTransportRegistry;
  registry->register_type(make_rch<ShmemType>());

  //FUTURE: when we're ready, add a ShmemInst to the default config, like so:
  /*
  TransportInst_rch default_inst =
    registry->create_inst(TransportRegistry::DEFAULT_INST_PREFIX + "0200_SHMEM",
                          UDP_NAME);
  registry->get_config(TransportRegistry::DEFAULT_CONFIG_NAME)
    ->sorted_insert(default_inst);
  */
  initialized = true;

  return 0;
}

ACE_FACTORY_DEFINE(OpenDDS_Shmem, ShmemLoader);
ACE_STATIC_SVC_DEFINE(
  ShmemLoader,
  ACE_TEXT("OpenDDS_Shmem"),
  ACE_SVC_OBJ_T,
  &ACE_SVC_NAME(ShmemLoader),
  ACE_Service_Type::DELETE_THIS | ACE_Service_Type::DELETE_OBJ,
  0)

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
