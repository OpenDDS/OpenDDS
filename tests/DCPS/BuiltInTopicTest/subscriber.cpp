// -*- C++ -*-
// ============================================================================
/**
 *  @file   subscriber.cpp
 *
 *  $Id$
 *
 *
 */
// ============================================================================


#include "DataReaderListener.h"
#include "MessengerTypeSupportImpl.h"
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/SubscriberImpl.h>
#include <dds/DCPS/transport/framework/TheTransportFactory.h>
#include <dds/DCPS/transport/simpleTCP/SimpleTcpConfiguration.h>

#include "dds/DCPS/transport/simpleTCP/SimpleTcp.h"

#include <ace/streams.h>
#include "ace/Get_Opt.h"

char PART_USER_DATA[] = "Initial DomainParticipant UserData";
char DR_USER_DATA[] = "Initial DataReader UserData";
char TOPIC_DATA[] = "Initial Topic TopicData";
char GROUP_DATA[] = "Initial GroupData";
char UPDATED_PART_USER_DATA[] = "Updated DomainParticipant UserData";
char UPDATED_DR_USER_DATA[] = "Updated DataReader UserData";
char UPDATED_TOPIC_DATA[] = "Updated Topic TopicData";
char UPDATED_GROUP_DATA[] = "Updated GroupData";


char synch_fname[] = "monitor1_done";

OpenDDS::DCPS::TransportIdType transport_impl_id = 1;
int num_messages = 10;

int parse_args (int argc, ACE_TCHAR *argv[])
{
  ACE_Get_Opt get_opts (argc, argv, "n:");
  int c;

  while ((c = get_opts ()) != -1)
  {
    switch (c)
    {
    case 'n':
      num_messages = ACE_OS::atoi (get_opts.opt_arg ());
      break;
    case '?':
    default:
      ACE_ERROR_RETURN ((LM_ERROR,
        "usage:  %s "
        "-n <num of messages> "
        "\n",
        argv [0]),
        -1);
    }
  }
  // Indicates sucessful parsing of the command line
  return 0;
}

int ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  ACE_DEBUG ((LM_DEBUG, "(%P|%t) subscriber main\n"));
  try
    {
      DDS::DomainParticipantFactory_var dpf;
      DDS::DomainParticipant_var participant;

      dpf = TheParticipantFactoryWithArgs(argc, argv);

      if (parse_args (argc, argv) == -1) {
        return -1;
      }

      DDS::DomainParticipantQos partQos;
      dpf->get_default_participant_qos(partQos);

      // set up user data in DP qos
      CORBA::ULong part_user_data_len 
        = static_cast<CORBA::ULong>(ACE_OS::strlen (PART_USER_DATA));
      partQos.user_data.value.length (part_user_data_len);
      partQos.user_data.value.replace (part_user_data_len, 
                                       part_user_data_len, 
                                       reinterpret_cast<CORBA::Octet*>(PART_USER_DATA));

      participant = dpf->create_participant(411,
                                            partQos,
                                            DDS::DomainParticipantListener::_nil(),
                                            ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil (participant.in ())) {
        cerr << "subscriber: create_participant failed." << endl;
        return 1 ;
      }

      ::Messenger::MessageTypeSupport_var mts = new ::Messenger::MessageTypeSupportImpl();

      if (DDS::RETCODE_OK != mts->register_type(participant.in (), "Messenger")) {
          cerr << "subscriber: Failed to register the MessageTypeTypeSupport." << endl;
          exit(1);
        }

      CORBA::String_var type_name = mts->get_type_name ();

      DDS::TopicQos topic_qos;
      participant->get_default_topic_qos(topic_qos);

      // set up topic data in topic qos
      CORBA::ULong topic_data_len = static_cast<CORBA::ULong>(ACE_OS::strlen (TOPIC_DATA));
      topic_qos.topic_data.value.length (topic_data_len);
      topic_qos.topic_data.value.replace (topic_data_len, topic_data_len, reinterpret_cast<CORBA::Octet*>(TOPIC_DATA));

      DDS::Topic_var topic = participant->create_topic("Movie Discussion List",
                                                        type_name.in (),
                                                        topic_qos,
                                                        DDS::TopicListener::_nil(),
                                                        ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil (topic.in ())) {
        cerr << "subscriber: Failed to create_topic." << endl;
        exit(1);
      }

      // Initialize the transport
      OpenDDS::DCPS::TransportImpl_rch tcp_impl = 
        TheTransportFactory->create_transport_impl (transport_impl_id, 
                                                    ::OpenDDS::DCPS::AUTO_CONFIG);

      // Create the subscriber and attach to the corresponding
      // transport.

      DDS::SubscriberQos sub_qos;
      participant->get_default_subscriber_qos (sub_qos);

      // set up group data in subscriber qos
      CORBA::ULong group_data_len = static_cast<CORBA::ULong> (ACE_OS::strlen (GROUP_DATA));
      sub_qos.group_data.value.length (group_data_len);
      sub_qos.group_data.value.replace (group_data_len, group_data_len, reinterpret_cast<CORBA::Octet*>(GROUP_DATA));

      DDS::Subscriber_var sub =
        participant->create_subscriber(sub_qos,
                                       DDS::SubscriberListener::_nil(),
                                       ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil (sub.in ())) {
        cerr << "subscriber: Failed to create_subscriber." << endl;
        exit(1);
      }

      // Attach the subscriber to the transport.
      OpenDDS::DCPS::SubscriberImpl* sub_impl =
        dynamic_cast< OpenDDS::DCPS::SubscriberImpl*> (sub.in ());
      if (0 == sub_impl) {
        cerr << "subscriber: Failed to obtain subscriber servant\n" << endl;
        exit(1);
      }

      OpenDDS::DCPS::AttachStatus status = sub_impl->attach_transport(tcp_impl.in());
      if (status != OpenDDS::DCPS::ATTACH_OK) {
        std::string status_str;
        switch (status) {
        case OpenDDS::DCPS::ATTACH_BAD_TRANSPORT:
          status_str = "ATTACH_BAD_TRANSPORT";
          break;
        case OpenDDS::DCPS::ATTACH_ERROR:
          status_str = "ATTACH_ERROR";
          break;
        case OpenDDS::DCPS::ATTACH_INCOMPATIBLE_QOS:
          status_str = "ATTACH_INCOMPATIBLE_QOS";
          break;
        default:
          status_str = "Unknown Status";
          break;
        }
        cerr << "subscriber: Failed to attach to the transport. Status == "
          << status_str.c_str() << endl;
        exit(1);
      }

      // activate the listener
      DDS::DataReaderListener_var listener (new DataReaderListenerImpl);
      DataReaderListenerImpl* listener_servant =
        dynamic_cast<DataReaderListenerImpl*>(listener.in());

      if (CORBA::is_nil (listener.in ())) {
        cerr << "subscriber: listener is nil." << endl;
        exit(1);
      }

      // Create the Datareaders
      DDS::DataReaderQos dr_qos;
      sub->get_default_datareader_qos (dr_qos);

      // set up user data in DR qos
      CORBA::ULong dr_user_data_len = static_cast<CORBA::ULong>(ACE_OS::strlen (DR_USER_DATA));
      dr_qos.user_data.value.length (dr_user_data_len);
      dr_qos.user_data.value.replace (dr_user_data_len, 
                                      dr_user_data_len, 
                                      reinterpret_cast<CORBA::Octet*>(DR_USER_DATA));

      DDS::DataReader_var dr = sub->create_datareader(topic.in (),
                                                      dr_qos,
                                                      listener.in (),
                                                      ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil (dr.in ())) {
        cerr << "subscriber: create_datareader failed." << endl;
        exit(1);
      }

      // Wait for Monitor 1 done.
      FILE* fp = ACE_OS::fopen (synch_fname, ACE_LIB_TEXT("r"));
      int i = 0;
      while (fp == 0 &&  i < 15)
      {
        ACE_DEBUG ((LM_DEBUG,
          ACE_LIB_TEXT("(%P|%t)waiting monitor1 done ...\n")));
        ACE_OS::sleep (1);
        ++i;
        fp = ACE_OS::fopen (synch_fname, ACE_LIB_TEXT("r"));
      }
      if (fp != 0)
        ACE_OS::fclose (fp);

      // Now change the changeable qos. The second monitor should get the updated qos from BIT.
      part_user_data_len = static_cast<CORBA::ULong>(ACE_OS::strlen (UPDATED_PART_USER_DATA));
      partQos.user_data.value.length (part_user_data_len);
      partQos.user_data.value.replace (part_user_data_len, 
                                       part_user_data_len, 
                                       reinterpret_cast<CORBA::Octet*>(UPDATED_PART_USER_DATA));
      participant->set_qos (partQos);

      dr_user_data_len = static_cast<CORBA::ULong>(ACE_OS::strlen (UPDATED_DR_USER_DATA));
      dr_qos.user_data.value.length (dr_user_data_len);
      dr_qos.user_data.value.replace (dr_user_data_len, 
                                      dr_user_data_len, 
                                      reinterpret_cast<CORBA::Octet*>(UPDATED_DR_USER_DATA));
      dr->set_qos (dr_qos);

      group_data_len = static_cast<CORBA::ULong> (ACE_OS::strlen (UPDATED_GROUP_DATA));
      sub_qos.group_data.value.length (group_data_len);
      sub_qos.group_data.value.replace (group_data_len, 
                                        group_data_len, 
                                        reinterpret_cast<CORBA::Octet*>(UPDATED_GROUP_DATA));
      sub->set_qos (sub_qos);

      topic_data_len = static_cast<CORBA::ULong>(ACE_OS::strlen (UPDATED_TOPIC_DATA));
      topic_qos.topic_data.value.length (topic_data_len);
      topic_qos.topic_data.value.replace (topic_data_len, 
                                          topic_data_len, 
                                          reinterpret_cast<CORBA::Octet*>(UPDATED_TOPIC_DATA));
      topic->set_qos (topic_qos);

      while ( listener_servant->num_reads() < num_messages) {
        ACE_OS::sleep (1);
      }

      if (!CORBA::is_nil (participant.in ())) {
        participant->delete_contained_entities();
      }
      if (!CORBA::is_nil (dpf.in ())) {
        dpf->delete_participant(participant.in ());
      }

      TheTransportFactory->release();
      TheServiceParticipant->shutdown ();
    }
  catch (CORBA::Exception& e)
    {
      cerr << "subscriber: SUB: Exception caught in main ():" << endl << e << endl;
      return 1;
    }

  return 0;
}
