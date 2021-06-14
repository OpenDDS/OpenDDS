// -*- C++ -*-
// ============================================================================
/**
 *  @file   publisher.cpp
 *
 *
 *
 */
// ============================================================================

#include "DataReaderListenerImpl.h"
#include "DataWriterListenerImpl.h"
#include "MessengerTypeSupportImpl.h"
#include "Writer.h"

#include "tests/Utils/ExceptionStreams.h"
#include "tests/Utils/StatusMatching.h"

#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/PublisherImpl.h>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/StaticIncludes.h>
#include <dds/DCPS/transport/framework/TransportExceptions.h>
#include <dds/DCPS/transport/framework/TransportRegistry.h>

#ifdef ACE_AS_STATIC_LIBS
#include <dds/DCPS/RTPS/RtpsDiscovery.h>
#include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#endif

#include <ace/Get_Opt.h>
#include <ace/OS_NS_unistd.h>
#include <ace/streams.h>

#include "dds/DCPS/GuardCondition.h"

using namespace Messenger;
using namespace std;

int PUB_LEASE_DURATION_SEC = 2;
const int SUB_LEASE_DURATION_SEC = 5;
int pub_assert_liveliness_period = PUB_LEASE_DURATION_SEC;
int pub_num_messages = 10;
int sub_total_num_messages = 20;
bool liveliness_lost_test = false;
int pub_num_liveliness_lost_callbacks = 2;
int sub_num_liveliness_change_callbacks = 8;

int
parse_args (int argc, ACE_TCHAR *argv[])
{
  ACE_Get_Opt get_opts (argc, argv, ACE_TEXT("t:n:lc:N:C:"));
  int c;

  while ((c = get_opts ()) != -1)
  {
    switch (c)
    {
    case 't':
      pub_assert_liveliness_period = ACE_OS::atoi (get_opts.opt_arg());
      break;
    case 'n':
      pub_num_messages = ACE_OS::atoi (get_opts.opt_arg());
      break;
    case 'l':
      liveliness_lost_test = true;
      break;
    case 'c':
      pub_num_liveliness_lost_callbacks = ACE_OS::atoi (get_opts.opt_arg());
      break;
    case 'N':
      sub_total_num_messages = ACE_OS::atoi (get_opts.opt_arg());
      break;
    case 'C':
      sub_num_liveliness_change_callbacks = ACE_OS::atoi (get_opts.opt_arg());
      break;
    case '?':
    default:
      ACE_ERROR_RETURN ((LM_ERROR,
        "usage:  %s "
        "[-t <pub_assert_liveliness_period>]\n"
        "[-n <pub_num_messages>]\n"
        "[-l]\n"
        "[-c <pub_num_callbacks>]\n",
        "[-N <sub_total_num_messages>]\n"
        "[-C <sub_num_callbacks>]\n",
        argv [0]),
        -1);
    }
  }
  // Indicates successful parsing of the command line
  return 0;
}

int ACE_TMAIN (int argc, ACE_TCHAR *argv[])
{
  int status = 0;
  DistributedConditionSet_rch dcs = OpenDDS::DCPS::make_rch<InMemoryDistributedConditionSet>();

  const int MY_DOMAIN = 111;

  DDS::DomainParticipantFactory_var dpf =
    TheParticipantFactoryWithArgs(argc, argv);
  DDS::DomainParticipant_var pub1_participant =
    dpf->create_participant(MY_DOMAIN,
                            PARTICIPANT_QOS_DEFAULT,
                            DDS::DomainParticipantListener::_nil(),
                            ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  DDS::DomainParticipant_var pub2_participant =
    dpf->create_participant(MY_DOMAIN,
                            PARTICIPANT_QOS_DEFAULT,
                            DDS::DomainParticipantListener::_nil(),
                            ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  DDS::DomainParticipant_var sub_participant =
    dpf->create_participant(MY_DOMAIN,
                            PARTICIPANT_QOS_DEFAULT,
                            DDS::DomainParticipantListener::_nil(),
                            ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  OpenDDS::DCPS::TransportConfig_rch cfg = TheTransportRegistry->get_config("pub1_part");
  if (!cfg.is_nil()) {
    TheTransportRegistry->bind_config(cfg, pub1_participant);
  }
  cfg = TheTransportRegistry->get_config("pub2_part");
  if (!cfg.is_nil()) {
    TheTransportRegistry->bind_config(cfg, pub2_participant);
  }
  cfg = TheTransportRegistry->get_config("sub_part");
  if (!cfg.is_nil()) {
    TheTransportRegistry->bind_config(cfg, sub_participant);
  }

  if (parse_args (argc, argv) == -1) {
    return -1;
  }

  MessageTypeSupport_var pub1_mts = new MessageTypeSupportImpl();
  MessageTypeSupport_var pub2_mts = new MessageTypeSupportImpl();
  MessageTypeSupport_var sub_mts = new MessageTypeSupportImpl();

  pub1_mts->register_type(pub1_participant.in (), "");
  pub2_mts->register_type(pub2_participant.in (), "");
  sub_mts->register_type(sub_participant.in (), "");

  CORBA::String_var pub1_type_name = pub1_mts->get_type_name ();
  CORBA::String_var pub2_type_name = pub2_mts->get_type_name ();
  CORBA::String_var sub_type_name = sub_mts->get_type_name ();

  DDS::Topic_var pub1_topic =
    pub1_participant->create_topic ("Movie Discussion List",
                                    pub1_type_name.in (),
                                    TOPIC_QOS_DEFAULT,
                                    DDS::TopicListener::_nil(),
                                    ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  DDS::Topic_var pub2_topic =
    pub2_participant->create_topic ("Movie Discussion List",
                                    pub2_type_name.in (),
                                    TOPIC_QOS_DEFAULT,
                                    DDS::TopicListener::_nil(),
                                    ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  DDS::Topic_var sub_topic =
    sub_participant->create_topic ("Movie Discussion List",
                                   sub_type_name.in (),
                                   TOPIC_QOS_DEFAULT,
                                   DDS::TopicListener::_nil(),
                                   ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  DDS::Publisher_var pub1_pub =
    pub1_participant->create_publisher(PUBLISHER_QOS_DEFAULT,
                                       DDS::PublisherListener::_nil(),
                                       ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  DDS::Publisher_var pub2_pub =
    pub2_participant->create_publisher(PUBLISHER_QOS_DEFAULT,
                                       DDS::PublisherListener::_nil(),
                                       ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  DDS::Subscriber_var sub_sub =
    sub_participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
                                       DDS::SubscriberListener::_nil(),
                                       ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  DataWriterListenerImpl* pub1_dwl1_servant = new DataWriterListenerImpl;
  ::DDS::DataWriterListener_var pub1_dwl1 (pub1_dwl1_servant);
  DataWriterListenerImpl* pub2_dwl2_servant = new DataWriterListenerImpl;
  ::DDS::DataWriterListener_var pub2_dwl2 (pub2_dwl2_servant);
  DataWriterListenerImpl* pub1_dwl2_servant = new DataWriterListenerImpl;
  ::DDS::DataWriterListener_var pub1_dwl2 (pub1_dwl2_servant);
  DataWriterListenerImpl* pub1_dwl3_servant = new DataWriterListenerImpl;
  ::DDS::DataWriterListener_var pub1_dwl3 (pub1_dwl3_servant);

  DataReaderListenerImpl* sub_drl_servant = new DataReaderListenerImpl(dcs);
  ::DDS::DataReaderListener_var sub_drl (sub_drl_servant);

  // Create the datawriters
  ::DDS::DataWriterQos pub1_dw_qos;
  pub1_pub->get_default_datawriter_qos (pub1_dw_qos);
  ::DDS::DataWriterQos pub2_dw2_qos;
  pub2_pub->get_default_datawriter_qos (pub2_dw2_qos);

  pub1_dw_qos.liveliness.kind = ::DDS::MANUAL_BY_PARTICIPANT_LIVELINESS_QOS;
  pub1_dw_qos.liveliness.lease_duration.sec = PUB_LEASE_DURATION_SEC;
  pub1_dw_qos.liveliness.lease_duration.nanosec = 0;
  pub2_dw2_qos.liveliness.kind = ::DDS::MANUAL_BY_PARTICIPANT_LIVELINESS_QOS;
  pub2_dw2_qos.liveliness.lease_duration.sec = PUB_LEASE_DURATION_SEC;
  pub2_dw2_qos.liveliness.lease_duration.nanosec = 0;

  ::DDS::DataReaderQos sub_dr_qos;
  sub_sub->get_default_datareader_qos (sub_dr_qos);

  sub_dr_qos.liveliness.lease_duration.sec = SUB_LEASE_DURATION_SEC ;
  sub_dr_qos.liveliness.lease_duration.nanosec = 0 ;
  sub_dr_qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;

  DDS::DataWriter_var pub1_dw1 =
    pub1_pub->create_datawriter(pub1_topic.in (),
                                pub1_dw_qos,
                                pub1_dwl1.in(),
                                ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  DDS::DataWriter_var pub2_dw1 =
    pub2_pub->create_datawriter(pub2_topic.in (),
                                pub2_dw2_qos,
                                pub2_dwl2.in(),
                                ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  pub1_dw_qos.liveliness.kind = ::DDS::MANUAL_BY_TOPIC_LIVELINESS_QOS;

  DDS::DataWriter_var pub1_dw2 =
    pub1_pub->create_datawriter(pub1_topic.in (),
                                pub1_dw_qos,
                                pub1_dwl2.in(),
                                ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  DDS::DataWriter_var pub1_dw3 =
    pub1_pub->create_datawriter(pub1_topic.in (),
                                pub1_dw_qos,
                                pub1_dwl3.in(),
                                ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  DDS::DataReader_var sub_dr = sub_sub->create_datareader(sub_topic.in (),
                                                          sub_dr_qos,
                                                          sub_drl,
                                                          ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  Utils::wait_match(sub_dr, 4);
  Utils::wait_match(pub1_dw1, 1);
  Utils::wait_match(pub2_dw1, 1);
  Utils::wait_match(pub1_dw2, 1);
  Utils::wait_match(pub1_dw3, 1);

  {
    Write_Samples writer1(pub1_dw1, "Manual_By_Participant_Sample_Writer_1");
    Assert_Participant_Liveliness writer2(pub2_dw1, "Manual_By_Participant_Assert_Writer_2");
    Write_Samples writer3(pub1_dw2, "Manual_By_Topic_Sample_Writer_1");
    Assert_Writer_Liveliness writer4(pub1_dw3, "Manual_By_Topic_Assert_Writer_2");

    writer1.start();
    writer2.start();
    writer3.start();
    writer4.start();

    writer1.end();
    writer2.end();
    writer3.end();
    writer4.end();

    dcs->wait_for("driver", SUBSCRIBER_ACTOR, CALLBACKS_DONE_CONDITION);
  }

  ACE_DEBUG((LM_INFO,
             "subscriber got %d of %d messages, "
             "and %d of %d callbacks\n",
             (int) sub_drl_servant->num_reads(), sub_total_num_messages,
             sub_drl_servant->num_liveliness_change_callbacks(), sub_num_liveliness_change_callbacks));

  if (sub_drl_servant->num_reads() != sub_total_num_messages) {
    ACE_ERROR((LM_ERROR,
               "(%P|%t) ERROR: subscriber did not receive expected number of messages. %d/%d\n",
               sub_drl_servant->num_reads(), sub_total_num_messages));
    status = 1;
  }

  const unsigned long actual = pub1_dwl1_servant->num_liveliness_lost_callbacks() +
    pub2_dwl2_servant->num_liveliness_lost_callbacks() +
    pub1_dwl2_servant->num_liveliness_lost_callbacks() +
    pub1_dwl3_servant->num_liveliness_lost_callbacks();

  if (liveliness_lost_test
      && static_cast<int>(actual) != pub_num_liveliness_lost_callbacks) {
    ACE_ERROR((LM_ERROR,
               "(%P|%t) ERROR: publisher did not receive expected liveliness lost callbacks. %d/%d\n",
               actual, pub_num_liveliness_lost_callbacks));
    status = 1;
  }

  pub1_participant->delete_contained_entities();
  dpf->delete_participant(pub1_participant.in ());
  pub2_participant->delete_contained_entities();
  dpf->delete_participant(pub2_participant.in ());
  sub_participant->delete_contained_entities();
  dpf->delete_participant(sub_participant.in ());

  TheServiceParticipant->shutdown ();

  return status;
}
