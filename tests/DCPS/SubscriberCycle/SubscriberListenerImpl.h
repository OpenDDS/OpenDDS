/*
 */

#ifndef SUBSCRIBER_CYCLE_SUSCRIBERLISTENERIMPL_H
#define SUBSCRIBER_CYCLE_SUSCRIBERLISTENERIMPL_H

#include <dds/DdsDcpsSubscriptionC.h>
#include <dds/DCPS/LocalObject.h>

class SubscriberListenerImpl
: public virtual ::OpenDDS::DCPS::LocalObject< ::DDS::SubscriberListener>
{
 public:

  SubscriberListenerImpl(std::size_t& received_samples,
                         std::size_t& missed_samples);

  const std::size_t& received_samples();

  const std::size_t& missed_samples();

  std::size_t samples_processed();

  virtual void on_data_on_readers(::DDS::Subscriber_ptr sub);

  virtual void on_liveliness_changed(::DDS::DataReader_ptr,
                                     const ::DDS::LivelinessChangedStatus&)
    throw(CORBA::SystemException);

  virtual void on_subscription_matched(::DDS::DataReader_ptr ,
                                       const ::DDS::SubscriptionMatchedStatus& )
    throw(CORBA::SystemException);

  virtual void on_data_available(::DDS::DataReader_ptr )
    throw(CORBA::SystemException);

  virtual void
    on_requested_deadline_missed (::DDS::DataReader_ptr reader,
                                  const ::DDS::RequestedDeadlineMissedStatus& status
                                  )
    throw(CORBA::SystemException);

  virtual void
    on_requested_incompatible_qos(::DDS::DataReader_ptr ,
                                  const ::DDS::RequestedIncompatibleQosStatus&)
    throw(CORBA::SystemException);

  virtual void
    on_sample_rejected(::DDS::DataReader_ptr,
                       const ::DDS::SampleRejectedStatus&)
    throw(CORBA::SystemException);

  virtual void
    on_sample_lost(::DDS::DataReader_ptr,
                   const ::DDS::SampleLostStatus&)
    throw(CORBA::SystemException);

 private:
  std::size_t& received_samples_;
  std::size_t& missed_samples_;

};

#endif
