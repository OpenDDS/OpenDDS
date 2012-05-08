/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "ShmemSendStrategy.h"
#include "ShmemDataLink.h"
#include "ShmemInst.h"

#include "dds/DCPS/transport/framework/NullSynchStrategy.h"

namespace OpenDDS {
namespace DCPS {

ShmemSendStrategy::ShmemSendStrategy(ShmemDataLink* link)
  : TransportSendStrategy(TransportInst_rch(link->config(), false),
                          0,  // synch_resource
                          link->transport_priority(),
                          new NullSynchStrategy)
  , link_(link)
{
}

ssize_t
ShmemSendStrategy::send_bytes_i(const iovec iov[], int n)
{
  //TODO
  return 0;
}

void
ShmemSendStrategy::stop_i()
{
}

} // namespace DCPS
} // namespace OpenDDS
