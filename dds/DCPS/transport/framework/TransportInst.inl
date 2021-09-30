/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "TransportDefs.h"
#include "EntryExit.h"


ACE_INLINE
OpenDDS::DCPS::TransportInst::TransportInst(const char* type,
                                            const OPENDDS_STRING& name)
  : transport_type_(type),
    queue_messages_per_pool_(DEFAULT_CONFIG_QUEUE_MESSAGES_PER_POOL),
    queue_initial_pools_(DEFAULT_CONFIG_QUEUE_INITIAL_POOLS),
    max_packet_size_(DEFAULT_CONFIG_MAX_PACKET_SIZE),
    max_samples_per_packet_(DEFAULT_CONFIG_MAX_SAMPLES_PER_PACKET),
    optimum_packet_size_(DEFAULT_CONFIG_OPTIMUM_PACKET_SIZE),
    thread_per_connection_(0),
    datalink_release_delay_(10000),
    datalink_control_chunks_(32),
    fragment_reassembly_timeout_(300),
    shutting_down_(false),
    name_(name)
{
  DBG_ENTRY_LVL("TransportInst", "TransportInst", 6);
  adjust_config_value();
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
