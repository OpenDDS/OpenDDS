#include "Writer.h"
#include "MessengerTypeSupportImpl.h"
#include "common.h"

#include <dds/DCPS/PublisherImpl.h>

using namespace std;

int parse_args(int argc, ACE_TCHAR *argv[])
{
  ACE_Get_Opt get_opts(argc, argv, "T:n:");
  int c;

  while ((c = get_opts()) != -1) {
    switch (c) {
    case 'T':
      synch_dir = get_opts.opt_arg();
      break;
    case 'n':
      num_messages = ACE_OS::atoi(get_opts.opt_arg());
      break;
    case '?':
    default:
      ACE_ERROR_RETURN((LM_ERROR,
        "usage:  %s "
        "-n <num of messages> "
        "-T <Where to look for monitor1_done>"
        "\n",
        argv[0]),
        -1);
    }
  }
  // Indicates successful parsing of the command line
  return 0;
}

int ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  try {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) publisher main\n"));

    DDS::DomainParticipantFactory_var dpf = TheParticipantFactoryWithArgs(argc, argv);
    if (parse_args(argc, argv) == -1) {
      return -1;
    }

    DDS::DomainParticipantQos partQos;
    dpf->get_default_participant_qos(partQos);

    // set up user data in DP qos
    CORBA::ULong part_user_data_len
      = static_cast<CORBA::ULong>(ACE_OS::strlen(PART_USER_DATA));
    partQos.user_data.value.length(part_user_data_len);
    partQos.user_data.value.replace(part_user_data_len,
                                    part_user_data_len,
                                    reinterpret_cast<CORBA::Octet*>(PART_USER_DATA));

    DDS::DomainParticipant_var participant =
      dpf->create_participant(111,
                              partQos,
                              DDS::DomainParticipantListener::_nil(),
                              ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (CORBA::is_nil(participant.in())) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) publisher: create_participant failed.")));
      return 1;
    }

    ::Messenger::MessageTypeSupport_var ts = new ::Messenger::MessageTypeSupportImpl();

    if (DDS::RETCODE_OK != ts->register_type(participant.in(), topic_type_name)) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) publisher: register_type failed.\n")));
      exit(1);
    }

    DDS::TopicQos topic_qos;
    participant->get_default_topic_qos(topic_qos);

    // set up topic data in topic qos
    CORBA::ULong topic_data_len = static_cast<CORBA::ULong>(ACE_OS::strlen(TOPIC_DATA));
    topic_qos.topic_data.value.length(topic_data_len);
    topic_qos.topic_data.value.replace(topic_data_len, topic_data_len, reinterpret_cast<CORBA::Octet*>(TOPIC_DATA));

    DDS::Topic_var topic = participant->create_topic(topic_name,
                                                     topic_type_name,
                                                     topic_qos,
                                                     DDS::TopicListener::_nil(),
                                                     ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (CORBA::is_nil(topic.in())) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) publisher: create_topic failed.\n")));
      exit(1);
    }

    DDS::PublisherQos pub_qos;
    participant->get_default_publisher_qos(pub_qos);

    // set up group data in group qos
    CORBA::ULong group_data_len = static_cast<CORBA::ULong>(ACE_OS::strlen(GROUP_DATA));
    pub_qos.group_data.value.length(group_data_len);
    pub_qos.group_data.value.replace(group_data_len, group_data_len, reinterpret_cast<CORBA::Octet*>(GROUP_DATA));

    DDS::Publisher_var pub = participant->create_publisher(pub_qos,
                                                           DDS::PublisherListener::_nil(),
                                                           ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (CORBA::is_nil(pub.in())) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) publisher: create_publisher failed.\n")));
      exit(1);
    }

    // Create the datawriter
    DDS::DataWriterQos dw_qos;
    pub->get_default_datawriter_qos(dw_qos);
    dw_qos.durability.kind = DDS::TRANSIENT_LOCAL_DURABILITY_QOS;
    dw_qos.reliability.kind = ::DDS::RELIABLE_RELIABILITY_QOS;
    dw_qos.resource_limits.max_samples_per_instance = 1000;
    dw_qos.history.kind = ::DDS::KEEP_ALL_HISTORY_QOS;
    dw_qos.representation.value.length(1);

#if defined(OPENDDS_SAFETY_PROFILE)
    dw_qos.representation.value[0] = DDS::XCDR2_DATA_REPRESENTATION;
#else
    dw_qos.representation.value[0] = OpenDDS::DCPS::UNALIGNED_CDR_DATA_REPRESENTATION;
#endif

    // set up user data in DW qos
    CORBA::ULong dw_user_data_len = static_cast<CORBA::ULong>(ACE_OS::strlen(DW_USER_DATA));
    dw_qos.user_data.value.length(dw_user_data_len);
    dw_qos.user_data.value.replace(dw_user_data_len,
                                   dw_user_data_len,
                                   reinterpret_cast<CORBA::Octet*>(DW_USER_DATA));

    DDS::DataWriter_var dw = pub->create_datawriter(topic.in(),
                                                    dw_qos,
                                                    DDS::DataWriterListener::_nil(),
                                                    ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (CORBA::is_nil(dw.in())) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) publisher: create_datawriter failed.\n")));
      exit(1);
    }

    // wait for Monitor 1 done
    FILE* fp = ACE_OS::fopen((synch_dir + mon1_fname).c_str(), ACE_TEXT("r"));
    while (fp == 0) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) waiting monitor1 done ...\n")));
      ACE_OS::sleep(1);
      fp = ACE_OS::fopen((synch_dir + mon1_fname).c_str(), ACE_TEXT("r"));
    }
    if (fp != 0) {
      ACE_OS::fclose(fp);
    }

    // Now change the changeable qos. The second monitor should get the updated qos from BIT.
    part_user_data_len = static_cast<CORBA::ULong>(ACE_OS::strlen(UPDATED_PART_USER_DATA));
    partQos.user_data.value.length(part_user_data_len);
    partQos.user_data.value.replace(part_user_data_len,
                                    part_user_data_len,
                                    reinterpret_cast<CORBA::Octet*>(UPDATED_PART_USER_DATA));
    participant->set_qos(partQos);

    dw_user_data_len = static_cast<CORBA::ULong>(ACE_OS::strlen(UPDATED_DW_USER_DATA));
    dw_qos.user_data.value.length(dw_user_data_len);
    dw_qos.user_data.value.replace(dw_user_data_len,
                                   dw_user_data_len,
                                   reinterpret_cast<CORBA::Octet*>(UPDATED_DW_USER_DATA));
    dw->set_qos(dw_qos);

    group_data_len = static_cast<CORBA::ULong>(ACE_OS::strlen(UPDATED_GROUP_DATA));
    pub_qos.group_data.value.length(group_data_len);
    pub_qos.group_data.value.replace(group_data_len,
                                     group_data_len,
                                     reinterpret_cast<CORBA::Octet*>(UPDATED_GROUP_DATA));
    pub->set_qos(pub_qos);

    topic_data_len = static_cast<CORBA::ULong>(ACE_OS::strlen(UPDATED_TOPIC_DATA));
    topic_qos.topic_data.value.length(topic_data_len);
    topic_qos.topic_data.value.replace(topic_data_len,
                                       topic_data_len,
                                       reinterpret_cast<CORBA::Octet*>(UPDATED_TOPIC_DATA));
    topic->set_qos(topic_qos);

    Writer* writer = new Writer(dw.in());

    writer->start();
    while (!writer->is_finished()) {
      ACE_Time_Value small_time(0,250000);
      ACE_OS::sleep(small_time);
    }

    // Cleanup
    writer->end();
    delete writer;

    // Wait for monitor2 to finish
    fp = ACE_OS::fopen((synch_dir + mon2_fname).c_str(), ACE_TEXT("r"));
    while (fp == 0) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) waiting monitor2 done ...\n")));
      ACE_OS::sleep(1);
      fp = ACE_OS::fopen((synch_dir + mon2_fname).c_str(), ACE_TEXT("r"));
    }
    if (fp != 0) {
      ACE_OS::fclose(fp);
    }

    participant->delete_contained_entities();
    dpf->delete_participant(participant.in());
    TheServiceParticipant->shutdown();

    ACE_DEBUG((LM_DEBUG, "(%P|%t) publisher main done\n"));

  } catch (CORBA::Exception& e) {
    e._tao_print_exception("publisher: PUB: Exception caught in main():", stderr);
    exit(1);
  }

  return 0;
}
