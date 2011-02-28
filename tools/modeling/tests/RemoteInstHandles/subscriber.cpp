
#include <model/Sync.h>
#include <ace/Log_Msg.h>

#include <dds/DCPS/WaitSet.h>
#include <dds/DCPS/BuiltInTopicUtils.h>

#ifdef ACE_AS_STATIC_LIBS
#include <dds/DCPS/transport/simpleTCP/SimpleTcp.h>
#endif

#include "model/RemoteInstHandlesTraits.h"
#include <model/NullReaderListener.h>
#include <model/Sync.h>

class ReaderListener : public OpenDDS::Model::NullReaderListener {
  public:
    ReaderListener(OpenDDS::Model::ReaderCondSync& rcs) : rcs_(rcs) {}
    virtual void on_data_available(DDS::DataReader_ptr reader)
        ACE_THROW_SPEC((CORBA::SystemException));
    virtual void on_subscription_matched(DDS::DataReader_ptr reader,
                     const ::DDS::SubscriptionMatchedStatus & status)
        ACE_THROW_SPEC((CORBA::SystemException));
  private:
    OpenDDS::Model::ReaderCondSync& rcs_;

};

// START OF EXISTING MESSENGER EXAMPLE LISTENER CODE

void
ReaderListener::on_data_available(DDS::DataReader_ptr reader)
ACE_THROW_SPEC((CORBA::SystemException))
{
  data1::MessageDataReader_var reader_i =
    data1::MessageDataReader::_narrow(reader);

  if (CORBA::is_nil(reader_i.in())) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("ERROR: %N:%l: on_data_available() -")
               ACE_TEXT(" _narrow failed!\n")));
    ACE_OS::exit(-1);
  }

  data1::Message msg;
  DDS::SampleInfo info;

  while (true) {
    DDS::ReturnCode_t error = reader_i->take_next_sample(msg, info);
    if (error == DDS::RETCODE_OK) {
      std::cout << "SampleInfo.sample_rank = " << info.sample_rank << std::endl;
      std::cout << "SampleInfo.instance_state = " << info.instance_state << std::endl;

      if (info.valid_data) {
        std::cout << "Message: subject    = " << msg.subject.in() << std::endl
                  << "         subject_id = " << msg.subject_id   << std::endl
                  << "         from       = " << msg.from.in()    << std::endl
                  << "         count      = " << msg.count        << std::endl
                  << "         text       = " << msg.text.in()    << std::endl;
        if (msg.count == 9) {
          rcs_.signal();
        }
      }
    } else {
      if (error != DDS::RETCODE_NO_DATA) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("ERROR: %N:%l: on_data_available() -")
                   ACE_TEXT(" take_next_sample failed!\n")));
      }
      break;
    }
  }
}

void 
ReaderListener::on_subscription_matched(DDS::DataReader_ptr reader,
                    const ::DDS::SubscriptionMatchedStatus & status)
                             ACE_THROW_SPEC((CORBA::SystemException))
{
/*
  std::cout << "on_subscription_matched" << std::endl;
  DDS::InstanceHandle_t bitHandle = status.last_publication_handle;
  DDS::PublicationBuiltinTopicData bitData;
  if (reader->get_matched_publication_data(bitData, bitHandle) == DDS::RETCODE_OK) {
    DDS::BuiltinTopicKey_t key =  bitData.key;
    std::cout << "subscriber: found participant key[0] " << key.value[0] << std::endl;
    std::cout << "subscriber: found participant key[1] " << key.value[1] << std::endl;
    std::cout << "subscriber: found participant key[2] " << key.value[2] << std::endl;
    DDS::BuiltinTopicKey_t participant_key =  bitData.participant_key;
    std::cout << "subscriber: found remote participant key[0] " << participant_key.value[0] << std::endl;
    std::cout << "subscriber: found remote participant key[1] " << participant_key.value[1] << std::endl;
    std::cout << "subscriber: found remote participant key[2] " << participant_key.value[2] << std::endl;
  }
    DDS::DomainParticipant_var reader_part = reader->get_subscriber()->get_participant();
    DDS::ParticipantBuiltinTopicData participant_data;
std::cout << "sub get_discovered_participant_data" << std::endl;
    DDS::ReturnCode_t error = reader_part->get_discovered_participant_data(
      participant_data,
      reader->get_instance_handle());
    if (error == DDS::RETCODE_OK) {
std::cout << "sub get_matched_subscription_data" << std::endl;
      DDS::BuiltinTopicKey_t key = participant_data.key;
      //if (reader->get_matched_subscription_data(sub_data, participant_data.instance_handle)== DDS::RETCODE_OK) {
        //DDS::BuiltinTopicKey_t key =  sub_data.participant_key;
        std::cout << "subscriber: my handle[0] " << key.value[0] << std::endl;
        std::cout << "subscriber: my handle[1] " << key.value[1] << std::endl;
        std::cout << "subscriber: my handle[2] " << key.value[2] << std::endl;
      //}
    } else {
      std::cout << "subscriber error " << error << " in get_discovered_participant_data" << std::endl;
    }
*/

}

// END OF EXISTING MESSENGER EXAMPLE LISTENER CODE

int ACE_TMAIN(int argc, ACE_TCHAR** argv)
{
  DDS::ReturnCode_t error;
  try {
    OpenDDS::Model::Application application(argc, argv);
    RemoteInstHandlesLib::DefaultRemoteInstHandlesType model(application, argc, argv);

    using OpenDDS::Model::RemoteInstHandlesLib::Elements;

    DDS::DataReader_var reader = model.reader(Elements::DataReaders::reader);
    DDS::DomainParticipant_var reader_part = reader->get_subscriber()->get_participant();

    DDS::Subscriber_var bit_sub(reader_part->get_builtin_subscriber());
    DDS::DataReader_var bit_dr(bit_sub->lookup_datareader(OpenDDS::DCPS::BUILT_IN_SUBSCRIPTION_TOPIC));
    DDS::SubscriptionBuiltinTopicDataDataReader_var bit_reader_i =
          DDS::SubscriptionBuiltinTopicDataDataReader::_narrow(bit_dr);
    DDS::SubscriptionBuiltinTopicData instance;
    DDS::SampleInfo info;
    error = bit_reader_i->read_next_sample(instance, info);
    if (error == DDS::RETCODE_OK) {
      std::cout << "subscriber bit sample" << std::endl;
      DDS::BuiltinTopicKey_t mykey = instance.participant_key;
      std::cout << "subscriber bit sample[0] = " << mykey.value[0] << std::endl;
      std::cout << "subscriber bit sample[1] = " << mykey.value[1] << std::endl;
      std::cout << "subscriber bit sample[2] = " << mykey.value[2] << std::endl;
    } else {
      std::cout << "subscriber bit sample error = " << error << std::endl;
    }

    
    ACE_SYNCH_MUTEX lock;
    ACE_Condition<ACE_SYNCH_MUTEX> condition(lock);
    OpenDDS::Model::ReaderCondSync rcs(reader, condition);
    DDS::DataReaderListener_var listener(new ReaderListener(rcs));
    reader->set_listener( listener.in(), OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    // Call on_data_available in case there are samples which are waiting
    listener->on_data_available(reader);

    // START OF EXISTING MESSENGER EXAMPLE CODE

    data1::MessageDataReader_var reader_i =
      data1::MessageDataReader::_narrow(reader);

    if (CORBA::is_nil(reader_i.in())) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("ERROR: %N:%l: main() -")
                        ACE_TEXT(" _narrow failed!\n")),
                       -1);
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
