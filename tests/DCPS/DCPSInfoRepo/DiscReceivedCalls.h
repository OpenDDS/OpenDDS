#ifndef DISCRECEIVEDCALLS_H_
#define DISCRECEIVEDCALLS_H_

#include "ace/Time_Value.h"
#include "tao/ORB.h"

#include <vector>

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

class DiscReceivedCalls
{
public:
  enum Called { ENABLE_SPECIFIC, ADD_ASSOC, ASSOC_COMPLETE, REM_ASSOC, UPDATE_INCOMP_QOS, UPDATE_SUB_PARAMS };

  DiscReceivedCalls ();

  bool expect(CORBA::ORB_var& orb, const unsigned int maxDelay, const std::vector<Called>& expected);

  bool expectNothing();

  void received(Called called);

private:
  size_t numReceived();

  static const char* toString(Called called);

  ACE_Thread_Mutex lock_;
  std::vector<Called> received_;
  size_t next_;
};


#endif /* DISCRECEIVEDCALLS_H_  */
