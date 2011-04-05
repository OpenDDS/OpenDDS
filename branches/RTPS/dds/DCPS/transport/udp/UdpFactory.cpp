/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "UdpFactory.h"
#include "UdpTransport.h"

namespace OpenDDS {
namespace DCPS {

int
UdpFactory::requires_reactor() const
{
  return 1;  // require reactor
}

TransportImpl*
UdpFactory::create()
{
  TransportImpl* transport_impl;
  ACE_NEW_RETURN(transport_impl, UdpTransport, 0);
  return transport_impl;
}

} // namespace DCPS
} // namespace OpenDDS
