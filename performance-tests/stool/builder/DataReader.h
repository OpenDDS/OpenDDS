#pragma once

#include "TopicManager.h"

#include "dds/DdsDcpsSubscriptionC.h"

namespace Builder {

class DataReader : public ListenerFactory<DDS::DataReaderListener> {
public:
  explicit DataReader(const DataReaderConfig& config, DataReaderReport& report, DDS::Subscriber_var& subscriber, const std::shared_ptr<TopicManager>& topics);
  ~DataReader();

  void enable();

  DataReaderReport& get_report() { return report_; }
  const DataReaderReport& get_report() const { return report_; }

protected:
  std::string name_;
  std::string topic_name_;
  std::string listener_type_name_;
  const uint32_t listener_status_mask_;
  const std::string transport_config_name_;
  DataReaderReport& report_;
  DDS::Subscriber_var subscriber_;
  DDS::Topic_var topic_;
  DDS::DataReaderListener_var listener_;
  DDS::DataReader_var datareader_;
};

using ReaderMap = std::map<std::string, std::shared_ptr<DataReader>>;

}

