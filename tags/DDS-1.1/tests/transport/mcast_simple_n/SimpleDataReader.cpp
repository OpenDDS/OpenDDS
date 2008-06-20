// -*- C++ -*-
//
// $Id$
#include "SimpleDataReader.h"
#include "dds/DCPS/transport/framework/ReceivedDataSample.h"
#include "ace/Log_Msg.h"


SimpleDataReader::SimpleDataReader()
  : sub_id_(0),
    num_messages_expected_(0),
    num_messages_received_(0)
{
}


SimpleDataReader::~SimpleDataReader()
{
}


void
SimpleDataReader::init(OpenDDS::DCPS::RepoId sub_id,
                       unsigned num_messages_expected)
{
  this->sub_id_                = sub_id;
  this->num_messages_expected_ = num_messages_expected;
  this->num_messages_received_ = 0;
  ACE_DEBUG((LM_DEBUG,
             "(%P|%t) SimpleDataReader initialized to expect %d messages.\n",
             this->num_messages_expected_));
}


void
SimpleDataReader::data_received(const OpenDDS::DCPS::ReceivedDataSample& sample)
{
  ACE_DEBUG((LM_DEBUG, "(%P|%t) Data has been received:\n"));
  ACE_DEBUG((LM_DEBUG, "(%P|%t) Message: \"%s\"\n", sample.sample_->rd_ptr()));
  ++this->num_messages_received_;
  ACE_DEBUG((LM_DEBUG,
             "(%P|%t) SimpleDataReader has now received %d of the %d "
             "expected messages.\n",
             this->num_messages_received_,this->num_messages_expected_));
}


void
SimpleDataReader::transport_lost()
{
  ACE_DEBUG((LM_DEBUG,
             "(%P|%t) The transport has been lost.\n"));
}


int
SimpleDataReader::received_test_message() const
{
  return (this->num_messages_received_ == this->num_messages_expected_)
         ? 1 : 0;
}
