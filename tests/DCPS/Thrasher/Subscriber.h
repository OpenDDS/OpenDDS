#ifndef DCPS_THRASHER_SUBSCRIBER_H
#define DCPS_THRASHER_SUBSCRIBER_H

#include "DataReaderListenerImpl.h"

#include <dds/DCPS/Service_Participant.h>

class Subscriber
{
public:
  Subscriber(DDS::DomainId_t domainId, size_t n_pub_threads, size_t expected_samples, bool durable);
  ~Subscriber();
  int wait_and_check_received() const;
private:
  void cleanup();
  const DDS::DomainId_t domainId_;
  const size_t expected_samples_;
  const bool durable_;
  DDS::DomainParticipantFactory_var dpf_;
  DDS::DomainParticipant_var dp_;
  DataReaderListenerImpl* listener_i_;
  DDS::DataReaderListener_var listener_;
  DDS::DataReader_var reader_;
};

#endif // DCPS_THRASHER_SUBSCRIBER_H
