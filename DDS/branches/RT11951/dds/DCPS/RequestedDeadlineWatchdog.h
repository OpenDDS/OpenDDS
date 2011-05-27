// -*- C++ -*-

//=============================================================================
/**
 *  @file   RequestedDeadlineWatchdog.h
 *
 *  $Id$
 *
 *  Requested deadline watchdog.
 *
 *  @author Ossama Othman <othmano@ociweb.com>
 */
//=============================================================================

#ifndef OPENDDS_REQUESTED_DEADLINE_WATCHDOG_H
#define OPENDDS_REQUESTED_DEADLINE_WATCHDOG_H


#include "dds/DdsDcpsSubscriptionC.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "dds/DCPS/Watchdog.h"

#include "ace/Reverse_Lock_T.h"


namespace OpenDDS
{
  namespace DCPS
  {

    /**
     * @class RequestedDeadlineWatchdog
     *
     * @brief Watchdog responsible calling the @c DataReaderListener
     *        when the deadline period expires.
     *
     * This watchdog object calls the
     * @c on_requested_deadline_missed() listener callback when the
     * configured finite deadline period expires.
     */
    class RequestedDeadlineWatchdog : public Watchdog
    {
    public:

      typedef ACE_Recursive_Thread_Mutex  lock_type;
      typedef ACE_Reverse_Lock<lock_type> reverse_lock_type;

      /// Constructor
      RequestedDeadlineWatchdog (
        ACE_Reactor * reactor,
        lock_type & lock,
        ::DDS::DeadlineQosPolicy qos,
        ::DDS::DataReaderListener_ptr listener,
        ::DDS::DataReader_ptr reader,
        ::DDS::RequestedDeadlineMissedStatus & status,
        CORBA::Long & last_total_count);

      /// Destructor
      virtual ~RequestedDeadlineWatchdog ();

      /// Operation to be executed when the associated timer expires.
      /**
       * This @c Watchdog object updates the
       * @c DDS::RequestedDeadlineMissed structure, and calls
       * @c DataReaderListener::on_requested_deadline_missed().
       */
      virtual void execute ();

      /// "Pet the dog", i.e. prevent the @c Watchdog from executing
      /// on timeout.
      void signal ();

    private:

      /// Lock for synchronization of @c status_ member.
      lock_type & lock_;

      /// Reverse lock used for releasing the @c lock_ listener upcall.
      reverse_lock_type reverse_lock_;

      /// Flag that indicates whether the watchdog has been signaled
      /// to not execute upon timer expiration.  This flag is reset to
      /// @c false after each deadline timeout.
      bool signaled_;

      /// Reference to listener that will be called when the deadline
      /// expires.
      ::DDS::DataReaderListener_var      listener_;

      /// Reference to DataReader passed to listener when the deadline
      /// expires.
      ::DDS::DataReader_var              reader_;

      /// Reference to the missed requested deadline status
      /// structure.
      ::DDS::RequestedDeadlineMissedStatus & status_;

      /// Last total_count when status was last checked.
      CORBA::Long & last_total_count_;

    };

  }
}

#endif  /* OPENDDS_REQUESTED_DEADLINE_WATCHDOG_H */
