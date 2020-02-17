#pragma once

#include "Common.h"
#include "ListenerFactory.h"
#include "TopicManager.h"

#include "dds/DdsDcpsPublicationC.h"

#include <map>

namespace Builder {

class DataWriter : public ListenerFactory<DDS::DataWriterListener> {
public:
  explicit DataWriter(const DataWriterConfig& config, DataWriterReport& report, DDS::Publisher_var& publisher, const std::shared_ptr<TopicManager>& topics);
  ~DataWriter();

  void enable();

  void detach_listener();

  DDS::DataWriter_var get_dds_datawriter() { return datawriter_; }
  const DDS::DataWriter_var get_dds_datawriter() const { return datawriter_; }

  DDS::DataWriterListener_var get_dds_datawriterlistener() { return listener_; }
  const DDS::DataWriterListener_var get_dds_datawriterlistener() const { return listener_; }

  DataWriterReport& get_report() { return report_; }
  const DataWriterReport& get_report() const { return report_; }

protected:
  std::string name_;
  std::string topic_name_;
  std::string listener_type_name_;
  const uint32_t listener_status_mask_;
  const std::string transport_config_name_;
  DataWriterReport& report_;
  DDS::Publisher_var publisher_;
  DDS::Topic_var topic_;
  DDS::DataWriterListener_var listener_;
  DDS::DataWriter_var datawriter_;
  PropertyIndex create_time_;
  PropertyIndex enable_time_;
  PropertyIndex last_discovery_time_;
};

using WriterMap = std::map<std::string, std::shared_ptr<DataWriter>>;

}

