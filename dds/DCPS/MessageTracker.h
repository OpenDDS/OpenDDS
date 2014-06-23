/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_MESSAGETRACKER_H
#define OPENDDS_DCPS_MESSAGETRACKER_H

#include <ace/Thread_Mutex.h>
#include <ace/Condition_Thread_Mutex.h>

namespace OpenDDS {
namespace DCPS {

  /**
   * A simple message tracker to use to wait until all messages have been
   * accounted for being continuing processing.
   */
  class  MessageTracker {
  public:
    MessageTracker();

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
    void wait_messages_pending();

  private:

    int         dropped_count_;
    int         delivered_count_; // Messages transmitted by transport layer
    int         sent_count_;      // Messages sent to transport layer

    ACE_Thread_Mutex lock_;

    /// All messages have been transported condition variable.
    ACE_Condition_Thread_Mutex done_condition_;

  };

} // namespace DCPS
} // namespace OpenDDS


#endif

