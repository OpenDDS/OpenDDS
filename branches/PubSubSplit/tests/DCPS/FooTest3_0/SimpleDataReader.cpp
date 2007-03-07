// -*- C++ -*-
//
// $Id$
#include  "SimpleDataReader.h"
#include  "dds/DCPS/transport/framework/ReceivedDataSample.h"
#include  "tests/DCPS/FooType3/FooDefC.h"
#include  "ace/Log_Msg.h"
#include  "dds/DCPS/DataSampleHeader.h"


SimpleDataReader::SimpleDataReader()
  : sub_id_(0),
    received_test_message_(0)
{
}


SimpleDataReader::~SimpleDataReader()
{
}


void
SimpleDataReader::init(TAO::DCPS::RepoId sub_id)
{
  this->sub_id_ = sub_id;
}


void
SimpleDataReader::data_received(const TAO::DCPS::ReceivedDataSample& sample)
{
  ACE_DEBUG((LM_DEBUG, "(%P|%t) Data has been received: %T\n"));

  const ::TAO::DCPS::DataSampleHeader & header = sample.header_;

  if (header.message_id_ != ::TAO::DCPS::DATAWRITER_LIVELINESS)
  {
    this->received_test_message_ ++;

    ::TAO::DCPS::Serializer serializer (sample.sample_);
    ::Xyz::Foo foo;
    serializer >> foo;
    ACE_DEBUG((LM_DEBUG, "(%P|%t) Message: a_long_value=%d handle_value=%d "
      "sample_sequence=%d writer_id=%d\n", foo.a_long_value, foo.handle_value,
      foo.sample_sequence, foo.writer_id));
  }
  else
  {
    ACE_DEBUG((LM_DEBUG, "(%P|%t) got liveliness message.\n"));
  }
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
  return this->received_test_message_;
}
