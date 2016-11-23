/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "Boilerplate.h"
#include <dds/DCPS/Service_Participant.h>
#include <model/Sync.h>
#include <stdexcept>
#include <iostream>

#include "dds/DCPS/StaticIncludes.h"
#ifdef ACE_AS_STATIC_LIBS
#include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#endif

using namespace examples::boilerplate;

int
ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  DDS::DomainParticipantFactory_var dpf;
  DDS::DomainParticipant_var participant;

  try {
    // Initialize DomainParticipantFactory, handling command line args
    dpf = TheParticipantFactoryWithArgs(argc, argv);

    bool keep_last_one = false;

    // Override message count
    long msg_count = 5000;
    if (argc > 1) {
      if (!ACE_OS::strcmp(ACE_TEXT("-keep-last-one"), argv[1])) {
        keep_last_one = true;
      } else {
        msg_count = ACE_OS::atoi(argv[1]);
      }
    }

    if (argc > 2) {
      if (!ACE_OS::strcmp(ACE_TEXT("-keep-last-one"), argv[2])) {
        keep_last_one = true;
      }
    }

    // Create domain participant
    participant = createParticipant(dpf);

    // Register type support and create topic
    DDS::Topic_var topic = createTopic(participant);

    // Create publisher
    DDS::Publisher_var publisher = createPublisher(participant);

    // Create data writer for the topic
    DDS::DataWriter_var writer = createDataWriter(publisher, topic, keep_last_one);

    // Safely downcast data writer to type-specific data writer
    Reliability::MessageDataWriter_var msg_writer = narrow(writer);

    {
      // Block until Subscriber is available
      OpenDDS::Model::WriterSync ws(writer);

      // Initialize samples
      Reliability::Message message;

      char number[20];

      for (int i = 0; i<msg_count; ++i) {
        // Prepare next sample
        sprintf(number, "foo %d", i);
        message.id = CORBA::string_dup(number);
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

      std::cout << "Waiting for acks from sub" << std::endl;
    }
  } catch (const CORBA::Exception& e) {
    e._tao_print_exception("Exception caught in main():");
    return -1;
  } catch (std::runtime_error& err) {
    ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("ERROR: main() - %s\n"),
                      err.what()), -1);
  } catch (std::string& msg) {
    ACE_ERROR_RETURN((LM_ERROR, ACE_TEXT("ERROR: main() - %s\n"),
                      msg.c_str()), -1);
  }

  // Clean-up!
  cleanup(participant, dpf);

  return 0;
}
