// -*- C++ -*-
//
// $Id$
// -*- C++ -*-
//
#include "Writer.h"
#include "common.h"
#include "../common/TestException.h"
#include "dds/DCPS/transport/framework/ReceivedDataSample.h"
#include "dds/DCPS/Marked_Default_Qos.h"
#include "dds/DCPS/Qos_Helper.h"
#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/DataWriterImpl.h"
#include "dds/DCPS/PublisherImpl.h"
#include "tests/DCPS/FooType4/FooDefTypeSupportImpl.h"

static const char * writer_address_str = "localhost:29876";

Writer::Writer(::DDS::DomainParticipant_ptr dp,
               ::DDS::Topic_ptr topic,
               int history_depth,
               int max_samples_per_instance)
: dp_(::DDS::DomainParticipant::_duplicate (dp))
{
  // Create the publisher
  pub_ = dp->create_publisher(PUBLISHER_QOS_DEFAULT,
                              ::DDS::PublisherListener::_nil());
  if (CORBA::is_nil (pub_.in ()))
  {
    ACE_ERROR ((LM_ERROR,
                ACE_TEXT("(%P|%t) create_publisher failed.\n")));
    throw TestException() ;
  }

  // Initialize the transport
  if (0 != init_transport() )
  {
    ACE_ERROR ((LM_ERROR,
                ACE_TEXT("(%P|%t) init_transport failed!\n")));
    throw TestException() ;
  }

  // Attach the publisher to the transport.
  OpenDDS::DCPS::PublisherImpl* pub_impl
    = dynamic_cast<OpenDDS::DCPS::PublisherImpl*> (pub_.in ());

  if (0 == pub_impl)
  {
    ACE_ERROR ((LM_ERROR,
                      ACE_TEXT("(%P|%t) Failed to obtain servant ::OpenDDS::DCPS::PublisherImpl\n")));
    throw TestException() ;
  }

  pub_impl->attach_transport(writer_transport_impl.in());

  // Create the datawriter
  ::DDS::DataWriterQos dw_qos;
  pub_->get_default_datawriter_qos (dw_qos);

  dw_qos.history.depth = history_depth  ;
  dw_qos.resource_limits.max_samples_per_instance =
            max_samples_per_instance ;
  dw_qos.liveliness.lease_duration.sec =
	  static_cast<CORBA::Long> (max_blocking_time.sec ());
  dw_qos.liveliness.lease_duration.nanosec = 0 ;

  dw_ = pub_->create_datawriter(topic,
                                dw_qos,
                                ::DDS::DataWriterListener::_nil());

  if (CORBA::is_nil (dw_.in ()))
  {
    ACE_ERROR ((LM_ERROR,
               ACE_TEXT("(%P|%t) create_datawriter failed.\n")));
    throw TestException() ;
  }

  ::Xyz::FooDataWriter_var foo_dw = ::Xyz::FooDataWriter::_narrow(
      dw_.in ());
  if (CORBA::is_nil (foo_dw.in ()))
  {
    ACE_ERROR ((LM_ERROR,
               ACE_TEXT("(%P|%t) ::Xyz::FooDataWriter::_narrow failed.\n")));
    throw TestException() ;
  }

  fast_dw_ = 
    dynamic_cast< ::Xyz::FooDataWriterImpl*> (foo_dw.in ());

}

void Writer::write (char message_id, const ::Xyz::Foo &foo)
{
  ::DDS::InstanceHandle_t handle (::OpenDDS::DCPS::HANDLE_NIL) ;

  switch(message_id)
  {
    case ::OpenDDS::DCPS::SAMPLE_DATA:
      ACE_OS::printf ("writing %c foo.x = %f foo.y = %f, foo.key = %d\n",
                      foo.c, foo.x, foo.y, foo.key);

      fast_dw_->write(foo,
                      handle);
      break;

    case ::OpenDDS::DCPS::INSTANCE_REGISTRATION:
      ACE_OS::printf ("registering foo.key = %d\n", foo.key) ;
      handle = fast_dw_->_cxx_register (foo);
      break;

    case ::OpenDDS::DCPS::DISPOSE_INSTANCE:
      ACE_OS::printf ("disposing foo.key = %d\n", foo.key) ;

      fast_dw_->dispose(foo,
                        handle);
      break;

    default:
      ACE_OS::printf ("Oops! message_id = %c\n", message_id) ;
      break;
  }
}


void
Writer::test1 ()
{
  ACE_DEBUG((LM_DEBUG,
              ACE_TEXT("(%P|%t) Writer::test1 \n")));

  try
  {
    ::Xyz::Foo foo;

    foo.x = 0.0 ;
    foo.y = 0.0 ;
    foo.key = I1 ;

    // register I1
    write(::OpenDDS::DCPS::INSTANCE_REGISTRATION, foo) ;

    foo.x = 1.0 ;
    foo.y = -1.0 ;
    foo.c = 'A';

    // write I1 value A
    write (::OpenDDS::DCPS::SAMPLE_DATA, foo) ;

    foo.key = I2 ;

    // register I2
    write (::OpenDDS::DCPS::INSTANCE_REGISTRATION, foo) ;
  }
  catch (const CORBA::Exception& ex)
  {
    ex._tao_print_exception ("Exception caught in test1:");
  }
}


void
Writer::test2 ()
{
  ACE_DEBUG((LM_DEBUG,
              ACE_TEXT("(%P|%t) Writer::test2 \n")));

  try
  {
    ::Xyz::Foo foo;

    foo.x = 2.0 ;
    foo.y = -1.0 ;
    foo.c = 'X';
    foo.key = I2 ;

    // write I2 value X
    write (::OpenDDS::DCPS::SAMPLE_DATA, foo) ;

    foo.x = 2.0 ;
    foo.y = -2.0 ;
    foo.c = 'B';
    foo.key = I1 ;

    // write I1 value B
    write (::OpenDDS::DCPS::SAMPLE_DATA, foo) ;

    foo.key = I3 ;

    // register I3
    write (::OpenDDS::DCPS::INSTANCE_REGISTRATION, foo) ;

    foo.x = 2.0 ;
    foo.y = -3.0 ;
    foo.c = 'Q';

    // write I3 value Q
    write (::OpenDDS::DCPS::SAMPLE_DATA, foo) ;
  }
  catch (const CORBA::Exception& ex)
  {
    ex._tao_print_exception ("Exception caught in test2:");
  }
}


void
Writer::test3 ()
{
  ACE_DEBUG((LM_DEBUG,
              ACE_TEXT("(%P|%t) Writer::test3 \n")));

  try
  {
    ::Xyz::Foo foo;

    foo.x = 3.0 ;
    foo.y = 0.0 ;
    foo.key = I1 ;

    // Dispose I1
    write (::OpenDDS::DCPS::DISPOSE_INSTANCE, foo) ;

    foo.x = 3.0 ;
    foo.y = -1.0 ;
    foo.c = 'C' ;

    // write I1 value C
    write (::OpenDDS::DCPS::SAMPLE_DATA, foo) ;

    // Dispose I1
    write (::OpenDDS::DCPS::DISPOSE_INSTANCE, foo) ;

    foo.x = 3.0 ;
    foo.y = -2.0 ;
    foo.c = 'D' ;

    // write I1 value D
    write (::OpenDDS::DCPS::SAMPLE_DATA, foo) ;

    foo.x = 3.0 ;
    foo.y = -4.0 ;
    foo.c = 'Y' ;
    foo.key = I2 ;

    // write I2 value Y
    write (::OpenDDS::DCPS::SAMPLE_DATA, foo) ;

    // Dispose I2
    write (::OpenDDS::DCPS::DISPOSE_INSTANCE, foo) ;
  }
  catch (const CORBA::Exception& ex)
  {
    ex._tao_print_exception ("Exception caught in test3:");
  }
}

void
Writer::test4 ()
{
  ACE_DEBUG((LM_DEBUG,
              ACE_TEXT("(%P|%t) Writer::test4 \n")));

  try
  {
    ::Xyz::Foo foo;

    foo.x = 4.0 ;
    foo.y = -1.0 ;
    foo.c = 'c' ;
    foo.key = I1 ;

    // write I1 value c
    write (::OpenDDS::DCPS::SAMPLE_DATA, foo) ;
  }
  catch (const CORBA::Exception& ex)
  {
    ex._tao_print_exception ("Exception caught in test4:");
  }
}

void
Writer::test5 ()
{
  ACE_DEBUG((LM_DEBUG,
              ACE_TEXT("(%P|%t) Writer::test5 \n")));

  try
  {
    ::Xyz::Foo foo;

    foo.x = 5.0 ;
    foo.y = -1.0 ;
    foo.c = 'd' ;
    foo.key = I1 ;

    // write I1 value d
    write (::OpenDDS::DCPS::SAMPLE_DATA, foo) ;
  }
  catch (const CORBA::Exception& ex)
  {
    ex._tao_print_exception ("Exception caught in test5:");
  }
}


void
Writer::test6 ()
{
  ACE_DEBUG((LM_DEBUG,
              ACE_TEXT("(%P|%t) Writer::test6 \n")));

  try
  {
    ::Xyz::Foo foo;

    foo.x = 6.0 ;
    foo.y = -1.0 ;
    foo.c = 'e' ;
    foo.key = I1 ;

    // write I1 value d
    write (::OpenDDS::DCPS::SAMPLE_DATA, foo) ;
  }
  catch (const CORBA::Exception& ex)
  {
    ex._tao_print_exception ("Exception caught in test6:");
  }
}


int Writer::init_transport ()
{
  int status = 0;

  writer_transport_impl
    = TheTransportFactory->create_transport_impl (PUB_TRAFFIC,
                                                  "SimpleTcp",
                                                  OpenDDS::DCPS::DONT_AUTO_CONFIG);

  OpenDDS::DCPS::TransportConfiguration_rch writer_config
    = TheTransportFactory->create_configuration (PUB_TRAFFIC, "SimpleTcp");

  OpenDDS::DCPS::SimpleTcpConfiguration* writer_tcp_config
    = static_cast <OpenDDS::DCPS::SimpleTcpConfiguration*> (writer_config.in ());

  ACE_INET_Addr writer_address (writer_address_str);
  writer_tcp_config->local_address_ = writer_address;
  writer_tcp_config->local_address_str_ = writer_address_str;

  if (writer_transport_impl->configure(writer_config.in()) != 0)
    {
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("(%P|%t) ::init_writer_tranport: ")
                 ACE_TEXT("Failed to configure the transport.\n")));
      status = 1;
    }

  return status;
}

Writer::~Writer()
{
  pub_->delete_datawriter(dw_.in ());
  // Clean up publisher objects
  // pub_->delete_contained_entities() ;

  dp_->delete_publisher(pub_.in ());

  //We have to wait a while to avoid the remove_association from DCPSInfo
  //called after the transport is release.
  ACE_OS::sleep (2);

  TheTransportFactory->release(PUB_TRAFFIC) ;
}
