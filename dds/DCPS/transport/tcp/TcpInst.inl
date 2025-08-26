/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "dds/DCPS/transport/framework/EntryExit.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

ACE_INLINE
OpenDDS::DCPS::TcpInst::TcpInst(const OPENDDS_STRING& name)
  : TransportInst("tcp", name)
{
  DBG_ENTRY_LVL("TcpInst", "TcpInst", 6);
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
