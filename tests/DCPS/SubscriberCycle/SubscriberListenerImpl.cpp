/*
 */

#include "SubscriberListenerImpl.h"
#include "FooTypeTypeSupportC.h"

SubscriberListenerImpl::SubscriberListenerImpl(std::size_t& received_samples,
                                               std::size_t& missed_samples)
  : received_samples_(received_samples)
  , missed_samples_(missed_samples)
{
}

const std::size_t &
SubscriberListenerImpl::received_samples()
{
  return received_samples_;
}

const std::size_t &
SubscriberListenerImpl::missed_samples()
{
  return missed_samples_;
}

std::size_t
SubscriberListenerImpl::samples_processed()
{
  return received_samples_ + missed_samples_;
}

void
SubscriberListenerImpl::on_data_on_readers(::DDS::Subscriber_ptr sub)
{

  ::DDS::DataReaderSeq_var readers(new ::DDS::DataReaderSeq(1));

  ::DDS::ReturnCode_t rc =
      sub->get_datareaders (readers, ::DDS::NOT_READ_SAMPLE_STATE,
                            ::DDS::ANY_VIEW_STATE, ::DDS::ANY_INSTANCE_STATE
                            );
  if (rc != ::DDS::RETCODE_OK)
    {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: SubscriberListenerImpl::on_data_on_readers ")
                 ACE_TEXT("could not get data readers\n")));

      return;
    }

  CORBA::ULong num_readers = readers->length();
  if (num_readers != 1)
    {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: SubscriberListenerImpl::on_data_on_readers ")
                 ACE_TEXT("did not get only one data reader\n")));

      return;
    }

  FooDataReader_var foo_reader =
    FooDataReader::_narrow(readers[static_cast<CORBA::ULong>(0)]);

  // The following is intentionally inefficient to simulate
  // backpressure with multiple writers; we should be especially
  // dull and take only one sample at a time.

  Foo foo;
  DDS::SampleInfo si;

  while (foo_reader->take_next_sample(foo, si) == DDS::RETCODE_OK)
    {
      if (si.valid_data)
        {
          ++received_samples_;
        }
    }

}

void
SubscriberListenerImpl::on_liveliness_changed(::DDS::DataReader_ptr,
                                              const ::DDS::LivelinessChangedStatus&)
  throw(CORBA::SystemException)
{
}

void
SubscriberListenerImpl::on_subscription_matched(::DDS::DataReader_ptr ,
                                                const ::DDS::SubscriptionMatchedStatus& )
  throw(CORBA::SystemException)
{
}

void SubscriberListenerImpl::on_data_available(::DDS::DataReader_ptr )
  throw(CORBA::SystemException)
{
}

void
SubscriberListenerImpl::on_requested_deadline_missed (::DDS::DataReader_ptr /*reader*/,
                                                      const ::DDS::RequestedDeadlineMissedStatus& /*status*/
                                                      )
  throw(CORBA::SystemException)
{
  ACE_DEBUG ((LM_DEBUG,
              ACE_TEXT("(%P|%t) Subscriber deadline missed\n")));

  ++missed_samples_;
}

void
SubscriberListenerImpl::on_requested_incompatible_qos(::DDS::DataReader_ptr ,
                                                      const ::DDS::RequestedIncompatibleQosStatus&)
  throw(CORBA::SystemException)
{
  ACE_DEBUG ((LM_DEBUG,
              ACE_TEXT("(%P|%t) Subscriber incompatible QoS\n")));
}

void
SubscriberListenerImpl::on_sample_rejected(::DDS::DataReader_ptr,
                                           const ::DDS::SampleRejectedStatus&)
  throw(CORBA::SystemException)
{
}

void
SubscriberListenerImpl::on_sample_lost(::DDS::DataReader_ptr,
                                       const ::DDS::SampleLostStatus&)
  throw(CORBA::SystemException)
{
}
