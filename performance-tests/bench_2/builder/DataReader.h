#pragma once

#include "TopicManager.h"

#include "dds/DdsDcpsSubscriptionC.h"

namespace Builder {

class DataReader : public ListenerFactory<DDS::DataReaderListener> {
public:
  explicit DataReader(const DataReaderConfig& config, DataReaderReport& report, DDS::Subscriber_var& subscriber, const std::shared_ptr<TopicManager>& topics);
  ~DataReader();

  void enable();

  void detach_listener();

  DDS::DataReader_var get_dds_datareader() { return datareader_; }
  const DDS::DataReader_var get_dds_datareader() const { return datareader_; }

  DDS::DataReaderListener_var get_dds_datareaderlistener() { return listener_; }
  const DDS::DataReaderListener_var get_dds_datareaderlistener() const { return listener_; }

  DataReaderReport& get_report() { return report_; }
  const DataReaderReport& get_report() const { return report_; }

  DDS::DataReaderQos& get_qos() { return qos_; }
  const DDS::DataReaderQos& get_qos() const { return qos_; }

  const std::string& get_topic_name() const { return topic_name_; }

protected:
  std::string name_;
  std::string topic_name_;
  std::string listener_type_name_;
  const uint32_t listener_status_mask_;
  Builder::PropertySeq listener_properties_;
  const std::string transport_config_name_;
  DataReaderReport& report_;
  DDS::Subscriber_var subscriber_;
  DDS::Topic_var topic_;
  DDS::DataReaderListener_var listener_;
  DDS::DataReader_var datareader_;
  PropertyIndex create_time_;
  PropertyIndex enable_time_;
  PropertyIndex last_discovery_time_;
  DDS::DataReaderQos qos_;
};

using ReaderMap = std::map<std::string, std::shared_ptr<DataReader>>;

}

