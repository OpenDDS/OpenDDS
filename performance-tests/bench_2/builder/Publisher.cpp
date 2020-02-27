#include "Publisher.h"

#include "PublisherListener.h"

#include "dds/DdsDcpsDomainC.h"
#include "dds/DCPS/transport/framework/TransportRegistry.h"

namespace Builder {

Publisher::Publisher(const PublisherConfig& config, PublisherReport& report, DDS::DomainParticipant_var& participant, const std::shared_ptr<TopicManager>& topics, WriterMap& writer_map)
  : name_(config.name.in())
  , listener_type_name_(config.listener_type_name.in())
  , listener_status_mask_(config.listener_status_mask)
  , transport_config_name_(config.transport_config_name.in())
  , report_(report)
  , participant_(participant)
{
  Log::log() << "Creating publisher: '" << name_ << "' and listener type name '" << listener_type_name_ << "'" << std::endl;

  // Customize QoS Object
  DDS::PublisherQos qos;
  participant_->get_default_publisher_qos(qos);

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
  listener_ = DDS::PublisherListener::_nil();
  if (!listener_type_name_.empty()) {
    listener_ = create_listener(listener_type_name_);
    if (!listener_) {
      std::stringstream ss;
      ss << "publisher listener creation failed for subscriber '" << name_ << "' with listener type name '" << listener_type_name_ << "'" << std::flush;
      throw std::runtime_error(ss.str());
    } else {
      PublisherListener* savvy_listener_ = dynamic_cast<PublisherListener*>(listener_.in());
      if (savvy_listener_) {
        savvy_listener_->set_publisher(*this);
      }
    }
  }

  publisher_ = participant_->create_publisher(qos, listener_, listener_status_mask_);
  if (CORBA::is_nil(publisher_.in())) {
    throw std::runtime_error("publisher creation failed");
  }

  if (!transport_config_name_.empty()) {
    Log::log() << "Binding config for publisher " << name_ << " (" << transport_config_name_ << ")" << std::endl;
    TheTransportRegistry->bind_config(transport_config_name_.c_str(), publisher_);
  }

  datawriters_.reset(new DataWriterManager(config.datawriters, report.datawriters, publisher_, topics, writer_map));
}

Publisher::~Publisher() {
  datawriters_.reset();
  Log::log() << "Deleting publisher: " << name_ << std::endl;
  if (!CORBA::is_nil(publisher_.in())) {
    if (participant_->delete_publisher(publisher_) != DDS::RETCODE_OK) {
      Log::log() << "Error deleting publisher: " << name_ << std::endl;
    }
  }
}

void Publisher::enable() {
  publisher_->enable();
  datawriters_->enable();
}

}

