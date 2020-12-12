/*
 */

#include <ace/Arg_Shifter.h>
#include <ace/Log_Msg.h>
#include <ace/OS_main.h>
#include <ace/OS_NS_stdlib.h>

#include <dds/DCPS/Service_Participant.h>

#include "ParticipantTask.h"

#include "dds/DCPS/StaticIncludes.h"
#ifdef ACE_AS_STATIC_LIBS
# include "dds/DCPS/RTPS/RtpsDiscovery.h"
# include "dds/DCPS/transport/rtps_udp/RtpsUdp.h"
#endif

namespace
{
  size_t num_threads = 1;
  size_t samples_per_thread = 1024;
  bool durable = false;
  unsigned int seed = time(0);

  void
  parse_args(int& argc, ACE_TCHAR** argv)
  {
    ACE_Arg_Shifter shifter(argc, argv);

    while (shifter.is_anything_left()) {
      const ACE_TCHAR* arg;

      if ((arg = shifter.get_the_parameter(ACE_TEXT("-s")))) {
        samples_per_thread = ACE_OS::atoi(arg);
        shifter.consume_arg();
      } else if ((arg = shifter.get_the_parameter(ACE_TEXT("-t")))) {
        num_threads = ACE_OS::atoi(arg);
        shifter.consume_arg();
      } else if (ACE_OS::strcmp(shifter.get_current(), ACE_TEXT("-d")) == 0) {
        durable = true;
        shifter.consume_arg();
      } else if ((arg = shifter.get_the_parameter(ACE_TEXT("-e")))) {
        seed = ACE_OS::atoi(arg);
        shifter.consume_arg();
      } else {
        shifter.ignore_arg();
      }
    }
  }
} // namespace

int
ACE_TMAIN(int argc, ACE_TCHAR** argv)
{
  try {
    DDS::DomainParticipantFactory_var dpf =
      TheParticipantFactoryWithArgs(argc, argv);
    parse_args(argc, argv);

    ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) -> PUBLISHER STARTED\n")));

    srand(seed);

    // Spawn Participant threads
    ParticipantTask task(samples_per_thread, durable);

    task.activate(DEFAULT_FLAGS, static_cast<int>(num_threads));
    task.wait();

    // Clean-up!
    ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t)    <- PUBLISHER SHUTDOWN\n")));
    TheServiceParticipant->shutdown();
  } catch (const CORBA::Exception& e) {
    e._tao_print_exception("caught in main()");
    return 1;
  }

  ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) <- PUBLISHER FINISHED\n")));

  return 0;
}
