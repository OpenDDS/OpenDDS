/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "MulticastLoader.h"
#include "MulticastGenerator.h"
#include "MulticastInst.h"

#include "dds/DCPS/transport/framework/TheTransportFactory.h"
#include "dds/DCPS/transport/framework/TransportType.h"

namespace OpenDDS {
namespace DCPS {

class MulticastType : public TransportType {
public:
  const char* name() { return "multicast"; }
  bool requires_reactor() { return true; }

  TransportInst* new_inst(const std::string& name)
  {
    return new MulticastInst(name);
  }
};

int
MulticastLoader::init(int /*argc*/, ACE_TCHAR* /*argv*/[])
{
  static bool initialized(false);

  if (initialized) return 0;  // already initialized

  TheTransportRegistry->register_type(new MulticastType);

  TheTransportFactory->register_generator(ACE_TEXT("multicast"),
                                          new MulticastGenerator);
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
