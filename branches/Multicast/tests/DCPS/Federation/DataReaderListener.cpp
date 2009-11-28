// -*- C++ -*-
//
// $Id$
#include "DataReaderListener.h"
#include "TestException.h"
#include "tests/DCPS/FooType5/FooDefTypeSupportC.h"
#include "tests/DCPS/FooType5/FooDefTypeSupportImpl.h"

// Only for Microsoft VC6
#if defined (_MSC_VER) && (_MSC_VER >= 1200) && (_MSC_VER < 1300)

// Added unused arguments with default value to work around with vc6
// bug on template function instantiation.
template <class DT, class DT_seq, class DR, class DR_ptr, class DR_var, class DR_impl>
int read (::DDS::DataReader_ptr reader, ::DDS::DataWriter_ptr writer,
          DT* dt = 0, DR* dr = 0, DR_ptr dr_ptr = 0, DR_var* dr_var = 0, DR_impl* dr_impl = 0)
{
  ACE_UNUSED_ARG (dt);
  ACE_UNUSED_ARG (dr);
  ACE_UNUSED_ARG (dr_ptr);
  ACE_UNUSED_ARG (dr_var);
  ACE_UNUSED_ARG (dr_impl);

#else

template <class DT, class DT_seq, class DR, class DR_ptr, class DR_var, class DR_impl>
int read (::DDS::DataReader_ptr reader, DT& foo)
{

#endif

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

    DR_impl* dr_servant = dynamic_cast<DR_impl*> (foo_dr.in ());

    ::DDS::SampleInfo si ;

    DDS::ReturnCode_t status  ;

    status = dr_servant->read_next_sample(foo, si) ;

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


// Implementation skeleton constructor
DataReaderListenerImpl::DataReaderListenerImpl( int expected)
 : samples_( 0),
   expected_( expected),
   condition_( this->lock_)
{
  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) DataReaderListenerImpl::DataReaderListenerImpl\n")
  ));
}

// Implementation skeleton destructor
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
  this->condition_.wait();
}

void DataReaderListenerImpl::on_requested_deadline_missed (
    ::DDS::DataReader_ptr reader,
    const ::DDS::RequestedDeadlineMissedStatus & status
)
ACE_THROW_SPEC ((
  CORBA::SystemException
))
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
ACE_THROW_SPEC ((
  CORBA::SystemException
))
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
ACE_THROW_SPEC ((
  CORBA::SystemException
))
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
ACE_THROW_SPEC ((
  CORBA::SystemException
))
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
ACE_THROW_SPEC ((
  CORBA::SystemException
))
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
ACE_THROW_SPEC ((
  CORBA::SystemException
))
{
  ::Xyz::FooNoKey foo;
  int ret = read <Xyz::FooNoKey,
      ::Xyz::FooNoKeySeq,
      ::Xyz::FooNoKeyDataReader,
      ::Xyz::FooNoKeyDataReader_ptr,
      ::Xyz::FooNoKeyDataReader_var,
      ::Xyz::FooNoKeyDataReaderImpl> (reader, foo);

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
    this->condition_.signal();
  }
}

void DataReaderListenerImpl::on_sample_lost(
  ::DDS::DataReader_ptr reader,
  const DDS::SampleLostStatus& status
)
ACE_THROW_SPEC ((
  CORBA::SystemException
))
{
  ACE_UNUSED_ARG(reader) ;
  ACE_UNUSED_ARG(status) ;

  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) DataReaderListenerImpl::on_sample_lost \n")
  ));
}

