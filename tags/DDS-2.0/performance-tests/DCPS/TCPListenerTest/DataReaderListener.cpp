// -*- C++ -*-
//
// $Id$
#include "DataReaderListener.h"
#include "dds/DCPS/Service_Participant.h"
#include "../TypeNoKeyBounded/PTDefTypeSupportC.h"
#include "../TypeNoKeyBounded/PTDefTypeSupportImpl.h"

extern long subscriber_delay_msec; // from common.h

template<class Tseq, class Iseq, class R, class R_ptr, class R_var, class Rimpl>
int read (::DDS::DataReader_ptr reader, bool use_zero_copy_reads)
{
  // Since our listener is on the DataReader we always know
  // that the "reader" parameter will be the one associated
  // with this listener so we could keep the R_var value
  // instead of narrowing it for each call but the 
  // performance gain would be very minimal.
  R_var var_dr
    = R::_narrow(reader);

  if (subscriber_delay_msec)
    {
      ACE_Time_Value delay ( subscriber_delay_msec / 1000,
                            (subscriber_delay_msec % 1000) * 1000);
      ACE_OS::sleep (delay);
    }

  const ::CORBA::Long max_read_samples = 100;
  Tseq samples(use_zero_copy_reads ? 0 : max_read_samples, max_read_samples);
  Iseq infos(  use_zero_copy_reads ? 0 : max_read_samples, max_read_samples);

  int samples_recvd = 0;
  DDS::ReturnCode_t status;
  // initialize to zero.

  status = var_dr->read (
    samples,
    infos,
    max_read_samples,
    ::DDS::NOT_READ_SAMPLE_STATE,
    ::DDS::ANY_VIEW_STATE,
    ::DDS::ANY_INSTANCE_STATE);

  if (status == ::DDS::RETCODE_OK)
    {
      samples_recvd = samples.length ();
    }
  else if (status == ::DDS::RETCODE_NO_DATA)
    {
      ACE_ERROR((LM_ERROR, " Empty read!\n"));
    }
  else
    {
        ACE_OS::printf (" read  data: Error: %d\n", status) ;
    }

  status = var_dr->return_loan(samples, infos);
  if (status != ::DDS::RETCODE_OK)
  {
      // TBD - why is printf used for errors?
        ACE_OS::printf (" return_loan: Error: %d\n", status) ;
      ACE_ERROR((LM_ERROR, " return_loan gave error status %d\n", status));

  }

  return samples_recvd;
}











// Implementation skeleton constructor
DataReaderListenerImpl::DataReaderListenerImpl (int num_publishers,
                                                int num_samples,
                                                int data_size,
                                                int read_interval,
                                                bool use_zero_copy_reads)
:
  samples_lost_count_(0),
  samples_rejected_count_(0),
  samples_received_count_(0),
  total_samples_count_(0),
  read_interval_ (read_interval),
  use_zero_copy_reads_(use_zero_copy_reads),
  num_publishers_(num_publishers),
  num_samples_ (num_samples),
  data_size_ (data_size),
  num_floats_per_sample_ (data_size)
  {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) DataReaderListenerImpl::DataReaderListenerImpl\n")));

    stats_.init(num_publishers_, num_samples_, data_size_, num_floats_per_sample_*4 + 8);

    int total_samples = num_publishers_ * num_samples_;
    if (0 != total_samples % read_interval_)
    {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("ERROR: read_interval %d is not an even divisor of the total number of samples %d\n")
        ACE_TEXT("ERROR: The subscriber will never finish!\n"),
        read_interval_,
        total_samples));
    }

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
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ))
  {
    ACE_UNUSED_ARG(reader);
    ACE_UNUSED_ARG(status);
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
      ACE_TEXT("(%P|%t) DataReaderListenerImpl::on_requested_incompatible_qos policyId=%d\n"),
      status.last_policy_id));
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
    ACE_DEBUG((LM_INFO,"(%P|%t) DataReaderListenerImpl::on_liveliness_changed called\n"));
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
    ACE_DEBUG((LM_INFO,"(%P|%t) DataReaderListenerImpl::on_subscription_matched called\n"));
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
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) DataReaderListenerImpl::on_sample_rejected\n")));

    GuardType guard(this->lock_);
    samples_rejected_count_ += status.total_count_change;
    total_samples_count_ += status.total_count_change;
    stats_.samples_received(status.total_count_change);
  }


void DataReaderListenerImpl::on_data_available(
    ::DDS::DataReader_ptr reader
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ))
  {
    ACE_UNUSED_ARG(reader) ;
    //ACE_DEBUG((LM_DEBUG,
    //  ACE_TEXT("(%P|%t) DataReaderListenerImpl::on_data_available\n")));

    GuardType guard(this->lock_);
    samples_received_count_++;
    total_samples_count_++;

    if (0 == total_samples_count_ % read_interval_)
    {
      // perform the read
      int samples_read = read_samples(reader);
      ACE_UNUSED_ARG(samples_read);
      //ACE_DEBUG((LM_DEBUG,
      //  ACE_TEXT("(%P|%t) DataReaderListenerImpl read %d samples\n"),
      //  samples_read));
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
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) DataReaderListenerImpl::on_sample_lost\n")));

    GuardType guard(this->lock_);
    samples_lost_count_ += status.total_count_change;
    total_samples_count_ += status.total_count_change;
    stats_.samples_received(status.total_count_change);
  }


int DataReaderListenerImpl::read_samples (::DDS::DataReader_ptr reader)
{
  int num_read = 0;

  switch ( num_floats_per_sample_ )
  {

  case 128:
      {
      num_read = read < ::Xyz::Pt128Seq,
                        ::DDS::SampleInfoSeq,
                      ::Xyz::Pt128DataReader,
                      ::Xyz::Pt128DataReader_ptr,
                      ::Xyz::Pt128DataReader_var,
                      ::Xyz::Pt128DataReaderImpl>
                          (reader, use_zero_copy_reads_);
      }
      break;

  case 512:
      {
      num_read = read < ::Xyz::Pt512Seq,
                        ::DDS::SampleInfoSeq,
                      ::Xyz::Pt512DataReader,
                      ::Xyz::Pt512DataReader_ptr,
                      ::Xyz::Pt512DataReader_var,
                      ::Xyz::Pt512DataReaderImpl>
                          (reader, use_zero_copy_reads_);
      }
      break;

  case 2048:
      {
      num_read = read < ::Xyz::Pt2048Seq,
                        ::DDS::SampleInfoSeq,
                      ::Xyz::Pt2048DataReader,
                      ::Xyz::Pt2048DataReader_ptr,
                      ::Xyz::Pt2048DataReader_var,
                      ::Xyz::Pt2048DataReaderImpl>
                          (reader, use_zero_copy_reads_);
      }
      break;

  case 8192:
      {
      num_read = read < ::Xyz::Pt8192Seq,
                        ::DDS::SampleInfoSeq,
                      ::Xyz::Pt8192DataReader,
                      ::Xyz::Pt8192DataReader_ptr,
                      ::Xyz::Pt8192DataReader_var,
                      ::Xyz::Pt8192DataReaderImpl>
                          (reader, use_zero_copy_reads_);
      }
      break;

  default:
      ACE_ERROR((LM_ERROR,"ERROR: bad data size %d\n", data_size_));
      return 0;
      break;
  };


  stats_.samples_received(num_read);

  return num_read;
}


bool DataReaderListenerImpl::is_finished ()
{
ACE_DEBUG((LM_DEBUG,
          ACE_TEXT("(%P|%t) DataReaderListenerImpl %d of %d samples received\n"),
          stats_.packet_count_,
          stats_.expected_packets_));

  bool is_finished = stats_.all_packets_received();
  if (is_finished)
    {
      stats_.dump();
    }

  return is_finished;
}
