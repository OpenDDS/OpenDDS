// -*- C++ -*-
//
// $Id$

#include "Writer.h"
#include "common.h"
#include "../common/TestSupport.h"


Writer::Writer(const Factory& f, DDS::DomainParticipantFactory_ptr factory, DDS::DomainParticipant_ptr participant, DDS::DataWriterListener_ptr listener) :
        dpf(DDS::DomainParticipantFactory::_duplicate(factory)),
        dp(DDS::DomainParticipant::_duplicate(participant)),
        pub(f.publisher(dp.in())),
        topic(f.topic(dp.in())),
        writer_(f.writer(pub.in(), topic.in(), listener)) { }

Writer::Writer(const Factory& f, DDS::DomainParticipantFactory_ptr factory, DDS::DomainParticipant_ptr participant, DDS::Publisher_ptr publisher, DDS::DataWriterListener_ptr listener) :
        dpf(DDS::DomainParticipantFactory::_duplicate(factory)),
        dp(DDS::DomainParticipant::_duplicate(participant)),
        pub(DDS::Publisher::_duplicate(publisher)),
        topic(f.topic(dp.in())),
        writer_(f.writer(pub.in(), topic.in(), listener)) { }

Writer::~Writer()
{
  // Clean up subscriber objects
  pub->delete_contained_entities();
  dp->delete_publisher(pub.in());
  dp->delete_topic(topic.in());
  dpf->delete_participant(dp.in());
}


//const int default_key = 101010;
//
//
//Worker::Worker(::DDS::Entity_ptr writer)
//: writer_ (::DDS::Entity::_duplicate (writer))
//{
//}
//
//int
//Worker::run_test (const ACE_Time_Value& duration)
//{
//  ACE_DEBUG((LM_DEBUG,
//              ACE_TEXT("(%P|%t) Writer::run_test begins.\n")));
//
//  ACE_Time_Value started = ACE_OS::gettimeofday ();
//  unsigned int pass = 0;
//
//  while(ACE_OS::gettimeofday() < started + duration)
//  {
//    ++pass;
//    try
//    {
//      ::Xyz::Foo foo;
//      //foo.key set below.
//      foo.x = -1;
//      foo.y = -1;
//
//      foo.key = default_key;
//
//      ::Xyz::FooDataWriter_var foo_dw
//        = ::Xyz::FooDataWriter::_narrow(writer_.in ());
//      TEST_ASSERT (! CORBA::is_nil (foo_dw.in ()));
//
//      ACE_DEBUG((LM_DEBUG,
//                ACE_TEXT("(%P|%t) %T Writer::run_test starting to write pass %d\n"),
//                pass));
//
//      ::DDS::InstanceHandle_t handle
//          = foo_dw->register_instance(foo);
//
//      foo.x = 5.0;
//      foo.y = (float)(pass) ;
//
//      foo_dw->write(foo,
//                    handle);
//
//      ACE_DEBUG((LM_DEBUG,
//                ACE_TEXT("(%P|%t) %T Writer::run_test done writing.\n")));
//
//      ACE_OS::sleep(1);
//    }
//    catch (const CORBA::Exception& ex)
//    {
//      ex._tao_print_exception ("Exception caught in run_test:");
//    }
//  }
//  ACE_DEBUG((LM_DEBUG,
//              ACE_TEXT("(%P|%t) Writer::run_test finished.\n")));
//  return 0;
//}
