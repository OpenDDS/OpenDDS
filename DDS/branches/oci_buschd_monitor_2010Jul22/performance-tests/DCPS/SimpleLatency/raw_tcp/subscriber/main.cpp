#include "TestDriver.h"
#include "TestException.h"
#include "ace/Log_Msg.h"


int
ACE_TMAIN (int argc, ACE_TCHAR *argv[])
{
  TestDriver driver;

  try
  {
    driver.run(argc, argv);
    return 0;
  }
  catch (const TestException&)
  {
    ACE_ERROR((LM_ERROR,
               "(%P|%t) Subscriber TestException.\n"));
  }
  catch (...)
  {
    ACE_ERROR((LM_ERROR,
               "(%P|%t) Subscriber unknown (...) exception.\n"));
  }

  return 1;
}
