/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "MulticastFactory.h"
#include "MulticastTransport.h"

namespace OpenDDS {
namespace DCPS {

MulticastFactory::~MulticastFactory()
{
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
