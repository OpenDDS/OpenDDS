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
  : TransportInst("tcp", name),
    enable_nagle_algorithm_(false),
    conn_retry_initial_delay_(500),
    conn_retry_backoff_multiplier_(2.0),
    conn_retry_attempts_(3),
    max_output_pause_period_(-1),
    passive_reconnect_duration_(2000),
    active_conn_timeout_period_(5000)
{
  DBG_ENTRY_LVL("TcpInst", "TcpInst", 6);
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
