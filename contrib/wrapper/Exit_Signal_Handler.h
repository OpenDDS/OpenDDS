// -*- C++ -*-

//=============================================================================
/**
 *  @file    Exit_Signal_Handler.h
 *
 *
 *  @author  Friedhelm Wolf (fwolf@dre.vanderbilt.edu)
 */
//=============================================================================

#ifndef DDS_WRAPPER_EXIT_SIGNAL_HANDLER_H_
#define DDS_WRAPPER_EXIT_SIGNAL_HANDLER_H_

#include <ace/Event_Handler.h>
#include <ace/Thread_Semaphore.h>

/**
 * @class Exit_Signal_Handler
 *
 * @brief this class handles the kill signal for this application in a nice
 *        way, releasing a semaphore.
 */
class Exit_Signal_Handler : public ACE_Event_Handler
{
public:

  /// ctor
  Exit_Signal_Handler (ACE_Thread_Semaphore & lock);

  /// dtor
  virtual ~Exit_Signal_Handler ();

  /// Called when object is signaled by OS (either via UNIX signals or
  /// when a Win32 object becomes signaled).
  virtual int handle_signal (int signum, siginfo_t *, ucontext_t *);

private:
  ACE_Thread_Semaphore & lock_;
};

#endif /* DDS_WRAPPER_EXIT_SIGNAL_HANDLER_H_ */
