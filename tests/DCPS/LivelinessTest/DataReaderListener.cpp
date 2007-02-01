// -*- C++ -*-
//
// $Id$
#include "DataReaderListener.h"
#include "common.h"
#include "../common/SampleInfo.h"
#include "dds/DdsDcpsSubscriptionC.h"
#include "dds/DCPS/Service_Participant.h"
#include "tests/DCPS/FooType4/FooTypeSupportC.h"
#include "tests/DCPS/FooType4/FooTypeSupportImpl.h"

// Implementation skeleton constructor
DataReaderListenerImpl::DataReaderListenerImpl (void) :
  liveliness_changed_count_(0)
  {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) DataReaderListenerImpl::DataReaderListenerImpl\n")));

    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) DataReaderListenerImpl::DataReaderListenerImpl\n")
      " use_take=%d num_ops_per_thread=%d\n",
      use_take, num_ops_per_thread
      ));
    ACE_UNUSED_ARG(max_samples_per_instance);
    ACE_UNUSED_ARG(history_depth);
    ACE_UNUSED_ARG(using_udp);

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

    liveliness_changed_count_++ ;

    if (0 == liveliness_changed_count_ % 2)
      {
        // becoming active
        if (status.active_count != 1 ||
            status.inactive_count != 0 ||
            status.active_count_change != 1 ||
            status.inactive_count_change != -1)

          ACE_ERROR ((LM_ERROR,
                      "ERROR: DataReaderListenerImpl::on_liveliness_changed"
                      " expected/got active_count 1/%d inactive_count 0/%d"
                      " active_count_change 1/%d inactive_count_change -1/%d\n",
                      status.active_count,
                      status.inactive_count,
                      status.active_count_change,
                      status.inactive_count_change ));
      }
    else
      {
        // becoming inactive
        if (status.active_count != 0 ||
            status.inactive_count != 1 ||
            status.active_count_change != -1 ||
            status.inactive_count_change != 1)

          ACE_ERROR ((LM_ERROR,
                      "ERROR: DataReaderListenerImpl::on_liveliness_changed"
                      " expected/got active_count 0/%d inactive_count 1/%d"
                      " active_count_change -1/%d inactive_count_change 1/%d\n",
                      status.active_count,
                      status.inactive_count,
                      status.active_count_change,
                      status.inactive_count_change ));
      }

    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("%T (%P|%t) DataReaderListenerImpl::on_liveliness_changed %d\n"),
               liveliness_changed_count_));
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
    ::Mine::FooDataReader_var foo_dr =
        ::Mine::FooDataReader::_narrow(reader);

    if (CORBA::is_nil (foo_dr.in ()))
      {
        ACE_ERROR ((LM_ERROR,
               ACE_TEXT("(%P|%t) ::Mine::FooDataReader::_narrow failed.\n")));
      }

    ::Mine::FooDataReaderImpl* dr_servant =
        ::TAO::DCPS::reference_to_servant< ::Mine::FooDataReaderImpl,
                                           ::Mine::FooDataReader_ptr>
            (foo_dr.in ());

    ::Mine::FooSeq foo(num_ops_per_thread) ;
    ::DDS::SampleInfoSeq si(num_ops_per_thread) ;

    DDS::ReturnCode_t status  ;
    if (use_take)
      {
        status = dr_servant->take(foo, si,
                              num_ops_per_thread,
                              ::DDS::NOT_READ_SAMPLE_STATE,
                              ::DDS::ANY_VIEW_STATE,
                              ::DDS::ANY_INSTANCE_STATE);
      }
    else
      {
        status = dr_servant->read(foo, si,
                              num_ops_per_thread,
                              ::DDS::NOT_READ_SAMPLE_STATE,
                              ::DDS::ANY_VIEW_STATE,
                              ::DDS::ANY_INSTANCE_STATE);
      }

    if (status == ::DDS::RETCODE_OK)
      {
        for (CORBA::ULong i = 0 ; i < si.length() ; i++)
        {
          ACE_DEBUG((LM_DEBUG,
              "%T %s foo[%d]: x = %f y = %f, key = %d\n",
              use_take ? "took": "read", i, foo[i].x, foo[i].y, foo[i].key));
          PrintSampleInfo(si[i]) ;
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

    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) DataReaderListenerImpl::on_sample_lost \n")));
  }

  void DataReaderListenerImpl::on_subscription_disconnected (
    ::DDS::DataReader_ptr reader,
    const ::TAO::DCPS::SubscriptionDisconnectedStatus & status
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ))
  {
    ACE_UNUSED_ARG(reader) ;
    ACE_UNUSED_ARG(status) ;

    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) DataReaderListenerImpl::on_subscription_disconnected \n")));
  }

  void DataReaderListenerImpl::on_subscription_reconnected (
    ::DDS::DataReader_ptr reader,
    const ::TAO::DCPS::SubscriptionReconnectedStatus & status
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ))
  {
    ACE_UNUSED_ARG(reader) ;
    ACE_UNUSED_ARG(status) ;

    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) DataReaderListenerImpl::on_subscription_reconnected \n")));
  }

  void DataReaderListenerImpl::on_subscription_lost (
    ::DDS::DataReader_ptr reader,
    const ::TAO::DCPS::SubscriptionLostStatus & status
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ))
  {
    ACE_UNUSED_ARG(reader) ;
    ACE_UNUSED_ARG(status) ;

    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) DataReaderListenerImpl::on_subscription_lost \n")));
  }

  void DataReaderListenerImpl::on_connection_deleted (
    ::DDS::DataReader_ptr
    )
    ACE_THROW_SPEC ((
    ::CORBA::SystemException
    ))
  {
    ACE_DEBUG ((LM_DEBUG, "(%P|%t)received on_connection_deleted  \n"));
  }

