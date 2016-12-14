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

#include <map>

class DataReaderListenerImpl
  : public virtual OpenDDS::DCPS::LocalObject<DDS::DataReaderListener> {
public:
  DataReaderListenerImpl();

  virtual ~DataReaderListenerImpl();

  virtual void on_requested_deadline_missed(
    DDS::DataReader_ptr reader,
    const DDS::RequestedDeadlineMissedStatus& status)
  throw(CORBA::SystemException);

  virtual void on_requested_incompatible_qos(
    DDS::DataReader_ptr reader,
    const DDS::RequestedIncompatibleQosStatus& status)
  throw(CORBA::SystemException);

  virtual void on_liveliness_changed(
    DDS::DataReader_ptr reader,
    const DDS::LivelinessChangedStatus& status)
  throw(CORBA::SystemException);

  virtual void on_subscription_matched(
    DDS::DataReader_ptr reader,
    const DDS::SubscriptionMatchedStatus& status)
  throw(CORBA::SystemException);

  virtual void on_sample_rejected(
    DDS::DataReader_ptr reader,
    const DDS::SampleRejectedStatus& status)
  throw(CORBA::SystemException);

  virtual void on_data_available(
    DDS::DataReader_ptr reader)
  throw(CORBA::SystemException);

  virtual void on_sample_lost(
    DDS::DataReader_ptr reader,
    const DDS::SampleLostStatus& status)
  throw(CORBA::SystemException);

  long num_samples() const {
    return num_samples_;
  }

  bool data_consistent() const;

  static const unsigned int NUM_PROCESSES = 2;
  static const unsigned int NUM_WRITERS_PER_PROCESS = 2;
  static const unsigned int NUM_SAMPLES_PER_WRITER = 10;

private:
  DDS::DataReader_var  reader_;
  long                 num_samples_;
  typedef std::set<CORBA::Long> Counts;
  typedef std::map<CORBA::Long, Counts> WriterCounts;
  typedef std::map<std::string, WriterCounts> ProcessWriters;
  ProcessWriters process_writers_;
  bool valid_;
  typedef std::string PROCESS_ID_STR;
  typedef CORBA::Long WRITER_ID, HIGHEST_SAMPLE_RECVD;
  std::map<PROCESS_ID_STR, std::map<WRITER_ID, HIGHEST_SAMPLE_RECVD> > processToWriterSamples_;
};

#endif /* DATAREADER_LISTENER_IMPL  */
