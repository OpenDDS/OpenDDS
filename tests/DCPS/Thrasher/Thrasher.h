#ifndef DCPS_THRASHER_THRASHER_H
#define DCPS_THRASHER_THRASHER_H

#include <dds/DdsDcpsDomainC.h>

class Thrasher
{
public:
  Thrasher(int& argc, ACE_TCHAR** argv);
  ~Thrasher() { cleanup(); }
  int run();
private:
  void parse_args(int& argc, ACE_TCHAR** argv);
  void cleanup();
  static const long DomainID = 42;
  static const int DefaultPubThreads = 1;
  static const std::size_t DefaultSamplesPerThread = 1024;
  static const std::size_t DefaultExpectedSamples = 1024;
  DDS::DomainParticipantFactory_var dpf_;
  int n_pub_threads_;
  std::size_t samples_per_thread_;
  std::size_t expected_samples_;
  bool durable_;
};

#endif // DCPS_THRASHER_THRASHER_H
