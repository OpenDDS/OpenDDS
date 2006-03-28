// -*- C++ -*-
//
// $Id$
#include "DataReaderListener.h"
#include "common.h"
#include "TestException.h"
#include "dds/DCPS/Service_Participant.h"
#include "tests/DCPS/FooType5/FooTypeSupportC.h"
#include "tests/DCPS/FooType5/FooTypeSupportImpl.h"
#include "tests/DCPS/FooType5/FooNoKeyTypeSupportC.h"
#include "tests/DCPS/FooType5/FooNoKeyTypeSupportImpl.h"

// Only for Microsoft VC6
#if defined (_MSC_VER) && (_MSC_VER >= 1200) && (_MSC_VER < 1300)

// Added unused arguments with default value to work around with vc6 
// bug on template function instantiation.
template <class DT, class DT_seq, class DR, class DR_ptr, class DR_var, class DR_impl>
int read (::DDS::DataReader_ptr reader,
          DT* dt = 0, DR* dr = 0, DR_ptr dr_ptr = 0, DR_var* dr_var = 0, DR_impl* dr_impl = 0)
{
  ACE_UNUSED_ARG (dt);
  ACE_UNUSED_ARG (dr);
  ACE_UNUSED_ARG (dr_ptr);
  ACE_UNUSED_ARG (dr_var);
  ACE_UNUSED_ARG (dr_impl);
  
#else

template <class DT, class DT_seq, class DR, class DR_ptr, class DR_var, class DR_impl>
int read (::DDS::DataReader_ptr reader)
{

#endif

  ACE_TRY_NEW_ENV
  {
    DR_var foo_dr 
      = DR::_narrow(reader ACE_ENV_ARG_PARAMETER);
    if (CORBA::is_nil (foo_dr.in ()))
    {
      ACE_ERROR ((LM_ERROR,
        ACE_TEXT("(%P|%t)read: _narrow failed.\n")));
      throw TestException() ;
    }

    DR_impl* dr_servant =
        ::TAO::DCPS::reference_to_servant< DR_impl, DR_ptr>
                (foo_dr.in () ACE_ENV_SINGLE_ARG_PARAMETER);

    char action[5] ;
    if (use_take)
    {
      ACE_OS::strcpy(action, "take") ;
    }
    else
    {
      ACE_OS::strcpy(action, "read") ;
    }

    DT foo;
    ::DDS::SampleInfo si ;

    DDS::ReturnCode_t status  ;
    if (use_take)
    {
      status = dr_servant->take_next_sample(foo, si) ;
    }
    else
    {
      status = dr_servant->read_next_sample(foo, si) ;
    }

    if (status == ::DDS::RETCODE_OK)
    {
      ACE_DEBUG((LM_DEBUG,  
        ACE_TEXT("(%P|%t)reader %X %s foo.x = %f foo.y = %f, foo.data_source = %d \n"),
        reader, action, foo.x, foo.y, foo.data_source));
      ACE_DEBUG((LM_DEBUG,  
        ACE_TEXT("(%P|%t) %s SampleInfo.sample_rank = %d \n"),
        action, si.sample_rank));
       
      if (results.add (foo) == -1)
      {
        return -1;
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

    if (use_take)
    {
      //  Verify the samples were actually taken
      //
      DT_seq foo(1) ;
      ::DDS::SampleInfoSeq si(1) ;

      DDS::ReturnCode_t status ;
      status = dr_servant->read(foo, si,
                  1,
                  ::DDS::READ_SAMPLE_STATE | ::DDS::NOT_READ_SAMPLE_STATE,
                  ::DDS::ANY_VIEW_STATE,
                  ::DDS::ANY_INSTANCE_STATE) ;

      if (status != ::DDS::RETCODE_NO_DATA)
      {
        ACE_ERROR_RETURN ((LM_ERROR,
                   ACE_TEXT("(%P|%t) Data found when all of it should have been taken.\n")),
                   -1);
      }
    }
  }
  ACE_CATCHANY
  {
    ACE_PRINT_EXCEPTION (ACE_ANY_EXCEPTION,
      "Exception caught in read:");
    return -1;
  }
  ACE_ENDTRY;

  return 0;
}


// Implementation skeleton constructor
DataReaderListenerImpl::DataReaderListenerImpl ()
  {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) DataReaderListenerImpl::DataReaderListenerImpl\n")));
  }
  
// Implementation skeleton destructor
DataReaderListenerImpl::~DataReaderListenerImpl (void)
  {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) DataReaderListenerImpl::~DataReaderListenerImpl\n")));
  }

void DataReaderListenerImpl::on_requested_deadline_missed (
    ::DDS::DataReader_ptr reader,
    const ::DDS::RequestedDeadlineMissedStatus & status
    ACE_ENV_ARG_DECL
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ))
  {
    ACE_UNUSED_ARG(reader);
    ACE_UNUSED_ARG(status);

    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) DataReaderListenerImpl::on_requested_deadline_missed\n")));
  }
  
void DataReaderListenerImpl::on_requested_incompatible_qos (
    ::DDS::DataReader_ptr reader,
    const ::DDS::RequestedIncompatibleQosStatus & status
    ACE_ENV_ARG_DECL
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ))
  {
    ACE_UNUSED_ARG(reader);
    ACE_UNUSED_ARG(status);

    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) DataReaderListenerImpl::on_requested_incompatible_qos\n")));
  }
  
void DataReaderListenerImpl::on_liveliness_changed (
    ::DDS::DataReader_ptr reader,
    const ::DDS::LivelinessChangedStatus & status
    ACE_ENV_ARG_DECL
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ))
  {
    ACE_UNUSED_ARG(reader);
    ACE_UNUSED_ARG(status);

    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) DataReaderListenerImpl::on_liveliness_changed\n")));
  }
  
void DataReaderListenerImpl::on_subscription_match (
    ::DDS::DataReader_ptr reader,
    const ::DDS::SubscriptionMatchStatus & status
    ACE_ENV_ARG_DECL
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ))
  {
    ACE_UNUSED_ARG(reader) ;
    ACE_UNUSED_ARG(status) ;

    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) DataReaderListenerImpl::on_subscription_match \n")));
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
      ACE_TEXT("(%P|%t) DataReaderListenerImpl::on_sample_rejected \n")));
  }
  
  void DataReaderListenerImpl::on_data_available(
    ::DDS::DataReader_ptr reader
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ))
  {
    //ACE_DEBUG((LM_DEBUG,
    //  ACE_TEXT("(%P|%t) DataReaderListenerImpl::on_data_available %d\n"), num_reads.value ()));

    num_reads ++;

    int ret = 0;

    if (no_key)
    {
      ret = read <Xyz::FooNoKey, 
            ::Mine::FooNoKeySeq, 
            ::Mine::FooNoKeyDataReader,
            ::Mine::FooNoKeyDataReader_ptr,
            ::Mine::FooNoKeyDataReader_var,
            ::Mine::FooNoKeyDataReaderImpl> (reader);
    }
    else
    {
      ret = read <Xyz::Foo, 
        ::Mine::FooSeq, 
        ::Mine::FooDataReader,
        ::Mine::FooDataReader_ptr,
        ::Mine::FooDataReader_var,
        ::Mine::FooDataReaderImpl> (reader);
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
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ))
  {
    ACE_UNUSED_ARG(reader) ;
    ACE_UNUSED_ARG(status) ;

    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) DataReaderListenerImpl::on_sample_lost \n")));
  }
  
