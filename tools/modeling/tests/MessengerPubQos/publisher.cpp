
#include <ace/Log_Msg.h>

#include <dds/DCPS/WaitSet.h>

#ifdef ACE_AS_STATIC_LIBS
#include <dds/DCPS/transport/tcp/Tcp.h>
#endif

#include "model/MessengerPubQosTraits.h"
#include <model/Sync.h>

int ACE_TMAIN(int argc, ACE_TCHAR** argv)
{
  try {
    OpenDDS::Model::Application application(argc, argv);
    MessengerPubQos::DefaultMessengerPubQosType model(application, argc, argv);

    using OpenDDS::Model::MessengerPubQos::Elements;

    DDS::DataWriter_var writer = model.writer( Elements::DataWriters::writer);
    DDS::Publisher_var publisher = writer->get_publisher();

    DDS::PublisherQos pub_qos;

    // START OF EXISTING MESSENGER EXAMPLE CODE

    data1::MessageDataWriter_var message_writer =
      data1::MessageDataWriter::_narrow(writer.in());

    if (CORBA::is_nil(message_writer.in())) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: %N:%l: main() -")
                          ACE_TEXT(" _narrow failed!\n")),
                         -1);
    }

    if (publisher->get_qos(pub_qos) != 0) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: %N:%l: main() -")
                          ACE_TEXT(" get_qos failed!\n")),
                         -1);
    }

    // was set to false for Pub
    if (pub_qos.entity_factory.autoenable_created_entities) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: %N:%l: main() -")
                          ACE_TEXT(" publisher has wrong autoenable value!\n")),
                         -1);
    } else {
      if (publisher->enable() != DDS::RETCODE_OK) {
        std::cout << "bad return code enabling publisher" << std::endl;
      }
      if (writer->enable() != DDS::RETCODE_OK) {
        std::cout << "bad return code enabling writer" << std::endl;
      }
    }

    char* buff = reinterpret_cast<char*>(pub_qos.group_data.value.get_buffer());
    std::cout << "Group data is:" << buff << std::endl;
    if (strcmp(buff, "eight is 8") != 0) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: %N:%l: main() -")
                          ACE_TEXT(" publisher has wrong group_data value\n")),
                         -1);
    }

    if (pub_qos.partition.name.length() != 2) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: %N:%l: main() -")
                          ACE_TEXT(" publisher has wrong # of partitions\n")),
                         -1);
    }

    if (strcmp(pub_qos.partition.name[0], "left") != 0) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: %N:%l: main() -")
                          ACE_TEXT(" publisher has wrong partition value\n")),
                         -1);
    }
    if (strcmp(pub_qos.partition.name[1], "right") != 0) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: %N:%l: main() -")
                          ACE_TEXT(" publisher has wrong partition value\n")),
                         -1);
    }

    if (pub_qos.presentation.access_scope != DDS::TOPIC_PRESENTATION_QOS) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: %N:%l: main() -")
                          ACE_TEXT(" publisher has wrong access scope\n")),
                         -1);
    }
    if (pub_qos.presentation.coherent_access != true) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: %N:%l: main() -")
                          ACE_TEXT(" publisher has wrong choerent access\n")),
                         -1);
    }
    if (pub_qos.presentation.ordered_access != true) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: %N:%l: main() -")
                          ACE_TEXT(" publisher has wrong ordered access\n")),
                         -1);
    }

    OpenDDS::Model::WriterSync ws(writer);
    {
      // Write samples
      data1::Message message;
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

