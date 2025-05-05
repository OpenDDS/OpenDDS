/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "MonitorFactoryImpl.h"

#include "SPMonitorImpl.h"
#include "DPMonitorImpl.h"
#include "TopicMonitorImpl.h"
#include "PublisherMonitorImpl.h"
#include "SubscriberMonitorImpl.h"
#include "DWMonitorImpl.h"
#include "DWPeriodicMonitorImpl.h"
#include "DRMonitorImpl.h"
#include "DRPeriodicMonitorImpl.h"
#include "TransportMonitorImpl.h"

#include "monitorC.h"
#include "monitorTypeSupportImpl.h"

#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/DomainParticipantImpl.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/DCPS_Utils.h>
#include <dds/DCPS/transport/framework/TransportRegistry.h>

#include <dds/DdsDcpsInfrastructureC.h>
#include <dds/DdsDcpsPublicationC.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace Monitor {

MonitorFactoryImpl::MonitorFactoryImpl()
{
}

MonitorFactoryImpl::~MonitorFactoryImpl()
{
  deinitialize();
}

DCPS::Monitor*
MonitorFactoryImpl::create_sp_monitor(DCPS::Service_Participant* sp)
{
  return new SPMonitorImpl(this, sp);
}

DCPS::Monitor*
MonitorFactoryImpl::create_dp_monitor(DCPS::DomainParticipantImpl* dp)
{
  if (dp->get_domain_id() == MONITOR_DOMAIN_ID) {
    return 0;
  }
  return new DPMonitorImpl(dp, this->dp_writer_);
}

ServiceParticipantReportDataWriter_ptr
MonitorFactoryImpl::get_sp_writer()
{
  return ServiceParticipantReportDataWriter::_duplicate(this->sp_writer_);
}

DCPS::Monitor*
MonitorFactoryImpl::create_topic_monitor(DCPS::TopicImpl* topic)
{
  return new TopicMonitorImpl(topic, this->topic_writer_);
}

DCPS::Monitor*
MonitorFactoryImpl::create_publisher_monitor(DCPS::PublisherImpl* pub)
{
  return new PublisherMonitorImpl(pub, this->pub_writer_);
}

DCPS::Monitor*
MonitorFactoryImpl::create_subscriber_monitor(DCPS::SubscriberImpl* sub)
{
  return new SubscriberMonitorImpl(sub, this->sub_writer_);
}

DCPS::Monitor*
MonitorFactoryImpl::create_data_writer_monitor(DCPS::DataWriterImpl* dw)
{
  return new DWMonitorImpl(dw, this->dw_writer_);
}

DCPS::Monitor*
MonitorFactoryImpl::create_data_writer_periodic_monitor(DCPS::DataWriterImpl* dw)
{
  return new DWPeriodicMonitorImpl(dw, this->dw_per_writer_);
}

DCPS::Monitor*
MonitorFactoryImpl::create_data_reader_monitor(DCPS::DataReaderImpl* dr)
{
  return new DRMonitorImpl(dr, this->dr_writer_);
}

DCPS::Monitor*
MonitorFactoryImpl::create_data_reader_periodic_monitor(DCPS::DataReaderImpl* dr)
{
  return new DRPeriodicMonitorImpl(dr, this->dr_per_writer_);
}

DCPS::Monitor*
MonitorFactoryImpl::create_transport_monitor(DCPS::TransportImpl* transport)
{
  return new TransportMonitorImpl(transport, this->transport_writer_);
}

DDS::DataWriter_ptr
MonitorFactoryImpl::create_data_writer(DDS::DomainParticipant_ptr participant,
                                       DDS::Publisher_ptr publisher,
                                       const char* type_name,
                                       const char* topic_name,
                                       const DDS::DataWriterQos& dw_qos)
{
  DDS::Topic_var topic =
    participant->create_topic(topic_name,
                              type_name,
                              TOPIC_QOS_DEFAULT,
                              DDS::TopicListener::_nil(),
                              DCPS::DEFAULT_STATUS_MASK);
  if (CORBA::is_nil(topic)) {
    ACE_DEBUG((LM_DEBUG, "MonitorFactoryImpl::create_data_writer(): Failed to create topic, name = %C\n", topic_name));
  }
  DDS::DataWriter_var writer =
    publisher->create_datawriter(topic.in(),
                                 dw_qos,
                                 DDS::DataWriterListener::_nil(),
                                 DCPS::DEFAULT_STATUS_MASK);
  if (CORBA::is_nil(writer)) {
    ACE_DEBUG((LM_DEBUG, "MonitorFactoryImpl::create_data_writer(): Failed to create data writer\n"));
  }

  return writer._retn();
}

void
MonitorFactoryImpl::initialize()
{
  DDS::DomainParticipantFactory_var dpf =
    TheServiceParticipant->get_domain_participant_factory();
  DDS::DomainParticipant_var participant =
    dpf->create_participant(MONITOR_DOMAIN_ID,
                            PARTICIPANT_QOS_DEFAULT,
                            DDS::DomainParticipantListener::_nil(),
                            DCPS::DEFAULT_STATUS_MASK);
  participant_ = participant;
  if (CORBA::is_nil(participant.in())) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("ERROR: %N:%l: MonitorFactoryImpl::initialize() -")
               ACE_TEXT(" create_participant failed!\n")));
  }

  DDS::Publisher_var publisher =
    participant->create_publisher(PUBLISHER_QOS_DEFAULT,
                                  DDS::PublisherListener::_nil(),
                                  DCPS::DEFAULT_STATUS_MASK);

  static const std::string config_name = DCPS::TransportRegistry::DEFAULT_INST_PREFIX
    + std::string("MonitorBITTransportConfig");
  DCPS::TransportConfig_rch config =
    TheTransportRegistry->get_config (config_name);
  if (config.is_nil ())
  {
    config = DCPS::TransportRegistry::instance()->create_config(config_name);

    std::string inst_name = DCPS::TransportRegistry::DEFAULT_INST_PREFIX
      + std::string("FederationBITTCPTransportInst");
    DCPS::TransportInst_rch inst =
      DCPS::TransportRegistry::instance()->create_inst(inst_name, "tcp");
    config->instances_.push_back(inst);
  }

  DCPS::TransportRegistry::instance()->bind_config(config, publisher.in());

  DDS::DataWriter_var writer;
  DDS::DataWriterQos dw_qos;
  publisher->get_default_datawriter_qos(dw_qos);
  dw_qos.durability.kind = DDS::TRANSIENT_LOCAL_DURABILITY_QOS;

  ServiceParticipantReportTypeSupport_var sp_ts =
    new ServiceParticipantReportTypeSupportImpl();
  ::DDS::ReturnCode_t ret = sp_ts->register_type(participant.in(), "");
  if (DDS::RETCODE_OK == ret) {
    CORBA::String_var sp_type_name = sp_ts->get_type_name();
    writer = create_data_writer(participant.in(),
                                publisher.in(),
                                sp_type_name.in(),
                                SERVICE_PARTICIPANT_MONITOR_TOPIC,
                                dw_qos);
    this->sp_writer_ =
      ServiceParticipantReportDataWriter::_narrow(writer.in());
    if (CORBA::is_nil(this->sp_writer_)) {
      ACE_DEBUG((LM_DEBUG, "MonitorFactoryImpl::initialize(): Failed to narrow sp_writer\n"));
    }
  } else {
    ACE_DEBUG((LM_DEBUG, "MonitorFactoryImpl::initialize(): Failed to register sp_ts\n"));
  }

  DomainParticipantReportTypeSupport_var dp_ts =
    new DomainParticipantReportTypeSupportImpl();
  ret = dp_ts->register_type(participant.in(), "");
  if (DDS::RETCODE_OK == ret) {
    CORBA::String_var dp_type_name = dp_ts->get_type_name();
    writer = create_data_writer(participant.in(),
                                publisher.in(),
                                dp_type_name.in(),
                                DOMAIN_PARTICIPANT_MONITOR_TOPIC,
                                dw_qos);
    this->dp_writer_ =
      DomainParticipantReportDataWriter::_narrow(writer.in());
    if (CORBA::is_nil(this->dp_writer_)) {
      ACE_DEBUG((LM_DEBUG, "MonitorFactoryImpl::initialize(): Failed to narrow dp_writer\n"));
    }
  } else {
    ACE_DEBUG((LM_DEBUG, "MonitorFactoryImpl::initialize(): Failed to register dp_ts\n"));
  }

  TopicReportTypeSupport_var topic_ts =
    new TopicReportTypeSupportImpl();
  ret = topic_ts->register_type(participant.in(), "");
  if (DDS::RETCODE_OK == ret) {
    CORBA::String_var topic_type_name = topic_ts->get_type_name();
    writer = create_data_writer(participant.in(),
                                publisher.in(),
                                topic_type_name.in(),
                                TOPIC_MONITOR_TOPIC,
                                dw_qos);
    this->topic_writer_ =
      TopicReportDataWriter::_narrow(writer.in());
    if (CORBA::is_nil(this->topic_writer_)) {
      ACE_DEBUG((LM_DEBUG, "MonitorFactoryImpl::initialize(): Failed to narrow topic_writer\n"));
    }
  } else {
    ACE_DEBUG((LM_DEBUG, "MonitorFactoryImpl::initialize(): Failed to register topic_ts\n"));
  }

  PublisherReportTypeSupport_var pub_ts =
    new PublisherReportTypeSupportImpl();
  ret = pub_ts->register_type(participant.in(), "");
  if (DDS::RETCODE_OK == ret) {
    CORBA::String_var pub_type_name = pub_ts->get_type_name();
    writer = create_data_writer(participant.in(),
                                publisher.in(),
                                pub_type_name.in(),
                                PUBLISHER_MONITOR_TOPIC,
                                dw_qos);
    this->pub_writer_ =
      PublisherReportDataWriter::_narrow(writer.in());
    if (CORBA::is_nil(this->pub_writer_)) {
      ACE_DEBUG((LM_DEBUG, "MonitorFactoryImpl::initialize(): Failed to narrow pub_writer\n"));
    }
  } else {
    ACE_DEBUG((LM_DEBUG, "MonitorFactoryImpl::initialize(): Failed to register pub_ts\n"));
  }

  SubscriberReportTypeSupport_var sub_ts =
    new SubscriberReportTypeSupportImpl();
  ret = sub_ts->register_type(participant.in(), "");
  if (DDS::RETCODE_OK == ret) {
    CORBA::String_var sub_type_name = sub_ts->get_type_name();
    writer = create_data_writer(participant.in(),
                                publisher.in(),
                                sub_type_name.in(),
                                SUBSCRIBER_MONITOR_TOPIC,
                                dw_qos);
    this->sub_writer_ =
      SubscriberReportDataWriter::_narrow(writer.in());
    if (CORBA::is_nil(this->sub_writer_)) {
      ACE_DEBUG((LM_DEBUG, "MonitorFactoryImpl::initialize(): Failed to narrow sub_writer\n"));
    }
  } else {
    ACE_DEBUG((LM_DEBUG, "MonitorFactoryImpl::initialize(): Failed to register sub_ts\n"));
  }

  DataWriterReportTypeSupport_var dw_ts =
    new DataWriterReportTypeSupportImpl();
  ret = dw_ts->register_type(participant.in(), "");
  if (DDS::RETCODE_OK == ret) {
    CORBA::String_var dw_type_name = dw_ts->get_type_name();
    writer = create_data_writer(participant.in(),
                                publisher.in(),
                                dw_type_name.in(),
                                DATA_WRITER_MONITOR_TOPIC,
                                dw_qos);
    this->dw_writer_ =
      DataWriterReportDataWriter::_narrow(writer.in());
    if (CORBA::is_nil(this->dw_writer_)) {
      ACE_DEBUG((LM_DEBUG, "MonitorFactoryImpl::initialize(): Failed to narrow dw_writer\n"));
    }
  } else {
    ACE_DEBUG((LM_DEBUG, "MonitorFactoryImpl::initialize(): Failed to register sp_ts\n"));
  }

  DataWriterPeriodicReportTypeSupport_var dw_per_ts =
    new DataWriterPeriodicReportTypeSupportImpl();
  ret = dw_per_ts->register_type(participant.in(), "");
  if (DDS::RETCODE_OK == ret) {
    CORBA::String_var dw_per_type_name = dw_per_ts->get_type_name();
    writer = create_data_writer(participant.in(),
                                publisher.in(),
                                dw_per_type_name.in(),
                                DATA_WRITER_PERIODIC_MONITOR_TOPIC,
                                dw_qos);
    this->dw_per_writer_ =
      DataWriterPeriodicReportDataWriter::_narrow(writer.in());
    if (CORBA::is_nil(this->dw_per_writer_)) {
      ACE_DEBUG((LM_DEBUG, "MonitorFactoryImpl::initialize(): Failed to narrow dw_per_writer\n"));
    }
  } else {
    ACE_DEBUG((LM_DEBUG, "MonitorFactoryImpl::initialize(): Failed to register dw_per_ts\n"));
  }

  DataReaderReportTypeSupport_var dr_ts =
    new DataReaderReportTypeSupportImpl();
  ret = dr_ts->register_type(participant.in(), "");
  if (DDS::RETCODE_OK == ret) {
    CORBA::String_var dr_type_name = dr_ts->get_type_name();
    writer = create_data_writer(participant.in(),
                                publisher.in(),
                                dr_type_name.in(),
                                DATA_READER_MONITOR_TOPIC,
                                dw_qos);
    this->dr_writer_ =
      DataReaderReportDataWriter::_narrow(writer.in());
    if (CORBA::is_nil(this->dr_writer_)) {
      ACE_DEBUG((LM_DEBUG, "MonitorFactoryImpl::initialize(): Failed to narrow dr_writer\n"));
    }
  } else {
    ACE_DEBUG((LM_DEBUG, "MonitorFactoryImpl::initialize(): Failed to register dr_ts\n"));
  }

  DataReaderPeriodicReportTypeSupport_var dr_per_ts =
    new DataReaderPeriodicReportTypeSupportImpl();
  ret = dr_per_ts->register_type(participant.in(), "");
  if (DDS::RETCODE_OK == ret) {
    CORBA::String_var dr_per_type_name = dr_per_ts->get_type_name();
    writer = create_data_writer(participant.in(),
                                publisher.in(),
                                dr_per_type_name.in(),
                                DATA_READER_PERIODIC_MONITOR_TOPIC,
                                dw_qos);
    this->dr_per_writer_ =
      DataReaderPeriodicReportDataWriter::_narrow(writer.in());
    if (CORBA::is_nil(this->dr_per_writer_)) {
      ACE_DEBUG((LM_DEBUG, "MonitorFactoryImpl::initialize(): Failed to narrow dr_per_writer\n"));
    }
  } else {
    ACE_DEBUG((LM_DEBUG, "MonitorFactoryImpl::initialize(): Failed to register dr_per_ts\n"));
  }

  TransportReportTypeSupport_var transport_ts =
    new TransportReportTypeSupportImpl();
  ret = transport_ts->register_type(participant.in(), "");
  if (DDS::RETCODE_OK == ret) {
    CORBA::String_var transport_type_name = transport_ts->get_type_name();
    writer = create_data_writer(participant.in(),
                                publisher.in(),
                                transport_type_name.in(),
                                TRANSPORT_MONITOR_TOPIC,
                                dw_qos);
    this->transport_writer_ =
      TransportReportDataWriter::_narrow(writer.in());
    if (CORBA::is_nil(this->transport_writer_)) {
      ACE_DEBUG((LM_DEBUG, "MonitorFactoryImpl::initialize(): Failed to narrow transport_writer\n"));
    }
  } else {
    ACE_DEBUG((LM_DEBUG, "MonitorFactoryImpl::initialize(): Failed to register transport_ts\n"));
  }
}

void MonitorFactoryImpl::deinitialize()
{
  if (participant_) {
    DDS::ReturnCode_t tmp = participant_->delete_contained_entities();
    if (tmp && DCPS::log_level >= DCPS::LogLevel::Error) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: MonitorFactoryImpl::deinitialize: "
        "delete_contained_entities returned %C\n",
        DCPS::retcode_to_string(tmp)));
    }

    DDS::DomainParticipantFactory_var dpf =
      TheServiceParticipant->get_domain_participant_factory();
    tmp = dpf->delete_participant(participant_);
    if (tmp && DCPS::log_level >= DCPS::LogLevel::Error) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: MonitorFactoryImpl::deinitialize: "
        "delete_participant returned %C\n",
        DCPS::retcode_to_string(tmp)));
    }

    participant_ = 0;
  }
}

int
MonitorFactoryImpl::service_initialize()
{
  return ACE_Service_Config::process_directive(ace_svc_desc_MonitorFactoryImpl);
}

}
}

using OpenDDS::Monitor::MonitorFactoryImpl;

ACE_FACTORY_DEFINE (OpenDDS_monitor, MonitorFactoryImpl)
ACE_STATIC_SVC_DEFINE (MonitorFactoryImpl,
                       ACE_TEXT ("OpenDDS_Monitor"),
                       ACE_SVC_OBJ_T,
                       &ACE_SVC_NAME (MonitorFactoryImpl),
                       ACE_Service_Type::DELETE_THIS |
                         ACE_Service_Type::DELETE_OBJ,
                       0)

OPENDDS_END_VERSIONED_NAMESPACE_DECL
