
#include <ace/Log_Msg.h>

#include <dds/DCPS/WaitSet.h>

#ifdef ACE_AS_STATIC_LIBS
#include <dds/DCPS/transport/tcp/Tcp.h>
#endif

#include "model/ReaderQosTraits.h"
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
    ReaderQos::DefaultReaderQosType model(application, argc, argv);

    using OpenDDS::Model::ReaderQos::Elements;

    DDS::DataReader_var reader = model.reader( Elements::DataReaders::reader);
    DDS::DataReaderQos reader_qos;

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

    if (reader->get_qos(reader_qos) != 0) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: %N:%l: main() -")
                          ACE_TEXT(" get_qos failed!\n")),
                         -1);
    }

    if (reader_qos.deadline.period.sec != 2) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: %N:%l: main() -")
                          ACE_TEXT(" wrong deadline!\n")),
                         -1);
    }
    if (reader_qos.deadline.period.nanosec != 4) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: %N:%l: main() -")
                          ACE_TEXT(" wrong deadline!\n")),
                         -1);
    }
    if (reader_qos.destination_order.kind !=
            DDS::BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: %N:%l: main() -")
                          ACE_TEXT(" wrong destination order!\n")),
                         -1);
    }
    if (reader_qos.durability.kind != DDS::PERSISTENT_DURABILITY_QOS) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: %N:%l: main() -")
                          ACE_TEXT(" wrong durability !\n")),
                         -1);
    }
    if (reader_qos.history.kind != DDS::KEEP_LAST_HISTORY_QOS) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: %N:%l: main() -")
                          ACE_TEXT(" wrong history kind\n")),
                         -1);
    }
    if (reader_qos.history.depth != 14) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: %N:%l: main() -")
                          ACE_TEXT(" wrong history depth\n")),
                         -1);
    }
    if (reader_qos.latency_budget.duration.sec != 1) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: %N:%l: main() -")
                          ACE_TEXT(" wrong latency budget!\n")),
                         -1);
    }
    if (reader_qos.latency_budget.duration.nanosec != 10) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: %N:%l: main() -")
                          ACE_TEXT(" wrong latency budget!\n")),
                         -1);
    }
    if (reader_qos.liveliness.kind != DDS::MANUAL_BY_TOPIC_LIVELINESS_QOS) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: %N:%l: main() -")
                          ACE_TEXT(" wrong liveliness!\n")),
                         -1);
    }
    if (reader_qos.ownership.kind != DDS::EXCLUSIVE_OWNERSHIP_QOS) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: %N:%l: main() -")
                          ACE_TEXT(" wrong ownership!\n")),
                         -1);
    }
    if (reader_qos.reader_data_lifecycle.autopurge_nowriter_samples_delay.sec != 12) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: %N:%l: main() -")
                          ACE_TEXT(" wrong rdl!\n")),
                         -1);
    }
    if (reader_qos.reader_data_lifecycle.autopurge_nowriter_samples_delay.nanosec != 11) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: %N:%l: main() -")
                          ACE_TEXT(" wrong rdl!\n")),
                         -1);
    }
    if (reader_qos.reader_data_lifecycle.autopurge_disposed_samples_delay.sec != 14) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: %N:%l: main() -")
                          ACE_TEXT(" wrong rdl!\n")),
                         -1);
    }
    if (reader_qos.reader_data_lifecycle.autopurge_disposed_samples_delay.nanosec != 13) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: %N:%l: main() -")
                          ACE_TEXT(" wrong rdl!\n")),
                         -1);
    }
    if (reader_qos.time_based_filter.minimum_separation.sec != 0) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: %N:%l: main() -")
                          ACE_TEXT(" wrong rdl!\n")),
                         -1);
    }
    if (reader_qos.time_based_filter.minimum_separation.nanosec != 3) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: %N:%l: main() -")
                          ACE_TEXT(" wrong rdl!\n")),
                         -1);
    }
    char* buff = reinterpret_cast<char*>(reader_qos.user_data.value.get_buffer());
    std::cout << "User data is:" << buff << std::endl;
    if (strcmp(buff, "seven is 7") != 0) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: %N:%l: main() -")
                          ACE_TEXT(" reader has wrong user_data value\n")),
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
