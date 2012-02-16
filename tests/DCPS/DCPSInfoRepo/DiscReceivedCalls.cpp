#include "DiscReceivedCalls.h"

#include <ace/OS_NS_sys_time.h>

#include <string>

DiscReceivedCalls::DiscReceivedCalls ()
: next_(0)
{
}


bool
DiscReceivedCalls::expect(CORBA::ORB_var& orb, const unsigned int maxDelay, const std::vector<Called>& expected)
{
  const size_t expectSize = expected.size();
  size_t numRcv = numReceived();
  ACE_Time_Value delay(1,0);
  ACE_Time_Value tod;
  const ACE_Time_Value end = ACE_OS::gettimeofday() + ACE_Time_Value(maxDelay, 0);
  while ((numRcv < expectSize) && ((tod = ACE_OS::gettimeofday()) < end))
    {
      orb->perform_work(delay);
      numRcv = numReceived();
    }
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, false);
  long matched = 0;
  bool failed = expectSize != numRcv;
  std::string receivedStr;
  while (numRcv-- > 0)
    {
      const Called next = received_[next_++];
      if (!receivedStr.empty())
        receivedStr += ",";
      receivedStr += toString(next);
      bool receivedMatched = false;
      long match = 1;
      for (std::vector<Called>::const_iterator exp = expected.begin(); exp != expected.end(); ++exp, match = match << 1)
        {
          if ((next == *exp) && ((matched & match) == 0))
            {
              matched |= match;
              receivedMatched = true;
              break;
            }
        }
      failed |= !receivedMatched;
    }

  if (failed || ((matched + 1) != (1 << expectSize)))
    {
      std::string expectedStr;
      for (std::vector<Called>::const_iterator exp = expected.begin(); exp != expected.end(); ++exp)
        {
          if (!expectedStr.empty())
            expectedStr += ",";
          expectedStr += toString(*exp);
        }
      ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: Expected to receive the following calls < %C >, instead received < %C >\n"),
        expectedStr.c_str(),
        receivedStr.c_str()));

      return false;
    }

  return true;
}

bool
DiscReceivedCalls::expectNothing()
{
  std::vector<Called> empty;
  CORBA::ORB_var fake;
  return expect(fake, 0, empty);
}

void
DiscReceivedCalls::received(Called called)
{
  ACE_GUARD(ACE_Thread_Mutex, g, lock_);
  received_.push_back(called);
}

size_t
DiscReceivedCalls::numReceived()
{
  ACE_GUARD_RETURN(ACE_Thread_Mutex, g, lock_, 0);
  return (received_.size() - next_);
}

const char*
DiscReceivedCalls::toString(Called called)
{
  switch (called)
  {
  case ENABLE_SPECIFIC:
    return "enable_specific";
  case ADD_ASSOC:
    return "add_association";
  case ASSOC_COMPLETE:
    return "association_complete";
  case REM_ASSOC:
    return "remove_association";
  case UPDATE_INCOMP_QOS:
    return "update_incompatible_qos";
  case UPDATE_SUB_PARAMS:
    return "update_subscription_params";
  default:
    break;
  }
  return "<undefined Called enum value>";
}
