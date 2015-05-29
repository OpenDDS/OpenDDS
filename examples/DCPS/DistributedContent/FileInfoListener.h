#ifndef FILEINFOLISTENER_H_
#define FILEINFOLISTENER_H_

#include <dds/DdsDcpsSubscriptionC.h>
#include <dds/DCPS/Definitions.h>
#include <dds/DCPS/LocalObject.h>

class AbstractionLayer;

class FileInfoListener
  : public virtual OpenDDS::DCPS::LocalObject<DDS::DataReaderListener>
{

public:


  FileInfoListener(AbstractionLayer* monitor);

  virtual ~FileInfoListener();


  virtual void on_requested_deadline_missed (
    ::DDS::DataReader_ptr reader,
    const ::DDS::RequestedDeadlineMissedStatus & status);


  virtual void on_requested_incompatible_qos (
    ::DDS::DataReader_ptr reader,
    const ::DDS::RequestedIncompatibleQosStatus & status);


  virtual void on_sample_rejected (
    ::DDS::DataReader_ptr reader,
    const ::DDS::SampleRejectedStatus & status
    );


  virtual void on_liveliness_changed (
    ::DDS::DataReader_ptr reader,
    const ::DDS::LivelinessChangedStatus & status);


  virtual void on_data_available (
    ::DDS::DataReader_ptr reader);


  virtual void on_subscription_matched (
    ::DDS::DataReader_ptr reader,
    const ::DDS::SubscriptionMatchedStatus & status);


  virtual void on_sample_lost (
    ::DDS::DataReader_ptr reader,
    const ::DDS::SampleLostStatus & status);

private:

  // Pointer to the change monitor.  This class does not take ownership.
  AbstractionLayer* change_monitor_;

};

#endif
