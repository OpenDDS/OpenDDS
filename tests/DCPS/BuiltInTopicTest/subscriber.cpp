#include "DataReaderListener.h"
#include "MessengerTypeSupportImpl.h"
#include "common.h"

#include <dds/DCPS/SubscriberImpl.h>

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
  int result = 0;
  ACE_DEBUG((LM_DEBUG, "(%P|%t) subscriber main\n"));
  try {
    DDS::DomainParticipantFactory_var dpf;
    DDS::DomainParticipant_var participant;

    dpf = TheParticipantFactoryWithArgs(argc, argv);
    if (parse_args(argc, argv) == -1) {
      return -1;
    }

    DDS::DomainParticipantQos partQos;
    dpf->get_default_participant_qos(partQos);

    // set up user data in DP qos
    CORBA::ULong part_user_data_len = static_cast<CORBA::ULong>(ACE_OS::strlen(PART_USER_DATA));
    partQos.user_data.value.length(part_user_data_len);
    partQos.user_data.value.replace(part_user_data_len,
                                    part_user_data_len,
                                    reinterpret_cast<CORBA::Octet*>(PART_USER_DATA));

    participant = dpf->create_participant(111,
                                          partQos,
                                          DDS::DomainParticipantListener::_nil(),
                                          ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (!participant) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) subscriber: create_participant failed.\n")));
      return 1;
    }

    ::Messenger::MessageTypeSupport_var mts = new ::Messenger::MessageTypeSupportImpl();

    if (DDS::RETCODE_OK != mts->register_type(participant.in(), topic_type_name)) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) subscriber: Failed to register the MessageTypeTypeSupport.\n")));
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
    if (!topic) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) subscriber: Failed to create_topic.\n")));
      exit(1);
    }

    // Create the subscriber
    DDS::SubscriberQos sub_qos;
    participant->get_default_subscriber_qos(sub_qos);

    // set up group data in subscriber qos
    CORBA::ULong group_data_len = static_cast<CORBA::ULong>(ACE_OS::strlen(GROUP_DATA));
    sub_qos.group_data.value.length(group_data_len);
    sub_qos.group_data.value.replace(group_data_len, group_data_len, reinterpret_cast<CORBA::Octet*>(GROUP_DATA));

    DDS::Subscriber_var sub = participant->create_subscriber(sub_qos,
                                                             DDS::SubscriberListener::_nil(),
                                                             ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (!sub) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) subscriber: Failed to create_subscriber.\n")));
      exit(1);
    }

    // activate the listener
    DDS::DataReaderListener_var listener(new DataReaderListenerImpl);
    if (!listener) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) subscriber: listener is nil.\n")));
      exit(1);
    }

    DataReaderListenerImpl* listener_servant = dynamic_cast<DataReaderListenerImpl*>(listener.in());
    if (!listener_servant) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) %N:%l main()")
                        ACE_TEXT(" ERROR: failed to obtain DataReaderListenerImpl!\n")), -1);
    }

    DDS::Subscriber_var builtin = participant->get_builtin_subscriber();
    DDS::DataReader_var bitdr = builtin->lookup_datareader(OpenDDS::DCPS::BUILT_IN_PUBLICATION_TOPIC);
    listener_servant->set_builtin_datareader(bitdr.in());

    // Create the Datareaders
    DDS::DataReaderQos dr_qos;
    sub->get_default_datareader_qos(dr_qos);
    dr_qos.representation.value.length(1);

#if defined(OPENDDS_SAFETY_PROFILE)
    dr_qos.representation.value[0] = DDS::XCDR2_DATA_REPRESENTATION;
#else
    dr_qos.representation.value[0] = OpenDDS::DCPS::UNALIGNED_CDR_DATA_REPRESENTATION;
#endif

    // set up user data in DR qos
    CORBA::ULong dr_user_data_len = static_cast<CORBA::ULong>(ACE_OS::strlen(DR_USER_DATA));
    dr_qos.user_data.value.length(dr_user_data_len);
    dr_qos.user_data.value.replace(dr_user_data_len,
                                   dr_user_data_len,
                                   reinterpret_cast<CORBA::Octet*>(DR_USER_DATA));

    DDS::DataReader_var dr = sub->create_datareader(topic.in(),
                                                    dr_qos,
                                                    listener.in(),
                                                    ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (!dr) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) subscriber: create_datareader failed.\n")));
      exit(1);
    }

    // Wait for Monitor 1 done.
    FILE* fp = ACE_OS::fopen((synch_dir + mon1_fname).c_str(), ACE_TEXT("r"));
    while (fp == 0) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) waiting monitor1 done ...\n")));
      ACE_OS::sleep(1);
      fp = ACE_OS::fopen((synch_dir + mon1_fname).c_str(), ACE_TEXT("r"));
    }
    if (fp) {
      ACE_OS::fclose(fp);
    }

    // Now change the changeable qos. The second monitor should get the updated qos from BIT.
    part_user_data_len = static_cast<CORBA::ULong>(ACE_OS::strlen(UPDATED_PART_USER_DATA));
    partQos.user_data.value.length(part_user_data_len);
    partQos.user_data.value.replace(part_user_data_len,
                                    part_user_data_len,
                                    reinterpret_cast<CORBA::Octet*>(UPDATED_PART_USER_DATA));
    participant->set_qos(partQos);

    dr_user_data_len = static_cast<CORBA::ULong>(ACE_OS::strlen(UPDATED_DR_USER_DATA));
    dr_qos.user_data.value.length(dr_user_data_len);
    dr_qos.user_data.value.replace(dr_user_data_len,
                                   dr_user_data_len,
                                   reinterpret_cast<CORBA::Octet*>(UPDATED_DR_USER_DATA));
    dr->set_qos(dr_qos);

    group_data_len = static_cast<CORBA::ULong>(ACE_OS::strlen(UPDATED_GROUP_DATA));
    sub_qos.group_data.value.length(group_data_len);
    sub_qos.group_data.value.replace(group_data_len,
                                     group_data_len,
                                     reinterpret_cast<CORBA::Octet*>(UPDATED_GROUP_DATA));
    sub->set_qos(sub_qos);

    topic_data_len = static_cast<CORBA::ULong>(ACE_OS::strlen(UPDATED_TOPIC_DATA));
    topic_qos.topic_data.value.length(topic_data_len);
    topic_qos.topic_data.value.replace(topic_data_len,
                                       topic_data_len,
                                       reinterpret_cast<CORBA::Octet*>(UPDATED_TOPIC_DATA));
    topic->set_qos(topic_qos);

    while (listener_servant->num_reads() < num_messages) {
      ACE_OS::sleep(1);
    }

    if (!listener_servant->read_bit_instance()) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) subscriber: Built in topic read failure.\n")));
      result = 1;
    }

    // Detach DataReaderListener to prevent it being called during
    // delete_contained_entities()
    dr->set_listener(0, 0);

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

    if (participant) {
      participant->delete_contained_entities();
    }
    if (dpf) {
      dpf->delete_participant(participant.in());
    }

    TheServiceParticipant->shutdown();

    ACE_DEBUG((LM_DEBUG, "(%P|%t) subscriber main done\n"));

  } catch (CORBA::Exception& e) {
    e._tao_print_exception("subscriber: SUB: Exception caught in main():", stderr);
    return 1;
  }

  return result;
}
