// -*- C++ -*-
//
#include "SimpleDataReader.h"
#include "dds/DCPS/transport/framework/ReceivedDataSample.h"
#include "dds/DCPS/GuidBuilder.h"

#include "ace/Log_Msg.h"
#include "ace/OS_NS_sys_time.h"

#include "dds/DCPS/transport/framework/EntryExit.h"

#include "TestException.h"

SimpleDataReader::SimpleDataReader(const OpenDDS::DCPS::RepoId& sub_id)
  : sub_id_(sub_id)
  , num_messages_expected_(0)
  , num_messages_received_(0)
{
  DBG_ENTRY("SimpleDataReader","SimpleDataReader");
}


SimpleDataReader::~SimpleDataReader()
{
  DBG_ENTRY("SimpleDataReader","~SimpleDataReader");
}


void
SimpleDataReader::init(const OpenDDS::DCPS::AssociationData& publication,
                       int num_msgs)
{
  DBG_ENTRY("SimpleDataReader","init");
  this->num_messages_expected_ = num_msgs;
  this->num_messages_received_ = 0;

  // Add the association between the local sub_id and the remote pub_id
  // to the transport via the TransportInterface.
  bool result = this->associate(publication, false /* active */);

  if (!result) {
    ACE_ERROR((LM_ERROR,
               "(%P|%t) SimpleDataReader::init() Failed to associate.\n"));
    throw TestException();
  }
}


void
SimpleDataReader::data_received(const OpenDDS::DCPS::ReceivedDataSample& sample)
{
  DBG_ENTRY("SimpleDataReader","data_received");

  ACE_DEBUG((LM_DEBUG, "(%P|%t) Data has been received:\n"));
  if (sample.sample_->length() < 25) {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) Message: [%C]\n", sample.sample_->rd_ptr()));
  }

  if (0 == num_messages_received_) {
    begin_recvd_ = ACE_OS::gettimeofday();
  }

  ++this->num_messages_received_;

  if (this->num_messages_received_ == this->num_messages_expected_) {
    finished_recvd_ = ACE_OS::gettimeofday();
  }
}


void
SimpleDataReader::transport_lost()
{
  DBG_ENTRY("SimpleDataReader","transport_lost");

  ACE_DEBUG((LM_DEBUG,
             "(%P|%t) The transport has been lost.\n"));
}


int
SimpleDataReader::received_test_message() const
{
  return (this->num_messages_received_ == this->num_messages_expected_) ? 1 : 0;
}

void
SimpleDataReader::print_time()
{
  ACE_Time_Value total = finished_recvd_ - begin_recvd_;
  ACE_DEBUG((LM_INFO,
    "(%P|%t) Total time required is %d.%d seconds.\n",
             total.sec(),
             total.usec() % 1000000));
}
