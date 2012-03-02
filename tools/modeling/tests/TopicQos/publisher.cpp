
#include <ace/Log_Msg.h>

#include <dds/DCPS/WaitSet.h>

#ifdef ACE_AS_STATIC_LIBS
#include <dds/DCPS/transport/tcp/Tcp.h>
#endif

#include "model/TopQosTraits.h"
#include <model/Sync.h>

int ACE_TMAIN(int argc, ACE_TCHAR** argv)
{
  try {
    OpenDDS::Model::Application application(argc, argv);
    TopQos::DefaultTopQosType model(application, argc, argv);

    using OpenDDS::Model::TopQos::Elements;

    DDS::DataWriter_var writer = model.writer( Elements::DataWriters::writer);

    // START OF EXISTING MESSENGER EXAMPLE CODE

    data1::MessageDataWriter_var message_writer =
      data1::MessageDataWriter::_narrow(writer.in());

    DDS::Topic_var topic = message_writer->get_topic();
    DDS::TopicQos topic_qos;

    if (CORBA::is_nil(message_writer.in())) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: %N:%l: main() -")
                          ACE_TEXT(" _narrow failed!\n")),
                         -1);
    }

    if (topic->get_qos(topic_qos) != 0) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: %N:%l: main() -")
                          ACE_TEXT(" get_qos failed!\n")),
                         -1);
    }
    if (topic_qos.deadline.period.sec != 1) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: %N:%l: main() -")
                          ACE_TEXT(" wrong deadline!\n")),
                         -1);
    }
    if (topic_qos.deadline.period.nanosec != 25) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: %N:%l: main() -")
                          ACE_TEXT(" wrong deadline!\n")),
                         -1);
    }
    if (topic_qos.destination_order.kind !=
            DDS::BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: %N:%l: main() -")
                          ACE_TEXT(" wrong destination order!\n")),
                         -1);
    }
    if (topic_qos.durability.kind != DDS::TRANSIENT_LOCAL_DURABILITY_QOS) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: %N:%l: main() -")
                          ACE_TEXT(" wrong durability !\n")),
                         -1);
    }
    if (topic_qos.durability_service.history_kind != DDS::KEEP_LAST_HISTORY_QOS) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: %N:%l: main() -")
                          ACE_TEXT(" wrong durability_service history_kind\n")),
                         -1);
    }
    if (topic_qos.durability_service.history_depth != 5) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: %N:%l: main() -")
                          ACE_TEXT(" wrong durability_service history_depth\n")),
                         -1);
    }
    if (topic_qos.durability_service.max_instances != 20) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: %N:%l: main() -")
                          ACE_TEXT(" wrong durability_service max_instances\n")),
                         -1);
    }
    if (topic_qos.durability_service.max_samples != 500) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: %N:%l: main() -")
                          ACE_TEXT(" wrong durability_service max_samples\n")),
                         -1);
    }
    if (topic_qos.durability_service.max_samples_per_instance != 100) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: %N:%l: main() -")
                          ACE_TEXT(" wrong durability_service max_samples_per_instance\n")),
                         -1);
    }
    if (topic_qos.history.kind != DDS::KEEP_LAST_HISTORY_QOS) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: %N:%l: main() -")
                          ACE_TEXT(" wrong history kind\n")),
                         -1);
    }
    if (topic_qos.history.depth != 14) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: %N:%l: main() -")
                          ACE_TEXT(" wrong history depth\n")),
                         -1);
    }
    if (topic_qos.latency_budget.duration.sec != 1) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: %N:%l: main() -")
                          ACE_TEXT(" wrong latency budget!\n")),
                         -1);
    }
    if (topic_qos.latency_budget.duration.nanosec != 10) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: %N:%l: main() -")
                          ACE_TEXT(" wrong latency budget!\n")),
                         -1);
    }
    if (topic_qos.lifespan.duration.sec != 8) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: %N:%l: main() -")
                          ACE_TEXT(" wrong lifespan!\n")),
                         -1);
    }
    if (topic_qos.lifespan.duration.nanosec != 7) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: %N:%l: main() -")
                          ACE_TEXT(" wrong lifespan!\n")),
                         -1);
    }
    if (topic_qos.liveliness.kind != DDS::MANUAL_BY_TOPIC_LIVELINESS_QOS) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: %N:%l: main() -")
                          ACE_TEXT(" wrong liveliness!\n")),
                         -1);
    }
    if (topic_qos.ownership.kind != DDS::EXCLUSIVE_OWNERSHIP_QOS) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: %N:%l: main() -")
                          ACE_TEXT(" wrong ownership!\n")),
                         -1);
    }
    if (topic_qos.reliability.kind != DDS::RELIABLE_RELIABILITY_QOS) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: %N:%l: main() -")
                          ACE_TEXT(" wrong reliability\n")),
                         -1);
    }
    if (topic_qos.resource_limits.max_instances != 20) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: %N:%l: main() -")
                          ACE_TEXT(" wrong resource_limits\n")),
                         -1);
    }
    if (topic_qos.resource_limits.max_samples != 1000) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: %N:%l: main() -")
                          ACE_TEXT(" wrong resource_limits\n")),
                         -1);
    }
    if (topic_qos.resource_limits.max_samples_per_instance != 100) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: %N:%l: main() -")
                          ACE_TEXT(" wrong resource_limits\n")),
                         -1);
    }
    if (topic_qos.transport_priority.value != 3) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: %N:%l: main() -")
                          ACE_TEXT(" wrong transport priority\n")),
                         -1);
    }
    char* buff = reinterpret_cast<char*>(topic_qos.topic_data.value.get_buffer());
    std::cout << "User data is:" << buff << std::endl;
    if (strcmp(buff, "nine is 9") != 0) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: %N:%l: main() -")
                          ACE_TEXT(" writer has wrong user_data value\n")),
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

