// -*- C++ -*-
//
#include "DataReaderListener.h"
#include "common.h"
#include "TestException.h"
#include "dds/DCPS/Service_Participant.h"
#include "tests/DCPS/FooType5/FooDefTypeSupportC.h"
#include "tests/DCPS/FooType5/FooDefTypeSupportImpl.h"
#include "ace/OS_NS_unistd.h"

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

    DDS::ReturnCode_t status = foo_dr->take_next_sample(foo, si) ;

    if (status == ::DDS::RETCODE_OK)
    {
      if (si.valid_data == 1)
      {
        num_reads++;
        ACE_DEBUG((LM_DEBUG,
          ACE_TEXT("(%P|%t) reader %X take foo.x = %f foo.y = %f, foo.data_source = %d, count = %d \n"),
          reader, foo.x, foo.y, foo.data_source, num_reads.value()));
        ACE_DEBUG((LM_DEBUG,
          ACE_TEXT("(%P|%t) SampleInfo.sample_rank = %d \n"), si.sample_rank));

        if (results.add (foo) == -1)
        {
          ACE_DEBUG((LM_ERROR,
            ACE_TEXT("(%P|%t) ERROR add of sample resulted in -1\n")));
          return -1;
        }
      }
      else if (si.instance_state == DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE)
      {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) instance is disposed\n")));
      }
      else if (si.instance_state == DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE)
      {
        ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) instance is unregistered\n")));
      }
      else
      {
        ACE_ERROR ((LM_ERROR, "(%P|%t) DataReaderListenerImpl::on_data_available:"
          " received unknown instance state %d\n", si.instance_state));
      }
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

    int ret = 0;

    if (no_key)
    {
      ret = read <Xyz::FooNoKey,
            ::Xyz::FooNoKeySeq,
            ::Xyz::FooNoKeyDataReader,
            ::Xyz::FooNoKeyDataReader_ptr,
            ::Xyz::FooNoKeyDataReader_var> (reader);
    }
    else
    {
      ret = read <Xyz::Foo,
        ::Xyz::FooSeq,
        ::Xyz::FooDataReader,
        ::Xyz::FooDataReader_ptr,
        ::Xyz::FooDataReader_var> (reader);
    }

    if (ret != 0)
    {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) DataReaderListenerImpl::on_data_available read failed.\n")));
    }

    if (op_interval_ms > 0)
    {
      ACE_Time_Value delay(op_interval_ms/1000,
                           op_interval_ms%1000 *1000);
      ACE_OS::sleep (delay);
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

  void DataReaderListenerImpl::on_subscription_disconnected (
    ::DDS::DataReader_ptr reader,
    const ::OpenDDS::DCPS::SubscriptionDisconnectedStatus & status
  )
  {
    ACE_UNUSED_ARG(reader) ;
    ACE_UNUSED_ARG(status) ;

    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) DataReaderListenerImpl::on_subscription_disconnected \n")));
  }

  void DataReaderListenerImpl::on_subscription_reconnected (
    ::DDS::DataReader_ptr reader,
    const ::OpenDDS::DCPS::SubscriptionReconnectedStatus & status
  )
  {
    ACE_UNUSED_ARG(reader) ;
    ACE_UNUSED_ARG(status) ;

    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) DataReaderListenerImpl::on_subscription_reconnected \n")));
  }


    void DataReaderListenerImpl::on_subscription_lost (
    ::DDS::DataReader_ptr reader,
    const ::OpenDDS::DCPS::SubscriptionLostStatus & status
  )
  {
    ACE_UNUSED_ARG(reader) ;
    ACE_UNUSED_ARG(status) ;

    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) DataReaderListenerImpl::on_subscription_lost \n")));
  }

    void DataReaderListenerImpl::on_connection_deleted (
    ::DDS::DataReader_ptr reader
  )
  {
    ACE_UNUSED_ARG(reader) ;

    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) DataReaderListenerImpl::on_connection_deleted \n")));
  }
