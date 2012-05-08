/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "ShmemInst.h"
#include "ShmemLoader.h"

#include "ace/Configuration.h"

#include <iostream>

namespace OpenDDS {
namespace DCPS {

ShmemInst::ShmemInst(const std::string& name)
  : TransportInst("shmem", name)
{
}

ShmemTransport*
ShmemInst::new_impl(const TransportInst_rch& inst)
{
  return new ShmemTransport(inst);
}

int
ShmemInst::load(ACE_Configuration_Heap& cf,
                ACE_Configuration_Section_Key& sect)
{
  TransportInst::load(cf, sect);
  //TODO: load specific config params
  return 0;
}

void
ShmemInst::dump(std::ostream& os)
{
  TransportInst::dump(os);
  //TODO: dump specific config params
}

} // namespace DCPS
} // namespace OpenDDS
