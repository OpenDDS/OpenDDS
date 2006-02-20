// -*- C++ -*-
//
// $Id$
// -*- C++ -*-
//
#include "Writer.h"
#include "../common/TestException.h"
#include "dds/DCPS/transport/framework/ReceivedDataSample.h"
#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/Serializer.h"
#include "tests/DCPS/FooType4/FooTypeSupportC.h"
#include "tests/DCPS/FooType4/FooTypeSupportImpl.h"

const int default_key = 101010;


Writer::Writer(::DDS::DataReader_ptr reader,
               int num_writes_per_thread, 
               int multiple_instances,
               int instance_id)
: num_writes_per_thread_(num_writes_per_thread),
  multiple_instances_ (multiple_instances),
  instance_id_(instance_id),
  reader_(reader)
{
}

void 
Writer::start ()
{
  ACE_DEBUG((LM_DEBUG,
              ACE_TEXT("(%P|%t) Writer::start \n")));

  ACE_TRY_NEW_ENV
  {
    ::TAO::DCPS::DataReaderImpl* dr_servant =
        reference_to_servant< ::TAO::DCPS::DataReaderImpl,
                              ::DDS::DataReader_ptr>
                           (reader_ ACE_ENV_SINGLE_ARG_PARAMETER);

    ::Xyz::Foo foo;
    foo.x = 0.0 ;
    foo.y = 0.0 ;

    SequenceNumber seq ;

    if (!multiple_instances_) 
    {
      foo.key = default_key;
    }
    
    for (int i = 0; i< num_writes_per_thread_; i ++)
    {
      seq++ ;

      foo.x = (float)i ;
      foo.y = (float)i ;
    
      if (multiple_instances_) 
      {
        foo.key = i + 1 ;
      }
      
      ACE_Time_Value now = ACE_OS::gettimeofday () ;

      ACE_OS::printf ("\"writing\" foo.x = %f foo.y = %f, foo.key = %d\n",
                      foo.x, foo.y, foo.key);
      ReceivedDataSample sample ;

      sample.header_.message_length_ = sizeof(foo) ;
      sample.header_.message_id_ = SAMPLE_DATA ; 
      sample.header_.sequence_ = seq.value_ ;
      sample.header_.publication_id_ = 1 ;
      sample.header_.source_timestamp_sec_ = now.sec() ;
      sample.header_.source_timestamp_nanosec_ = now.usec() * 1000 ;

      sample.sample_ =  new ACE_Message_Block(sizeof(foo)) ;
      
      Serializer ser(sample.sample_) ;
      ser << foo ;

      dr_servant->data_received(sample) ;
    }
  }
  ACE_CATCHANY
  {
    ACE_PRINT_EXCEPTION (ACE_ANY_EXCEPTION,
      "Exception caught in svc:");
  }
  ACE_ENDTRY;
}

