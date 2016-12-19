// -*- C++ -*-
//
#include "SubListener.h"
#include "DDSPerfTestTypeSupportImpl.h"
#include "DDSPerfTestTypeSupportC.h"
#include <dds/DCPS/Service_Participant.h>
#include <ace/streams.h>

using namespace DDSPerfTest;

PubDataReaderListenerImpl::PubDataReaderListenerImpl()
  : writer_ (),
    reader_ (),
    dr_servant_(),
    dw_servant_(),
    handle_(),
    sample_num_(1),
    done_ (0),
    use_zero_copy_(false)
{
}

void PubDataReaderListenerImpl::init(DDS::DataReader_ptr dr,
                                     DDS::DataWriter_ptr dw,
                                     bool use_zero_copy_read)
{
  this->writer_ = DDS::DataWriter::_duplicate (dw);
  this->reader_ = DDS::DataReader::_duplicate (dr);
  use_zero_copy_ = use_zero_copy_read;

  this->dw_servant_ =
    AckMessageDataWriter::_narrow (this->writer_.in ());
  DDSPerfTest::AckMessage msg;
  this->handle_ = this->dw_servant_->register_instance(msg);

  this->dr_servant_ =
    PubMessageDataReader::_unchecked_narrow(this->reader_.in());
}

PubDataReaderListenerImpl::~PubDataReaderListenerImpl ()
{
}

void PubDataReaderListenerImpl::on_data_available(DDS::DataReader_ptr)
{
    CORBA::Long seqnum;

    if (use_zero_copy_)
    {
      ::CORBA::Long max_read_samples = 1;
      DDSPerfTest::PubMessageSeq message(0, max_read_samples);
      DDS::SampleInfoSeq              si(0, max_read_samples, 0);
      // Use the reader data member (instead of the argument) for efficiency
      // reasons
      this->dr_servant_->take(message, si, max_read_samples,
                              ::DDS::NOT_READ_SAMPLE_STATE,
                              ::DDS::ANY_VIEW_STATE,
                              ::DDS::ANY_INSTANCE_STATE);

      seqnum = message[0].seqnum;
      this->dr_servant_->return_loan(message, si);
    }
    else
    {
      DDSPerfTest::PubMessage message;
      DDS::SampleInfo si;
      // Use the reader data member (instead of the argument) for efficiency
      // reasons
      this->dr_servant_->take_next_sample(message, si) ;

      seqnum = message.seqnum;
    }

    if (seqnum == 0)
    {
      this->done_ = 1;
      return;
    }

    if (seqnum != this->sample_num_)
    {
      fprintf(stderr,
              "ERROR - TAO_Sub: recieved seqnum %d on %d\n",
              seqnum, this->sample_num_);
      exit (1);
    }

    DDSPerfTest::AckMessage msg;
    msg.seqnum = seqnum;

    this->dw_servant_->write (msg, this->handle_);

    this->sample_num_++;
    return;
}

void PubDataReaderListenerImpl::on_requested_deadline_missed (
    DDS::DataReader_ptr,
    const DDS::RequestedDeadlineMissedStatus &)
{
}

void PubDataReaderListenerImpl::on_requested_incompatible_qos (
    DDS::DataReader_ptr,
    const DDS::RequestedIncompatibleQosStatus &)
{
}

void PubDataReaderListenerImpl::on_liveliness_changed (
    DDS::DataReader_ptr,
    const DDS::LivelinessChangedStatus &)
{
}

void PubDataReaderListenerImpl::on_subscription_matched (
    DDS::DataReader_ptr,
    const DDS::SubscriptionMatchedStatus &)
{
}

void PubDataReaderListenerImpl::on_sample_rejected(
    DDS::DataReader_ptr,
    const DDS::SampleRejectedStatus&)
{
}

void PubDataReaderListenerImpl::on_sample_lost(
  DDS::DataReader_ptr,
  const DDS::SampleLostStatus&)
{
}
