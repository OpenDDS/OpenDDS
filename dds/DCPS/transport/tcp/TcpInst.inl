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
  , pub_address_str_(*this, &TcpInst::pub_address_str, &TcpInst::pub_address_str)
  , enable_nagle_algorithm_(*this, &TcpInst::enable_nagle_algorithm, &TcpInst::enable_nagle_algorithm)
  , conn_retry_initial_delay_(*this, &TcpInst::conn_retry_initial_delay, &TcpInst::conn_retry_initial_delay)
  , conn_retry_backoff_multiplier_(*this, &TcpInst::conn_retry_backoff_multiplier, &TcpInst::conn_retry_backoff_multiplier)
  , conn_retry_attempts_(*this, &TcpInst::conn_retry_attempts, &TcpInst::conn_retry_attempts)
  , max_output_pause_period_(*this, &TcpInst::max_output_pause_period, &TcpInst::max_output_pause_period)
  , passive_reconnect_duration_(*this, &TcpInst::passive_reconnect_duration, &TcpInst::passive_reconnect_duration)
  , active_conn_timeout_period_(*this, &TcpInst::active_conn_timeout_period, &TcpInst::active_conn_timeout_period)
{
  DBG_ENTRY_LVL("TcpInst", "TcpInst", 6);
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
