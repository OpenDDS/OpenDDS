// -*- C++ -*-

//=============================================================================
/**
 *  @file    Exit_Signal_Handler.cpp
 *
 *
 * @author   Friedhelm Wolf (fwolf@dre.vanderbilt.edu)
 */
//=============================================================================

#include <iostream>
#include "Exit_Signal_Handler.h"

Exit_Signal_Handler::Exit_Signal_Handler (ACE_Thread_Semaphore & lock)
  : lock_ (lock)
{
}

Exit_Signal_Handler::~Exit_Signal_Handler ()
{
}

int
Exit_Signal_Handler::handle_signal (int , siginfo_t *, ucontext_t *)
{
  return lock_.release ();
}
