// -*- C++ -*-
// ============================================================================
/**
 *  @file   publisher.cpp
 *
 *  $Id$
 *
 *
 */
// ============================================================================

#include "MessengerTypeSupportImpl.h"
#include "Writer.h"
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/PublisherImpl.h>
#include <dds/DCPS/transport/framework/TheTransportFactory.h>
#include <dds/DCPS/transport/simpleTCP/SimpleTcpConfiguration.h>

#include "dds/DCPS/transport/simpleTCP/SimpleTcp.h"

#include <ace/streams.h>
#include "ace/Get_Opt.h"

char PART_USER_DATA[] = "Initial DomainParticipant UserData";
char DW_USER_DATA[] = "Initial DataWriter UserData";
char TOPIC_DATA[] = "Initial Topic TopicData";
char GROUP_DATA[] = "Initial GroupData";
char UPDATED_PART_USER_DATA[] = "Updated DomainParticipant UserData";
char UPDATED_DW_USER_DATA[] = "Updated DataWriter UserData";
char UPDATED_TOPIC_DATA[] = "Updated Topic TopicData";
char UPDATED_GROUP_DATA[] = "Updated GroupData";

char synch_fname[] = "monitor1_done";

OpenDDS::DCPS::TransportIdType transport_impl_id = 1;
int num_messages = 10;

int
parse_args (int argc, char *argv[])
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

int main (int argc, char *argv[]) {
  try
    {
      ACE_DEBUG ((LM_DEBUG, "(%P|%t) publisher main\n"));

      DDS::DomainParticipantFactory_var dpf =
        TheParticipantFactoryWithArgs(argc, argv);

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

      DDS::DomainParticipant_var participant =
        dpf->create_participant(411,
                                partQos,
                                DDS::DomainParticipantListener::_nil());
      if (CORBA::is_nil (participant.in ())) {
        cerr << "publisher: create_participant failed." << endl;
        return 1;
      }

      ::Messenger::MessageTypeSupport_var ts = new ::Messenger::MessageTypeSupportImpl();

      if (DDS::RETCODE_OK != ts->register_type(participant.in (), "Messenger")) {
        cerr << "publisher: register_type failed." << endl;
        exit(1);
      }

      CORBA::String_var type_name = ts->get_type_name ();

      DDS::TopicQos topic_qos;
      participant->get_default_topic_qos(topic_qos);

      // set up topic data in topic qos
      CORBA::ULong topic_data_len = static_cast<CORBA::ULong>(ACE_OS::strlen (TOPIC_DATA));
      topic_qos.topic_data.value.length (topic_data_len);
      topic_qos.topic_data.value.replace (topic_data_len, topic_data_len, reinterpret_cast<CORBA::Octet*>(TOPIC_DATA));

      DDS::Topic_var topic =
        participant->create_topic ("Movie Discussion List",
                                   type_name.in (),
                                   topic_qos,
                                   DDS::TopicListener::_nil());
      if (CORBA::is_nil (topic.in ())) {
        cerr << "publisher: create_topic failed." << endl;
        exit(1);
      }

      OpenDDS::DCPS::TransportImpl_rch tcp_impl =
        TheTransportFactory->create_transport_impl (transport_impl_id,
                                                    ::OpenDDS::DCPS::AUTO_CONFIG);

      DDS::PublisherQos pub_qos;
      participant->get_default_publisher_qos (pub_qos);

      // set up group data in group qos
      CORBA::ULong group_data_len = static_cast<CORBA::ULong> (ACE_OS::strlen (GROUP_DATA));
      pub_qos.group_data.value.length (group_data_len);
      pub_qos.group_data.value.replace (group_data_len, group_data_len, reinterpret_cast<CORBA::Octet*>(GROUP_DATA));

      DDS::Publisher_var pub =
        participant->create_publisher(pub_qos,
        DDS::PublisherListener::_nil());
      if (CORBA::is_nil (pub.in ())) {
        cerr << "publisher: create_publisher failed." << endl;
        exit(1);
      }

      // Attach the publisher to the transport.
      OpenDDS::DCPS::PublisherImpl* pub_impl =
        dynamic_cast< OpenDDS::DCPS::PublisherImpl*>(pub.in ());
      if (0 == pub_impl) {
        cerr << "publisher: Failed to obtain publisher servant" << endl;
        exit(1);
      }

      OpenDDS::DCPS::AttachStatus status = pub_impl->attach_transport(tcp_impl.in());
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
        cerr << "publisher: Failed to attach to the transport. Status == "
          << status_str.c_str() << endl;
        exit(1);
      }

      // Create the datawriter
      DDS::DataWriterQos dw_qos;
      pub->get_default_datawriter_qos (dw_qos);
      dw_qos.durability.kind = DDS::TRANSIENT_LOCAL_DURABILITY_QOS;
      dw_qos.reliability.kind  = ::DDS::RELIABLE_RELIABILITY_QOS;
      dw_qos.resource_limits.max_samples_per_instance = 1000;
      dw_qos.history.kind  = ::DDS::KEEP_ALL_HISTORY_QOS;
      
      // set up user data in DW qos
      CORBA::ULong dw_user_data_len = static_cast<CORBA::ULong>(ACE_OS::strlen (DW_USER_DATA));
      dw_qos.user_data.value.length (dw_user_data_len);
      dw_qos.user_data.value.replace (dw_user_data_len, 
                                      dw_user_data_len, 
                                      reinterpret_cast<CORBA::Octet*>(DW_USER_DATA));

      DDS::DataWriter_var dw =
        pub->create_datawriter(topic.in (),
                               dw_qos,
                               DDS::DataWriterListener::_nil());
      if (CORBA::is_nil (dw.in ())) {
        cerr << "publisher: create_datawriter failed." << endl;
        exit(1);
      }

      // wait for Monitor 1 done
      FILE* fp = ACE_OS::fopen (synch_fname, ACE_LIB_TEXT("r"));
      int i = 0;
      while (fp == 0 &&  i < 15)
      {
        ACE_DEBUG ((LM_DEBUG,
          ACE_LIB_TEXT("(%P|%t)waiting monitor1 done ...\n")));
        ACE_OS::sleep (1);
        ++ i;
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

      dw_user_data_len = static_cast<CORBA::ULong>(ACE_OS::strlen (UPDATED_DW_USER_DATA));
      dw_qos.user_data.value.length (dw_user_data_len);
      dw_qos.user_data.value.replace (dw_user_data_len, 
                                      dw_user_data_len, 
                                      reinterpret_cast<CORBA::Octet*>(UPDATED_DW_USER_DATA));
      dw->set_qos (dw_qos);

      group_data_len = static_cast<CORBA::ULong> (ACE_OS::strlen (UPDATED_GROUP_DATA));
      pub_qos.group_data.value.length (group_data_len);
      pub_qos.group_data.value.replace (group_data_len, 
                                        group_data_len, 
                                        reinterpret_cast<CORBA::Octet*>(UPDATED_GROUP_DATA));
      pub->set_qos (pub_qos);

      topic_data_len = static_cast<CORBA::ULong>(ACE_OS::strlen (UPDATED_TOPIC_DATA));
      topic_qos.topic_data.value.length (topic_data_len);
      topic_qos.topic_data.value.replace (topic_data_len, 
                                          topic_data_len, 
                                          reinterpret_cast<CORBA::Octet*>(UPDATED_TOPIC_DATA));
      topic->set_qos (topic_qos);

      Writer* writer = new Writer(dw.in());

      writer->start ();
      while ( !writer->is_finished()) {
        ACE_Time_Value small(0,250000);
        ACE_OS::sleep (small);
      }

      // Cleanup
      writer->end ();
      delete writer;
      participant->delete_contained_entities();
      dpf->delete_participant(participant.in ());
      TheTransportFactory->release();
      TheServiceParticipant->shutdown ();
  }
  catch (CORBA::Exception& e)
    {
       cerr << "publisher: PUB: Exception caught in main.cpp:" << endl
         << e << endl;
      exit(1);
    }

  return 0;
}
