#include "Participant.h"

#include "ParticipantListener.h"

#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/transport/framework/TransportRegistry.h"

namespace Builder {

Participant::Participant(const ParticipantConfig& config, ParticipantReport& report,
  ReaderMap& reader_map, WriterMap& writer_map, ContentFilteredTopicMap& cft_map)
  : name_(config.name.in())
  , domain_(config.domain)
  , listener_type_name_(config.listener_type_name.in())
  , listener_status_mask_(config.listener_status_mask)
  , listener_properties_(config.listener_properties)
  , transport_config_name_(config.transport_config_name.in())
  , report_(report)
{
  Log::log() << "Creating participant: '" << name_ << "' in domain " << domain_ << std::endl;

  // Customize QoS Object
  DDS::DomainParticipantFactory_var dpf = TheParticipantFactory;
  DDS::DomainParticipantQos qos;
  dpf->get_default_participant_qos(qos);

  //UserDataQosPolicyMask user_data;
  APPLY_QOS_MASK(qos, config.qos, config.qos_mask, user_data, value);
  //EntityFactoryQosPolicyMask entity_factory;
  APPLY_QOS_MASK(qos, config.qos, config.qos_mask, entity_factory, autoenable_created_entities);
  //PropertyQosPolicyMask property;
  APPLY_QOS_MASK(qos, config.qos, config.qos_mask, property, value);
  APPLY_QOS_MASK(qos, config.qos, config.qos_mask, property, binary_value);

  // Create Listener From Factory
  listener_ = DDS::DomainParticipantListener::_nil();
  if (!listener_type_name_.empty()) {
    listener_ = create_listener(listener_type_name_, listener_properties_);
    if (!listener_) {
      std::stringstream ss;
      ss << "participant listener creation failed for subscriber '" << name_ << "' with listener type name '" << listener_type_name_ << "'" << std::flush;
      throw std::runtime_error(ss.str());
    } else {
      ParticipantListener* savvy_listener_ = dynamic_cast<ParticipantListener*>(listener_.in());
      if (savvy_listener_) {
        savvy_listener_->set_participant(*this);
      }
    }
  }

  // Create Participant
  participant_ = dpf->create_participant(domain_, qos, listener_, listener_status_mask_);
  if (!participant_) {
    throw std::runtime_error("participant creation failed");
  }

  // Bind Transport Config
  if (!transport_config_name_.empty()) {
    Log::log() << "Binding config for participant " << name_ << " (" << transport_config_name_ << ")" << std::endl;
    TheTransportRegistry->bind_config(transport_config_name_.c_str(), participant_);
  }

  // Determine Types To Register
  std::vector<std::string> type_names;
  if (config.type_names.length() == 0) {
    type_names = get_type_names();
  } else {
    for (CORBA::ULong i = 0; i < config.type_names.length(); ++i) {
      type_names.push_back(config.type_names[i].in());
    }
  }

  // Register Types
  for (std::vector<std::string>::const_iterator it = type_names.begin(); it != type_names.end(); ++it) {
    DDS::TypeSupport_var ts = get_type_support(*it);
    if (!ts || ts->register_type(participant_.in(), "") != DDS::RETCODE_OK) {
      std::stringstream ss;
      ss << "participant type support registration failed for participant '" << name_ << "' with type name '" << *it << "'" << std::flush;
      throw std::runtime_error(ss.str());
    }
  }

  topics_.reset(new TopicManager(config.topics, participant_, cft_map));
  subscribers_.reset(new SubscriberManager(config.subscribers, report.subscribers, participant_, topics_, reader_map, cft_map));
  publishers_.reset(new PublisherManager(config.publishers, report.publishers, participant_, topics_, writer_map));
}

Participant::~Participant()
{
  detach_listeners();
  publishers_.reset();
  subscribers_.reset();
  topics_.reset();

  if (participant_) {
    Log::log() << "deleting entities for participant " << name_ << std::endl;
    participant_->delete_contained_entities();
    DDS::DomainParticipantFactory_var dpf = TheParticipantFactory;
    dpf->delete_participant(participant_);
  }
}

bool Participant::enable(bool throw_on_error)
{
  bool success = (participant_->enable() == DDS::RETCODE_OK);
  if (!success && throw_on_error) {
    std::stringstream ss;
    ss << "failed to enable participant '" << name_ << "'" << std::flush;
    throw std::runtime_error(ss.str());
  }
  return success && topics_->enable(throw_on_error) &&
    subscribers_->enable(throw_on_error) &&
    publishers_->enable(throw_on_error);
}

void Participant::detach_listeners()
{
  if (listener_) {
    ParticipantListener* savvy_listener = dynamic_cast<ParticipantListener*>(listener_.in());
    if (savvy_listener) {
      savvy_listener->unset_participant(*this);
    }
    if (participant_) {
      participant_->set_listener(0, OpenDDS::DCPS::NO_STATUS_MASK);
    }
    listener_ = 0;
  }
  topics_->detach_listeners();
  subscribers_->detach_listeners();
  publishers_->detach_listeners();
}

}
