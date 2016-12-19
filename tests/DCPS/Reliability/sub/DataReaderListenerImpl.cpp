/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DataReaderListenerImpl.h"
#include "Boilerplate.h"
#include "ace/OS_NS_unistd.h"
#include <iostream>

using namespace examples::boilerplate;

DataReaderListenerImpl::DataReaderListenerImpl()
  : sample_count_(0)
  , expected_count_(0)
  , expected_seq_(0)
  , sleep_length_(0)
  , num_sleeps_(0)
{
}

void
DataReaderListenerImpl::on_sample_lost(DDS::DataReader_ptr , const DDS::SampleLostStatus& status)
{
  std::cout << "Lost sample: " << status.total_count_change << std::endl;
}

void
DataReaderListenerImpl::on_sample_rejected(DDS::DataReader_ptr , const DDS::SampleRejectedStatus& status)
{
  std::cout << "Rejected sample: " << status.total_count_change << " Reason: "
            << status.last_reason << std::endl;
}

void
DataReaderListenerImpl::on_data_available(DDS::DataReader_ptr reader)
{
  // Safely downcast data reader to type-specific data reader
  Reliability::MessageDataReader_var reader_i = narrow(reader);

  take_samples(reader_i);
}

void DataReaderListenerImpl::on_requested_incompatible_qos (
     DDS::DataReader_ptr ,
     const DDS::RequestedIncompatibleQosStatus& )
{
  std::cout << "Subscriber incompatible QOS" << std::endl;
}

void DataReaderListenerImpl::on_sample(Reliability::Message& msg)
{
  if (sample_count_ == 0) {
    expected_count_ = msg.expected;
  } else if (msg.expected != expected_count_) {
    std::cout << "Error: expected_count changed to " << msg.expected
              << std::endl;
  }

  if (expected_seq_ != msg.count) {
    std::cout << "Expected: " << expected_seq_
              << " Received: " << msg.count << std::endl;
  } else {
    std::cout << "Received sample: " << msg.count << std::endl;
    // Next message
    expected_seq_ = msg.count + 1;
  }
  ++sample_count_;
  if (num_sleeps_ && (((sample_count_ + 1) % (expected_count_/num_sleeps_)) == 0)) {
    std::cout << "Got sample " << sample_count_ + 1 << " sleeping" << std::endl;
    ACE_OS::sleep(sleep_length_);
  }

}
