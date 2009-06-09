// -*- C++ -*-
//
// $Id$
#include "SimpleDataReader.h"
#include "dds/DCPS/transport/framework/ReceivedDataSample.h"
#include "ace/Log_Msg.h"
#include "ace/OS.h"

#include "dds/DCPS/transport/framework/EntryExit.h"


SimpleDataReader::SimpleDataReader()
  : sub_id_( OpenDDS::DCPS::GUID_UNKNOWN),
    num_messages_expected_(0),
    num_messages_received_(0),
    sequence_(0)
{
}


SimpleDataReader::~SimpleDataReader()
{
}


void
SimpleDataReader::init(OpenDDS::DCPS::RepoId sub_id,
                       unsigned num_messages_expected)
{
  // TURN_ON_VERBOSE_DEBUG ;
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
  VDBG((LM_DEBUG, "(%P|%t) Data has been received:\n"));
  VDBG((LM_DEBUG, "(%P|%t) Message: \"%s\"\n", sample.sample_->rd_ptr()));

  ++this->num_messages_received_;

  int nap_for = 5;
  int nap_every = 1000;
  int nap_secs = 1;

  int nap_on = this->num_messages_received_ % nap_every;

  if (nap_on < nap_for)
    {
      VDBG((LM_DEBUG,
                 "(%P|%t) SimpleDataReader napping for %d second(s).\n",
                 nap_secs));

      ACE_OS::sleep(nap_secs);
    }

  VDBG((LM_DEBUG,
             "(%P|%t) SimpleDataReader has now received %d of the %d "
             "expected messages.\n",
             this->num_messages_received_,this->num_messages_expected_));

  this->sequence_ = sample.header_.sequence_;
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
  return this->sequence_ ;
}
