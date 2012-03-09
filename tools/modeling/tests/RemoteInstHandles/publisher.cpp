
#include <ace/Log_Msg.h>
#include <ace/Condition_T.h>

#include <dds/DCPS/WaitSet.h>

#ifdef ACE_AS_STATIC_LIBS
#include <dds/DCPS/transport/tcp/Tcp.h>
#endif

#include "model/RemoteInstHandlesTraits.h"
#include <model/NullWriterListener.h>
#include <model/Sync.h>

bool matched = false;
ACE_SYNCH_MUTEX lock;
ACE_Condition<ACE_SYNCH_MUTEX> condition(lock);
data1::Message message;

namespace RIH {
  class WriterListener : public OpenDDS::Model::NullWriterListener {
    virtual void on_publication_matched(DDS::DataWriter_ptr writer,
                    const ::DDS::PublicationMatchedStatus & status);
  };
};

void RIH::WriterListener::on_publication_matched(DDS::DataWriter_ptr writer,
                             const ::DDS::PublicationMatchedStatus & status)
{
  std::cout << "pub match" << std::endl;
  if (status.current_count_change == 1) {
    DDS::InstanceHandle_t bitHandle = status.last_subscription_handle;
    DDS::SubscriptionBuiltinTopicData bitData;
    if (writer->get_matched_subscription_data(bitData, bitHandle) == DDS::RETCODE_OK) {
      DDS::BuiltinTopicKey_t key =  bitData.key;
      DDS::BuiltinTopicKey_t participant_key =  bitData.participant_key;
      std::cout << "publisher: found remote participant key[0] " << participant_key.value[0] << std::endl;
      std::cout << "publisher: found remote participant key[1] " << participant_key.value[1] << std::endl;
      std::cout << "publisher: found remote participant key[2] " << participant_key.value[2] << std::endl;
      message.subscriber_key[0] = participant_key.value[0];
      message.subscriber_key[1] = participant_key.value[1];
      message.subscriber_key[2] = participant_key.value[2];

      ACE_GUARD(ACE_SYNCH_MUTEX, conditionGuard, condition.mutex());
      matched = true;
      condition.broadcast();
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
    writer->set_listener(listener.in(), OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (writer->enable() != DDS::RETCODE_OK) {
      std::cout << "writer enable failure" << std::endl;
    }


    // START OF EXISTING MESSENGER EXAMPLE CODE
    std::cout << "pub waiting for sync" << std::endl;
    OpenDDS::Model::WriterSync ws(writer);
    {
      std::cout << "pub sync'd" << std::endl;
      data1::MessageDataWriter_var message_writer =
        data1::MessageDataWriter::_narrow(writer);

      if (CORBA::is_nil(message_writer.in())) {
          ACE_ERROR((LM_ERROR,
                     ACE_TEXT("(%P|%t) ERROR: %N:%l: main() -")
                     ACE_TEXT(" _narrow failed!\n")));
          return -1;
      }

      std::cout << "pub waiting for match" << std::endl;
      ACE_GUARD_RETURN(ACE_SYNCH_MUTEX, conditionGuard, condition.mutex(), -1);
      while (!matched) {
        condition.wait();
      }
      // Write samples
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
      std::cout << "pub waiting for acks" << std::endl;
    }
    std::cout << "pub got acks" << std::endl;

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

  std::cout << "pub exiting" << std::endl;
  return 0;
}

