/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <ace/Argv_Type_Converter.h>
#include <ace/Get_Opt.h>
#include <ace/Log_Msg.h>
#include <ace/OS_NS_stdlib.h>

#include <dds/DdsDcpsInfrastructureC.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/SubscriberImpl.h>
#include <dds/DCPS/WaitSet.h>
#include <dds/DCPS/transport/framework/TheTransportFactory.h>

#ifdef ACE_AS_STATIC_LIBS
#include <dds/DCPS/transport/simpleTCP/SimpleTcp.h>
#endif

#include "SPMDataReaderListener.h"
#include "DPMDataReaderListener.h"
#include "TopicMDataReaderListener.h"
#include <dds/monitor/monitorTypeSupportImpl.h>

namespace {

OpenDDS::DCPS::TransportIdType transport_impl_id = 1;

} // namespace

int
ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  try {
    // Initialize DomainParticipantFactory
    DDS::DomainParticipantFactory_var dpf =
      TheParticipantFactoryWithArgs(argc, argv);

    // Create DomainParticipant
    DDS::DomainParticipant_var participant =
      dpf->create_participant(OpenDDS::DCPS::MONITOR_DOMAIN_ID,
                              PARTICIPANT_QOS_DEFAULT,
                              DDS::DomainParticipantListener::_nil(),
                              OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(participant.in())) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l main()")
                        ACE_TEXT(" ERROR: create_participant() failed!\n")), -1);
    }

    // Create Subscriber
    DDS::Subscriber_var sub =
      participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
                                     DDS::SubscriberListener::_nil(),
                                     OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(sub.in())) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l main()")
                        ACE_TEXT(" ERROR: create_subscriber() failed!\n")), -1);
    }

    // Initialize Transport
    OpenDDS::DCPS::TransportImpl_rch transport_impl =
      TheTransportFactory->create_transport_impl(transport_impl_id,
                                                 OpenDDS::DCPS::AUTO_CONFIG);

    OpenDDS::DCPS::AttachStatus status = transport_impl->attach(sub.in());

    if (status != OpenDDS::DCPS::ATTACH_OK) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l main()")
                        ACE_TEXT(" ERROR: attach() failed!\n")), -1);
    }

    DDS::DataReaderQos dr_qos;
    sub->get_default_datareader_qos(dr_qos);
    dr_qos.durability.kind = DDS::TRANSIENT_LOCAL_DURABILITY_QOS;

    // Register Type (OpenDDS::DCPS::ServiceParticipantReport)
    OpenDDS::DCPS::ServiceParticipantReportTypeSupport_var sp_ts =
      new OpenDDS::DCPS::ServiceParticipantReportTypeSupportImpl();

    if (sp_ts->register_type(participant.in(), "") != DDS::RETCODE_OK) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l main()")
                        ACE_TEXT(" ERROR: register_type() failed!\n")), -1);
    }

    // Create Topic
    CORBA::String_var sp_type_name = sp_ts->get_type_name();
    DDS::Topic_var sp_topic =
      participant->create_topic(OpenDDS::DCPS::SERVICE_PARTICIPANT_MONITOR_TOPIC,
                                sp_type_name.in(),
                                TOPIC_QOS_DEFAULT,
                                DDS::TopicListener::_nil(),
                                OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(sp_topic.in())) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l main()")
                        ACE_TEXT(" ERROR: create_topic() failed!\n")), -1);
    }

    // Create DataReader
    DDS::DataReaderListener_var sp_listener(new SPMDataReaderListenerImpl);

    DDS::DataReader_var reader =
      sub->create_datareader(sp_topic.in(),
                             dr_qos,
                             sp_listener.in(),
                             OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(reader.in())) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l main()")
                        ACE_TEXT(" ERROR: create_datareader() failed!\n")), -1);
    }

    // Register Type (OpenDDS::DCPS::DomainParticipantReport)
    OpenDDS::DCPS::DomainParticipantReportTypeSupport_var dp_ts =
      new OpenDDS::DCPS::DomainParticipantReportTypeSupportImpl();

    if (dp_ts->register_type(participant.in(), "") != DDS::RETCODE_OK) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l main()")
                        ACE_TEXT(" ERROR: register_type() failed!\n")), -1);
    }

    // Create Topic
    CORBA::String_var dp_type_name = dp_ts->get_type_name();
    DDS::Topic_var dp_topic =
      participant->create_topic(OpenDDS::DCPS::DOMAIN_PARTICIPANT_MONITOR_TOPIC,
                                dp_type_name.in(),
                                TOPIC_QOS_DEFAULT,
                                DDS::TopicListener::_nil(),
                                OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(dp_topic.in())) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l main()")
                        ACE_TEXT(" ERROR: create_topic() failed!\n")), -1);
    }

    // Create DataReader
    DDS::DataReaderListener_var dp_listener(new DPMDataReaderListenerImpl);

    reader = sub->create_datareader(dp_topic.in(),
                                    dr_qos,
                                    dp_listener.in(),
                                    OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(reader.in())) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l main()")
                        ACE_TEXT(" ERROR: create_datareader() failed!\n")), -1);
    }

    // Register Type (OpenDDS::DCPS::TopicReport)
    OpenDDS::DCPS::TopicReportTypeSupport_var topic_ts =
      new OpenDDS::DCPS::TopicReportTypeSupportImpl();

    if (topic_ts->register_type(participant.in(), "") != DDS::RETCODE_OK) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l main()")
                        ACE_TEXT(" ERROR: register_type() failed!\n")), -1);
    }

    // Create Topic
    CORBA::String_var topic_type_name = topic_ts->get_type_name();
    DDS::Topic_var topic_topic =
      participant->create_topic(OpenDDS::DCPS::TOPIC_MONITOR_TOPIC,
                                topic_type_name.in(),
                                TOPIC_QOS_DEFAULT,
                                DDS::TopicListener::_nil(),
                                OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(topic_topic.in())) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l main()")
                        ACE_TEXT(" ERROR: create_topic() failed!\n")), -1);
    }

    // Create DataReader
    DDS::DataReaderListener_var topic_listener(new TopicMDataReaderListenerImpl);

    reader = sub->create_datareader(topic_topic.in(),
                                    dr_qos,
                                    topic_listener.in(),
                                    OpenDDS::DCPS::DEFAULT_STATUS_MASK);

    if (CORBA::is_nil(reader.in())) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l main()")
                        ACE_TEXT(" ERROR: create_datareader() failed!\n")), -1);
    }

    while (1) {
      ACE_OS::sleep(1);
    }
    // Clean-up!
    participant->delete_contained_entities();
    dpf->delete_participant(participant.in());

    TheTransportFactory->release();
    TheServiceParticipant->shutdown();

  } catch (const CORBA::Exception& e) {
    e._tao_print_exception("Exception caught in main():");
    return -1;
  }

  return 0;
}
