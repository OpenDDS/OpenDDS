#ifndef DDSENTITIES_HEADER
#define DDSENTITIES_HEADER

#include "StatusListener.h"

#include "BenchTypeSupportImpl.h"
#include "PropertyStatBlock.h"

#include <dds/DdsDcpsInfrastructureC.h>
#include <dds/DdsDcpsPublicationC.h>
#include <dds/DCPS/Marked_Default_Qos.h>

class DdsEntities {
public:
  DdsEntities(DDS::DomainParticipantFactory_var dpf, int domain);
  DdsEntities() : dpf_{}, participant_{}, status_reader_{} {}

  ~DdsEntities();

  DDS::DomainParticipantFactory_var dpf_;
  DDS::DomainParticipant_var participant_;

  Bench::NodeController::StatusTypeSupport_var status_ts_;
  DDS::Topic_var status_topic_;
  Bench::TestController::AllocatedScenarioTypeSupport_var allocated_scenario_ts_;
  DDS::Topic_var allocated_scenario_topic_;
  Bench::NodeController::ReportTypeSupport_var report_ts_;
  DDS::Topic_var report_topic_;

  DDS::Publisher_var publisher_;
  DDS::DataWriter_var allocated_scenario_writer_;
  Bench::TestController::AllocatedScenarioDataWriter_var scenario_writer_impl_;

  DDS::Subscriber_var subscriber_;
  DDS::DataReader_var status_reader_;
  Bench::NodeController::StatusDataReader_var status_reader_impl_;
  DDS::DataReader_var report_reader_;
  Bench::NodeController::ReportDataReader_var report_reader_impl_;

  StatusListener status_listener_;
};

#endif
