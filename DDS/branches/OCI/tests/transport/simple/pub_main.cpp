#include "PubDriver.h"
#include "TestException.h"
#include "ace/Log_Msg.h"

#include "dds/DCPS/transport/framework/EntryExit.h"


int
main(int argc, char* argv[])
{
  //TURN_ON_VERBOSE_DEBUG;
  DBG_ENTRY("pub_main.cpp","main");

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

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
             "Create the PubDriver object.\n"));

  PubDriver driver;

  try
  {
    VDBG((LM_DEBUG, "(%P|%t) DBG:   "
               "Tell the PubDriver object to run(argc,argv).\n"));

    driver.run(argc, argv);

    VDBG((LM_DEBUG, "(%P|%t) DBG:   "
               "PubDriver object has completed running.  "
               "Exit process with success code (0).\n"));

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

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
             "PubDriver object has completed running due to an exception.  "
             "Exit process with failure code (1).\n"));

  return 1;
}
