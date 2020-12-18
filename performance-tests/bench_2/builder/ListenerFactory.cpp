#include "ListenerFactory.h"

namespace Builder {

void register_domain_participant_listener(const std::string& name, const std::function<DDS::DomainParticipantListener::_var_type(const Builder::PropertySeq&)>& factory)
{
  Log::log() << "Listener registration created for name '" << name << "'" << std::endl;
  if (!ListenerFactory<DDS::DomainParticipantListener>::register_listener_factory(name, factory)) {
    std::stringstream ss;
    ss << "unable to register listener factory with name '" << name << "'" << std::flush;
    throw std::runtime_error(ss.str());
  }
}

void register_topic_listener(const std::string& name, const std::function<DDS::TopicListener::_var_type(const Builder::PropertySeq&)>& factory)
{
  Log::log() << "Listener registration created for name '" << name << "'" << std::endl;
  if (!ListenerFactory<DDS::TopicListener>::register_listener_factory(name, factory)) {
    std::stringstream ss;
    ss << "unable to register listener factory with name '" << name << "'" << std::flush;
    throw std::runtime_error(ss.str());
  }
}

void register_publisher_listener(const std::string& name, const std::function<DDS::PublisherListener::_var_type(const Builder::PropertySeq&)>& factory)
{
  Log::log() << "Listener registration created for name '" << name << "'" << std::endl;
  if (!ListenerFactory<DDS::PublisherListener>::register_listener_factory(name, factory)) {
    std::stringstream ss;
    ss << "unable to register listener factory with name '" << name << "'" << std::flush;
    throw std::runtime_error(ss.str());
  }
}

void register_subscriber_listener(const std::string& name, const std::function<DDS::SubscriberListener::_var_type(const Builder::PropertySeq&)>& factory)
{
  Log::log() << "Listener registration created for name '" << name << "'" << std::endl;
  if (!ListenerFactory<DDS::SubscriberListener>::register_listener_factory(name, factory)) {
    std::stringstream ss;
    ss << "unable to register listener factory with name '" << name << "'" << std::flush;
    throw std::runtime_error(ss.str());
  }
}

void register_data_writer_listener(const std::string& name, const std::function<DDS::DataWriterListener::_var_type(const Builder::PropertySeq&)>& factory)
{
  Log::log() << "Listener registration created for name '" << name << "'" << std::endl;
  if (!ListenerFactory<DDS::DataWriterListener>::register_listener_factory(name, factory)) {
    std::stringstream ss;
    ss << "unable to register listener factory with name '" << name << "'" << std::flush;
    throw std::runtime_error(ss.str());
  }
}

void register_data_reader_listener(const std::string& name, const std::function<DDS::DataReaderListener::_var_type(const Builder::PropertySeq&)>& factory)
{
  Log::log() << "Listener registration created for name '" << name << "'" << std::endl;
  if (!ListenerFactory<DDS::DataReaderListener>::register_listener_factory(name, factory)) {
    std::stringstream ss;
    ss << "unable to register listener factory with name '" << name << "'" << std::flush;
    throw std::runtime_error(ss.str());
  }
}

}
