/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_WATCHDOG_H
#define OPENDDS_WATCHDOG_H

#include "ace/config-all.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "dds/DdsDcpsInfrastructureC.h"
#include "dds/DCPS/WatchdogTimer.h"
#include <map>
#include <vector>

class ACE_Time_Value;

namespace OpenDDS {
namespace DCPS {

/**
 * @class WatchDog
 *
 * @brief Watchdog abstract base class.
 *
 * A @c Watchdog object executes an operation each time a
 * corresponding timer expires.  The timer is managed by this
 * class.  However, it is the responsibility of the @c Watchdog
 * owner, for example, to run the @c ACE_Reactor event loop.
 * The @c Watchdog timer will not fire, otherwise.
 *
 * @note This class is by design OpenDDS agnostic so that it may
 *       eventually be used outside of OpenDDS.
 */

class Watchdog {
public:

  /// Constructor
  Watchdog(ACE_Reactor * reactor,
           ACE_Time_Value const & interval);

  /// Destructor
  virtual ~Watchdog();

  /// Operation to be executed when the associated timer expires
  /// or whenever samples are received/sent.
  /// The @c timer_called flag indicates if it's called from
  /// reator handle_timeout() or upon a sample receiving/sending.
  virtual void execute(void const * act, bool timer_called) = 0;

  /// Reset the @c Watchdog timer interval, i.e. time between
  /// recurring timer expirations.
  /**
   * @note The new interval takes effect after the next
   *       expiration.  This behavior is dictated by the
   *       @c ACE_Reactor.
   */
  void reset_interval(ACE_Time_Value const & interval);

  /// Schedure with the @c Watchdog timer interval, i.e. time between
  /// recurring timer expirations.
  long schedule_timer(void* const act, const ACE_Time_Value& interval);

  /// Cancel a specific timer.
  int cancel_timer(long const & timer_id);

  /// Cancel all associated timers.
  void cancel_all();

  /// Re-schedule timer with new interval.
  virtual void reschedule_deadline() = 0;

  /// Reset interval for a specific timer.
  int  reset_timer_interval(long const & timer_id);

protected:

  // Reactor with which the timer will be registered.
  ACE_Reactor * const reactor_;

  // Event handler that handles timeout.
  WatchdogTimer timer_;

  // Current time interval.
  ACE_Time_Value interval_;
};

} // namespace DCPS
} // namespace OpenDDS

#endif  /* OPENDDS_WATCHDOG_H */
