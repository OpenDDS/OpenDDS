// -*- C++ -*-
//
// $Id$
#include "DataReaderListenerImpl.h"
#include "common.h"
#include "../common/SampleInfo.h"
#include "dds/DdsDcpsSubscriptionC.h"
#include "dds/DCPS/Service_Participant.h"
#include "tests/DCPS/FooType4/FooDefTypeSupportC.h"
#include "tests/DCPS/FooType4/FooDefTypeSupportImpl.h"

// Implementation skeleton constructor
DataReaderListenerImpl::DataReaderListenerImpl (void) :
  deadline_missed_(0)
{
}

// Implementation skeleton destructor
DataReaderListenerImpl::~DataReaderListenerImpl (void)
  {
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
      ACE_TEXT("(%P|%t) DataReaderListenerImpl::on_requested_deadline_missed\n")));
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
      ACE_TEXT("(%P|%t) DataReaderListenerImpl::on_requested_incompatible_qos\n")));
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
    // if we have gone from active to inactive, then we missed a deadline
    if((status.active_count_change < 0) &&
       (status.inactive_count_change > 0))
    {
      ++deadline_missed_;
    }

    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) DataReaderListenerImpl::on_liveliness_changed: ")
      ACE_TEXT("active=%d, inactive=%d, ")
      ACE_TEXT("activeDelta=%d, inactiveDelta=%d ")
      ACE_TEXT("deadline_missed=%d\n"),
      status.active_count,
      status.inactive_count,
      status.active_count_change,
      status.inactive_count_change,
      deadline_missed_
    ));
  }

void DataReaderListenerImpl::on_subscription_match (
    ::DDS::DataReader_ptr reader,
    const ::DDS::SubscriptionMatchStatus & status
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
    ::Xyz::FooDataReader_var foo_dr =
        ::Xyz::FooDataReader::_narrow(reader);

    if (CORBA::is_nil (foo_dr.in ()))
      {
        ACE_ERROR ((LM_ERROR,
               ACE_TEXT("(%P|%t) ::Xyz::FooDataReader::_narrow failed.\n")));
      }

    ::Xyz::FooDataReaderImpl* dr_servant
      = dynamic_cast< ::Xyz::FooDataReaderImpl*>(foo_dr.in());

    const int num_ops_per_thread = 100;
    ::Xyz::FooSeq foo(num_ops_per_thread) ;
    ::DDS::SampleInfoSeq si(num_ops_per_thread) ;

    DDS::ReturnCode_t status  ;
    status = dr_servant->read(foo, si,
                          num_ops_per_thread,
                          ::DDS::NOT_READ_SAMPLE_STATE,
                          ::DDS::ANY_VIEW_STATE,
                          ::DDS::ANY_INSTANCE_STATE);

    if (status == ::DDS::RETCODE_OK)
      {
        for (CORBA::ULong i = 0 ; i < si.length() ; i++)
        {
          last_si_ = si[i] ;
        }
      }
      else if (status == ::DDS::RETCODE_NO_DATA)
      {
        ACE_OS::fprintf (stderr, "read returned ::DDS::RETCODE_NO_DATA\n") ;
      }
      else
      {
        ACE_OS::fprintf (stderr, "read - Error: %d\n", status) ;
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

  }

  void DataReaderListenerImpl::on_subscription_disconnected (
    ::DDS::DataReader_ptr reader,
    const ::OpenDDS::DCPS::SubscriptionDisconnectedStatus & status
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ))
  {
    ACE_UNUSED_ARG(reader) ;
    ACE_UNUSED_ARG(status) ;

  }

  void DataReaderListenerImpl::on_subscription_reconnected (
    ::DDS::DataReader_ptr reader,
    const ::OpenDDS::DCPS::SubscriptionReconnectedStatus & status
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ))
  {
    ACE_UNUSED_ARG(reader) ;
    ACE_UNUSED_ARG(status) ;

  }

  void DataReaderListenerImpl::on_subscription_lost (
    ::DDS::DataReader_ptr reader,
    const ::OpenDDS::DCPS::SubscriptionLostStatus & status
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ))
  {
    ACE_UNUSED_ARG(reader) ;
    ACE_UNUSED_ARG(status) ;

  }

  void DataReaderListenerImpl::on_connection_deleted (
    ::DDS::DataReader_ptr
    )
    ACE_THROW_SPEC ((
    ::CORBA::SystemException
    ))
  {
  }
