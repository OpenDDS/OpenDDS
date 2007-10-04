// -*- C++ -*-
//
// $Id$
#include  "SimpleDataReader.h"
#include  "dds/DCPS/transport/framework/ReceivedDataSample.h"
#include  "tests/DCPS/FooType3Unbounded/FooDefC.h"
#include  "ace/Log_Msg.h"
#include  "dds/DCPS/DataSampleHeader.h"


SimpleDataReader::SimpleDataReader()
  : sub_id_(0),
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
  // Shouldn't be printing this all the time.
  //ACE_DEBUG((LM_DEBUG, "(%P|%t) Data has been received:\n"));

  ::TAO::DCPS::Serializer serializer (sample.sample_);
  ::Xyz::Foo foo;
  serializer >> foo;

  // Shouldn't be printing this all the time.
  //ACE_DEBUG((LM_DEBUG, "(%P|%t) Message: a_long_value=%d handle_value=%d "
  //"sample_sequence=%d writer_id=%d \n", foo.a_long_value, foo.handle_value,
  //foo.sample_sequence, foo.writer_id));

  // Shouldn't be printing this all the time.
  /*
    CORBA::ULong len = foo.unbounded_data.length ();

    for (CORBA::ULong i = 0; i < len; i ++)
    {
    ACE_DEBUG((LM_DEBUG, "(%P|%t)         unbounded_data[%d]=%f \n",
    i, foo.unbounded_data[i]));
    }
  */

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
