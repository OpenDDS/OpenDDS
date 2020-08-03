/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DATAREADER_LISTENER_IMPL
#define DATAREADER_LISTENER_IMPL

#include <dds/DCPS/LocalObject.h>
#include <dds/DdsDcpsSubscriptionC.h>

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include <map>
#include <set>

class DataReaderListenerImpl
  : public virtual OpenDDS::DCPS::LocalObject<DDS::DataReaderListener> {
public:
  DataReaderListenerImpl(
    size_t writer_process_count, size_t writers_per_process, size_t samples_per_writer,
    unsigned data_field_length_offset);

  virtual ~DataReaderListenerImpl();

  virtual void on_requested_deadline_missed(
    DDS::DataReader_ptr reader,
    const DDS::RequestedDeadlineMissedStatus& status);

  virtual void on_requested_incompatible_qos(
    DDS::DataReader_ptr reader,
    const DDS::RequestedIncompatibleQosStatus& status);

  virtual void on_liveliness_changed(
    DDS::DataReader_ptr reader,
    const DDS::LivelinessChangedStatus& status);

  virtual void on_subscription_matched(
    DDS::DataReader_ptr reader,
    const DDS::SubscriptionMatchedStatus& status);

  virtual void on_sample_rejected(
    DDS::DataReader_ptr reader,
    const DDS::SampleRejectedStatus& status);

  virtual void on_data_available(
    DDS::DataReader_ptr reader);

  virtual void on_sample_lost(
    DDS::DataReader_ptr reader,
    const DDS::SampleLostStatus& status);

  size_t num_samples() const
  {
    return num_samples_;
  }

  bool data_consistent() const;

private:
  DDS::DataReader_var  reader_;
  size_t num_samples_;
  typedef CORBA::Long ProcessId, WriterId, SampleId;
  typedef std::set<SampleId> Counts;
  typedef std::map<WriterId, Counts> WriterCounts;
  typedef std::map<ProcessId, WriterCounts> ProcessWriters;
  /// Holds the highest sample id for a given process/writer pair
  std::map<ProcessId, std::map<WriterId, SampleId> > processToWriterSamples_;
  ProcessWriters process_writers_;
  bool valid_;
  size_t writer_process_count_;
  size_t writers_per_process_;
  size_t samples_per_writer_;
  unsigned data_field_length_offset_;
};

#endif /* DATAREADER_LISTENER_IMPL  */
