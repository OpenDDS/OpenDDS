/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_MESSAGETRACKER_H
#define OPENDDS_DCPS_MESSAGETRACKER_H

#include "dds/DCPS/dcps_export.h"
#include "dds/DCPS/PoolAllocator.h"
#include "ace/Thread_Mutex.h"
#include "ace/Condition_Thread_Mutex.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

  /**
   * A simple message tracker to use to wait until all messages have been
   * accounted for being continuing processing.
   */
  class OpenDDS_Dcps_Export MessageTracker {
  public:
    MessageTracker(const OPENDDS_STRING& msg_src);

    /**
     * Indicate that a message has been to the transport layer.
     */
    void message_sent();

    /**
     * Indicate that a message has been delivered by the transport layer.
     */
    void message_delivered();

    /**
     * Indicate that a message has been dropped by the transport layer.
     */
    void message_dropped();

    /**
     * Answer if there are any messages that have not been accounted for.
     */
    bool pending_messages();

    /**
     * Block until all messages have been account for.
     */
    void wait_messages_pending(OPENDDS_STRING& caller_message);

    /**
     * Provide a timestamp for the passed in time.
     * @deprecated Remove and replace with ACE::timestamp
     * once TAO 1.6a is no longer supported
     */
    static ACE_TCHAR*
    timestamp (const ACE_Time_Value& time_value,
               ACE_TCHAR date_and_time[],
               size_t date_and_timelen);

    /**
     * For testing.
     */
    int dropped_count();

  private:

    const OPENDDS_STRING msg_src_;         // Source of tracked messages
    int                  dropped_count_;
    int                  delivered_count_; // Messages transmitted by transport layer
    int                  sent_count_;      // Messages sent to transport layer

    ACE_Thread_Mutex lock_;

    /// All messages have been transported condition variable.
    ACE_Condition_Thread_Mutex done_condition_;
  };

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif

