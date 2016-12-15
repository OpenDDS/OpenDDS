// -*- C++ -*-
//
#include "DataReaderListener.h"
#include "dds/DCPS/Service_Participant.h"
#include "../TypeNoKeyBounded/PTDefTypeSupportC.h"
#include "../TypeNoKeyBounded/PTDefTypeSupportImpl.h"


template<class Tseq, class R, class R_ptr, class R_var>
::DDS::ReturnCode_t read (::DDS::DataReader_ptr reader,
                          ACE_Array<bool>& pub_finished)
{
  R_var var_dr
    = R::_narrow(reader);

  const ::CORBA::Long max_read_samples = 100;
  Tseq samples(max_read_samples);
  ::DDS::SampleInfoSeq infos(max_read_samples);

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
      if ((samples[samples.length () - 1].sequence_num == -1)
          || (samples[0].sequence_num == -1))
        {
          for (unsigned i = 0; i < samples.length (); i ++)
            {
              if (samples[i].sequence_num != -1)
                {
                  samples_recvd ++;
                }
              else
                {
                  pub_finished[samples[i].data_source] = true;
                }
            }
        }
      else
        {
          samples_recvd = samples.length ();
        }
    }
  else if (status == ::DDS::RETCODE_NO_DATA)
    {
      ACE_ERROR((LM_ERROR, " Empty read!\n"));
    }
  else
    {
      ACE_OS::printf (" read  data: Error: %d\n", status) ;
    }

  return samples_recvd;
}

DataReaderListenerImpl::DataReaderListenerImpl (int num_publishers,
                                                int num_samples,
                                                int data_size,
                                                int read_interval)
:
  samples_lost_count_(0),
  samples_rejected_count_(0),
  samples_received_count_(0),
  total_samples_count_(0),
  read_interval_ (read_interval),
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

    pub_finished_.size(stats_.num_publishers_);
    for (unsigned j = 0; j < stats_.num_publishers_; j++)
      {
        pub_finished_[j] = false;
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
  {
    ACE_UNUSED_ARG(reader);
    ACE_UNUSED_ARG(status);
  }


void DataReaderListenerImpl::on_requested_incompatible_qos (
    ::DDS::DataReader_ptr reader,
    const ::DDS::RequestedIncompatibleQosStatus & status
  )
  {
    ACE_UNUSED_ARG(reader);
    ACE_UNUSED_ARG(status);
  }


void DataReaderListenerImpl::on_liveliness_changed (
    ::DDS::DataReader_ptr reader,
    const ::DDS::LivelinessChangedStatus & status
  )
  {
    ACE_UNUSED_ARG(reader);
    ACE_UNUSED_ARG(status);
  }


void DataReaderListenerImpl::on_subscription_matched (
    ::DDS::DataReader_ptr reader,
    const ::DDS::SubscriptionMatchedStatus & status
  )
  {
    ACE_UNUSED_ARG(reader) ;
    ACE_UNUSED_ARG(status) ;
  }


void DataReaderListenerImpl::on_sample_rejected(
    ::DDS::DataReader_ptr reader,
    const DDS::SampleRejectedStatus& status
  )
  {
    ACE_UNUSED_ARG(reader) ;
    ACE_TString str = ACE_TEXT("");

    if (::DDS::REJECTED_BY_SAMPLES_PER_INSTANCE_LIMIT == status.last_reason)
    {
      str += ACE_TEXT(" Samples Per Instance Limit");
    }

    if (::DDS::REJECTED_BY_INSTANCES_LIMIT == status.last_reason)
    {
      str += ACE_TEXT(" Instances Limit");
    }

    if (::DDS::REJECTED_BY_SAMPLES_LIMIT == status.last_reason)
    {
      str += ACE_TEXT(" Samples Limit");
    }

    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) DataReaderListenerImpl::on_sample_rejected for %s\n"),
      str.c_str() ));

    GuardType guard(this->lock_);
    samples_rejected_count_ += status.total_count_change;
    total_samples_count_ += status.total_count_change;
    stats_.samples_received(status.total_count_change);
  }


void DataReaderListenerImpl::on_data_available(
    ::DDS::DataReader_ptr reader
  )
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
                      ::Xyz::Pt128DataReader,
                      ::Xyz::Pt128DataReader_ptr,
                      ::Xyz::Pt128DataReader_var>
                        (
                        reader,
                        pub_finished_);
    }
    break;

  case 512:
    {
      num_read = read < ::Xyz::Pt512Seq,
                      ::Xyz::Pt512DataReader,
                      ::Xyz::Pt512DataReader_ptr,
                      ::Xyz::Pt512DataReader_var>
                        (
                        reader,
                        pub_finished_);
    }
    break;

  case 2048:
    {
      num_read = read < ::Xyz::Pt2048Seq,
                      ::Xyz::Pt2048DataReader,
                      ::Xyz::Pt2048DataReader_ptr,
                      ::Xyz::Pt2048DataReader_var>
                        (
                        reader,
                        pub_finished_);
    }
    break;

  case 8192:
    {
      num_read = read < ::Xyz::Pt8192Seq,
                      ::Xyz::Pt8192DataReader,
                      ::Xyz::Pt8192DataReader_ptr,
                      ::Xyz::Pt8192DataReader_var>
                        (
                        reader,
                        pub_finished_);
    }
    break;

  default:
    ACE_ERROR((LM_ERROR,"ERROR: bad data size %d\n", data_size_));
    return 0;
    break;
  };

  stats_.samples_received(num_read);

  if (num_read < read_interval_)
    {
      check_finished();
    }

  return num_read;
}


bool DataReaderListenerImpl::is_finished ()
{
ACE_DEBUG((LM_DEBUG,
          ACE_TEXT("(%P|%t) DataReaderListenerImpl %d of %d samples received\n"),
          stats_.packet_count_,
          stats_.expected_packets_));

  bool all_finished = true;
  for (unsigned j = 0; j < stats_.num_publishers_; j++)
    {
      all_finished = all_finished && pub_finished_[j];
    }

  if (all_finished)
    {
      stats_.dump();
    }

  return all_finished;
}


void DataReaderListenerImpl::check_finished ()
{
  bool all_finished = true;
  for (unsigned j = 0; j < stats_.num_publishers_; j++)
    {
      all_finished = all_finished && pub_finished_[j];
    }

  if (all_finished)
    {
      stats_.finished();
    }
}
