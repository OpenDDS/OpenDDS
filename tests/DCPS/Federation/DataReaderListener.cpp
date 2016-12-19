// -*- C++ -*-
//
#include "DataReaderListener.h"
#include "TestException.h"
#include "tests/DCPS/FooType5/FooDefTypeSupportC.h"
#include "tests/DCPS/FooType5/FooDefTypeSupportImpl.h"
#include "tests/Utils/ExceptionStreams.h"

template <class DT, class DT_seq, class DR, class DR_ptr, class DR_var>
int read (::DDS::DataReader_ptr reader, DT& foo)
{
  try
  {
    DR_var foo_dr
      = DR::_narrow(reader);
    if (CORBA::is_nil (foo_dr.in ()))
    {
      ACE_ERROR ((LM_ERROR,
        ACE_TEXT("(%P|%t) DataReaderListenerImpl::read - _narrow failed.\n")));
      throw BadReaderException() ;
    }

    ::DDS::SampleInfo si ;
    DDS::ReturnCode_t const status = foo_dr->read_next_sample(foo, si) ;

    if (status == ::DDS::RETCODE_OK)
    {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) DataReaderListenerImpl::read %X foo.x = %f foo.y = %f, foo.data_source = %d \n"),
        reader, foo.x, foo.y, foo.data_source));
    }
    else if (status == ::DDS::RETCODE_NO_DATA)
    {
      ACE_ERROR_RETURN ((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: DataReaderListenerImpl::reader received ::DDS::RETCODE_NO_DATA!\n")),
        -1);
    }
    else
    {
      ACE_ERROR_RETURN ((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: DataReaderListenerImpl::read status==%d\n"), status),
        -1);
    }
  }
  catch (const CORBA::Exception& ex)
  {
    ex._tao_print_exception ("(%P|%t) DataReaderListenerImpl::read - ");
    return -1;
  }

  return 0;
}

DataReaderListenerImpl::DataReaderListenerImpl( int expected)
 : samples_( 0),
   expected_( expected),
   condition_( this->lock_)
{
  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) DataReaderListenerImpl::DataReaderListenerImpl\n")
  ));
}

DataReaderListenerImpl::~DataReaderListenerImpl (void)
{
  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) DataReaderListenerImpl::~DataReaderListenerImpl ")
    ACE_TEXT("after %d samples\n"),
    this->samples_
  ));
}

void
DataReaderListenerImpl::waitForCompletion()
{
  ACE_GUARD (ACE_SYNCH_MUTEX, g, this->lock_);
  std::cout << "Subscriber waiting for complete signal" << std::endl;
  this->condition_.wait();
  std::cout << "Subscriber Got complete signal" << std::endl;
}

void DataReaderListenerImpl::on_requested_deadline_missed (
    ::DDS::DataReader_ptr reader,
    const ::DDS::RequestedDeadlineMissedStatus & status
)
{
  ACE_UNUSED_ARG(reader);
  ACE_UNUSED_ARG(status);

  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) DataReaderListenerImpl::on_requested_deadline_missed\n")
  ));
}

void DataReaderListenerImpl::on_requested_incompatible_qos (
  ::DDS::DataReader_ptr reader,
  const ::DDS::RequestedIncompatibleQosStatus & status
)
{
  ACE_UNUSED_ARG(reader);
  ACE_UNUSED_ARG(status);

  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) DataReaderListenerImpl::on_requested_incompatible_qos\n")
  ));
}

void DataReaderListenerImpl::on_liveliness_changed (
  ::DDS::DataReader_ptr reader,
  const ::DDS::LivelinessChangedStatus & status
)
{
  ACE_UNUSED_ARG(reader);
  ACE_UNUSED_ARG(status);

  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) DataReaderListenerImpl::on_liveliness_changed\n")
  ));
}

void DataReaderListenerImpl::on_subscription_matched (
  ::DDS::DataReader_ptr reader,
  const ::DDS::SubscriptionMatchedStatus & status
)
{
  ACE_UNUSED_ARG(reader) ;
  ACE_UNUSED_ARG(status) ;

  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) DataReaderListenerImpl::on_subscription_matched \n")
  ));
}

void DataReaderListenerImpl::on_sample_rejected(
  ::DDS::DataReader_ptr reader,
  const DDS::SampleRejectedStatus& status
)
{
  ACE_UNUSED_ARG(reader) ;
  ACE_UNUSED_ARG(status) ;

  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) DataReaderListenerImpl::on_sample_rejected \n")
  ));
}

void DataReaderListenerImpl::on_data_available(
  ::DDS::DataReader_ptr reader
)
{
  ::Xyz::FooNoKey foo;
  int ret = read <Xyz::FooNoKey,
      ::Xyz::FooNoKeySeq,
      ::Xyz::FooNoKeyDataReader,
      ::Xyz::FooNoKeyDataReader_ptr,
      ::Xyz::FooNoKeyDataReader_var> (reader, foo);

  if (ret != 0)
  {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) DataReaderListenerImpl::on_data_available read failed.\n")
    ));

  } else {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) DataReaderListenerImpl::on_data_available sample %d\n"),
      this->samples_
    ));
  }

  if( ++this->samples_ >= this->expected_) {
    std::cout << "Subscriber signaling complete" << std::endl;
    this->condition_.signal();
  }
}

void DataReaderListenerImpl::on_sample_lost(
  ::DDS::DataReader_ptr reader,
  const DDS::SampleLostStatus& status
)
{
  ACE_UNUSED_ARG(reader) ;
  ACE_UNUSED_ARG(status) ;

  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) DataReaderListenerImpl::on_sample_lost \n")
  ));
}

