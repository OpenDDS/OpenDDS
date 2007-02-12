// -*- C++ -*-
//
// $Id$
#include "SimpleDataReader.h"
#include "dds/DCPS/transport/framework/ReceivedDataSample.h"
#include "ace/Log_Msg.h"
#include "ace/OS.h"
#include "ace/OS_NS_sys_time.h"

#include "dds/DCPS/transport/framework/EntryExit.h"


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
SimpleDataReader::init(TAO::DCPS::RepoId sub_id,
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
SimpleDataReader::data_received(const TAO::DCPS::ReceivedDataSample& sample)
{
  ACE_UNUSED_ARG(sample);

  ACE_DEBUG((LM_DEBUG, "(%P|%t) Data has been received:\n"));
//  ACE_DEBUG((LM_DEBUG, "(%P|%t) Message: \"%s\"\n", sample.sample_->rd_ptr()));

  if (0 == num_messages_received_)
    {
      begin_recvd_ = ACE_OS::gettimeofday();
    }

  ++this->num_messages_received_;

  if (this->num_messages_received_ == this->num_messages_expected_)
    {
      finished_recvd_ = ACE_OS::gettimeofday();
    }

  int nap_for = 10;
  int nap_secs = 1;

#if 0
  if (this->num_messages_received_ == 320)
    {
      TURN_ON_VERBOSE_DEBUG;
    }

  if (this->num_messages_received_ == 360)
    {
      TURN_OFF_VERBOSE_DEBUG;
    }
#endif

  int nap_on = this->num_messages_expected_ + 1;
//  int nap_every = 500;
//  int nap_on = this->num_messages_received_ % nap_every;

  if (nap_on < nap_for)
    {
      ACE_DEBUG((LM_DEBUG,
                 "(%P|%t) SimpleDataReader napping for %d second(s).\n",
                 nap_secs));

      ACE_OS::sleep(nap_secs);
    }

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


void
SimpleDataReader::print_time()
{
  ACE_Time_Value total = finished_recvd_ - begin_recvd_;
  ACE_ERROR((LM_ERROR,
    "(%P|%t) Total time required is %d.%d seconds.\n",
             total.sec(),
             total.usec() % 1000000));
}
