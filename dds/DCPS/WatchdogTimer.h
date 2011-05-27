/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_WATCHDOG_TIMER_H
#define OPENDDS_WATCHDOG_TIMER_H

#include"ace/Event_Handler.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

class ACE_Time_Value;

namespace OpenDDS {
namespace DCPS {

class Watchdog;

/**
 * @class WatchdogTimer
 *
 * @brief Event handler responsible for calling watchdog when
 *        timer expires.
 *
 * This event handler is triggered when its corresponding timer
 * interval expires.  It calls back on the watchdog object
 * associated with it.
 */
class WatchdogTimer
  : public ACE_Event_Handler {
public:

  /// Constructor.
  WatchdogTimer(Watchdog * dog);

  /// Destructor.
  virtual ~WatchdogTimer();

protected:

  /// Template method called when deadline period has expired.
  /**
   * This @c ACE_Event_Handler template method is called when the
   * deadline period has expired.  The appropriate listener or
   * condition method will be called.
   */
  virtual int handle_timeout(ACE_Time_Value const & current_time,
                             void const * act);

private:

  /// Pointer to @c Watchdog object which will be called when the
  /// corresponding timer expires.
  Watchdog * const watchdog_;

};

} // namespace DCPS
} // namespace OpenDDS

#endif  /* OPENDDS_WATCHDOG_TIMER_H */
