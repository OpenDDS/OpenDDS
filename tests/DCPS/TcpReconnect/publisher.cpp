/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <ace/Get_Opt.h>
#include <ace/Log_Msg.h>
#include <ace/OS_NS_stdlib.h>
#include "ace/Arg_Shifter.h"
#include "ace/Process.h"
#include "ace/OS_NS_unistd.h"

#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/PublisherImpl.h>
#include <dds/DCPS/Service_Participant.h>
#include "tests/Utils/StatusMatching.h"

#include "dds/DCPS/StaticIncludes.h"
#ifdef ACE_AS_STATIC_LIBS
#include <dds/DCPS/transport/udp/Udp.h>
#include <dds/DCPS/transport/multicast/Multicast.h>
#include <dds/DCPS/RTPS/RtpsDiscovery.h>
#include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#include <dds/DCPS/transport/shmem/Shmem.h>
#endif

#include "MessengerTypeSupportImpl.h"
#include "Writer.h"
#include "Args.h"

int stub_kills = 1;
int stub_duration = 4;

bool dw_reliable() {
  OpenDDS::DCPS::TransportConfig_rch gc = TheTransportRegistry->global_config();
  return !(gc->instances_[0]->transport_type_ == "udp");
}

int ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  DDS::DomainParticipantFactory_var dpf;
  DDS::DomainParticipant_var participant;

  try {
    ACE_Process process;

    std::cout << "Starting publisher" << std::endl;
    {
      ACE_TString stubCmd(ACE_TEXT(""));
      ACE_TString stubArgs(ACE_TEXT(""));
      ACE_TString stub_ready_filename(ACE_TEXT(""));

      ACE_Arg_Shifter_T<ACE_TCHAR> shifter(argc, argv);
      while (shifter.is_anything_left()) {
        const ACE_TCHAR* currentArg = 0;
        if ((currentArg = shifter.get_the_parameter(ACE_TEXT("-stubCmd"))) != 0) {
          stubCmd += currentArg;
          ACE_DEBUG((LM_INFO, ACE_TEXT("stubCmd: %s\n"), stubCmd.c_str()));
          shifter.consume_arg();
        } else if ((currentArg = shifter.get_the_parameter(ACE_TEXT("-stubp"))) != 0) {
          stubArgs += ACE_TEXT("-p");
          stubArgs += currentArg;
          ACE_DEBUG((LM_INFO, ACE_TEXT("stubArgs: %s\n"), stubArgs.c_str()));
          shifter.consume_arg();
        } else if ((currentArg = shifter.get_the_parameter(ACE_TEXT("-stubs"))) != 0) {
          stubArgs += ACE_TEXT(" -s");
          stubArgs += currentArg;
          ACE_DEBUG((LM_INFO, ACE_TEXT("stubArgs: %s\n"), stubArgs.c_str()));
          shifter.consume_arg();
        } else if ((currentArg = shifter.get_the_parameter(ACE_TEXT("-stubv"))) != 0) {
          stubArgs += ACE_TEXT(" -v");
          ACE_DEBUG((LM_INFO, ACE_TEXT("stubArgs: %s\n"), stubArgs.c_str()));
          shifter.consume_arg();
        } else if ((currentArg = shifter.get_the_parameter(ACE_TEXT("-stub_ready_file"))) != 0) {
          stubArgs += ACE_TEXT(" -stub_ready_file:");
          stubArgs += currentArg;
          stub_ready_filename = currentArg;
          ACE_DEBUG((LM_INFO, ACE_TEXT("stubArgs: %s\n"), stubArgs.c_str()));
          shifter.consume_arg();
        } else {
          shifter.ignore_arg();
        }

      }

      ACE_Process_Options options;
      ACE_TString command_line = stubCmd + ACE_TEXT(" ") + stubArgs;
      ACE_DEBUG((LM_INFO, ACE_TEXT("stub command line: %s\n"), command_line.c_str()));

      if (options.command_line(command_line.c_str()) != 0)
        ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT ("%p\n"), ACE_TEXT("set options")) ,-1);

      if (stub_ready_filename.empty())
        ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("%p\n"), ACE_TEXT("-stub_ready_file required.")), -1);

#ifdef ACE_WIN32
      options.creation_flags(CREATE_NEW_PROCESS_GROUP);
#endif

      pid_t pid = process.spawn(options);

      if (pid == ACE_INVALID_PID)
        ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT ("%p\n"), ACE_TEXT("spawn")) ,-1);

      // Wait for the stub to be ready.
      FILE* stub_ready = 0;
      do
        {
          ACE_Time_Value small_time(0,250000);
          ACE_OS::sleep (small_time);
          stub_ready = ACE_OS::fopen (stub_ready_filename.c_str (), ACE_TEXT("r"));
        } while (0 == stub_ready);

      ACE_OS::fclose(stub_ready);

      // Initialize DomainParticipantFactory
      dpf = TheParticipantFactoryWithArgs(argc, argv);

      std::cout << "Starting publisher with " << argc << " args" << std::endl;
      int error;
      if ((error = parse_args(argc, argv)) != 0) {
        return error;
      }

      // Create DomainParticipant
      participant = dpf->create_participant(4,
                                PARTICIPANT_QOS_DEFAULT,
                                DDS::DomainParticipantListener::_nil(),
                                OpenDDS::DCPS::DEFAULT_STATUS_MASK);

      if (CORBA::is_nil(participant.in())) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l: main()")
                          ACE_TEXT(" ERROR: create_participant failed!\n")),
                         -1);
      }

      // Register TypeSupport (Messenger::Message)
      Messenger::MessageTypeSupport_var mts =
        new Messenger::MessageTypeSupportImpl();

      if (mts->register_type(participant.in(), "") != DDS::RETCODE_OK) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l: main()")
                          ACE_TEXT(" ERROR: register_type failed!\n")),
                         -1);
      }

      // Create Topic
      CORBA::String_var type_name = mts->get_type_name();
      DDS::Topic_var topic =
        participant->create_topic("Movie Discussion List",
                                  type_name.in(),
                                  TOPIC_QOS_DEFAULT,
                                  DDS::TopicListener::_nil(),
                                  OpenDDS::DCPS::DEFAULT_STATUS_MASK);

      if (CORBA::is_nil(topic.in())) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l: main()")
                          ACE_TEXT(" ERROR: create_topic failed!\n")),
                         -1);
      }

      // Create Publisher
      DDS::Publisher_var pub =
        participant->create_publisher(PUBLISHER_QOS_DEFAULT,
                                      DDS::PublisherListener::_nil(),
                                      OpenDDS::DCPS::DEFAULT_STATUS_MASK);

      if (CORBA::is_nil(pub.in())) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l: main()")
                          ACE_TEXT(" ERROR: create_publisher failed!\n")),
                         -1);
      }

      DDS::DataWriterQos qos;
      pub->get_default_datawriter_qos(qos);
      if (dw_reliable()) {
        std::cout << "Reliable DataWriter" << std::endl;
        qos.history.kind = DDS::KEEP_ALL_HISTORY_QOS;
        qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;
      }

      // Create DataWriter
      DDS::DataWriter_var dw =
        pub->create_datawriter(topic.in(),
                               qos,
                               DDS::DataWriterListener::_nil(),
                               OpenDDS::DCPS::DEFAULT_STATUS_MASK);

      if (CORBA::is_nil(dw.in())) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("%N:%l: main()")
                          ACE_TEXT(" ERROR: create_datawriter failed!\n")),
                         -1);
      }
      // Block until Subscriber is available
      Utils::wait_match(dw, 1, Utils::GTE);

      // Start writing threads
      std::cout << "Creating Writer" << std::endl;
      Writer* writer = new Writer(dw.in());
      std::cout << "Starting Writer" << std::endl;
      writer->start();

      int stubKillCount = stub_kills;
      std::cout << "Publisher will kill stub " << stub_kills << " times." << std::endl;
      while (stubKillCount > 0) {
        ACE_OS::sleep(stub_duration);
        ACE_DEBUG((LM_DEBUG, "(%P|%t) publisher killing connection for the %d time\n", stub_kills - stubKillCount + 1));
        std::cout << "Kill stub (connection) for the " << stub_kills - stubKillCount + 1 << " time" << std::endl;

# ifdef ACE_WIN32
        GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT, process.getpid());
# else
        process.kill();
# endif
        process.wait();
        writer->increment_phase();
        ACE_OS::sleep(5);
        pid = process.spawn(options);
        std::cout << "stub (connection) reconnected after killing " << stub_kills - stubKillCount + 1 << " time" << std::endl;
        ACE_DEBUG((LM_DEBUG, "(%P|%t) publisher reconnected after killing connection for the %d time\n", stub_kills - stubKillCount + 1));
        --stubKillCount;
      }

      while (!writer->is_finished()) {
        ACE_Time_Value small_time(0, 250000);
        ACE_OS::sleep(small_time);
      }

      std::cout << "Writer finished " << std::endl;
      writer->end();

      if (wait_for_acks) {
        std::cout << "Writer wait for ACKS" << std::endl;

        DDS::Duration_t timeout =
          { DDS::DURATION_INFINITE_SEC, DDS::DURATION_INFINITE_NSEC };
        dw->wait_for_acknowledgments(timeout);
      } else {
        // let any missed multicast/rtps messages get re-delivered
        ACE_Time_Value small_time(0, 250000);
        ACE_OS::sleep(small_time);
      }

      std::cout << "deleting DW" << std::endl;
      delete writer;
    }
    // Clean-up!
    participant->delete_contained_entities();
    dpf->delete_participant(participant.in());
    TheServiceParticipant->shutdown();

# ifdef ACE_WIN32
    GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT, process.getpid());
# else
    process.kill();
# endif
    process.wait();

  } catch (const CORBA::Exception& e) {
    e._tao_print_exception("Exception caught in main():");
    ACE_OS::exit(-1);
  }

  return 0;
}
