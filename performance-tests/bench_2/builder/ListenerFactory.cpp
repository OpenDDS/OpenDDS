#include "ListenerFactory.h"

namespace Builder {

Bench_Builder_Export
void RegisterDomainParticipantListener(const std::string& name, const std::function<DDS::DomainParticipantListener::_var_type(const Builder::PropertySeq&)>& factory)
{
  Log::log() << "Listener registration created for name '" << name << "'" << std::endl;
  if (!ListenerFactory<DDS::DomainParticipantListener>::register_listener_factory(name, factory)) {
    std::stringstream ss;
    ss << "unable to register listener factory with name '" << name << "'" << std::flush;
    throw std::runtime_error(ss.str());
  }
}

Bench_Builder_Export
void RegisterTopicListener(const std::string& name, const std::function<DDS::TopicListener::_var_type(const Builder::PropertySeq&)>& factory)
{
  Log::log() << "Listener registration created for name '" << name << "'" << std::endl;
  if (!ListenerFactory<DDS::TopicListener>::register_listener_factory(name, factory)) {
    std::stringstream ss;
    ss << "unable to register listener factory with name '" << name << "'" << std::flush;
    throw std::runtime_error(ss.str());
  }
}

Bench_Builder_Export
void RegisterPublisherListener(const std::string& name, const std::function<DDS::PublisherListener::_var_type(const Builder::PropertySeq&)>& factory)
{
  Log::log() << "Listener registration created for name '" << name << "'" << std::endl;
  if (!ListenerFactory<DDS::PublisherListener>::register_listener_factory(name, factory)) {
    std::stringstream ss;
    ss << "unable to register listener factory with name '" << name << "'" << std::flush;
    throw std::runtime_error(ss.str());
  }
}

Bench_Builder_Export
void RegisterSubscriberListener(const std::string& name, const std::function<DDS::SubscriberListener::_var_type(const Builder::PropertySeq&)>& factory)
{
  Log::log() << "Listener registration created for name '" << name << "'" << std::endl;
  if (!ListenerFactory<DDS::SubscriberListener>::register_listener_factory(name, factory)) {
    std::stringstream ss;
    ss << "unable to register listener factory with name '" << name << "'" << std::flush;
    throw std::runtime_error(ss.str());
  }
}

Bench_Builder_Export
void RegisterDataWriterListener(const std::string& name, const std::function<DDS::DataWriterListener::_var_type(const Builder::PropertySeq&)>& factory)
{
  Log::log() << "Listener registration created for name '" << name << "'" << std::endl;
  if (!ListenerFactory<DDS::DataWriterListener>::register_listener_factory(name, factory)) {
    std::stringstream ss;
    ss << "unable to register listener factory with name '" << name << "'" << std::flush;
    throw std::runtime_error(ss.str());
  }
}

Bench_Builder_Export
void RegisterDataReaderListener(const std::string& name, const std::function<DDS::DataReaderListener::_var_type(const Builder::PropertySeq&)>& factory)
{
  Log::log() << "Listener registration created for name '" << name << "'" << std::endl;
  if (!ListenerFactory<DDS::DataReaderListener>::register_listener_factory(name, factory)) {
    std::stringstream ss;
    ss << "unable to register listener factory with name '" << name << "'" << std::flush;
    throw std::runtime_error(ss.str());
  }
}

}


