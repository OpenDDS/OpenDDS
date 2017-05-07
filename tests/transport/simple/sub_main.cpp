#include "SubDriver.h"
#include "TestException.h"

#include "ace/Log_Msg.h"
#include "ace/OS_NS_strings.h"

#include "dds/DCPS/transport/tcp/Tcp.h"
#include "dds/DCPS/transport/shmem/Shmem.h"
#include "dds/DCPS/transport/framework/EntryExit.h"
#include "dds/DCPS/Service_Participant.h"

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

  for (int i = 1; i < argc; ++i) {
    if (0 == ACE_OS::strcasecmp(argv[i], ACE_TEXT("-q"))) {
      const u_long mask = ACE_LOG_MSG->priority_mask(ACE_Log_Msg::PROCESS);
      ACE_LOG_MSG->priority_mask(mask & ~LM_DEBUG, ACE_Log_Msg::PROCESS);
    }
  }

  try
  {
    TheServiceParticipant->get_domain_participant_factory();
  }
  catch (const CORBA::BAD_PARAM& ex) {
    ex._tao_print_exception("Exception caught in sub_main.cpp:");
    return 1;
  }

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
             "Create the SubDriver object.\n"));

  SubDriver driver;

  try
  {
    VDBG((LM_DEBUG, "(%P|%t) DBG:   "
               "Tell the SubDriver object to run(argc,argv).\n"));

    driver.run(argc, argv);

    VDBG((LM_DEBUG, "(%P|%t) DBG:   "
               "SubDriver object has completed running.  "
               "Exit process with success code (0).\n"));

    return 0;
  }
  catch (const TestException&)
  {
    ACE_ERROR((LM_ERROR,
               "(%P|%t) SubDriver TestException.\n"));
  }
  catch (...)
  {
    ACE_ERROR((LM_ERROR,
               "(%P|%t) SubDriver unknown (...) exception.\n"));
  }

  VDBG((LM_DEBUG, "(%P|%t) DBG:   "
             "SubDriver object has completed running due to an exception.  "
             "Exit process with failure code (1).\n"));

  return 1;
}
