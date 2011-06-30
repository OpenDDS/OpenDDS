/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "UdpGenerator.h"
#include "UdpFactory.h"
#include "UdpInst.h"

#include "dds/DCPS/transport/framework/TransportDefs.h"

#include <sstream>

namespace OpenDDS {
namespace DCPS {

TransportImplFactory*
UdpGenerator::new_factory()
{
  TransportImplFactory* factory;
  ACE_NEW_RETURN(factory, UdpFactory, 0);
  return factory;
}

TransportInst*
UdpGenerator::new_configuration(const TransportIdType id)
{
  std::ostringstream name;
  name << id;
  TransportInst* configuration;
  ACE_NEW_RETURN(configuration, UdpInst(name.str()), 0);
  return configuration;
}

void
UdpGenerator::default_transport_ids(TransportIdList& ids)
{
  ids.clear();
  ids.push_back(DEFAULT_UDP_ID);
}

} // namespace DCPS
} // namespace OpenDDS
