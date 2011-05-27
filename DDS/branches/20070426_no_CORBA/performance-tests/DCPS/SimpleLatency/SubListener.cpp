// -*- C++ -*-
//
// $Id$
#include "SubListener.h"
#include "PubMessageTypeSupportImpl.h"
#include "AckMessageTypeSupportImpl.h"
#include "PubMessageTypeSupportC.h"
#include "AckMessageTypeSupportC.h"
#include <dds/DCPS/Service_Participant.h>
#include <ace/streams.h>

using namespace DDSPerfTest;

// Implementation skeleton constructor
PubDataReaderListenerImpl::PubDataReaderListenerImpl()
  : writer_ (),
    reader_ (),
    dr_servant_(0),
    dw_servant_(0),
    handle_(),
    sample_num_(1),
    done_ (0)
{
}

void PubDataReaderListenerImpl::init(DDS::DataReader_ptr dr, 
                                     DDS::DataWriter_ptr dw)
{
  this->writer_ = DDS::DataWriter::_duplicate (dw);
  this->reader_ = DDS::DataReader::_duplicate (dr);

  AckMessageDataWriter_var ackmessage_dw =
    AckMessageDataWriter::_narrow (this->writer_.in ());
  this->dw_servant_ = 
    TAO::DCPS::reference_to_servant<AckMessageDataWriterImpl>(ackmessage_dw.in());
  DDSPerfTest::AckMessage msg;
  this->handle_ = this->dw_servant_->_cxx_register (msg);

  PubMessageDataReader_var pubmessage_dr = 
    PubMessageDataReader::_unchecked_narrow(this->reader_.in());
  this->dr_servant_ =
    TAO::DCPS::reference_to_servant<PubMessageDataReaderImpl>(pubmessage_dr.in());
}

// Implementation skeleton destructor
PubDataReaderListenerImpl::~PubDataReaderListenerImpl ()
{
}

void PubDataReaderListenerImpl::on_data_available(DDS::DataReader_ptr)
  throw (CORBA::SystemException)
{
    DDSPerfTest::PubMessage message;
    DDS::SampleInfo si;
    // Use the reader data member (instead of the argument) for efficiency 
    // reasons
    this->dr_servant_->take_next_sample(message, si) ;

    if (message.seqnum == 0)
    {
      this->done_ = 1;
      return;
    }

    
    if (message.seqnum != this->sample_num_)
    {
      fprintf(stderr, 
              "ERROR - TAO_Sub: recieved seqnum %d on %d\n",
              message.seqnum, this->sample_num_);
      exit (1);
    }

    DDSPerfTest::AckMessage msg;
    msg.seqnum = message.seqnum;

    this->dw_servant_->write (msg, this->handle_);

    this->sample_num_++;
    return;    
}

void PubDataReaderListenerImpl::on_requested_deadline_missed (
    DDS::DataReader_ptr,
    const DDS::RequestedDeadlineMissedStatus &)
  throw (CORBA::SystemException)
{
}

void PubDataReaderListenerImpl::on_requested_incompatible_qos (
    DDS::DataReader_ptr,
    const DDS::RequestedIncompatibleQosStatus &)
  throw (CORBA::SystemException)
{
}

void PubDataReaderListenerImpl::on_liveliness_changed (
    DDS::DataReader_ptr,
    const DDS::LivelinessChangedStatus &)
  throw (CORBA::SystemException)
{
}

void PubDataReaderListenerImpl::on_subscription_match (
    DDS::DataReader_ptr,
    const DDS::SubscriptionMatchStatus &)
  throw (CORBA::SystemException)
{
}

void PubDataReaderListenerImpl::on_sample_rejected(
    DDS::DataReader_ptr,
    const DDS::SampleRejectedStatus&)
  throw (CORBA::SystemException)
{
}

void PubDataReaderListenerImpl::on_sample_lost(
  DDS::DataReader_ptr,
  const DDS::SampleLostStatus&)
  throw (CORBA::SystemException)
{
}
