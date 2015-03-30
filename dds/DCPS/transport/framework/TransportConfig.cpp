/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "TransportConfig.h"
#include "TransportInst.h"

#if !defined (__ACE_INLINE__)
# include "TransportConfig.inl"
#endif /* !__ACE_INLINE__ */

namespace OpenDDS {
namespace DCPS {

TransportConfig::TransportConfig(const OPENDDS_STRING& name)
  : swap_bytes_(false)
  , passive_connect_duration_(DEFAULT_PASSIVE_CONNECT_DURATION)
  , name_(name)
{}

TransportConfig::~TransportConfig()
{}

void
TransportConfig::sorted_insert(const TransportInst_rch& inst)
{
  const OPENDDS_STRING name = inst->name();
  OPENDDS_VECTOR(TransportInst_rch)::iterator it = instances_.begin();
  while (it != instances_.end() && (*it)->name() < name) {
    ++it;
  }
  instances_.insert(it, inst);
}

}
}
