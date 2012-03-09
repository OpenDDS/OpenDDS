
#include <ace/Log_Msg.h>
#include <ace/Condition_T.h>

#include <dds/DCPS/WaitSet.h>
#include <dds/DCPS/BuiltInTopicUtils.h>

#ifdef ACE_AS_STATIC_LIBS
#include <dds/DCPS/transport/tcp/Tcp.h>
#endif

#include "model/RemoteInstHandlesTraits.h"
#include <model/NullReaderListener.h>
#include <model/Sync.h>

bool matched = false;
ACE_SYNCH_MUTEX lock;
ACE_Condition<ACE_SYNCH_MUTEX> condition(lock);
::data1::BitKey sub_internal_key;
::data1::BitKey pub_external_key;

class BitSubscriptionListener : public OpenDDS::Model::NullReaderListener {
public:
  virtual void on_data_available(DDS::DataReader_ptr reader);
};

void
BitSubscriptionListener::on_data_available(DDS::DataReader_ptr reader)
{
  std::cout << "sub got bit data" << std::endl;
  DDS::SubscriptionBuiltinTopicDataDataReader_var bit_reader_i =
        DDS::SubscriptionBuiltinTopicDataDataReader::_narrow(reader);
  DDS::SubscriptionBuiltinTopicData msg;
  DDS::SampleInfo info;
  DDS::ReturnCode_t error = bit_reader_i->read_next_sample(msg, info);
  if (error == DDS::RETCODE_OK) {
    ACE_GUARD(ACE_SYNCH_MUTEX, conditionGuard, condition.mutex());
    matched = true;
    sub_internal_key[0] = msg.participant_key.value[0];
    sub_internal_key[1] = msg.participant_key.value[1];
    sub_internal_key[2] = msg.participant_key.value[2];
    condition.broadcast();
  } else if (error != DDS::RETCODE_NO_DATA) {
    std::cout << "take next sample - error " << error << std::endl;
  }
}

class ReaderListener : public OpenDDS::Model::NullReaderListener {
public:
  ReaderListener(OpenDDS::Model::ReaderCondSync& rcs) : rcs_(rcs) {}
  virtual void on_data_available(DDS::DataReader_ptr reader);
private:
  OpenDDS::Model::ReaderCondSync& rcs_;
  DDS::DomainParticipant_var reader_part_;
  DDS::BuiltinTopicKey_t subkey_;
};

// START OF EXISTING MESSENGER EXAMPLE LISTENER CODE

void
ReaderListener::on_data_available(DDS::DataReader_ptr reader)
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
        pub_external_key[0] = msg.subscriber_key[0];
        pub_external_key[1] = msg.subscriber_key[1];
        pub_external_key[2] = msg.subscriber_key[2];
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

// END OF EXISTING MESSENGER EXAMPLE LISTENER CODE

int ACE_TMAIN(int argc, ACE_TCHAR** argv)
{
  try {
    std::cout << "sub running" << std::endl;
    OpenDDS::Model::Application application(argc, argv);
    RemoteInstHandlesLib::DefaultRemoteInstHandlesType model(application, argc, argv);

    using OpenDDS::Model::RemoteInstHandlesLib::Elements;

    DDS::DataReader_var reader = model.reader(Elements::DataReaders::reader);
    DDS::DataReaderListener_var bit_listener(new BitSubscriptionListener());
    DDS::DomainParticipant_var reader_part = reader->get_subscriber()->get_participant();

    DDS::Subscriber_var bit_sub(reader_part->get_builtin_subscriber());
    DDS::DataReader_var bit_dr(bit_sub->lookup_datareader(OpenDDS::DCPS::BUILT_IN_SUBSCRIPTION_TOPIC));
    bit_dr->set_listener(bit_listener.in(), OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    std::cout << "sub calling synthetic oda" << std::endl;
    if (bit_dr->enable() != DDS::RETCODE_OK) {
      std::cout << "bit enable failure" << std::endl;
    }
    bit_listener->on_data_available(bit_dr);

    ACE_SYNCH_MUTEX lock;
    ACE_Condition<ACE_SYNCH_MUTEX> condition(lock);
    OpenDDS::Model::ReaderCondSync rcs(reader,
                                       condition);
    DDS::DataReaderListener_var listener(new ReaderListener(rcs));
    reader->set_listener( listener.in(), OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    // Call on_data_available in case there are samples which are waiting
    if (reader->enable() != DDS::RETCODE_OK) {
      std::cout << "reader enable failure" << std::endl;
    }
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
    std::cout << "sub leaving scope" << std::endl;

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

  std::cout << "sub waiting for match" << std::endl;
  ACE_GUARD_RETURN(ACE_SYNCH_MUTEX, conditionGuard, condition.mutex(), -1);
  while (!matched) {
    condition.wait();
  }

  if ((sub_internal_key[0] != pub_external_key[0])  ||
      (sub_internal_key[1] != pub_external_key[1])  ||
      (sub_internal_key[2] != pub_external_key[2])) {
    std::cout << "Subscriber Error: "
              << sub_internal_key[0] << "."
              << sub_internal_key[1] << "."
              << sub_internal_key[2] << " does not match "
              << pub_external_key[0] << "."
              << pub_external_key[1] << "."
              << pub_external_key[2] << std::endl;
    return -1;
  } else {
    std::cout << "Subscriber Match: "
              << sub_internal_key[0] << "."
              << sub_internal_key[1] << "."
              << sub_internal_key[2] << " matches "
              << pub_external_key[0] << "."
              << pub_external_key[1] << "."
              << pub_external_key[2] << std::endl;
  }

  return 0;
}
