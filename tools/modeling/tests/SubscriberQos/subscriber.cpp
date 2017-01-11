
#include <ace/Log_Msg.h>

#include <dds/DCPS/WaitSet.h>

#ifdef ACE_AS_STATIC_LIBS
#include <dds/DCPS/transport/tcp/Tcp.h>
#endif

#include "model/SubQosTraits.h"
#include <model/NullReaderListener.h>
#include <model/Sync.h>

class ReaderListener : public OpenDDS::Model::NullReaderListener {
  public:
    ReaderListener(OpenDDS::Model::ReaderCondSync& rcs) : rcs_(rcs) {}
    virtual void on_data_available(DDS::DataReader_ptr reader);
  private:
    OpenDDS::Model::ReaderCondSync& rcs_;

};

// START OF EXISTING MESSENGER EXAMPLE LISTENER CODE

void
ReaderListener::on_data_available(DDS::DataReader_ptr reader)
{
  MessageDataReader_var reader_i =
    MessageDataReader::_narrow(reader);

  if (CORBA::is_nil(reader_i.in())) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("ERROR: %N:%l: on_data_available() -")
               ACE_TEXT(" _narrow failed!\n")));
    ACE_OS::exit(-1);
  }

  Message msg;
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

// END OF EXISTING MESSENGER EXAMPLE LISTENER CODE

int ACE_TMAIN(int argc, ACE_TCHAR** argv)
{
  try {
    OpenDDS::Model::Application application(argc, argv);
    SubQos::DefaultSubQosType model(application, argc, argv);

    using OpenDDS::Model::SubQos::Elements;

    DDS::DataReader_var reader = model.reader( Elements::DataReaders::reader);
    DDS::Subscriber_var subscriber = reader->get_subscriber();

    ACE_SYNCH_MUTEX lock;
    ACE_Condition<ACE_SYNCH_MUTEX> condition(lock);
    OpenDDS::Model::ReaderCondSync rcs(reader, condition);
    DDS::DataReaderListener_var listener(new ReaderListener(rcs));
    reader->set_listener( listener.in(), OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    // Call on_data_available in case there are samples which are waiting
    listener->on_data_available(reader);

    // START OF EXISTING MESSENGER EXAMPLE CODE

    MessageDataReader_var reader_i =
      MessageDataReader::_narrow(reader);

    if (CORBA::is_nil(reader_i.in())) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("ERROR: %N:%l: main() -")
                        ACE_TEXT(" _narrow failed!\n")),
                       -1);
    }

    DDS::SubscriberQos sub_qos;

    if (subscriber->get_qos(sub_qos) != 0) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: %N:%l: main() -")
                          ACE_TEXT(" get_qos failed!\n")),
                         -1);
    }

    // was set to false for Sub
    if (sub_qos.entity_factory.autoenable_created_entities) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: %N:%l: main() -")
                          ACE_TEXT(" subscriber has wrong autoenable value!\n")),
                         -1);
    } else {
      if (subscriber->enable() != DDS::RETCODE_OK) {
        std::cout << "bad return code enabling subscriber" << std::endl;
      }
      if (reader->enable() != DDS::RETCODE_OK) {
        std::cout << "bad return code enabling reader" << std::endl;
      }
    }

    char* buff = reinterpret_cast<char*>(sub_qos.group_data.value.get_buffer());
    std::cout << "Group data is:" << buff << std::endl;
    if (strcmp(buff, "eight is 8") != 0) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: %N:%l: main() -")
                          ACE_TEXT(" subscriber has wrong group_data value\n")),
                         -1);
    }

    if (sub_qos.partition.name.length() != 1) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: %N:%l: main() -")
                          ACE_TEXT(" subscriber has wrong # of partitions\n")),
                         -1);
    }

    if (strcmp(sub_qos.partition.name[0], "*") != 0) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: %N:%l: main() -")
                          ACE_TEXT(" subscriber has wrong partition value\n")),
                         -1);
    }

    if (sub_qos.presentation.access_scope != DDS::TOPIC_PRESENTATION_QOS) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: %N:%l: main() -")
                          ACE_TEXT(" subscriber has wrong access scope\n")),
                         -1);
    }
    if (sub_qos.presentation.coherent_access != true) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: %N:%l: main() -")
                          ACE_TEXT(" subscriber has wrong choerent access\n")),
                         -1);
    }
    if (sub_qos.presentation.ordered_access != true) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: %N:%l: main() -")
                          ACE_TEXT(" subscriber has wrong ordered access\n")),
                         -1);
    }

    OpenDDS::Model::ReaderSync rs(reader);

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
