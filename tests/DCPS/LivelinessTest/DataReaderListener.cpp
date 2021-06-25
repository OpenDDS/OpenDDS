// -*- C++ -*-
//
#include "common.h"
#include "DataReaderListener.h"

#include <tests/DCPS/common/SampleInfo.h>
#include <tests/DCPS/FooType4/FooDefTypeSupportC.h>
#include <tests/DCPS/FooType4/FooDefTypeSupportImpl.h>

#include <dds/DCPS/Service_Participant.h>

#include <dds/DdsDcpsSubscriptionC.h>

DataReaderListenerImpl::DataReaderListenerImpl(DistributedConditionSet_rch dcs,
                                               const OPENDDS_STRING& actor)
  : dcs_(dcs)
  , actor_(actor)
  , liveliness_lost_count_(0)
  , liveliness_gained_count_(0)
  , liveliness_changed_count_(0)
  {
    last_status_.alive_count = 0;
    last_status_.not_alive_count = 0;
    last_status_.alive_count_change = 0;
    last_status_.not_alive_count_change = 0;
    last_status_.last_publication_handle = DDS::HANDLE_NIL;

    //Initialize last_si_
    last_si_.sample_state = ::DDS::NOT_READ_SAMPLE_STATE;
    last_si_.view_state = ::DDS::NEW_VIEW_STATE;
    last_si_.instance_state = ::DDS::ALIVE_INSTANCE_STATE;
    last_si_.source_timestamp.sec = ::DDS::TIME_INVALID_SEC;
    last_si_.source_timestamp.nanosec = ::DDS::TIME_INVALID_NSEC;
    last_si_.instance_handle = ::DDS::HANDLE_NIL;
    last_si_.publication_handle = ::DDS::HANDLE_NIL;
    last_si_.disposed_generation_count = 0;
    last_si_.no_writers_generation_count = 0;
    last_si_.sample_rank = 0;
    last_si_.generation_rank = 0;
    last_si_.absolute_generation_rank = 0;
    last_si_.valid_data = false;
    last_si_.opendds_reserved_publication_seq = 0;

    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) DataReaderListenerImpl::DataReaderListenerImpl")
      ACE_TEXT(" use_take=%d num_ops_per_thread=%d\n"),
      use_take, num_ops_per_thread
      ));
    ACE_UNUSED_ARG(max_samples_per_instance);
    ACE_UNUSED_ARG(history_depth);

  }

DataReaderListenerImpl::~DataReaderListenerImpl(void)
  {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) DataReaderListenerImpl::~DataReaderListenerImpl\n")));
  }

void DataReaderListenerImpl::on_requested_deadline_missed(
    ::DDS::DataReader_ptr reader,
    const ::DDS::RequestedDeadlineMissedStatus & status)
  {
    ACE_UNUSED_ARG(reader);
    ACE_UNUSED_ARG(status);
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) DataReaderListenerImpl::on_requested_deadline_missed\n")));
  }

void DataReaderListenerImpl::on_requested_incompatible_qos(
    ::DDS::DataReader_ptr reader,
    const ::DDS::RequestedIncompatibleQosStatus & status)
  {
    ACE_UNUSED_ARG(reader);
    ACE_UNUSED_ARG(status);
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) DataReaderListenerImpl::on_requested_incompatible_qos\n")));
  }

void DataReaderListenerImpl::on_liveliness_changed(
    ::DDS::DataReader_ptr reader,
    const ::DDS::LivelinessChangedStatus & status)
  {
    ACE_UNUSED_ARG(reader);
    ACE_UNUSED_ARG(status);

    if (status.alive_count_change > 0) {
      OPENDDS_ASSERT(status.alive_count_change == 1);
      OPENDDS_ASSERT(status.not_alive_count_change <= 0);
      ++liveliness_gained_count_;
      dcs_->post(actor_, "LIVELINESS_GAINED_" + OpenDDS::DCPS::to_dds_string(liveliness_gained_count_));
    }
    if (status.not_alive_count_change > 0) {
      OPENDDS_ASSERT(status.alive_count_change <= 0);
      OPENDDS_ASSERT(status.not_alive_count_change == 1);
      ++liveliness_lost_count_;
      dcs_->post(actor_, "LIVELINESS_LOST_" + OpenDDS::DCPS::to_dds_string(liveliness_gained_count_));
    }

    ++liveliness_changed_count_;

    if (liveliness_changed_count_ > 1) {
      if (last_status_.alive_count == 0 && last_status_.not_alive_count == 0) {
        ACE_ERROR ((LM_ERROR,
          "ERROR: DataReaderListenerImpl::on_liveliness_changed"
          " Both alive_count and not_alive_count 0 should not happen at liveliness_changed_count %d\n",
          liveliness_changed_count_));
      } else if (status.alive_count == 0 && status.not_alive_count == 0) {
        ACE_DEBUG ((LM_DEBUG, "(%P|%t) DataReaderListenerImpl::on_liveliness_changed - this is the time callback\n"));
      } else {
        ::DDS::LivelinessChangedStatus expected_status;
        // expect the alive_count either 0 or 1
        expected_status.alive_count = 1 - last_status_.alive_count;
        expected_status.not_alive_count = 1 - last_status_.not_alive_count;
        expected_status.alive_count_change = status.alive_count - last_status_.alive_count;
        expected_status.not_alive_count_change = status.not_alive_count - last_status_.not_alive_count;

        if (status.alive_count != expected_status.alive_count ||
            status.not_alive_count != expected_status.not_alive_count ||
            status.alive_count_change != expected_status.alive_count_change ||
            status.not_alive_count_change != expected_status.not_alive_count_change) {
          ACE_ERROR ((LM_ERROR,
            "ERROR: DataReaderListenerImpl::on_liveliness_changed"
            " expected/got alive_count %d/%d not_alive_count %d/%d"
            " alive_count_change %d/%d not_alive_count_change %d/%d\n",
            expected_status.alive_count, status.alive_count,
            expected_status.not_alive_count, status.not_alive_count,
            expected_status.alive_count_change, status.alive_count_change,
            expected_status.not_alive_count_change, status.not_alive_count_change ));
        }
      }
    }

    last_status_ = status;
    ACE_DEBUG((LM_DEBUG,
      "(%P|%t) %T DataReaderListenerImpl::on_liveliness_changed %d\n"
      "alive_count %d not_alive_count %d alive_count_change %d not_alive_count_change %d\n",
      liveliness_changed_count_, status.alive_count, status.not_alive_count,
      status.alive_count_change, status.not_alive_count_change));
  }

void DataReaderListenerImpl::on_subscription_matched(
    ::DDS::DataReader_ptr reader,
    const ::DDS::SubscriptionMatchedStatus & status)
  {
    ACE_UNUSED_ARG(reader) ;
    ACE_UNUSED_ARG(status) ;
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) DataReaderListenerImpl::on_subscription_matched\n")));
  }

  void DataReaderListenerImpl::on_sample_rejected(
    ::DDS::DataReader_ptr reader,
    const DDS::SampleRejectedStatus& status)
  {
    ACE_UNUSED_ARG(reader) ;
    ACE_UNUSED_ARG(status) ;
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) DataReaderListenerImpl::on_sample_rejected\n")));
  }

  void DataReaderListenerImpl::on_data_available(
    ::DDS::DataReader_ptr reader)
  {
    ::Xyz::FooDataReader_var foo_dr =
        ::Xyz::FooDataReader::_narrow(reader);

    if (CORBA::is_nil (foo_dr.in ())) {
      ACE_ERROR ((LM_ERROR,
        ACE_TEXT("(%P|%t) ::Xyz::FooDataReader::_narrow failed.\n")));
    }

    ::Xyz::FooSeq foo(num_ops_per_thread) ;
    ::DDS::SampleInfoSeq si(num_ops_per_thread) ;

    DDS::ReturnCode_t status  ;
    if (use_take) {
      status = foo_dr->take(foo, si,
        num_ops_per_thread,
        ::DDS::NOT_READ_SAMPLE_STATE,
        ::DDS::ANY_VIEW_STATE,
        ::DDS::ANY_INSTANCE_STATE);
    } else {
      status = foo_dr->read(foo, si,
        num_ops_per_thread,
        ::DDS::NOT_READ_SAMPLE_STATE,
        ::DDS::ANY_VIEW_STATE,
        ::DDS::ANY_INSTANCE_STATE);
    }

    if (status == ::DDS::RETCODE_OK) {
      for (CORBA::ULong i = 0 ; i < si.length() ; ++i) {
        ACE_DEBUG((LM_DEBUG,
          "%T %C foo[%d]: x = %f y = %f, key = %d\n",
          use_take ? "took": "read", i, foo[i].x, foo[i].y, foo[i].key));
        PrintSampleInfo(si[i]) ;
        last_si_ = si[i] ;
      }
    } else if (status == ::DDS::RETCODE_NO_DATA) {
      ACE_OS::fprintf (stderr, "read returned ::DDS::RETCODE_NO_DATA\n") ;
    } else {
      ACE_OS::fprintf (stderr, "read - Error: %d\n", status) ;
    }
  }

  void DataReaderListenerImpl::on_sample_lost(
    ::DDS::DataReader_ptr reader,
    const DDS::SampleLostStatus& status)
  {
    ACE_UNUSED_ARG(reader) ;
    ACE_UNUSED_ARG(status) ;
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) DataReaderListenerImpl::on_sample_lost\n")));
  }

  void DataReaderListenerImpl::on_subscription_disconnected(
    ::DDS::DataReader_ptr reader,
    const ::OpenDDS::DCPS::SubscriptionDisconnectedStatus & status)
  {
    ACE_UNUSED_ARG(reader) ;
    ACE_UNUSED_ARG(status) ;
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) DataReaderListenerImpl::on_subscription_disconnected\n")));
  }

  void DataReaderListenerImpl::on_subscription_reconnected(
    ::DDS::DataReader_ptr reader,
    const ::OpenDDS::DCPS::SubscriptionReconnectedStatus & status)
  {
    ACE_UNUSED_ARG(reader) ;
    ACE_UNUSED_ARG(status) ;
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) DataReaderListenerImpl::on_subscription_reconnected\n")));
  }

  void DataReaderListenerImpl::on_subscription_lost(
    ::DDS::DataReader_ptr reader,
    const ::OpenDDS::DCPS::SubscriptionLostStatus & status)
  {
    ACE_UNUSED_ARG(reader) ;
    ACE_UNUSED_ARG(status) ;
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) DataReaderListenerImpl::on_subscription_lost\n")));
  }

  void DataReaderListenerImpl::on_budget_exceeded(
    ::DDS::DataReader_ptr,
    const ::OpenDDS::DCPS::BudgetExceededStatus&)
  {
    ACE_DEBUG ((LM_DEBUG, "(%P|%t) received on_budget_exceeded\n"));
  }
