/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "MulticastGenerator.h"
#include "MulticastFactory.h"
#include "MulticastInst.h"

#include "dds/DCPS/transport/framework/TransportDefs.h"

#include <sstream>

namespace OpenDDS {
namespace DCPS {

TransportImplFactory*
MulticastGenerator::new_factory()
{
  TransportImplFactory* factory;
  ACE_NEW_RETURN(factory, MulticastFactory, 0);
  return factory;
}

TransportInst*
MulticastGenerator::new_configuration(const TransportIdType id)
{
  std::ostringstream name;
  name << id;
  TransportInst* configuration;
  ACE_NEW_RETURN(configuration, MulticastInst(name.str()), 0);
  return configuration;
}

void
MulticastGenerator::default_transport_ids(TransportIdList& ids)
{
  ids.clear();
  ids.push_back(DEFAULT_MULTICAST_ID);
}

} // namespace DCPS
} // namespace OpenDDS
