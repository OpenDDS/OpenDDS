/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "MulticastFactory.h"
#include "MulticastTransport.h"

namespace OpenDDS {
namespace DCPS {

int
MulticastFactory::requires_reactor() const
{
  return 1;  // require reactor
}

TransportImpl*
MulticastFactory::create()
{
  TransportImpl* transport_impl;
  ACE_NEW_RETURN(transport_impl, MulticastTransport, 0);
  return transport_impl;
}

} // namespace DCPS
} // namespace OpenDDS
