#include "DdsEntities.h"
#include "StatusListener.h"

#include <dds/DdsDcpsInfrastructureC.h>
#include <dds/DCPS/Service_Participant.h>

#include <stdexcept>

using namespace Bench::NodeController;

DdsEntities::DdsEntities(
  DDS::DomainParticipantFactory_var dpf,
  int domain)
  : dpf_(dpf)
  , participant_(dpf->create_participant(domain, PARTICIPANT_QOS_DEFAULT,
                                         nullptr, OpenDDS::DCPS::DEFAULT_STATUS_MASK))
{
  if (!participant_) {
    throw std::runtime_error("create_participant failed");
  }

  // Create Topics
  status_ts_ = new StatusTypeSupportImpl;
  if (status_ts_->register_type(participant_, "")) {
    throw std::runtime_error("register_type failed for Status");
  }
  char* status_type_name = status_ts_->get_type_name();
  status_topic_ = participant_->create_topic(
    status_topic_name, status_type_name, TOPIC_QOS_DEFAULT, nullptr,
    OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!status_topic_) {
    throw std::runtime_error("create_topic status failed");
  }
  delete [] status_type_name;
  allocated_scenario_ts_ = new Bench::TestController::AllocatedScenarioTypeSupportImpl;
  if (allocated_scenario_ts_->register_type(participant_, "")) {
    throw std::runtime_error("register_type failed for Config");
  }
  char* allocated_scenario_type_name = allocated_scenario_ts_->get_type_name();
  allocated_scenario_topic_ = participant_->create_topic(
    allocated_scenario_topic_name, allocated_scenario_type_name, TOPIC_QOS_DEFAULT, nullptr,
    OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!allocated_scenario_topic_) {
    throw std::runtime_error("create_topic config failed");
  }
  delete [] allocated_scenario_type_name;
  report_ts_ = new ReportTypeSupportImpl;
  if (report_ts_->register_type(participant_, "")) {
    throw std::runtime_error("register_type failed for Report");
  }
  char* report_type_name = report_ts_->get_type_name();
  report_topic_ = participant_->create_topic(
    report_topic_name, report_type_name, TOPIC_QOS_DEFAULT, nullptr,
    OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!report_topic_) {
    throw std::runtime_error("create_topic report failed");
  }
  delete [] report_type_name;

  // Create DataWriters
  publisher_ = participant_->create_publisher(
    PUBLISHER_QOS_DEFAULT, nullptr, OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!publisher_) {
    throw std::runtime_error("create_publisher failed");
  }
  DDS::DataWriterQos dw_qos;
  publisher_->get_default_datawriter_qos(dw_qos);
  dw_qos.history.kind = DDS::KEEP_ALL_HISTORY_QOS;
  dw_qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;
  dw_qos.durability.kind = DDS::TRANSIENT_LOCAL_DURABILITY_QOS;
  allocated_scenario_writer_ = publisher_->create_datawriter(
    allocated_scenario_topic_, dw_qos, nullptr,
    OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!allocated_scenario_writer_) {
    throw std::runtime_error("create_datawriter config failed");
  }
  scenario_writer_impl_ = Bench::TestController::AllocatedScenarioDataWriter::_narrow(allocated_scenario_writer_);
  if (!scenario_writer_impl_) {
    throw std::runtime_error("narrow writer config failed");
  }

  // Create DataReaders
  subscriber_ = participant_->create_subscriber(
    SUBSCRIBER_QOS_DEFAULT, nullptr, OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!subscriber_) {
    throw std::runtime_error("create_subscriber failed");
  }
  DDS::DataReaderQos dr_qos;
  subscriber_->get_default_datareader_qos(dr_qos);
  dr_qos.history.kind = DDS::KEEP_LAST_HISTORY_QOS;
  dr_qos.history.depth = 1;
  dr_qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;
  dr_qos.durability.kind = DDS::TRANSIENT_LOCAL_DURABILITY_QOS;
  status_reader_ = subscriber_->create_datareader(
    status_topic_, dr_qos, &status_listener_,
    OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!status_reader_) {
    throw std::runtime_error("create_datareader status failed");
  }
  status_reader_impl_ = StatusDataReader::_narrow(status_reader_);
  if (!status_reader_impl_) {
    throw std::runtime_error("narrow status reader failed");
  }
  subscriber_->get_default_datareader_qos(dr_qos);
  dr_qos.history.kind = DDS::KEEP_ALL_HISTORY_QOS;
  dr_qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;
  report_reader_ = subscriber_->create_datareader(
    report_topic_, dr_qos, nullptr,
    OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  if (!report_reader_) {
    throw std::runtime_error("create_datareader report failed");
  }
  report_reader_impl_ = ReportDataReader::_narrow(report_reader_);
  if (!report_reader_impl_) {
    throw std::runtime_error("narrow report reader failed");
  }
}


DdsEntities::~DdsEntities() {
  if (status_reader_) {
    status_reader_->set_listener(0, 0);
  }
  if (participant_) {
    participant_->delete_contained_entities();
  }
  if (dpf_) {
    dpf_->delete_participant(participant_);
    TheServiceParticipant->shutdown();
  }
}
