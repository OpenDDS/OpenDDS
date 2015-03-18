
#include <ace/Log_Msg.h>
#include <ace/ARGV.h>

#include <dds/DCPS/WaitSet.h>

#ifdef ACE_AS_STATIC_LIBS
#include <dds/DCPS/transport/tcp/Tcp.h>
#endif

#include "model/MultiInstanceTraits.h"
#include <model/Sync.h>

template <class ModelType>
int run_instance(ModelType& model, int subject_id) {
    using OpenDDS::Model::MultiInstance::Elements;

    DDS::DataWriter_var writer = model.writer( Elements::DataWriters::writer);

    // START OF EXISTING MESSENGER EXAMPLE CODE

    data1::MessageDataWriter_var message_writer =
      data1::MessageDataWriter::_narrow(writer.in());

    if (CORBA::is_nil(message_writer.in())) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: %N:%l: run_instance() -")
                          ACE_TEXT(" _narrow failed!\n")),
                         -1);
    }
    {
      OpenDDS::Model::WriterSync ws(writer);
      {
        // Write samples
        data1::Message message;
        message.subject_id = subject_id;

        message.from       = CORBA::string_dup("Comic Book Guy");
        message.subject    = CORBA::string_dup("Review");
        message.text       = CORBA::string_dup("Worst. Movie. Ever.");
        message.count      = 0;

        for (int i = 0; i < 10; i++) {
          DDS::ReturnCode_t error = message_writer->write(message, DDS::HANDLE_NIL);
          ++message.count;

          if (error != DDS::RETCODE_OK) {
            ACE_ERROR((LM_ERROR,
                       ACE_TEXT("(%P|%t) ERROR: %N:%l: run_instance() -")
                       ACE_TEXT(" write returned %d!\n"), error));
          }
        }
      }
    }
  OpenDDS::Model::WriterSync::wait_unmatch(writer);
  return 0;
}

int ACE_TMAIN(int argc, ACE_TCHAR** argv)
{
  int result;
  ACE_ARGV argv_copy(argc, argv);
  ACE_ARGV argv_copy2(argc, argv);
  try {
    OpenDDS::Model::Application application(argc, argv);
    {
      std::cout << "Creating primary publisher instance" << std::endl;
      MultiInstance::PrimaryMultiInstanceType primary_model(application,
                                                            argc,
                                                            argv_copy.argv());
      std::cout << "Running primary publisher instance" << std::endl;
      result = run_instance(primary_model, 86);
      std::cout << "Primary publisher instance complete" << std::endl;
    }
    if (!result) {
      int argc_copy = argv_copy.argc();
      std::cout << "Creating secondary publisher instance" << std::endl;
      MultiInstance::SecondaryMultiInstanceType secondary_model(application,
                                                                argc_copy,
                                                                argv_copy2.argv());
      std::cout << "Running secondary publisher instance" << std::endl;
      result = run_instance(secondary_model, 99);
      std::cout << "Secondary publisher instance complete" << std::endl;
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
  std::cout << "Publisher exiting" << std::endl;
  return result;
}

