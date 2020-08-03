/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef DATAREADER_LISTENER_IMPL
#define DATAREADER_LISTENER_IMPL

#include <dds/DdsDcpsSubscriptionC.h>

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "Options.h"

#include <map>

class DataReaderListenerImpl
  : public virtual OpenDDS::DCPS::LocalObject<DDS::DataReaderListener> {
public:
  DataReaderListenerImpl(const Options& options,
                         const std::string& process,
                         unsigned int participant,
                         unsigned int writer);

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

  bool done() const;
  void report_errors() const;

private:
  bool done(bool report) const;

  mutable ACE_Thread_Mutex mutex_;
  const Options& options_;
  DDS::DataReader_var reader_;
  typedef std::set<int> Counts;
  typedef std::map<int, Counts> WriterCounts;
  typedef std::map<int, WriterCounts> ParticipantWriters;
  typedef std::map<int, ParticipantWriters> ProcessParticipants;

  ProcessParticipants processes_;
  std::string id_;
  const unsigned long expected_num_samples_;
  unsigned long num_samples_;
};

#endif /* DATAREADER_LISTENER_IMPL  */
