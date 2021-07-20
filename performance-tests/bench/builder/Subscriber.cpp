#include "Subscriber.h"

#include "SubscriberListener.h"

#include "dds/DdsDcpsDomainC.h"
#include "dds/DCPS/transport/framework/TransportRegistry.h"

namespace Builder {

Subscriber::Subscriber(const SubscriberConfig& config, SubscriberReport& report, DDS::DomainParticipant_var& participant,
  const std::shared_ptr<TopicManager>& topics, ReaderMap& reader_map, const ContentFilteredTopicMap& cft_map)
  : name_(config.name.in())
  , listener_type_name_(config.listener_type_name.in())
  , listener_status_mask_(config.listener_status_mask)
  , listener_properties_(config.listener_properties)
  , transport_config_name_(config.transport_config_name.in())
  , report_(report)
  , participant_(participant)
{
  Log::log() << "Creating subscriber: '" << name_ << "' and listener type name '" << listener_type_name_ << "'" << std::endl;

  // Customize QoS Object
  DDS::SubscriberQos qos;
  participant_->get_default_subscriber_qos(qos);

  //PresentationQosPolicyMask presentation;
  APPLY_QOS_MASK(qos, config.qos, config.qos_mask, presentation, access_scope);
  APPLY_QOS_MASK(qos, config.qos, config.qos_mask, presentation, coherent_access);
  APPLY_QOS_MASK(qos, config.qos, config.qos_mask, presentation, ordered_access);
  //PartitionQosPolicyMask partition;
  APPLY_QOS_MASK(qos, config.qos, config.qos_mask, partition, name);
  //GroupDataQosPolicyMask group_data;
  APPLY_QOS_MASK(qos, config.qos, config.qos_mask, group_data, value);
  //EntityFactoryQosPolicyMask entity_factory;
  APPLY_QOS_MASK(qos, config.qos, config.qos_mask, entity_factory, autoenable_created_entities);

  // Create Listener From Factory
  listener_ = DDS::SubscriberListener::_nil();
  if (!listener_type_name_.empty()) {
    listener_ = create_listener(listener_type_name_, listener_properties_);
    if (!listener_) {
      std::stringstream ss;
      ss << "subscriber listener creation failed for subscriber '" << name_ << "' with listener type name '" << listener_type_name_ << "'" << std::flush;
      throw std::runtime_error(ss.str());
    } else {
      SubscriberListener* savvy_listener = dynamic_cast<SubscriberListener*>(listener_.in());
      if (savvy_listener) {
        savvy_listener->set_subscriber(*this);
      }
    }
  }

  subscriber_ = participant_->create_subscriber(qos, listener_, listener_status_mask_);
  if (CORBA::is_nil(subscriber_.in())) {
    throw std::runtime_error("subscriber creation failed");
  }

  if (!transport_config_name_.empty()) {
    Log::log() << "Binding config for subscriber " << name_ << " (" << transport_config_name_ << ")" << std::endl;
    TheTransportRegistry->bind_config(transport_config_name_.c_str(), subscriber_);
  }

  datareaders_.reset(new DataReaderManager(config.datareaders, report.datareaders, subscriber_, topics, reader_map, cft_map));
}

Subscriber::~Subscriber()
{
  detach_listeners();
  datareaders_.reset();
  Log::log() << "Deleting subscriber: " << name_ << std::endl;
  if (!CORBA::is_nil(subscriber_.in())) {
    if (participant_->delete_subscriber(subscriber_) != DDS::RETCODE_OK) {
      Log::log() << "Error deleting subscriber: " << name_ << std::endl;
    }
  }
}

bool Subscriber::enable(bool throw_on_error)
{
  bool success = (subscriber_->enable() == DDS::RETCODE_OK);
  if (!success && throw_on_error) {
    std::stringstream ss;
    ss << "failed to enable subscriber '" << name_ << "'" << std::flush;
    throw std::runtime_error(ss.str());
  }
  return success && datareaders_->enable(throw_on_error);
}

void Subscriber::detach_listeners()
{
  if (listener_) {
    SubscriberListener* savvy_listener = dynamic_cast<SubscriberListener*>(listener_.in());
    if (savvy_listener) {
      savvy_listener->unset_subscriber(*this);
    }
    if (subscriber_) {
      subscriber_->set_listener(0, OpenDDS::DCPS::NO_STATUS_MASK);
    }
    listener_ = 0;
  }
  datareaders_->detach_listeners();
}

}
