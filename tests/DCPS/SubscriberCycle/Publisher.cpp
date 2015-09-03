/*
 */

#include <ace/Arg_Shifter.h>
#include <ace/Log_Msg.h>
#include <ace/OS_main.h>
#include <ace/OS_NS_stdlib.h>

#include <dds/DCPS/Service_Participant.h>

#include "ParticipantTask.h"

#include "dds/DCPS/StaticIncludes.h"

namespace
{
  size_t num_threads = 1;
  size_t samples_per_thread = 1024;
  int delay_between_pubs_msec = 1000;
  int deadline_msec = 250;

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
      else if ((arg = shifter.get_the_parameter(ACE_TEXT("-d"))))
      {
        delay_between_pubs_msec = ACE_OS::atoi(arg);
        shifter.consume_arg();
      }
      else if ((arg = shifter.get_the_parameter(ACE_TEXT("-l"))))
      {
        deadline_msec = ACE_OS::atoi(arg);
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

  ::CORBA::Long sec = deadline_msec / 1000;
  ::CORBA::ULong remainder_msec = (deadline_msec - 1000*sec);
  ::CORBA::ULong nanosec = remainder_msec * 1000000;

  DDS::Duration_t const deadline_period =
    {
      sec,
      nanosec
    };

  try
  {
    DDS::DomainParticipantFactory_var dpf =
      TheParticipantFactoryWithArgs(argc, argv);

    // Spawn Participant threads
    ParticipantTask task(samples_per_thread, delay_between_pubs_msec, deadline_period);

    task.activate(DEFAULT_FLAGS, static_cast<int>(num_threads));
    task.wait();

    TheServiceParticipant->shutdown();
  }
  catch (const CORBA::Exception& e)
  {
    e._tao_print_exception("caught in Publisher main()");
    return 1;
  }

  ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) <- PUBLISHER FINISHED\n")));

  return 0;
}
