
#include <model/Sync.h>
#include <ace/Log_Msg.h>

#include <dds/DCPS/WaitSet.h>

#ifdef ACE_AS_STATIC_LIBS
#include <dds/DCPS/transport/simpleTCP/SimpleTcp.h>
#endif

#include "model/RemoteInstHandlesTraits.h"
#include <model/NullWriterListener.h>
#include <model/Sync.h>

namespace RIH {
  class WriterListener : public OpenDDS::Model::NullWriterListener {
    virtual void on_publication_matched(DDS::DataWriter_ptr writer,
                    const ::DDS::PublicationMatchedStatus & status)
        ACE_THROW_SPEC((CORBA::SystemException)); 
  };
};

void RIH::WriterListener::on_publication_matched(DDS::DataWriter_ptr writer,
                             const ::DDS::PublicationMatchedStatus & status)
ACE_THROW_SPEC((CORBA::SystemException))
{
  if (status.current_count_change == 1) {
    DDS::InstanceHandle_t bitHandle = status.last_subscription_handle;
    DDS::SubscriptionBuiltinTopicData bitData;
    if (writer->get_matched_subscription_data(bitData, bitHandle) == DDS::RETCODE_OK) {
      DDS::BuiltinTopicKey_t key =  bitData.key;
      DDS::BuiltinTopicKey_t participant_key =  bitData.participant_key;
      std::cout << "publisher: found remote participant key[0] " << participant_key.value[0] << std::endl;
      std::cout << "publisher: found remote participant key[1] " << participant_key.value[1] << std::endl;
      std::cout << "publisher: found remote participant key[2] " << participant_key.value[2] << std::endl;
    }
  }
}

int ACE_TMAIN(int argc, ACE_TCHAR** argv)
{
  try {
    OpenDDS::Model::Application application(argc, argv);
    RemoteInstHandlesLib::DefaultRemoteInstHandlesType model(application, argc, argv);

    using OpenDDS::Model::RemoteInstHandlesLib::Elements;

    DDS::DataWriter_var writer = model.writer(Elements::DataWriters::writer);
    DDS::DataWriterListener_var listener(new RIH::WriterListener);
    writer->set_listener( listener.in(), OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    // START OF EXISTING MESSENGER EXAMPLE CODE

    data1::MessageDataWriter_var message_writer =
      data1::MessageDataWriter::_narrow(writer.in());

    if (CORBA::is_nil(message_writer.in())) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: %N:%l: main() -")
                          ACE_TEXT(" _narrow failed!\n")),
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

