
#include <ace/Log_Msg.h>

#include <dds/DCPS/WaitSet.h>

#ifdef ACE_AS_STATIC_LIBS
#include <dds/DCPS/transport/tcp/Tcp.h>
#endif

#include "model/MessengerDpQosTraits.h"
#include <model/Sync.h>

int ACE_TMAIN(int argc, ACE_TCHAR** argv)
{
  try {
    OpenDDS::Model::Application application(argc, argv);
    MessengerDpQos::DefaultMessengerDpQosType model(application, argc, argv);

    using OpenDDS::Model::MessengerDpQos::Elements;

    DDS::DataWriter_var writer = model.writer( Elements::DataWriters::writer);
    DDS::Publisher_var publisher = writer->get_publisher();
    DDS::DomainParticipant_var participant = publisher->get_participant();

    DDS::DomainParticipantQos part_qos;

    // START OF EXISTING MESSENGER EXAMPLE CODE

    MessageDataWriter_var message_writer =
      MessageDataWriter::_narrow(writer.in());

    if (CORBA::is_nil(message_writer.in())) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: %N:%l: main() -")
                          ACE_TEXT(" _narrow failed!\n")),
                         -1);
    }

    if (participant->get_qos(part_qos) != 0) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: %N:%l: main() -")
                          ACE_TEXT(" get_qos failed!\n")),
                         -1);
    }
    // was set to false for DP
    if (part_qos.entity_factory.autoenable_created_entities) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: %N:%l: main() -")
                          ACE_TEXT(" participant has wrong autoenable value!\n")),
                         -1);
    } else {
      std::cout << "enabling publisher" << std::endl;
      if (participant->enable() != DDS::RETCODE_OK) {
        std::cout << "bad return code enabling participant" << std::endl;
      }
      if (publisher->enable() != DDS::RETCODE_OK) {
        std::cout << "bad return code enabling publisher" << std::endl;
      }
      if (writer->enable() != DDS::RETCODE_OK) {
        std::cout << "bad return code enabling writer" << std::endl;
      }
    }
    char* buff = reinterpret_cast<char*>(part_qos.user_data.value.get_buffer());
    std::cout << "User data is:" << buff << std::endl;
    if (strcmp(buff, "seven is 7") != 0) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: %N:%l: main() -")
                          ACE_TEXT(" participant has wrong user_data value!\n")),
                         -1);
    }

    OpenDDS::Model::WriterSync ws(writer);
    {
      // Write samples
      Message message;
      message.subject_id = 99;

      message.from       = CORBA::string_dup("Comic Book Guy");
      message.subject    = CORBA::string_dup("Review");
      message.text       = CORBA::string_dup("Worst. Movie. Ever.");
      message.count      = 0;

      for (int i = 0; i < 10; i++) {
        DDS::ReturnCode_t error = message_writer->write(message, DDS::HANDLE_NIL);
        ++message.count;

        if (error != DDS::RETCODE_OK) {
          ACE_ERROR((LM_ERROR,
                     ACE_TEXT("(%P|%t) ERROR: %N:%l: main() -")
                     ACE_TEXT(" write returned %d!\n"), error));
        }
      }
    }

    // END OF EXISTING MESSENGER EXAMPLE CODE
  } catch (const CORBA::Exception& e) {
    e._tao_print_exception("Exception caught in main():");
    return -1;

  } catch( const std::exception& ex) {
    ACE_ERROR_RETURN((LM_ERROR,
                      ACE_TEXT("(%P|%t) ERROR: %N:%l: main() -")
                      ACE_TEXT(" Exception caught: %C\n"),
                      ex.what()),
                     -1);
  }

  return 0;
}

