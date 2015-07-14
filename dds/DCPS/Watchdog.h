/*
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

#include "dds/DCPS/ReactorInterceptor.h"

#include "ace/Time_Value.h"

namespace OpenDDS {
namespace DCPS {

/**
 * @brief Watchdog abstract base class.
 *
 * A @c Watchdog object executes an operation each time a
 * corresponding timer expires.  The timer is managed by this
 * class.  However, it is the responsibility of the @c Watchdog
 * owner, for example, to run the @c ACE_Reactor event loop.
 * The @c Watchdog timer will not fire, otherwise.
 */
class Watchdog : public ReactorInterceptor {
protected:

  explicit Watchdog(const ACE_Time_Value& interval);

  virtual ~Watchdog();

private:
  int handle_timeout(const ACE_Time_Value&, const void* act)
  {
    return execute(act, true), 0;
  }

  /// Operation to be executed when the associated timer expires
  /// or whenever samples are received/sent.
  /// The @c timer_called flag indicates if it's called from
  /// reator handle_timeout() or upon a sample receiving/sending.
  virtual void execute(const void* act, bool timer_called) = 0;

  /// Re-schedule timer with new interval.
  virtual void reschedule_deadline() = 0;

  bool reactor_is_shut_down() const;

public:
  /// Reset the @c Watchdog timer interval, i.e. time between
  /// recurring timer expirations.
  /**
   * @note The new interval takes effect after the next
   *       expiration.  This behavior is dictated by the
   *       @c ACE_Reactor.
   */
  void reset_interval(const ACE_Time_Value& interval);

  /// Schedule with the @c Watchdog timer interval, i.e. time between
  /// recurring timer expirations.
  long schedule_timer(const void* act, const ACE_Time_Value& interval);

  /// Cancel a specific timer.
  int cancel_timer(long timer_id);

  /// Cancel all associated timers.
  void cancel_all();

  /// Reset interval for a specific timer.
  int reset_timer_interval(long timer_id);

protected:
  /// Current time interval.
  ACE_Time_Value interval_;
};

} // namespace DCPS
} // namespace OpenDDS

#endif  /* OPENDDS_WATCHDOG_H */
