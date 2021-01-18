// -*- C++ -*-
// ============================================================================
/**
 *  @file   subscriber.cpp
 *
 *
 *
 */
// ============================================================================


#include "DataReaderListener.h"
#include "MessengerTypeSupportImpl.h"

#include "tests/Utils/ExceptionStreams.h"

#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/SubscriberImpl.h>
#include <dds/DCPS/BuiltInTopicUtils.h>
#include <dds/DCPS/StaticIncludes.h>
#include <dds/DCPS/DomainParticipantImpl.h> //Debug

#include <ace/streams.h>
#include <ace/Get_Opt.h>

using namespace std;

char PART_USER_DATA[] = "Initial DomainParticipant UserData";
char DR_USER_DATA[] = "Initial DataReader UserData";
char TOPIC_DATA[] = "Initial Topic TopicData";
char GROUP_DATA[] = "Initial GroupData";
char UPDATED_PART_USER_DATA[] = "Updated DomainParticipant UserData";
char UPDATED_DR_USER_DATA[] = "Updated DataReader UserData";
char UPDATED_TOPIC_DATA[] = "Updated Topic TopicData";
char UPDATED_GROUP_DATA[] = "Updated GroupData";

ACE_TString synch_dir;
ACE_TCHAR synch_fname[] = ACE_TEXT("monitor1_done");

int num_messages = 10;

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
                                          OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (CORBA::is_nil(participant.in())) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) subscriber: create_participant failed.\n")));
      return 1;
    }

    // Debug begin
    OpenDDS::DCPS::DomainParticipantImpl* part_servant =
      dynamic_cast<OpenDDS::DCPS::DomainParticipantImpl*>(participant.in());
    ACE_DEBUG((LM_DEBUG, ACE_TEXT("Subscriber's participant ID: %C\n"),
               OpenDDS::DCPS::to_string(part_servant->get_id()).c_str()));
    // Debug end

    ::Messenger::MessageTypeSupport_var mts = new ::Messenger::MessageTypeSupportImpl();

    if (DDS::RETCODE_OK != mts->register_type(participant.in(), "Messenger")) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) subscriber: Failed to register the MessageTypeTypeSupport.\n")));
      exit(1);
    }

    DDS::TopicQos topic_qos;
    participant->get_default_topic_qos(topic_qos);

    // set up topic data in topic qos
    CORBA::ULong topic_data_len = static_cast<CORBA::ULong>(ACE_OS::strlen(TOPIC_DATA));
    topic_qos.topic_data.value.length(topic_data_len);
    topic_qos.topic_data.value.replace(topic_data_len, topic_data_len,
                                       reinterpret_cast<CORBA::Octet*>(TOPIC_DATA));

    DDS::Topic_var topic = participant->create_topic("Movie Discussion List",
                                                     "Messenger",
                                                     topic_qos,
                                                     DDS::TopicListener::_nil(),
                                                     OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (CORBA::is_nil(topic.in())) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) subscriber: Failed to create_topic.\n")));
      exit(1);
    }

    // Create the subscriber

    DDS::SubscriberQos sub_qos;
    participant->get_default_subscriber_qos(sub_qos);

    // set up group data in subscriber qos
    CORBA::ULong group_data_len = static_cast<CORBA::ULong>(ACE_OS::strlen(GROUP_DATA));
    sub_qos.group_data.value.length(group_data_len);
    sub_qos.group_data.value.replace(group_data_len, group_data_len,
                                     reinterpret_cast<CORBA::Octet*>(GROUP_DATA));

    DDS::Subscriber_var sub = participant->create_subscriber(sub_qos,
                                                             DDS::SubscriberListener::_nil(),
                                                             OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (CORBA::is_nil(sub.in())) {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) subscriber: Failed to create_subscriber.\n")));
      exit(1);
    }

    // activate the listener
    DDS::DataReaderListener_var listener(new DataReaderListenerImpl);
    if (CORBA::is_nil(listener.in())) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) subscriber: listener is nil.\n")));
      exit(1);
    }

    DataReaderListenerImpl* listener_servant = dynamic_cast<DataReaderListenerImpl*>(listener.in());
    if (!listener_servant) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) %N:%l main()")
                        ACE_TEXT(" ERROR: listener_servant is nil (dynamic_cast failed)!\n")), -1);
    }

    DDS::Subscriber_var builtin = participant->get_builtin_subscriber();
    DDS::DataReader_var bitdr =
      builtin->lookup_datareader(OpenDDS::DCPS::BUILT_IN_PUBLICATION_TOPIC);
    listener_servant->set_builtin_datareader(bitdr.in());

    // Create the Datareaders
    DDS::DataReaderQos dr_qos;
    sub->get_default_datareader_qos(dr_qos);

    // set up user data in DR qos
    CORBA::ULong dr_user_data_len = static_cast<CORBA::ULong>(ACE_OS::strlen(DR_USER_DATA));
    dr_qos.user_data.value.length(dr_user_data_len);
    dr_qos.user_data.value.replace(dr_user_data_len,
                                   dr_user_data_len,
                                   reinterpret_cast<CORBA::Octet*>(DR_USER_DATA));

    DDS::DataReader_var dr = sub->create_datareader(topic.in(),
                                                    dr_qos,
                                                    listener.in(),
                                                    OpenDDS::DCPS::DEFAULT_STATUS_MASK);
    if (CORBA::is_nil(dr.in())) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) subscriber: create_datareader failed.\n")));
      exit(1);
    }

    // Wait for Monitor 1 done.
    FILE* fp = ACE_OS::fopen((synch_dir + synch_fname).c_str(), ACE_TEXT("r"));
    int i = 0;
    while (fp == 0 && i < 15) {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) waiting monitor1 to finish...\n")));
      ACE_OS::sleep(1);
      ++i;
      fp = ACE_OS::fopen((synch_dir + synch_fname).c_str(), ACE_TEXT("r"));
    }
    if (fp) {
      ACE_OS::fclose(fp);
    }
    // Debug begin
    else {
      ACE_DEBUG((LM_DEBUG, ACE_TEXT("Monitor1 probably hasn't finished yet!\n")));
    }
    // Debug end

    // Give time for the second monitor to initialize
    ACE_OS::sleep(3);

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

    if (!CORBA::is_nil(participant.in())) {
      participant->delete_contained_entities();
    }
    if (!CORBA::is_nil(dpf.in())) {
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
