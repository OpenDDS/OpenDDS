#include "PubDriver.h"
#include "TestException.h"
#include "ace/Log_Msg.h"

#include "dds/DCPS/StaticIncludes.h"


int
ACE_TMAIN (int argc, ACE_TCHAR *argv[])
{
  ACE_LOG_MSG->priority_mask(LM_TRACE     |
                             LM_DEBUG     |
                             LM_INFO      |
                             LM_NOTICE    |
                             LM_WARNING   |
                             LM_ERROR     |
                             LM_CRITICAL  |
                             LM_ALERT     |
                             LM_EMERGENCY,
                             ACE_Log_Msg::PROCESS);


  PubDriver driver;

  try
  {
    driver.run(argc, argv);
    return 0;
  }
  catch (const TestException&)
  {
    ACE_ERROR((LM_ERROR,
               "(%P|%t) PubDriver TestException.\n"));
  }
  catch (...)
  {
    ACE_ERROR((LM_ERROR,
               "(%P|%t) PubDriver unknown (...) exception.\n"));
  }

  return 1;
}
