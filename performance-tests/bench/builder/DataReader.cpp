#include "DataReader.h"

#include "DataReaderListener.h"

#include "dds/DCPS/transport/framework/TransportRegistry.h"

namespace Builder {

DataReader::DataReader(const DataReaderConfig& config, DataReaderReport& report, DDS::Subscriber_var& subscriber,
  const std::shared_ptr<TopicManager>& topics, const ContentFilteredTopicMap& cft_map)
  : name_(config.name.in())
  , topic_name_(config.topic_name.in())
  , listener_type_name_(config.listener_type_name.in())
  , listener_status_mask_(config.listener_status_mask)
  , listener_properties_(config.listener_properties)
  , transport_config_name_(config.transport_config_name.in())
  , report_(report)
  , subscriber_(subscriber)
  , create_time_(get_or_create_property(report_.properties, "create_time", Builder::PVK_TIME))
  , enable_time_(get_or_create_property(report_.properties, "enable_time", Builder::PVK_TIME))
  , last_discovery_time_(get_or_create_property(report_.properties, "last_discovery_time", Builder::PVK_TIME))
{
  Log::log() << "Creating datareader: '" << name_ << "' with topic name '" << topic_name_
    << "' and listener type name '" << listener_type_name_ << "'" << std::endl;

  // Associate user tags to the report
  report_.tags = config.tags;

  // Customize QoS Object
  subscriber_->get_default_datareader_qos(qos_);

  //DurabilityQosPolicyMask durability;
  APPLY_QOS_MASK(qos_, config.qos, config.qos_mask, durability, kind);
  //DeadlineQosPolicyMask deadline;
  APPLY_QOS_MASK(qos_, config.qos, config.qos_mask, deadline, period);
  //LatencyBudgetQosPolicyMask latency_budget;
  APPLY_QOS_MASK(qos_, config.qos, config.qos_mask, latency_budget, duration);
  //LivelinessQosPolicyMask liveliness;
  APPLY_QOS_MASK(qos_, config.qos, config.qos_mask, liveliness, kind);
  APPLY_QOS_MASK(qos_, config.qos, config.qos_mask, liveliness, lease_duration);
  //ReliabilityQosPolicyMask reliability;
  APPLY_QOS_MASK(qos_, config.qos, config.qos_mask, reliability, kind);
  APPLY_QOS_MASK(qos_, config.qos, config.qos_mask, reliability, max_blocking_time);
  //DestinationOrderQosPolicyMask destination_order;
  APPLY_QOS_MASK(qos_, config.qos, config.qos_mask, destination_order, kind);
  //HistoryQosPolicyMask history;
  APPLY_QOS_MASK(qos_, config.qos, config.qos_mask, history, kind);
  APPLY_QOS_MASK(qos_, config.qos, config.qos_mask, history, depth);
  //ResourceLimitsQosPolicyMask resource_limits;
  APPLY_QOS_MASK(qos_, config.qos, config.qos_mask, resource_limits, max_samples);
  APPLY_QOS_MASK(qos_, config.qos, config.qos_mask, resource_limits, max_instances);
  APPLY_QOS_MASK(qos_, config.qos, config.qos_mask, resource_limits, max_samples_per_instance);
  //UserDataQosPolicyMask user_data;
  APPLY_QOS_MASK(qos_, config.qos, config.qos_mask, user_data, value);
  //OwnershipQosPolicyMask ownership;
  APPLY_QOS_MASK(qos_, config.qos, config.qos_mask, ownership, kind);
  //TimeBasedFilterQosPolicyMask time_based_filter;
  APPLY_QOS_MASK(qos_, config.qos, config.qos_mask, time_based_filter, minimum_separation);
  //ReaderDataLifecycleQosPolicyMask reader_data_lifecycle;
  APPLY_QOS_MASK(qos_, config.qos, config.qos_mask, reader_data_lifecycle, autopurge_nowriter_samples_delay);
  APPLY_QOS_MASK(qos_, config.qos, config.qos_mask, reader_data_lifecycle, autopurge_disposed_samples_delay);

  // Create Listener From Factory
  listener_ = DDS::DataReaderListener::_nil();
  if (!listener_type_name_.empty()) {
    listener_ = create_listener(listener_type_name_, listener_properties_);
    if (!listener_) {
      std::stringstream ss;
      ss << "datareader listener creation failed for datareader '" << name_
        << "' with listener type name '" << listener_type_name_ << "'" << std::flush;
      throw std::runtime_error(ss.str());
    } else {
      DataReaderListener* savvy_listener = dynamic_cast<DataReaderListener*>(listener_.in());
      if (savvy_listener) {
        savvy_listener->set_datareader(*this);
      }
    }
  }

  enable_time_->value.time_prop(ZERO);
  last_discovery_time_->value.time_prop(Builder::ZERO);

  create_time_->value.time_prop(get_hr_time());

  DDS::SubscriberQos subscriber_qos;
  if (subscriber_->get_qos(subscriber_qos) == DDS::RETCODE_OK && subscriber_qos.entity_factory.autoenable_created_entities == true) {
    enable_time_->value.time_prop(create_time_->value.time_prop());
  }

  auto topic_ptr = topics->get_topic_by_name(topic_name_);
  if (topic_ptr) {
    DDS::Topic_var topic = topic_ptr->get_dds_topic();
    datareader_ = subscriber_->create_datareader(topic, qos_, listener_, listener_status_mask_);
  } else {
#ifndef OPENDDS_NO_CONTENT_FILTERED_TOPIC
    DDS::ContentFilteredTopic_var content_filtered_topic;
    auto it = cft_map.find(topic_name_);
    if (it != cft_map.end()) {
      content_filtered_topic = it->second;
    }

    if (!content_filtered_topic) {
      std::stringstream ss;
      ss << "Topic lookup failed in datareader '" << config.name << "' for cft topic '" << topic_name_ << "'" << std::flush;
      throw std::runtime_error(ss.str());
    }

    datareader_ = subscriber_->create_datareader(content_filtered_topic, qos_, listener_, listener_status_mask_);
#else
  ACE_UNUSED_ARG(cft_map);
#endif
  }

  if (CORBA::is_nil(datareader_.in())) {
    throw std::runtime_error("datareader creation failed");
  }

  if (!transport_config_name_.empty()) {
    Log::log() << "Binding config for datareader " << name_ << " (" << transport_config_name_ << ")" << std::endl;
    TheTransportRegistry->bind_config(transport_config_name_.c_str(), datareader_);
  }
}

DataReader::~DataReader()
{
  detach_listener();
  Log::log() << "Deleting datareader: " << name_ << std::endl;
  if (!CORBA::is_nil(datareader_.in())) {
    if (subscriber_->delete_datareader(datareader_) != DDS::RETCODE_OK) {
      Log::log() << "Error deleting datareader: " << name_ << std::endl;
    }
  }
}

bool DataReader::enable(bool throw_on_error)
{
  if (enable_time_->value.time_prop() == ZERO) {
    enable_time_->value.time_prop(get_hr_time());
  }
  bool result = (datareader_->enable() == DDS::RETCODE_OK);
  if (!result && throw_on_error) {
    std::stringstream ss;
    ss << "failed to enable datareader '" << name_ << "'" << std::flush;
    throw std::runtime_error(ss.str());
  }
  return result;
}

void DataReader::detach_listener()
{
  if (listener_) {
    DataReaderListener* savvy_listener_ = dynamic_cast<DataReaderListener*>(listener_.in());
    if (savvy_listener_) {
      savvy_listener_->unset_datareader(*this);
    }
    if (datareader_) {
      datareader_->set_listener(0, OpenDDS::DCPS::NO_STATUS_MASK);
    }
    listener_ = 0;
  }
}

}
