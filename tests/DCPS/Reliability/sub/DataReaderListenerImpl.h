/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DATAREADER_LISTENER_IMPL_H
#define DATAREADER_LISTENER_IMPL_H

#include <tools/modeling/codegen/model/NullReaderListener.h>
#include <ace/Global_Macros.h>

#include <dds/DdsDcpsSubscriptionC.h>
#include <dds/DCPS/LocalObject.h>
#include <dds/DCPS/Definitions.h>
#include <ctime>
#include "ReliabilityTypeSupportImpl.h"

using OpenDDS::Model::NullReaderListener;

class DataReaderListenerImpl
  : public virtual OpenDDS::DCPS::LocalObject<NullReaderListener> {
public:
  DataReaderListenerImpl();

  virtual void on_sample_lost(
    DDS::DataReader_ptr reader, const DDS::SampleLostStatus& status);

  virtual void on_sample_rejected(
    DDS::DataReader_ptr reader, const DDS::SampleRejectedStatus& status);

  virtual void on_data_available(
    DDS::DataReader_ptr reader);

  virtual void on_requested_incompatible_qos (
    DDS::DataReader_ptr reader,
    const DDS::RequestedIncompatibleQosStatus& status
  );

  unsigned long long sample_count() { return sample_count_; }
  unsigned long long expected_count() { return expected_count_; }

  void set_sleep_length(int sleep_length) { sleep_length_ = sleep_length;};
  void set_num_sleeps(int num_sleeps) { num_sleeps_ = num_sleeps;};

  protected:
  virtual void take_samples(
    Reliability::MessageDataReader_var reader_i
  ) = 0;

  void on_sample(Reliability::Message& msg);

  long sample_count_;
  long expected_count_;
  long expected_seq_;
  int sleep_length_;
  int num_sleeps_;
};

#endif /* DATAREADER_LISTENER_IMPL_H */
