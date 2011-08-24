/*
 * $Id$
 */

#include <ace/Arg_Shifter.h>
#include <ace/Log_Msg.h>
#include <ace/OS_main.h>
#include <ace/OS_NS_stdlib.h>

#include <dds/DCPS/Service_Participant.h>

#include "ParticipantTask.h"

#ifdef ACE_AS_STATIC_LIBS
# include <dds/DCPS/transport/tcp/Tcp.h>
#endif

namespace
{
  size_t num_threads = 1;
  size_t samples_per_thread = 1024;

  void
  parse_args(int& argc, ACE_TCHAR** argv)
  {
    ACE_Arg_Shifter shifter(argc, argv);

    while (shifter.is_anything_left())
    {
      const ACE_TCHAR* arg;

      if ((arg = shifter.get_the_parameter(ACE_TEXT("-s"))))
      {
        samples_per_thread = ACE_OS::atoi(arg);
        shifter.consume_arg();
      }
      else if ((arg = shifter.get_the_parameter(ACE_TEXT("-t"))))
      {
        num_threads = ACE_OS::atoi(arg);
        shifter.consume_arg();
      }
      else
      {
        shifter.ignore_arg();
      }
    }
  }
} // namespace

int
ACE_TMAIN(int argc, ACE_TCHAR** argv)
{
  parse_args(argc, argv);

  ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) -> PUBLISHER STARTED\n")));

  try
  {
    TheParticipantFactoryWithArgs(argc, argv);

    // Spawn Participant threads
    ParticipantTask task(samples_per_thread);

    task.activate(DEFAULT_FLAGS, num_threads);
    task.wait();

    // Clean-up!
    TheServiceParticipant->shutdown();
  }
  catch (const CORBA::Exception& e)
  {
    e._tao_print_exception("caught in main()");
    return 1;
  }

  ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) <- PUBLISHER FINISHED\n")));

  return 0;
}
