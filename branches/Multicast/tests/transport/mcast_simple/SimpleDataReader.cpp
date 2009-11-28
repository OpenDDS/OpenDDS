// -*- C++ -*-
//
// $Id$
#include "SimpleDataReader.h"
#include "dds/DCPS/transport/framework/ReceivedDataSample.h"
#include "dds/DCPS/GuidBuilder.h"
#include "ace/Log_Msg.h"

#include "dds/DCPS/transport/framework/EntryExit.h"


SimpleDataReader::SimpleDataReader()
  : sub_id_(OpenDDS::DCPS::GuidBuilder::create ()),
    received_test_message_(0)
{
  DBG_ENTRY("SimpleDataReader","SimpleDataReader");
}


SimpleDataReader::~SimpleDataReader()
{
  DBG_ENTRY("SimpleDataReader","~SimpleDataReader");
}


void
SimpleDataReader::init(OpenDDS::DCPS::RepoId sub_id)
{
  DBG_ENTRY("SimpleDataReader","init");

  this->sub_id_ = sub_id;
}


void
SimpleDataReader::data_received(const OpenDDS::DCPS::ReceivedDataSample& sample)
{
  DBG_ENTRY("SimpleDataReader","data_received");

  ACE_DEBUG((LM_DEBUG, "(%P|%t) Data has been received:\n"));
  ACE_DEBUG((LM_DEBUG, "(%P|%t) Message: [%C]\n", sample.sample_->rd_ptr()));
  this->received_test_message_ = 1;
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
  return this->received_test_message_;
}
