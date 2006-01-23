// -*- C++ -*-
//
// $Id$

#include "Reader.h"
#include "common.h"
#include "../common/SampleInfo.h"
#include "../common/TestException.h"
#include "DataReaderListener.h"
#include "dds/DCPS/transport/framework/ReceivedDataSample.h"
#include "dds/DCPS/Marked_Default_Qos.h"
#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/Serializer.h"
#include "tests/DCPS/FooType4/FooTypeSupportC.h"
#include "tests/DCPS/FooType4/FooTypeSupportImpl.h"

static const char * reader_address_str = "127.0.0.1:16789";

Reader::Reader(::DDS::DomainParticipant_ptr dp, 
               int history_depth,
               int max_samples_per_instance)
    : max_samples_per_instance_(max_samples_per_instance), 
    dp_(::DDS::DomainParticipant::_duplicate (dp))
{
  sub_ = dp->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
                             ::DDS::SubscriberListener::_nil()
                             ACE_ENV_ARG_PARAMETER);
  ACE_TRY_CHECK;
  if (CORBA::is_nil (sub_.in ()))
  {
    ACE_ERROR ((LM_ERROR,
               ACE_TEXT("(%P|%t) create_subscriber failed.\n")));
    throw TestException() ;
  }

  // Initialize the transport
  if (0 != init_transport() )
  {
    ACE_ERROR ((LM_ERROR,
                 ACE_TEXT("(%P|%t) init_transport failed!\n")));
    throw TestException() ;
  }

  // Attach the subscriber to the transport.
  ::TAO::DCPS::SubscriberImpl* sub_impl 
          = reference_to_servant< ::TAO::DCPS::SubscriberImpl,
                                  ::DDS::Subscriber_ptr>
                          (sub_.in () ACE_ENV_SINGLE_ARG_PARAMETER);
  ACE_TRY_CHECK;

  if (0 == sub_impl)
  {
    ACE_ERROR ((LM_ERROR,
               ACE_TEXT("(%P|%t) Failed to obtain servant ::TAO::DCPS::SubscriberImpl\n")));
    throw TestException() ;
  }
  sub_impl->attach_transport(reader_transport_impl.in());

  ::DDS::TopicDescription_var description =
      dp->lookup_topicdescription(MY_TOPIC ACE_ENV_ARG_PARAMETER);
  ACE_TRY_CHECK;
  if (CORBA::is_nil (description.in ()))
  {
    ACE_ERROR ((LM_ERROR,
               ACE_TEXT("(%P|%t) lookup_topicdescription failed.\n")));
    throw TestException() ;
  }

  ::DDS::DataReaderQos dr_qos;
  sub_->get_default_datareader_qos (dr_qos);

  dr_qos.history.depth = history_depth  ;
  dr_qos.resource_limits.max_samples_per_instance =
            max_samples_per_instance ;

  dr_qos.liveliness.lease_duration.sec = max_blocking_time.sec() ;
  dr_qos.liveliness.lease_duration.nanosec = 0 ;

  PortableServer::POA_var poa = TheServiceParticipant->the_poa ();

  CORBA::Object_var obj = poa->servant_to_reference(&drl_servant_
      ACE_ENV_ARG_PARAMETER);
  ACE_CHECK;

  ::DDS::DataReaderListener_var drl
      = ::DDS::DataReaderListener::_narrow (obj.in () ACE_ENV_ARG_PARAMETER);
  ACE_CHECK;

  ::DDS::DataReader_var dr = sub_->create_datareader(description.in (),
                                dr_qos,
//                                ::DDS::DataReaderListener::_nil()
                                drl.in ()
                                ACE_ENV_ARG_PARAMETER);
  ACE_TRY_CHECK;

  if (CORBA::is_nil (dr.in ()))
  {
    ACE_ERROR ((LM_ERROR,
               ACE_TEXT("(%P|%t) create_datareader failed.\n")));
    throw TestException() ;
  }
}


void 
Reader::read (const SampleInfoMap& si_map,
              ::DDS::SampleStateMask ss,
              ::DDS::ViewStateMask vs,
              ::DDS::InstanceStateMask is)
{
  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) Reader::read \n")));
      
  const int factor = 20;
  const int timeout_sec = 10 ;
  ACE_Time_Value small(0,1000000/factor);
  int timeout_loops = timeout_sec * factor;

  ACE_TRY_NEW_ENV
  {
    ::DDS::DataReaderSeq_var readers = new ::DDS::DataReaderSeq(10);
    bool found(false) ;
    while (timeout_loops-- > 0) // Danger! fix later
    {
      sub_->get_datareaders (
                    readers.out (),
                    ss,
                    vs,
                    is );
      if (readers->length () > 0)
      {
        found = true ;
        break ;
      }
      ACE_OS::sleep (small);
    }

    if (!found)
    {
      ACE_ERROR ((LM_ERROR,
                  ACE_TEXT("(%P|%t) ERROR: timeout waiting for data.\n")));
      throw TestException() ;
    }
    else
    {
      ACE_DEBUG ((LM_DEBUG,
                  ACE_TEXT("(%P|%t) get_datareaders returned %d reader(s)\n"),
                  readers->length ()));
    }

    ::Mine::FooSeq foo(max_samples_per_instance_) ;
    ::DDS::SampleInfoSeq si(max_samples_per_instance_) ;

    for (CORBA::ULong i = 0 ; i < readers->length() ; i++)
    {
      ::Mine::FooDataReader_var foo_dr
          = ::Mine::FooDataReader::_narrow(readers[i]
                                           ACE_ENV_ARG_PARAMETER);
      
      if (CORBA::is_nil (foo_dr.in ()))
      {
        ACE_ERROR ((LM_ERROR,
               ACE_TEXT("(%P|%t) ::Mine::FooDataReader::_narrow failed.\n")));
        throw TestException() ;
      }

      ::Mine::FooDataReaderImpl* dr_servant =
          reference_to_servant< ::Mine::FooDataReaderImpl,
                                ::Mine::FooDataReader_ptr>
              (foo_dr.in () ACE_ENV_SINGLE_ARG_PARAMETER);

      DDS::ReturnCode_t status  ;
      status = dr_servant->read(foo, si,
                  max_samples_per_instance_,
                  ss,
                  vs,
                  is) ;

      if (status == ::DDS::RETCODE_OK)
      {
        for (CORBA::ULong i = 0 ; i < si.length() ; i++)
        {
          ACE_OS::printf ("foo[%d] - %c : x = %f y = %f, key = %d\n",
                          i, foo[i].c, foo[i].x, foo[i].y, foo[i].key);
          PrintSampleInfo(si[i]) ;

          SampleInfoMap::const_iterator it = si_map.find(foo[i].c);
          if (it == si_map.end())
          {
            ACE_OS::printf ("read - Error: %c not returned\n", foo[i].c) ;
            throw TestException() ;
          }
          else
          {
            if (si[i] != it->second)
            {
              ACE_OS::printf ("read - Error: %c SampleInfo != expected\n",
                              foo[i].c) ;
              throw TestException() ;
            }
          }
        }
      }
      else if (status == ::DDS::RETCODE_NO_DATA)
      {
        ACE_OS::printf ("read returned ::DDS::RETCODE_NO_DATA\n") ;
        throw TestException() ;
      }
      else
      {
        ACE_OS::printf ("read - Error: %d\n", status) ;
        throw TestException() ;
      }
    }
  }
  ACE_CATCHANY
  {
    ACE_PRINT_EXCEPTION (ACE_ANY_EXCEPTION,
      "Exception caught in read:");
    throw TestException() ;
  }
  ACE_ENDTRY;
}


int Reader::init_transport ()
{
  int status = 0;

  reader_transport_impl 
    = TheTransportFactory->create_transport_impl (SUB_TRAFFIC, "SimpleTcp", TAO::DCPS::DONT_AUTO_CONFIG);

  TAO::DCPS::TransportConfiguration_rch reader_config 
    = TheTransportFactory->create_configuration (SUB_TRAFFIC, "SimpleTcp");

  TAO::DCPS::SimpleTcpConfiguration* reader_tcp_config 
    = static_cast <TAO::DCPS::SimpleTcpConfiguration*> (reader_config.in ());

  ACE_INET_Addr reader_address (reader_address_str);
  reader_tcp_config->local_address_ = reader_address;

  if (reader_transport_impl->configure(reader_config.in()) != 0)
    {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ::init_reader_tranport: ")
                 ACE_TEXT("Failed to configure the transport.\n")));
      status = 1;
    }

  return status;
}


Reader::~Reader()
{
  PortableServer::POA_var poa = TheServiceParticipant->the_poa ();
  PortableServer::ObjectId_var id =
    poa->servant_to_id (&drl_servant_ ACE_ENV_ARG_PARAMETER);
  ACE_TRY_CHECK;
  poa->deactivate_object (id.in () ACE_ENV_ARG_PARAMETER);
  ACE_TRY_CHECK;

  sub_->delete_contained_entities() ;
  dp_->delete_subscriber(sub_.in () ACE_ENV_ARG_PARAMETER);
  ACE_TRY_CHECK;

  //We have to wait a while to avoid the remove_association from DCPSInfo
  //called after the transport is release.
  ACE_OS::sleep (5);

  TheTransportFactory->release(SUB_TRAFFIC) ;
}
