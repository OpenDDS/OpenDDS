// -*- C++ -*-
//
// $Id$
#include  "SimpleDataReader.h"
#include  "dds/DCPS/transport/framework/ReceivedDataSample.h"
#include  "tests/DCPS/FooType3/FooDefTypeSupportImpl.h"
#include  "ace/Log_Msg.h"
#include  "dds/DCPS/DataSampleHeader.h"
#include "dds/DCPS/debug.h"

#include "dds/DCPS/Serializer.h"

SimpleDataReader::SimpleDataReader()
  : sub_id_( OpenDDS::DCPS::GUID_UNKNOWN),
    received_test_message_(0),
    receive_delay_msec_ (0)
{
}


SimpleDataReader::~SimpleDataReader()
{
}


void
SimpleDataReader::init(OpenDDS::DCPS::RepoId sub_id,
                       int               receive_delay_msec)
{
  this->sub_id_ = sub_id;
  this->receive_delay_msec_ = receive_delay_msec;
}


void
SimpleDataReader::data_received(const OpenDDS::DCPS::ReceivedDataSample& sample)
{
  ::OpenDDS::DCPS::Serializer serializer (sample.sample_);
  ::Xyz::Foo foo;
  serializer >> foo;

  if (OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG, "(%P|%t)Received sample: %d\n", foo.sample_sequence));
  }
  
  // Shouldn't be printing this all the time.
  //ACE_DEBUG((LM_DEBUG, "(%P|%t) Message: a_long_value=%d handle_value=%d "
  //"sample_sequence=%d writer_id=%d\n", foo.a_long_value, foo.handle_value,
  //foo.sample_sequence, foo.writer_id));

  this->received_test_message_ ++;

  if (receive_delay_msec_ > 0)
  {
    ACE_Time_Value delay (receive_delay_msec_/1000, receive_delay_msec_%1000 * 1000);
    ACE_OS::sleep (delay);
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
