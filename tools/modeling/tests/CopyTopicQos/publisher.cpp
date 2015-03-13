#include <ace/Log_Msg.h>

#include <dds/DCPS/WaitSet.h>

#ifdef ACE_AS_STATIC_LIBS
#include <dds/DCPS/transport/tcp/Tcp.h>
#endif

#include "model/CopyTopicQosTraits.h"
#include <model/Sync.h>

int ACE_TMAIN(int argc, ACE_TCHAR** argv)
{
  try {
    OpenDDS::Model::Application application(argc, argv);
    CopyTopicQosLib::DefaultCopyTopicQosType model(application, argc, argv);

    using OpenDDS::Model::CopyTopicQosLib::Elements;

    // writer1  copy true
    // writer2 copy false
    DDS::DataWriter_var writer  = model.writer( Elements::DataWriters::writer1);
    DDS::DataWriter_var writer2 = model.writer( Elements::DataWriters::writer2);

    // START OF EXISTING MESSENGER EXAMPLE CODE

    data1::MessageDataWriter_var message_writer =
      data1::MessageDataWriter::_narrow(writer.in());

    if (CORBA::is_nil(message_writer.in())) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: %N:%l: main() -")
                          ACE_TEXT(" _narrow failed!\n")),
                         -1);
    }

    DDS::DataWriterQos writer_qos, writer2_qos;
    if (writer->get_qos(writer_qos) != 0) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: %N:%l: main() -")
                          ACE_TEXT(" get_qos failed!\n")),
                         -1);
    }
    if (writer2->get_qos(writer2_qos) != 0) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: %N:%l: main() -")
                          ACE_TEXT(" get_qos failed!\n")),
                         -1);
    }

    // Should get set when copy is true
    if (writer_qos.destination_order.kind != DDS::BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS) {
        std::cout << "publisher, wrong destintion order" << std::endl;
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: %N:%l: main() -")
                          ACE_TEXT(" get_qos gave wrong value!\n")),
                         -1);
    }

    // Should not get set when copy is false
    if (writer2_qos.destination_order.kind == DDS::BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS) {
        std::cout << "publisher, wrong destintion order" << std::endl;
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: %N:%l: main() -")
                          ACE_TEXT(" get_qos gave wrong value!\n")),
                         -1);
    }
    {
      OpenDDS::Model::WriterSync ws(writer);
      {
        // Write samples
        data1::Message message;
        message.subject_id = 99;

        message.from       = CORBA::string_dup("Comic Book Guy");
        message.subject    = CORBA::string_dup("Review");
        message.text       = CORBA::string_dup("Worst. Movie. Ever.");
        message.count      = 0;

        std::cout << "publisher sending"  << std::endl;
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
    }
    OpenDDS::Model::WriterSync::wait_unmatch(writer);

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

  std::cout << "publisher exiting" << std::endl;
  return 0;
}

