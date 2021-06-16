#ifndef DCPS_THRASHER_PUBLISHER_H
#define DCPS_THRASHER_PUBLISHER_H

#include <FooTypeTypeSupportImpl.h>

#include <dds/DdsDcpsInfrastructureC.h>

#include <memory>

class Publisher
{
public:
  typedef std::unique_ptr<Publisher> Ptr;
  Publisher(const long domain_id, std::size_t samples_per_thread, bool durable, const int thread_index);
  ~Publisher() { cleanup(); }
  void publish();
private:
  static std::string create_pfx(const int thread_index);
  void cleanup();
  void configure_transport();
  const long domain_id_;
  const std::size_t samples_per_thread_;
  const bool durable_;
  const int thread_index_;
  const std::string pfx_;
  DDS::DomainParticipantFactory_var dpf_;
  DDS::DomainParticipant_var dp_;
  DDS::DataWriter_var dw_;
  FooDataWriter_var writer_;
};

#endif // DCPS_THRASHER_PUBLISHER_H
