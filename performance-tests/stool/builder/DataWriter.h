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
};

using WriterMap = std::map<std::string, std::shared_ptr<DataWriter>>;

}

