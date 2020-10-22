#include "DataWriter.h"

#include "DataWriterListener.h"

#include "dds/DCPS/transport/framework/TransportRegistry.h"

namespace Builder {

DataWriter::DataWriter(const DataWriterConfig& config, DataWriterReport& report, DDS::Publisher_var& publisher, const std::shared_ptr<TopicManager>& topics)
  : name_(config.name.in())
  , topic_name_(config.topic_name.in())
  , listener_type_name_(config.listener_type_name.in())
  , listener_status_mask_(config.listener_status_mask)
  , listener_properties_(config.listener_properties)
  , transport_config_name_(config.transport_config_name.in())
  , report_(report)
  , publisher_(publisher)
  , create_time_(get_or_create_property(report_.properties, "create_time", Builder::PVK_TIME))
  , enable_time_(get_or_create_property(report_.properties, "enable_time", Builder::PVK_TIME))
  , last_discovery_time_(get_or_create_property(report_.properties, "last_discovery_time", Builder::PVK_TIME))
{
  Log::log() << "Creating datawriter: '" << name_ << "' with topic name '" << topic_name_
    << "' and listener type name '" << listener_type_name_ << "'" << std::endl;

  auto topic_ptr = topics->get_topic_by_name(topic_name_);
  if (!topic_ptr) {
    std::stringstream ss;
    ss << "topic lookup failed in datawriter '" << name_ << "' for topic '" << topic_name_ << "'" << std::flush;
    throw std::runtime_error(ss.str());
  }
  topic_ = topic_ptr->get_dds_topic();

  DDS::DataWriterQos qos;
  publisher_->get_default_datawriter_qos(qos);

  //DurabilityQosPolicyMask durability;
  APPLY_QOS_MASK(qos, config.qos, config.qos_mask, durability, kind);
  //DurabilityServiceQosPolicyMask durability_service;
  APPLY_QOS_MASK(qos, config.qos, config.qos_mask, durability_service, service_cleanup_delay);
  APPLY_QOS_MASK(qos, config.qos, config.qos_mask, durability_service, history_kind);
  APPLY_QOS_MASK(qos, config.qos, config.qos_mask, durability_service, history_depth);
  APPLY_QOS_MASK(qos, config.qos, config.qos_mask, durability_service, max_samples);
  APPLY_QOS_MASK(qos, config.qos, config.qos_mask, durability_service, max_instances);
  APPLY_QOS_MASK(qos, config.qos, config.qos_mask, durability_service, max_samples_per_instance);
  //DeadlineQosPolicyMask deadline;
  APPLY_QOS_MASK(qos, config.qos, config.qos_mask, deadline, period);
  //LatencyBudgetQosPolicyMask latency_budget;
  APPLY_QOS_MASK(qos, config.qos, config.qos_mask, latency_budget, duration);
  //LivelinessQosPolicyMask liveliness;
  APPLY_QOS_MASK(qos, config.qos, config.qos_mask, liveliness, kind);
  APPLY_QOS_MASK(qos, config.qos, config.qos_mask, liveliness, lease_duration);
  //ReliabilityQosPolicyMask reliability;
  APPLY_QOS_MASK(qos, config.qos, config.qos_mask, reliability, kind);
  APPLY_QOS_MASK(qos, config.qos, config.qos_mask, reliability, max_blocking_time);
  //DestinationOrderQosPolicyMask destination_order;
  APPLY_QOS_MASK(qos, config.qos, config.qos_mask, destination_order, kind);
  //HistoryQosPolicyMask history;
  APPLY_QOS_MASK(qos, config.qos, config.qos_mask, history, kind);
  APPLY_QOS_MASK(qos, config.qos, config.qos_mask, history, depth);
  //ResourceLimitsQosPolicyMask resource_limits;
  APPLY_QOS_MASK(qos, config.qos, config.qos_mask, resource_limits, max_samples);
  APPLY_QOS_MASK(qos, config.qos, config.qos_mask, resource_limits, max_instances);
  APPLY_QOS_MASK(qos, config.qos, config.qos_mask, resource_limits, max_samples_per_instance);
  //TransportPriorityQosPolicyMask transport_priority;
  APPLY_QOS_MASK(qos, config.qos, config.qos_mask, transport_priority, value);
  //LifespanQosPolicyMask lifespan;
  APPLY_QOS_MASK(qos, config.qos, config.qos_mask, lifespan, duration);
  //UserDataQosPolicyMask user_data;
  APPLY_QOS_MASK(qos, config.qos, config.qos_mask, user_data, value);
  //OwnershipQosPolicyMask ownership;
  APPLY_QOS_MASK(qos, config.qos, config.qos_mask, ownership, kind);
  //OwnershipStrengthQosPolicyMask ownership_strength;
  APPLY_QOS_MASK(qos, config.qos, config.qos_mask, ownership_strength, value);
  //WriterDataLifecycleQosPolicyMask writer_data_lifecycle;
  APPLY_QOS_MASK(qos, config.qos, config.qos_mask, writer_data_lifecycle, autodispose_unregistered_instances);

  // Create Listener From Factory
  listener_ = DDS::DataWriterListener::_nil();
  if (!listener_type_name_.empty()) {
    listener_ = create_listener(listener_type_name_, listener_properties_);
    if (!listener_) {
      std::stringstream ss;
      ss << "datareader listener creation failed for datawriter '" << name_
        << "' with listener type name '" << listener_type_name_ << "'" << std::flush;
      throw std::runtime_error(ss.str());
    } else {
      DataWriterListener* savvy_listener_ = dynamic_cast<DataWriterListener*>(listener_.in());
      if (savvy_listener_) {
        savvy_listener_->set_datawriter(*this);
      }
    }
  }

  enable_time_->value.time_prop(ZERO);
  last_discovery_time_->value.time_prop(Builder::ZERO);

  create_time_->value.time_prop(get_hr_time());

  DDS::PublisherQos publisher_qos;
  if (publisher_->get_qos(publisher_qos) == DDS::RETCODE_OK && publisher_qos.entity_factory.autoenable_created_entities == true) {
    enable_time_->value.time_prop(create_time_->value.time_prop());
  }

  datawriter_ = publisher_->create_datawriter(topic_, qos, listener_, listener_status_mask_);
  if (CORBA::is_nil(datawriter_.in())) {
    throw std::runtime_error("datawriter creation failed");
  }

  if (!transport_config_name_.empty()) {
    Log::log() << "Binding config for datawriter " << name_ << " (" << transport_config_name_ << ")" << std::endl;
    TheTransportRegistry->bind_config(transport_config_name_.c_str(), datawriter_);
  }
}

DataWriter::~DataWriter() {
  Log::log() << "Deleting datawriter: " << name_ << std::endl;
  if (!CORBA::is_nil(datawriter_.in())) {
    if (publisher_->delete_datawriter(datawriter_) != DDS::RETCODE_OK) {
      Log::log() << "Error deleting datawriter: " << name_ << std::endl;
    }
  }
}

void DataWriter::enable() {
  if (enable_time_->value.time_prop() == ZERO) {
    enable_time_->value.time_prop(get_hr_time());
    datawriter_->enable();
  }
}

void DataWriter::detach_listener() {
  if (listener_) {
    DataWriterListener* savvy_listener_ = dynamic_cast<DataWriterListener*>(listener_.in());
    if (savvy_listener_) {
      savvy_listener_->unset_datawriter(*this);
    }
    if (datawriter_) {
      datawriter_->set_listener(DDS::DataWriterListener::_nil(), 0);
    }
    listener_ = DDS::DataWriterListener::_nil();
  }
}

}

