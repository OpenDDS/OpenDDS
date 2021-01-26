#include "Topic.h"

#include "TopicListener.h"
#include "TypeSupportRegistry.h"

#include "dds/DdsDcpsDomainC.h"
#include "dds/DCPS/transport/framework/TransportRegistry.h"

namespace Builder {

Topic::Topic(const TopicConfig& config, DDS::DomainParticipant_var& participant,
  ContentFilteredTopicMap& content_filtered_topics_map)
  : name_(config.name.in())
  , type_name_((strlen(config.type_name.in()) == 0 && TypeSupportRegistry::get_type_names().size() == 1) ? TypeSupportRegistry::get_type_names().front() : config.type_name.in())
  , listener_type_name_(config.listener_type_name.in())
  , listener_status_mask_(config.listener_status_mask)
  , listener_properties_(config.listener_properties)
  , transport_config_name_(config.transport_config_name.in())
  , content_filtered_topics_(config.content_filtered_topics)
  , participant_(participant)
{
  Log::log()
    << "Creating topic: '" << name_ << "' with type name '" << type_name_
    << "' and listener type name '" << listener_type_name_ << "'" << std::endl;

  // Customize QoS Object
  DDS::TopicQos qos;
  participant_->get_default_topic_qos(qos);

  // TopicDataQosPolicyMask topic_data;
  APPLY_QOS_MASK(qos, config.qos, config.qos_mask, topic_data, value);
  // DurabilityQosPolicyMask durability;
  APPLY_QOS_MASK(qos, config.qos, config.qos_mask, durability, kind);
  // DurabilityServiceQosPolicyMask durability_service;
  APPLY_QOS_MASK(qos, config.qos, config.qos_mask, durability_service, service_cleanup_delay);
  APPLY_QOS_MASK(qos, config.qos, config.qos_mask, durability_service, history_kind);
  APPLY_QOS_MASK(qos, config.qos, config.qos_mask, durability_service, history_depth);
  APPLY_QOS_MASK(qos, config.qos, config.qos_mask, durability_service, max_samples);
  APPLY_QOS_MASK(qos, config.qos, config.qos_mask, durability_service, max_instances);
  APPLY_QOS_MASK(qos, config.qos, config.qos_mask, durability_service, max_samples_per_instance);
  // DeadlineQosPolicyMask deadline;
  APPLY_QOS_MASK(qos, config.qos, config.qos_mask, deadline, period);
  // LatencyBudgetQosPolicyMask latency_budget;
  APPLY_QOS_MASK(qos, config.qos, config.qos_mask, latency_budget, duration);
  // LivelinessQosPolicyMask liveliness;
  APPLY_QOS_MASK(qos, config.qos, config.qos_mask, liveliness, kind);
  APPLY_QOS_MASK(qos, config.qos, config.qos_mask, liveliness, lease_duration);
  // ReliabilityQosPolicyMask reliability;
  APPLY_QOS_MASK(qos, config.qos, config.qos_mask, reliability, kind);
  APPLY_QOS_MASK(qos, config.qos, config.qos_mask, reliability, max_blocking_time);
  // DestinationOrderQosPolicyMask destination_order;
  APPLY_QOS_MASK(qos, config.qos, config.qos_mask, destination_order, kind);
  // HistoryQosPolicyMask history;
  APPLY_QOS_MASK(qos, config.qos, config.qos_mask, history, kind);
  APPLY_QOS_MASK(qos, config.qos, config.qos_mask, history, depth);
  // ResourceLimitsQosPolicyMask resource_limits;
  APPLY_QOS_MASK(qos, config.qos, config.qos_mask, resource_limits, max_samples);
  APPLY_QOS_MASK(qos, config.qos, config.qos_mask, resource_limits, max_instances);
  APPLY_QOS_MASK(qos, config.qos, config.qos_mask, resource_limits, max_samples_per_instance);
  // TransportPriorityQosPolicyMask transport_priority;
  APPLY_QOS_MASK(qos, config.qos, config.qos_mask, transport_priority, value);
  // LifespanQosPolicyMask lifespan;
  APPLY_QOS_MASK(qos, config.qos, config.qos_mask, lifespan, duration);
  // OwnershipQosPolicyMask ownership;
  APPLY_QOS_MASK(qos, config.qos, config.qos_mask, ownership, kind);

  // Create Listener From Factory
  listener_ = DDS::TopicListener::_nil();
  if (!listener_type_name_.empty()) {
    listener_ = create_listener(listener_type_name_, listener_properties_);
    if (!listener_) {
      std::stringstream ss;
      ss << "topic listener creation failed for topic '" << name_ << "' with listener type name '" << listener_type_name_ << "'" << std::flush;
      throw std::runtime_error(ss.str());
    }
  }

  // Create Topic
  topic_ = participant_->create_topic(name_.c_str(), type_name_.c_str(), qos, listener_, listener_status_mask_);
  if (CORBA::is_nil(topic_.in())) {
    std::stringstream ss;
    ss << "topic creation failed for topic '" << name_ << "'" << std::flush;
    throw std::runtime_error(ss.str());
  }

#ifndef OPENDDS_NO_CONTENT_FILTERED_TOPIC
  for (unsigned int i = 0; i != content_filtered_topics_.length(); ++i) {
    DDS::ContentFilteredTopic_var cft =
      participant_->create_contentfilteredtopic(content_filtered_topics_[i].cft_name,
        topic_,
        content_filtered_topics_[i].cft_expression,
        content_filtered_topics_[i].cft_parameters);

    if (CORBA::is_nil(cft.in())) {
      throw std::runtime_error("topic creation failed");
    }

    content_filtered_topics_map[content_filtered_topics_[i].cft_name.in()] = cft;
  }
#else
  ACE_UNUSED_ARG(content_filtered_topics_map);
#endif

  // Bind Transport Config
  if (!transport_config_name_.empty()) {
    Log::log() << "Binding config for topic " << name_ << " (" << transport_config_name_ << ")" << std::endl;
    TheTransportRegistry->bind_config(transport_config_name_.c_str(), topic_);
  }
}

Topic::~Topic()
{
  detach_listener();
}

const std::string& Topic::get_name() const
{
  return name_;
}

DDS::Topic_var& Topic::get_dds_topic()
{
  return topic_;
}

bool Topic::enable(bool throw_on_error)
{
  bool result = (topic_->enable() == DDS::RETCODE_OK);
  if (!result && throw_on_error) {
    std::stringstream ss;
    ss << "failed to enable topic '" << name_ << "'" << std::flush;
    throw std::runtime_error(ss.str());
  }
  return result;
}

void Topic::detach_listener()
{
  if (listener_) {
    TopicListener* savvy_listener = dynamic_cast<TopicListener*>(listener_.in());
    if (savvy_listener) {
      savvy_listener->unset_topic(*this);
    }
    if (topic_) {
      topic_->set_listener(0, OpenDDS::DCPS::NO_STATUS_MASK);
    }
    listener_ = 0;
  }
}

}
