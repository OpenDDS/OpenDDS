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
  , pool_size_(16 * 1024 * 1024)
  , datalink_control_size_(4 * 1024)
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

  GET_CONFIG_VALUE(cf, sect, ACE_TEXT("pool_size"), pool_size_, size_t)
  GET_CONFIG_VALUE(cf, sect, ACE_TEXT("datalink_control_size"),
                   datalink_control_size_, size_t)
  return 0;
}

void
ShmemInst::dump(std::ostream& os)
{
  TransportInst::dump(os);
  os << formatNameForDump("pool_size") << pool_size_ << "\n"
     << formatNameForDump("datalink_control_size") << datalink_control_size_
     << std::endl;
}

} // namespace DCPS
} // namespace OpenDDS
