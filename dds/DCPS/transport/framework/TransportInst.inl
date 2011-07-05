/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "TransportDefs.h"
#include "PerConnectionSynchStrategy.h"
#include "EntryExit.h"


ACE_INLINE
OpenDDS::DCPS::TransportInst::TransportInst(const char* type,
                                            const std::string& name,
                                            ThreadSynchStrategy* send_strategy)
  : transport_type_(type),
    swap_bytes_(0),
    queue_messages_per_pool_(DEFAULT_CONFIG_QUEUE_MESSAGES_PER_POOL),
    queue_initial_pools_(DEFAULT_CONFIG_QUEUE_INITIAL_POOLS),
    max_packet_size_(DEFAULT_CONFIG_MAX_PACKET_SIZE),
    max_samples_per_packet_(DEFAULT_CONFIG_MAX_SAMPLES_PER_PACKET),
    optimum_packet_size_(DEFAULT_CONFIG_OPTIMUM_PACKET_SIZE),
    thread_per_connection_(0),
    datalink_release_delay_(10000),
    datalink_control_chunks_(32),
    name_(name),
    send_thread_strategy_(send_strategy)
{
  DBG_ENTRY_LVL("TransportInst", "TransportInst", 6);
  this->adjust_config_value();
}

ACE_INLINE
void
OpenDDS::DCPS::TransportInst::send_thread_strategy
(const ThreadSynchStrategy_rch& strategy)
{
  DBG_ENTRY_LVL("TransportInst","send_thread_strategy",6);
  this->send_thread_strategy_ = strategy;
}

ACE_INLINE
OpenDDS::DCPS::ThreadSynchStrategy_rch
OpenDDS::DCPS::TransportInst::send_thread_strategy() const
{
  DBG_ENTRY_LVL("TransportInst","send_thread_strategy",6);
  return this->send_thread_strategy_;
}

ACE_INLINE
void
OpenDDS::DCPS::TransportInst::adjust_config_value()
{
  // Ensure that the number of samples put into the packet does
  // not exceed the allowed number of io vectors to be sent by the OS.
  size_t old_value = max_samples_per_packet_;

  if ((2 * max_samples_per_packet_ + 1) > MAX_SEND_BLOCKS) {
    max_samples_per_packet_ = (MAX_SEND_BLOCKS + 1) / 2 - 1;
    ACE_DEBUG((LM_NOTICE,
               ACE_TEXT("(%P|%t) NOTICE: \"max_samples_per_packet\" is adjusted from %u to %u\n"),
               old_value, max_samples_per_packet_));
  }
}
