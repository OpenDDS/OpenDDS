#ifndef DCPS_THRASHER_PUBLISHER_H
#define DCPS_THRASHER_PUBLISHER_H

#include <FooTypeTypeSupportImpl.h>

#include <dds/DdsDcpsInfrastructureC.h>
#include <dds/DCPS/unique_ptr.h>

class Publisher
{
public:
  //Note: OpenDDS::DCPS::unique_ptr instead of std::unique_ptr is used here to support c++03
  typedef OpenDDS::DCPS::unique_ptr<Publisher> Ptr;
  Publisher(const long domain_id, std::size_t samples_per_thread, bool durable, const int thread_index);
  ~Publisher() { cleanup(); }
  int publish();
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
