/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "TransportDefs.h"
#include "EntryExit.h"

#include "dds/DCPS/ConfigStoreImpl.h"

ACE_INLINE
OpenDDS::DCPS::TransportInst::TransportInst(const char* type,
                                            const OPENDDS_STRING& name)
  : transport_type_(type)
  , shutting_down_(false)
  , name_(name)
  , config_prefix_(ConfigPair::canonicalize("OPENDDS_TRANSPORT_" + name_))
{
  DBG_ENTRY_LVL("TransportInst", "TransportInst", 6);
}
