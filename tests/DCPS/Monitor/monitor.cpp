/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <ace/Argv_Type_Converter.h>
#include <ace/Get_Opt.h>
#include <ace/Log_Msg.h>
#include <ace/OS_NS_stdlib.h>
#include <ace/OS_NS_unistd.h>

#include <dds/DdsDcpsInfrastructureC.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/SubscriberImpl.h>
#include <dds/DCPS/WaitSet.h>

#include "dds/DCPS/StaticIncludes.h"

#include "SPMDataReaderListener.h"
#include "DPMDataReaderListener.h"
#include "TopicMDataReaderListener.h"
#include "PublisherMDataReaderListener.h"
#include "SubscriberMDataReaderListener.h"
#include "DWMDataReaderListener.h"
#include "DWPerMDataReaderListener.h"
#include "DRMDataReaderListener.h"
#include "DRPerMDataReaderListener.h"
#include "TransportMDataReaderListener.h"
#include <dds/monitor/monitorTypeSupportImpl.h>
#include <fstream>

DDS::DataReader_ptr
create_data_reader(DDS::DomainParticipant_ptr participant,
                   DDS::Subscriber_ptr subscriber,
                   const char* type_name,
                   const char* topic_name,
                   const DDS::DataReaderQos& dr_qos,
                   DDS::DataReaderListener_ptr drl)
{
  DDS::Topic_var topic =
    participant->create_topic(topic_name,
                              type_name,
                              TOPIC_QOS_DEFAULT,
                              DDS::TopicListener::_nil(),
                              OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (CORBA::is_nil(topic)) {
    ACE_DEBUG((LM_DEBUG, "create_data_reader(): Failed to create topic, name = %C\n", topic_name));
  }
  DDS::DataReader_var reader =
    subscriber->create_datareader(topic.in(),
                                  dr_qos,
                                  drl,
                                  OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (CORBA::is_nil(reader)) {
    ACE_DEBUG((LM_DEBUG, "create_data_reader(): Failed to create data reader\n"));
  }

  return reader._retn();
}

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

    DDS::DataReaderQos dr_qos;
    sub->get_default_datareader_qos(dr_qos);
    dr_qos.durability.kind = DDS::TRANSIENT_LOCAL_DURABILITY_QOS;
    DDS::DataReader_var reader;

    // Register for OpenDDS::DCPS::ServiceParticipantReport
    OpenDDS::DCPS::ServiceParticipantReportTypeSupport_var sp_ts =
      new OpenDDS::DCPS::ServiceParticipantReportTypeSupportImpl();
    if (sp_ts->register_type(participant.in(), "") != DDS::RETCODE_OK) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l main()")
                        ACE_TEXT(" ERROR: register_type() failed!\n")), -1);
    }
    CORBA::String_var sp_type_name = sp_ts->get_type_name();
    DDS::DataReaderListener_var sp_drl(new SPMDataReaderListenerImpl);
    reader = create_data_reader(participant.in(),
                                sub.in(),
                                sp_type_name.in(),
                                OpenDDS::DCPS::SERVICE_PARTICIPANT_MONITOR_TOPIC,
                                dr_qos,
                                sp_drl);
    if (CORBA::is_nil(reader.in())) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l main()")
                        ACE_TEXT(" ERROR: create_datareader() failed!\n")), -1);
    }

    // Register for OpenDDS::DCPS::DomainParticipantReport
    OpenDDS::DCPS::DomainParticipantReportTypeSupport_var dp_ts =
      new OpenDDS::DCPS::DomainParticipantReportTypeSupportImpl();
    if (dp_ts->register_type(participant.in(), "") != DDS::RETCODE_OK) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l main()")
                        ACE_TEXT(" ERROR: register_type() failed!\n")), -1);
    }
    CORBA::String_var dp_type_name = dp_ts->get_type_name();
    DDS::DataReaderListener_var dp_drl(new DPMDataReaderListenerImpl);
    reader = create_data_reader(participant.in(),
                                sub.in(),
                                dp_type_name.in(),
                                OpenDDS::DCPS::DOMAIN_PARTICIPANT_MONITOR_TOPIC,
                                dr_qos,
                                dp_drl);
    if (CORBA::is_nil(reader.in())) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l main()")
                        ACE_TEXT(" ERROR: create_datareader() failed!\n")), -1);
    }

    // Register for OpenDDS::DCPS::TopicReport
    OpenDDS::DCPS::TopicReportTypeSupport_var topic_ts =
      new OpenDDS::DCPS::TopicReportTypeSupportImpl();
    if (topic_ts->register_type(participant.in(), "") != DDS::RETCODE_OK) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l main()")
                        ACE_TEXT(" ERROR: register_type() failed!\n")), -1);
    }
    CORBA::String_var topic_type_name = topic_ts->get_type_name();
    DDS::DataReaderListener_var topic_drl(new TopicMDataReaderListenerImpl);
    reader = create_data_reader(participant.in(),
                                sub.in(),
                                topic_type_name.in(),
                                OpenDDS::DCPS::TOPIC_MONITOR_TOPIC,
                                dr_qos,
                                topic_drl);
    if (CORBA::is_nil(reader.in())) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l main()")
                        ACE_TEXT(" ERROR: create_datareader() failed!\n")), -1);
    }

    // Register for OpenDDS::DCPS::PublisherReport
    OpenDDS::DCPS::PublisherReportTypeSupport_var publisher_ts =
      new OpenDDS::DCPS::PublisherReportTypeSupportImpl();
    if (publisher_ts->register_type(participant.in(), "") != DDS::RETCODE_OK) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l main()")
                        ACE_TEXT(" ERROR: register_type() failed!\n")), -1);
    }
    CORBA::String_var publisher_type_name = publisher_ts->get_type_name();
    DDS::DataReaderListener_var publisher_drl(new PublisherMDataReaderListenerImpl);
    reader = create_data_reader(participant.in(),
                                sub.in(),
                                publisher_type_name.in(),
                                OpenDDS::DCPS::PUBLISHER_MONITOR_TOPIC,
                                dr_qos,
                                publisher_drl);
    if (CORBA::is_nil(reader.in())) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l main()")
                        ACE_TEXT(" ERROR: create_datareader() failed!\n")), -1);
    }

    // Register for OpenDDS::DCPS::SubscriberReport
    OpenDDS::DCPS::SubscriberReportTypeSupport_var subscriber_ts =
      new OpenDDS::DCPS::SubscriberReportTypeSupportImpl();
    if (subscriber_ts->register_type(participant.in(), "") != DDS::RETCODE_OK) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l main()")
                        ACE_TEXT(" ERROR: register_type() failed!\n")), -1);
    }
    CORBA::String_var subscriber_type_name = subscriber_ts->get_type_name();
    DDS::DataReaderListener_var subscriber_drl(new SubscriberMDataReaderListenerImpl);
    reader = create_data_reader(participant.in(),
                                sub.in(),
                                subscriber_type_name.in(),
                                OpenDDS::DCPS::SUBSCRIBER_MONITOR_TOPIC,
                                dr_qos,
                                subscriber_drl);
    if (CORBA::is_nil(reader.in())) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l main()")
                        ACE_TEXT(" ERROR: create_datareader() failed!\n")), -1);
    }

    // Register for OpenDDS::DCPS::DataWriterReport
    OpenDDS::DCPS::DataWriterReportTypeSupport_var dw_ts =
      new OpenDDS::DCPS::DataWriterReportTypeSupportImpl();
    if (dw_ts->register_type(participant.in(), "") != DDS::RETCODE_OK) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l main()")
                        ACE_TEXT(" ERROR: register_type() failed!\n")), -1);
    }
    CORBA::String_var dw_type_name = dw_ts->get_type_name();
    DDS::DataReaderListener_var dw_drl(new DWMDataReaderListenerImpl);
    reader = create_data_reader(participant.in(),
                                sub.in(),
                                dw_type_name.in(),
                                OpenDDS::DCPS::DATA_WRITER_MONITOR_TOPIC,
                                dr_qos,
                                dw_drl);
    if (CORBA::is_nil(reader.in())) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l main()")
                        ACE_TEXT(" ERROR: create_datareader() failed!\n")), -1);
    }

    // Register for OpenDDS::DCPS::DataWriterPeriodicReport
    OpenDDS::DCPS::DataWriterPeriodicReportTypeSupport_var dwper_ts =
      new OpenDDS::DCPS::DataWriterPeriodicReportTypeSupportImpl();
    if (dwper_ts->register_type(participant.in(), "") != DDS::RETCODE_OK) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l main()")
                        ACE_TEXT(" ERROR: register_type() failed!\n")), -1);
    }
    CORBA::String_var dwper_type_name = dwper_ts->get_type_name();
    DDS::DataReaderListener_var dwper_drl(new DWPerMDataReaderListenerImpl);
    reader = create_data_reader(participant.in(),
                                sub.in(),
                                dwper_type_name.in(),
                                OpenDDS::DCPS::DATA_WRITER_PERIODIC_MONITOR_TOPIC,
                                dr_qos,
                                dwper_drl);
    if (CORBA::is_nil(reader.in())) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l main()")
                        ACE_TEXT(" ERROR: create_datareader() failed!\n")), -1);
    }

    // Register for OpenDDS::DCPS::DataReaderReport
    OpenDDS::DCPS::DataReaderReportTypeSupport_var dr_ts =
      new OpenDDS::DCPS::DataReaderReportTypeSupportImpl();
    if (dr_ts->register_type(participant.in(), "") != DDS::RETCODE_OK) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l main()")
                        ACE_TEXT(" ERROR: register_type() failed!\n")), -1);
    }
    CORBA::String_var dr_type_name = dr_ts->get_type_name();
    DDS::DataReaderListener_var dr_drl(new DRMDataReaderListenerImpl);
    reader = create_data_reader(participant.in(),
                                sub.in(),
                                dr_type_name.in(),
                                OpenDDS::DCPS::DATA_READER_MONITOR_TOPIC,
                                dr_qos,
                                dr_drl);
    if (CORBA::is_nil(reader.in())) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l main()")
                        ACE_TEXT(" ERROR: create_datareader() failed!\n")), -1);
    }

    // Register for OpenDDS::DCPS::DataReaderPeriodicReport
    OpenDDS::DCPS::DataReaderPeriodicReportTypeSupport_var drper_ts =
      new OpenDDS::DCPS::DataReaderPeriodicReportTypeSupportImpl();
    if (drper_ts->register_type(participant.in(), "") != DDS::RETCODE_OK) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l main()")
                        ACE_TEXT(" ERROR: register_type() failed!\n")), -1);
    }
    CORBA::String_var drper_type_name = drper_ts->get_type_name();
    DDS::DataReaderListener_var drper_drl(new DRPerMDataReaderListenerImpl);
    reader = create_data_reader(participant.in(),
                                sub.in(),
                                drper_type_name.in(),
                                OpenDDS::DCPS::DATA_READER_PERIODIC_MONITOR_TOPIC,
                                dr_qos,
                                drper_drl);
    if (CORBA::is_nil(reader.in())) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l main()")
                        ACE_TEXT(" ERROR: create_datareader() failed!\n")), -1);
    }

    // Register for OpenDDS::DCPS::TransportReport
    OpenDDS::DCPS::TransportReportTypeSupport_var transport_ts =
      new OpenDDS::DCPS::TransportReportTypeSupportImpl();
    if (transport_ts->register_type(participant.in(), "") != DDS::RETCODE_OK) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l main()")
                        ACE_TEXT(" ERROR: register_type() failed!\n")), -1);
    }
    CORBA::String_var transport_type_name = transport_ts->get_type_name();
    DDS::DataReaderListener_var transport_drl(new TransportMDataReaderListenerImpl);
    reader = create_data_reader(participant.in(),
                                sub.in(),
                                transport_type_name.in(),
                                OpenDDS::DCPS::TRANSPORT_MONITOR_TOPIC,
                                dr_qos,
                                transport_drl);
    if (CORBA::is_nil(reader.in())) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("%N:%l main()")
                        ACE_TEXT(" ERROR: create_datareader() failed!\n")), -1);
    }

    std::ofstream ofs("mon_ready.txt");
    ofs << "Ready" << std::endl;
    ofs.close();
    while (1) {
      ACE_OS::sleep(1);
    }
    // Clean-up!
    participant->delete_contained_entities();
    dpf->delete_participant(participant.in());

    TheServiceParticipant->shutdown();

  } catch (const CORBA::Exception& e) {
    e._tao_print_exception("Exception caught in main():");
    return -1;
  }

  return 0;
}
