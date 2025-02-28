/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "Boilerplate.h"

#include <tests/Utils/StatusMatching.h>
#include <tests/Utils/DistributedConditionSet.h>

#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/SafetyProfileStreams.h>
#include <dds/DCPS/StaticIncludes.h>
#ifdef ACE_AS_STATIC_LIBS
#  include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#endif

#include <stdexcept>
#include <iostream>

using namespace examples::boilerplate;

int
ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  // Coordination across processes.
  DistributedConditionSet_rch distributed_condition_set =
    OpenDDS::DCPS::make_rch<FileBasedDistributedConditionSet>();

  DDS::DomainParticipantFactory_var dpf;
  DDS::DomainParticipant_var participant;

  try {
    // Initialize DomainParticipantFactory, handling command line args
    dpf = TheParticipantFactoryWithArgs(argc, argv);

    // Override message count
    long msg_count = 5000;
    if (argc > 1) {
        msg_count = ACE_OS::atoi(argv[1]);
    }
    if (msg_count < 0 || msg_count > 5000) {
      ACE_ERROR_RETURN((LM_ERROR,
        ACE_TEXT("ERROR: %N:%l: main() -")
        ACE_TEXT(" specified msg_count outside range!\n")), -1);
    }

    // Create domain participant
    participant = createParticipant(dpf);

    // Register type support and create topic
    DDS::Topic_var topic = createTopic(participant);

    // Create publisher
    DDS::Publisher_var publisher = createPublisher(participant);

    // Create data writer for the topic
    DDS::DataWriter_var writer = createDataWriter(publisher, topic);

    // Safely downcast data writer to type-specific data writer
    Reliability::MessageDataWriter_var msg_writer = narrow(writer);

    // Block until Subscriber is available
    distributed_condition_set->wait_for("publisher", "subscriber", "subscriber ready");

    // Initialize samples
    Reliability::Message message;

    for (int i = 0; i < msg_count; ++i) {
      // Prepare next sample
      const OpenDDS::DCPS::String number = "foo " + OpenDDS::DCPS::to_dds_string(i);
      message.id = CORBA::string_dup(number.c_str());
      message.name = "foo";
      message.count = (long)i;
      message.expected = msg_count;

      // Publish the message
      DDS::ReturnCode_t error = DDS::RETCODE_TIMEOUT;
      while (error == DDS::RETCODE_TIMEOUT) {
        ACE_ERROR((LM_ERROR, "Trying to send: %d\n", i));
        error = msg_writer->write(message, DDS::HANDLE_NIL);
        if (error == DDS::RETCODE_TIMEOUT) {
          ACE_ERROR((LM_ERROR, "Timeout, resending %d\n", i));
        } else if (error != DDS::RETCODE_OK) {
          ACE_ERROR((LM_ERROR,
                     ACE_TEXT("ERROR: %N:%l: main() -")
                     ACE_TEXT(" write returned %d!\n"), error));
        }
      }
    }

    distributed_condition_set->wait_for("publisher", "subscriber", "subscriber done");

  } catch (const CORBA::Exception& e) {
    e._tao_print_exception("Exception caught in main():");
    return -1;
  } catch (std::runtime_error& err) {
    ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("ERROR: main() - %C\n"),
                      err.what()), -1);
  } catch (std::string& msg) {
    ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("ERROR: main() - %C\n"),
                      msg.c_str()), -1);
  }

  // Clean-up!
  cleanup(participant, dpf);

  return 0;
}
