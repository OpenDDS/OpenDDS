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
#include "tests/DCPS/FooType4/FooTypeSupportImpl.h"

static const char * writer_address_str = "127.0.0.1:29876";

Writer::Writer(::DDS::DomainParticipant_ptr dp,
               ::DDS::Topic_ptr topic,
               int history_depth,
               int max_samples_per_instance) 
: dp_(::DDS::DomainParticipant::_duplicate (dp))
{
  // Create the publisher
  pub_ = dp->create_publisher(PUBLISHER_QOS_DEFAULT,
                              ::DDS::PublisherListener::_nil()
                              ACE_ENV_ARG_PARAMETER);
  ACE_TRY_CHECK;
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
  ::TAO::DCPS::PublisherImpl* pub_impl 
      = reference_to_servant< ::TAO::DCPS::PublisherImpl,
                              ::DDS::Publisher_ptr>
                              (pub_ ACE_ENV_SINGLE_ARG_PARAMETER);
  ACE_TRY_CHECK;

  if (0 == pub_impl)
  {
    ACE_ERROR ((LM_ERROR,
                      ACE_TEXT("(%P|%t) Failed to obtain servant ::TAO::DCPS::PublisherImpl\n")));
    throw TestException() ;
  }

  pub_impl->attach_transport(writer_transport_impl.in());

  // Create the datawriter
  ::DDS::DataWriterQos dw_qos;
  pub_->get_default_datawriter_qos (dw_qos);

  dw_qos.history.depth = history_depth  ;
  dw_qos.resource_limits.max_samples_per_instance =
            max_samples_per_instance ;

  dw_ = pub_->create_datawriter(topic,
                                dw_qos,
                                ::DDS::DataWriterListener::_nil()
                                ACE_ENV_ARG_PARAMETER);
  ACE_TRY_CHECK;

  if (CORBA::is_nil (dw_.in ()))
  {
    ACE_ERROR ((LM_ERROR,
               ACE_TEXT("(%P|%t) create_datawriter failed.\n")));
    throw TestException() ;
  }

  ::Mine::FooDataWriter_var foo_dw = ::Mine::FooDataWriter::_narrow(
      dw_ ACE_ENV_ARG_PARAMETER);
  if (CORBA::is_nil (foo_dw.in ()))
  {
    ACE_ERROR ((LM_ERROR,
               ACE_TEXT("(%P|%t) ::Mine::FooDataWriter::_narrow failed.\n")));
    throw TestException() ;
  }

  fast_dw_ = reference_to_servant< ::Mine::FooDataWriterImpl,
                                   ::Mine::FooDataWriter_ptr>
                (foo_dw.in () ACE_ENV_SINGLE_ARG_PARAMETER);

}

void Writer::write (char message_id, const ::Xyz::Foo &foo)
{
  ::DDS::InstanceHandle_t handle (::TAO::DCPS::HANDLE_NIL) ;

  switch(message_id)
  {
    case SAMPLE_DATA:
      ACE_OS::printf ("writing %c foo.x = %f foo.y = %f, foo.key = %d\n",
                      foo.c, foo.x, foo.y, foo.key);

      fast_dw_->write(foo, 
                      handle 
                      ACE_ENV_ARG_PARAMETER);
      break;

    case INSTANCE_REGISTRATION:
      ACE_OS::printf ("registering foo.key = %d\n", foo.key) ;
      handle = fast_dw_->_cxx_register (foo ACE_ENV_ARG_PARAMETER);
      break;

    case DISPOSE_INSTANCE:
      ACE_OS::printf ("disposing foo.key = %d\n", foo.key) ;

      fast_dw_->dispose(foo, 
                        handle 
                        ACE_ENV_ARG_PARAMETER);
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

  ACE_TRY_NEW_ENV
  {
    ::Xyz::Foo foo;

    foo.x = 0.0 ;
    foo.y = 0.0 ;
    foo.key = I1 ;

    // register I1
    write(INSTANCE_REGISTRATION, foo) ;

    foo.x = 1.0 ;
    foo.y = -1.0 ;
    foo.c = 'A';

    // write I1 value A
    write (SAMPLE_DATA, foo) ;

    foo.key = I2 ;

    // register I2
    write (INSTANCE_REGISTRATION, foo) ;
  }
  ACE_CATCHANY
  {
    ACE_PRINT_EXCEPTION (ACE_ANY_EXCEPTION,
      "Exception caught in test1:");
  }
  ACE_ENDTRY;
}


void 
Writer::test2 ()
{
  ACE_DEBUG((LM_DEBUG,
              ACE_TEXT("(%P|%t) Writer::test2 \n")));

  ACE_TRY_NEW_ENV
  {
    ::Xyz::Foo foo;

    foo.x = 2.0 ;
    foo.y = -1.0 ;
    foo.c = 'X';
    foo.key = I2 ;

    // write I2 value X
    write (SAMPLE_DATA, foo) ;

    foo.x = 2.0 ;
    foo.y = -2.0 ;
    foo.c = 'B';
    foo.key = I1 ;

    // write I1 value B
    write (SAMPLE_DATA, foo) ;

    foo.key = I3 ;

    // register I3
    write (INSTANCE_REGISTRATION, foo) ;

    foo.x = 2.0 ;
    foo.y = -3.0 ;
    foo.c = 'Q';

    // write I3 value Q
    write (SAMPLE_DATA, foo) ;
  }
  ACE_CATCHANY
  {
    ACE_PRINT_EXCEPTION (ACE_ANY_EXCEPTION,
      "Exception caught in test2:");
  }
  ACE_ENDTRY;
}


void 
Writer::test3 ()
{
  ACE_DEBUG((LM_DEBUG,
              ACE_TEXT("(%P|%t) Writer::test3 \n")));

  ACE_TRY_NEW_ENV
  {
    ::Xyz::Foo foo;

    foo.x = 3.0 ;
    foo.y = 0.0 ;
    foo.key = I1 ;

    // Dispose I1
    write (DISPOSE_INSTANCE, foo) ;

    foo.x = 3.0 ;
    foo.y = -1.0 ;
    foo.c = 'C' ;

    // write I1 value C
    write (SAMPLE_DATA, foo) ;

    // Dispose I1
    write (DISPOSE_INSTANCE, foo) ;

    foo.x = 3.0 ;
    foo.y = -2.0 ;
    foo.c = 'D' ;

    // write I1 value D
    write (SAMPLE_DATA, foo) ;

    foo.x = 3.0 ;
    foo.y = -4.0 ;
    foo.c = 'Y' ;
    foo.key = I2 ;

    // write I2 value Y
    write (SAMPLE_DATA, foo) ;

    // Dispose I2
    write (DISPOSE_INSTANCE, foo) ;
  }
  ACE_CATCHANY
  {
    ACE_PRINT_EXCEPTION (ACE_ANY_EXCEPTION,
      "Exception caught in test3:");
  }
  ACE_ENDTRY;
}

void 
Writer::test4 ()
{
  ACE_DEBUG((LM_DEBUG,
              ACE_TEXT("(%P|%t) Writer::test4 \n")));

  ACE_TRY_NEW_ENV
  {
    ::Xyz::Foo foo;

    foo.x = 4.0 ;
    foo.y = -1.0 ;
    foo.c = 'c' ;
    foo.key = I1 ;

    // write I1 value c
    write (SAMPLE_DATA, foo) ;
  }
  ACE_CATCHANY
  {
    ACE_PRINT_EXCEPTION (ACE_ANY_EXCEPTION,
      "Exception caught in test4:");
  }
  ACE_ENDTRY;
}

void 
Writer::test5 ()
{
  ACE_DEBUG((LM_DEBUG,
              ACE_TEXT("(%P|%t) Writer::test5 \n")));

  ACE_TRY_NEW_ENV
  {
    ::Xyz::Foo foo;

    foo.x = 5.0 ;
    foo.y = -1.0 ;
    foo.c = 'd' ;
    foo.key = I1 ;

    // write I1 value d
    write (SAMPLE_DATA, foo) ;
  }
  ACE_CATCHANY
  {
    ACE_PRINT_EXCEPTION (ACE_ANY_EXCEPTION,
      "Exception caught in test5:");
  }
  ACE_ENDTRY;
}


void 
Writer::test6 ()
{
  ACE_DEBUG((LM_DEBUG,
              ACE_TEXT("(%P|%t) Writer::test6 \n")));

  ACE_TRY_NEW_ENV
  {
    ::Xyz::Foo foo;

    foo.x = 6.0 ;
    foo.y = -1.0 ;
    foo.c = 'e' ;
    foo.key = I1 ;

    // write I1 value d
    write (SAMPLE_DATA, foo) ;
  }
  ACE_CATCHANY
  {
    ACE_PRINT_EXCEPTION (ACE_ANY_EXCEPTION,
      "Exception caught in test6:");
  }
  ACE_ENDTRY;
}


int Writer::init_transport ()
{
  int status = 0;

  TAO::DCPS::SimpleTcpConfiguration_rch writer_config =
      new TAO::DCPS::SimpleTcpConfiguration();

  writer_transport_impl =
      TheTransportFactory->create(PUB_TRAFFIC,
                                  SIMPLE_TCP);

  ACE_INET_Addr writer_address (writer_address_str);
  writer_config->local_address_ = writer_address;

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
  pub_->delete_datawriter(dw_.in () ACE_ENV_ARG_PARAMETER);
  ACE_TRY_CHECK;
  // Clean up publisher objects
  // pub_->delete_contained_entities() ;

  dp_->delete_publisher(pub_.in () ACE_ENV_ARG_PARAMETER);
  ACE_TRY_CHECK;

  //We have to wait a while to avoid the remove_association from DCPSInfo
  //called after the transport is release.
  ACE_OS::sleep (5);

  TheTransportFactory->release(PUB_TRAFFIC) ;
}
