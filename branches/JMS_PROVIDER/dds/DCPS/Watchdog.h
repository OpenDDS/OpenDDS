// -*- C++ -*-

//=============================================================================
/**
 *  @file   Watchdog.h
 *
 *  $Id$
 *
 *  @c Watchdog abstract base class.
 *
 *  @author Ossama Othman <othmano@ociweb.com>
 */
//=============================================================================

#ifndef OPENDDS_WATCHDOG_H
#define OPENDDS_WATCHDOG_H


#include "ace/config-all.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "dds/DCPS/WatchdogTimer.h"

class ACE_Time_Value;

namespace OpenDDS
{
  namespace DCPS
  {

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
    class Watchdog
    {
    public:

      /// Constructor
      Watchdog (ACE_Reactor * reactor,
                ACE_Time_Value const & interval);

      /// Destructor
      virtual ~Watchdog ();

      /// Operation to be executed when the associated timer expires.
      virtual void execute () = 0;

      /// Reset the @c Watchdog timer interval, i.e. time between
      /// recurring timer expirations.
      /**
       * @note The new interval takes effect after the next
       *       expiration.  This behavior is dictated by the
       *       @c ACE_Reactor.
       */
      void reset_interval (ACE_Time_Value const & interval);

      /// "Pet the dog", i.e. prevent the @c Watchdog from executing
      /// on timeout.
      // void signal ();

    private:

      // Reactor with which the timer will be registered.
      ACE_Reactor * const reactor_;

      WatchdogTimer timer_;

      /// Timer identifier.  Only necessary to cancel timer.
      long timer_id_;

    };

  }
}

#endif  /* OPENDDS_WATCHDOG_H */
