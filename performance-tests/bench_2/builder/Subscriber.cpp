#include "Subscriber.h"

#include "SubscriberListener.h"

#include "dds/DdsDcpsDomainC.h"
#include "dds/DCPS/transport/framework/TransportRegistry.h"

namespace Builder {

Subscriber::Subscriber(const SubscriberConfig& config, SubscriberReport& report, DDS::DomainParticipant_var& participant, const std::shared_ptr<TopicManager>& topics, ReaderMap& reader_map)
  : name_(config.name.in())
  , listener_type_name_(config.listener_type_name.in())
  , listener_status_mask_(config.listener_status_mask)
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
    listener_ = create_listener(listener_type_name_);
    if (!listener_) {
      std::stringstream ss;
      ss << "subscriber listener creation failed for subscriber '" << name_ << "' with listener type name '" << listener_type_name_ << "'" << std::flush;
      throw std::runtime_error(ss.str());
    } else {
      SubscriberListener* savvy_listener_ = dynamic_cast<SubscriberListener*>(listener_.in());
      if (savvy_listener_) {
        savvy_listener_->set_subscriber(*this);
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

  datareaders_.reset(new DataReaderManager(config.datareaders, report.datareaders, subscriber_, topics, reader_map));
}

Subscriber::~Subscriber() {
  datareaders_.reset();
  Log::log() << "Deleting subscriber: " << name_ << std::endl;
  if (!CORBA::is_nil(subscriber_.in())) {
    if (participant_->delete_subscriber(subscriber_) != DDS::RETCODE_OK) {
      Log::log() << "Error deleting subscriber: " << name_ << std::endl;
    }
  }
}

void Subscriber::enable() {
  subscriber_->enable();
  datareaders_->enable();
}

}

