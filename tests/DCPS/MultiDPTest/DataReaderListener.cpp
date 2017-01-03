// -*- C++ -*-
//
#include "DataReaderListener.h"
#include "common.h"
#include "TestException.h"
#include "dds/DCPS/Service_Participant.h"
#include "tests/DCPS/FooType5/FooDefTypeSupportC.h"
#include "tests/DCPS/FooType5/FooDefTypeSupportImpl.h"

template <class DT, class DT_seq, class DR, class DR_ptr, class DR_var>
int read (::DDS::DataReader_ptr reader)
{
  try
  {
    DR_var foo_dr
      = DR::_narrow(reader);
    if (CORBA::is_nil (foo_dr.in ()))
    {
      ACE_ERROR ((LM_ERROR,
        ACE_TEXT("(%P|%t) read: _narrow failed.\n")));
      throw TestException() ;
    }

    DT foo;
    ::DDS::SampleInfo si ;

    DDS::ReturnCode_t const status = foo_dr->read_next_sample(foo, si) ;

    if (status == ::DDS::RETCODE_OK)
    {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) reader %X foo.x = %f foo.y = %f, foo.data_source = %d \n"),
        reader, foo.x, foo.y, foo.data_source));
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) SampleInfo.sample_rank = %d \n"),
        si.sample_rank));
    }
    else if (status == ::DDS::RETCODE_NO_DATA)
    {
      ACE_ERROR_RETURN ((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: reader received ::DDS::RETCODE_NO_DATA!\n")),
        -1);
    }
    else
    {
      ACE_ERROR_RETURN ((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: read  foo: Error: %d\n"), status),
        -1);
    }
  }
  catch (const CORBA::Exception& ex)
  {
    ex._tao_print_exception ("Exception caught in read:");
    return -1;
  }

  return 0;
}


DataReaderListenerImpl::DataReaderListenerImpl ()
  {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) DataReaderListenerImpl::DataReaderListenerImpl\n")));
  }

DataReaderListenerImpl::~DataReaderListenerImpl (void)
  {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) DataReaderListenerImpl::~DataReaderListenerImpl\n")));
  }

void DataReaderListenerImpl::on_requested_deadline_missed (
    ::DDS::DataReader_ptr reader,
    const ::DDS::RequestedDeadlineMissedStatus & status
  )
  {
    ACE_UNUSED_ARG(reader);
    ACE_UNUSED_ARG(status);

    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) DataReaderListenerImpl::on_requested_deadline_missed\n")));
  }

void DataReaderListenerImpl::on_requested_incompatible_qos (
    ::DDS::DataReader_ptr reader,
    const ::DDS::RequestedIncompatibleQosStatus & status
  )
  {
    ACE_UNUSED_ARG(reader);
    ACE_UNUSED_ARG(status);

    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) DataReaderListenerImpl::on_requested_incompatible_qos\n")));
  }

void DataReaderListenerImpl::on_liveliness_changed (
    ::DDS::DataReader_ptr reader,
    const ::DDS::LivelinessChangedStatus & status
  )
  {
    ACE_UNUSED_ARG(reader);
    ACE_UNUSED_ARG(status);

    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) DataReaderListenerImpl::on_liveliness_changed\n")));
  }

void DataReaderListenerImpl::on_subscription_matched (
    ::DDS::DataReader_ptr reader,
    const ::DDS::SubscriptionMatchedStatus & status
  )
  {
    ACE_UNUSED_ARG(reader) ;
    ACE_UNUSED_ARG(status) ;

    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) DataReaderListenerImpl::on_subscription_matched \n")));
  }

  void DataReaderListenerImpl::on_sample_rejected(
    ::DDS::DataReader_ptr reader,
    const DDS::SampleRejectedStatus& status
  )
  {
    ACE_UNUSED_ARG(reader) ;
    ACE_UNUSED_ARG(status) ;

    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) DataReaderListenerImpl::on_sample_rejected \n")));
  }

  void DataReaderListenerImpl::on_data_available(
    ::DDS::DataReader_ptr reader
  )
  {
    //ACE_DEBUG((LM_DEBUG,
    //  ACE_TEXT("(%P|%t) DataReaderListenerImpl::on_data_available %d\n"), num_reads.value ()));

    num_reads ++;

    int ret = read <Xyz::Foo,
        ::Xyz::FooSeq,
        ::Xyz::FooDataReader,
        ::Xyz::FooDataReader_ptr,
        ::Xyz::FooDataReader_var> (reader);

    if (ret != 0)
    {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) DataReaderListenerImpl::on_data_available read failed.\n")));
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
      ACE_TEXT("(%P|%t) DataReaderListenerImpl::on_sample_lost \n")));
  }

