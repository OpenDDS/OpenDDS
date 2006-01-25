// -*- C++ -*-
//
// $Id$

#include "common.h"
#include "tests/DCPS/common/TestSupport.h"
// Add the TransportImpl.h before TransportImpl_rch.h is included to  
// resolve the build problem that the class is not defined when 
// RcHandle<T> template is instantiated.
#include "dds/DCPS/transport/framework/TransportImpl.h"
#include "dds/DCPS/transport/simpleTCP/SimpleTcpConfiguration.h"
#include "dds/DCPS/transport/framework/TheTransportFactory.h"
#include "dds/DCPS/BuiltInTopicUtils.h"
#include "tests/DCPS/FooType4/FooTypeSupportImpl.h"
#include <string>

const long  TEST_DOMAIN   = 911;
const char* TEST_TOPIC    = "foo";
const char* TEST_TOPIC_TYPE     = "foo";
const char * reader_address_str = "";
const char * writer_address_str = "";
int default_key = 101010;
int num_writes = 1;

Subscriber_var bit_subscriber;
DomainParticipant_var participant;
Topic_var topic;
Publisher_var publisher;
DataWriter_var datawriter;
Subscriber_var subscriber;
DataReader_var datareader;
DomainParticipantFactory_var participant_factory;

DomainParticipantImpl* participant_servant = 0;
TopicImpl* topic_servant = 0;
PublisherImpl* publisher_servant = 0;
DataWriterImpl* datawriter_servant = 0;
SubscriberImpl* subscriber_servant = 0;
DataReaderImpl* datareader_servant = 0;


int ignore_kind = DONT_IGNORE;

TAO::DCPS::TransportImpl_rch reader_transport_impl;
TAO::DCPS::TransportImpl_rch writer_transport_impl;

int init_transport ()
{
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
    return -1;
  }

  writer_transport_impl
    = TheTransportFactory->create_transport_impl (PUB_TRAFFIC, "SimpleTcp", TAO::DCPS::DONT_AUTO_CONFIG);

  TAO::DCPS::TransportConfiguration_rch writer_config 
    = TheTransportFactory->create_configuration (PUB_TRAFFIC, "SimpleTcp");

  TAO::DCPS::SimpleTcpConfiguration* writer_tcp_config 
    = static_cast <TAO::DCPS::SimpleTcpConfiguration*> (writer_config.in ());

  ACE_INET_Addr writer_address (writer_address_str);
  writer_tcp_config->local_address_ = writer_address;

  if (writer_transport_impl->configure(writer_config.in()) != 0)
  {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("(%P|%t) ::init_writer_tranport: ")
      ACE_TEXT("Failed to configure the transport.\n")));
    return -1;
  }
  return 0;
}


int attach_publisher_transport ()
{
  TAO::DCPS::AttachStatus attach_status 
    = publisher_servant->attach_transport(writer_transport_impl.in());

  if (attach_status != TAO::DCPS::ATTACH_OK)
    {
      // We failed to attach to the transport for some reason.
      std::string status_str;

      switch (attach_status)
        {
          case TAO::DCPS::ATTACH_BAD_TRANSPORT:
            status_str = "ATTACH_BAD_TRANSPORT";
            break;
          case TAO::DCPS::ATTACH_ERROR:
            status_str = "ATTACH_ERROR";
            break;
          case TAO::DCPS::ATTACH_INCOMPATIBLE_QOS:
            status_str = "ATTACH_INCOMPATIBLE_QOS";
            break;
          default:
            status_str = "Unknown Status";
            break;
        }

      ACE_ERROR_RETURN ((LM_ERROR,
                        ACE_TEXT("(%P|%t) Failed to attach publisher with the transport. ")
                        ACE_TEXT("AttachStatus == %s\n"),
                        status_str.c_str()),
                        -1);
    }
  
  return 0;
}


int attach_subscriber_transport ()
{
  TAO::DCPS::AttachStatus attach_status 
    = subscriber_servant->attach_transport(reader_transport_impl.in());
  if (attach_status != TAO::DCPS::ATTACH_OK)
    {
      // We failed to attach to the transport for some reason.
      std::string status_str;

      switch (attach_status)
        {
          case TAO::DCPS::ATTACH_BAD_TRANSPORT:
            status_str = "ATTACH_BAD_TRANSPORT";
            break;
          case TAO::DCPS::ATTACH_ERROR:
            status_str = "ATTACH_ERROR";
            break;
          case TAO::DCPS::ATTACH_INCOMPATIBLE_QOS:
            status_str = "ATTACH_INCOMPATIBLE_QOS";
            break;
          default:
            status_str = "Unknown Status";
            break;
        }

      ACE_ERROR_RETURN ((LM_ERROR,
                        ACE_TEXT("(%P|%t) Failed to attach subscriber with the transport. ")
                        ACE_TEXT("AttachStatus == %s\n"),
                        status_str.c_str()),
                        -1);
    }

  return 0;
}


int ignore ()
{
#if !defined (DDS_HAS_MINIMUM_BIT)
  switch (ignore_kind)
  {
  case IGNORE_PARTICIPANT:
    {
      // Normally a client application would use some Qos like
      // the USER_DATA to find the Built-In Topic InstanceHandle_t
      // value for an entity (e.g. a DomainParticipant) but this
      // test knows everything and can use the DCPSInfo RepoID.
      ::TAO::DCPS::RepoId part_id = participant_servant->get_id ();
      //SHH one of these should be the subscriber participant and the other should be the publisher participant.
      ::TAO::DCPS::RepoId ignore_id = participant_servant->get_id ();

      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) IGNORE_PARTICIPANT,  participant %d ignore participant %d .\n"),
                 part_id, ignore_id));

      ::DDS::InstanceHandleSeq handles;
      ::TAO::DCPS::ReaderIdSeq ignore_ids;
      ignore_ids.length (1);
      ignore_ids[0] = ignore_id;

      ::TAO::DCPS::BIT_Helper_2 < ::DDS::ParticipantBuiltinTopicDataDataReader,
                    ::DDS::ParticipantBuiltinTopicDataDataReader_var,
                    ::DDS::ParticipantBuiltinTopicDataSeq,
                    ::TAO::DCPS::ReaderIdSeq > hh;
       
      ::DDS::ReturnCode_t ret 
        = hh.repo_ids_to_instance_handles(participant_servant, 
          ::TAO::DCPS::BUILT_IN_PARTICIPANT_TOPIC, 
          ignore_ids, 
          handles);

      if (ret != ::DDS::RETCODE_OK)
      {
        ACE_ERROR ((LM_ERROR,
          ACE_TEXT("(%P|%t) IGNORE_PARTICIPANT, ")
          ACE_TEXT(" repo_ids_to_instance_handles returned error %d\n"),
          ret));
        return -1;
      }

      ret = participant->ignore_participant (handles[0] ACE_ENV_ARG_PARAMETER);
      ACE_CHECK;

      if (ret != ::DDS::RETCODE_OK)
      {
        ACE_ERROR ((LM_ERROR,
                    ACE_TEXT("(%P|%t) IGNORE_PARTICIPANT, ")
                    ACE_TEXT(" participant %d ignore participant %d returned error %d\n"), 
                    part_id, ignore_id, ret));
        return -1;
      }
    }
    break;

  case IGNORE_TOPIC:
    {
      ::TAO::DCPS::RepoId part_id = participant_servant->get_id ();
      ::TAO::DCPS::RepoId ignore_id = topic_servant->get_id ();

      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) IGNORE_TOPIC, participant %d ignore topic %d .\n"),
                 part_id, ignore_id));

      ::DDS::InstanceHandleSeq handles;
      ::TAO::DCPS::ReaderIdSeq ignore_ids;
      ignore_ids.length (1);
      ignore_ids[0] = ignore_id;

      ::TAO::DCPS::BIT_Helper_2 < ::DDS::TopicBuiltinTopicDataDataReader,
                    ::DDS::TopicBuiltinTopicDataDataReader_var,
                    ::DDS::TopicBuiltinTopicDataSeq,
                    ::TAO::DCPS::ReaderIdSeq > hh;
       
      ::DDS::ReturnCode_t ret 
        = hh.repo_ids_to_instance_handles(participant_servant, 
          ::TAO::DCPS::BUILT_IN_TOPIC_TOPIC, 
          ignore_ids, 
          handles);

      if (ret != ::DDS::RETCODE_OK)
      {
        ACE_ERROR ((LM_ERROR,
          ACE_TEXT("(%P|%t) IGNORE_TOPIC, ")
          ACE_TEXT(" repo_ids_to_instance_handles returned error %d.\n"),
          ret));
        return -1;
      }

      ret = participant->ignore_topic (handles[0] ACE_ENV_ARG_PARAMETER);
      ACE_CHECK;

      if (ret != ::DDS::RETCODE_OK)
      {
        ACE_ERROR ((LM_ERROR,
                    ACE_TEXT("(%P|%t) IGNORE_TOPIC, ")
                    ACE_TEXT(" ignore_topic %d return error %d\n"), 
                    handles[0], ret));
        return -1;
      }
    }
    break;
  case IGNORE_PUBLICATION:
    {
      ::TAO::DCPS::RepoId part_id = participant_servant->get_id ();
      ::TAO::DCPS::RepoId ignore_id = datawriter_servant->get_publication_id ();

      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) IGNORE_PUBLICATION, participant %d ignore publication %d .\n"),
                 part_id, ignore_id));

      ::DDS::InstanceHandleSeq handles;
      ::TAO::DCPS::ReaderIdSeq ignore_ids;
      ignore_ids.length (1);
      ignore_ids[0] = ignore_id;

      ::TAO::DCPS::BIT_Helper_2 < 
                    ::DDS::PublicationBuiltinTopicDataDataReader,
                    ::DDS::PublicationBuiltinTopicDataDataReader_var,
                    ::DDS::PublicationBuiltinTopicDataSeq,
                    ::TAO::DCPS::ReaderIdSeq > hh;
       
      ::DDS::ReturnCode_t ret 
        = hh.repo_ids_to_instance_handles(participant_servant, 
          ::TAO::DCPS::BUILT_IN_PUBLICATION_TOPIC, 
          ignore_ids, 
          handles);

      if (ret != ::DDS::RETCODE_OK)
      {
        ACE_ERROR ((LM_ERROR,
          ACE_TEXT("(%P|%t) IGNORE_PUBLICATION, ")
          ACE_TEXT(" repo_ids_to_instance_handles returned error %d\n"),
          ret));
        return -1;
      }

      ret = participant->ignore_publication (handles[0] ACE_ENV_ARG_PARAMETER);
      ACE_CHECK;

      if (ret != ::DDS::RETCODE_OK)
      {
        ACE_ERROR ((LM_ERROR,
                    ACE_TEXT("(%P|%t) IGNORE_PUBLICATION, ")
                    ACE_TEXT(" ignore_publication %d return error %d\n"),
                    handles[0], ret));
        return -1;
      }
    }
    break;
  case IGNORE_SUBSCRIPTION:
    {
      ::TAO::DCPS::RepoId part_id = participant_servant->get_id ();
      ::TAO::DCPS::RepoId ignore_id = datareader_servant->get_subscription_id ();

      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) IGNORE_SUBSCRIPTION, participant %d ignore subscription %d .\n"),
                 part_id, ignore_id));

      ::DDS::InstanceHandleSeq handles;
      ::TAO::DCPS::ReaderIdSeq ignore_ids;
      ignore_ids.length (1);
      ignore_ids[0] = ignore_id;

      ::TAO::DCPS::BIT_Helper_2 < ::DDS::SubscriptionBuiltinTopicDataDataReader,
                    ::DDS::SubscriptionBuiltinTopicDataDataReader_var,
                    ::DDS::SubscriptionBuiltinTopicDataSeq,
                    ::TAO::DCPS::ReaderIdSeq > hh;
       
      ::DDS::ReturnCode_t ret 
        = hh.repo_ids_to_instance_handles(participant_servant, 
          ::TAO::DCPS::BUILT_IN_SUBSCRIPTION_TOPIC, 
          ignore_ids, 
          handles);

      if (ret != ::DDS::RETCODE_OK)
      {
        ACE_ERROR ((LM_ERROR,
          ACE_TEXT("(%P|%t) IGNORE_SUBSCRIPTION, ")
          ACE_TEXT(" repo_ids_to_instance_handles returned error %d\n"),
          ret));
        return -1;
      }

      ret = participant->ignore_subscription (handles[0] ACE_ENV_ARG_PARAMETER);
      ACE_CHECK;

      if (ret != ::DDS::RETCODE_OK)
      {
        ACE_ERROR ((LM_ERROR,
                    ACE_TEXT("(%P|%t) IGNORE_SUBSCRIPTION, ")
                    ACE_TEXT(" ignore_subscription %d returned error 5d\n"), 
                    handles[0], ret));
        return -1;
      }
    }
    break;
  default:
    ACE_ERROR ((LM_ERROR,
      ACE_TEXT("(%P|%t) ignore, ")
      ACE_TEXT(" unknown ignore kind %d\n"), ignore_kind));
    return -1;
    break;
  }
  
  return 0;
#else

  return -1;
#endif // !defined (DDS_HAS_MINIMUM_BIT)
}

int write ()
{
  ACE_DEBUG((LM_DEBUG,
              ACE_TEXT("(%P|%t) write begins.\n")));

  ACE_TRY_NEW_ENV
  {
    ::Xyz::Foo foo;
    foo.x = -1;
    foo.y = -1;
    foo.key = default_key;
    
    ::Mine::FooDataWriter_var foo_dw 
      = ::Mine::FooDataWriter::_narrow(datawriter.in () ACE_ENV_ARG_PARAMETER);
    TEST_CHECK (! CORBA::is_nil (foo_dw.in ()));

    ::DDS::InstanceHandle_t handle 
        = foo_dw->_cxx_register (foo ACE_ENV_ARG_PARAMETER);

    for (int i = 0; i< num_writes; i ++)
    {

      foo.x = (float)i;

      foo_dw->write(foo, 
                    handle 
                    ACE_ENV_ARG_PARAMETER);
      ACE_TRY_CHECK;
    }
   
    ACE_DEBUG((LM_DEBUG,
             ACE_TEXT("(%P|%t) write  done\n")));
  }
  ACE_CATCHALL
  {
    ACE_ERROR_RETURN ((LM_ERROR, "(%P|%t) Exception caught in write."), -1);
  }
  ACE_ENDTRY;

  return 0;
}

int read (int expect_success)
{
  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) read begins. \n")));
  
  ACE_TRY_NEW_ENV
  {
    ::Mine::FooDataReader_var foo_dr 
      = ::Mine::FooDataReader::_narrow(datareader.in () ACE_ENV_ARG_PARAMETER);
    if (CORBA::is_nil (foo_dr.in ()))
    {
      ACE_ERROR_RETURN ((LM_ERROR,
                 ACE_TEXT("(%P|%t) ::Mine::FooDataReader::_narrow failed.\n")),
                 -1);
    }

    ::Mine::FooDataReaderImpl* dr_servant =
        reference_to_servant< ::Mine::FooDataReaderImpl,
                             ::Mine::FooDataReader_ptr>
                (foo_dr.in () ACE_ENV_SINGLE_ARG_PARAMETER);

    int num_reads = 0;
    int num_received = 0;

    bool init_val = false;
    ACE_Array<bool> messages(num_writes, init_val);

    while ( num_reads < num_writes)
    {

      ::Xyz::Foo foo;
      ::DDS::SampleInfo si ;

      DDS::ReturnCode_t status = dr_servant->read_next_sample(foo, si) ;
      num_reads ++;

      if (status == ::DDS::RETCODE_OK)
      {
        if (!expect_success) 
        {
          ACE_ERROR_RETURN ((LM_ERROR, 
                             ACE_TEXT("(%P|%t) ERROR: received a message but did not expect too!\n"))
                             , -1);
          
        }
        num_received++;

        if (default_key != foo.key)
        {
          ACE_ERROR ((LM_ERROR, 
            ACE_TEXT("(%P|%t) ERROR: reader received incorrect key!\n")
                      ));
        }
        int msg_num = (int) foo.x;
        if ((0 <= msg_num) && (msg_num< num_writes))
        {
          messages[msg_num] = true;
        }
        else
        {
          ACE_ERROR ((LM_ERROR, 
                      ACE_TEXT("(%P|%t) ERROR: reader received unexpected message number %d!\n"),
                      msg_num));
        }

        ACE_OS::fprintf(stderr, "foo.x = %f foo.y = %f, foo.key = %d\n",
                        foo.x, foo.y, foo.key);
      }
      else if (status == ::DDS::RETCODE_NO_DATA)
      {
        if (expect_success) 
        {
          ACE_ERROR ((LM_ERROR, 
            ACE_TEXT("(%P|%t) ERROR: read returned ::DDS::RETCODE_NO_DATA!\n")));
          ACE_OS::sleep(1);
        }
      }
      else
      {
        ACE_ERROR ((LM_ERROR, 
          ACE_TEXT("(%P|%t) ERROR: read returned error %d\n"), status));
      }
    }

    int num_expected = num_writes;
    if (ignore_kind != DONT_IGNORE)
    {
       num_expected = 0;
    }

    if (num_received == num_expected)
    {
      for (int k = 0; k < num_expected; k ++)
      {
        if ( !messages[k] )
        {
          ACE_ERROR_RETURN ((LM_ERROR, 
                             ACE_TEXT("(%P|%t) ERROR: reader did not receive message %d!\n"),
                             k), -1);
        }
      }
    }
    else
    {
       ACE_ERROR_RETURN ((LM_ERROR, 
                          ACE_TEXT("(%P|%t) ERROR: reader received %d messages expected %d ")
                          ACE_TEXT("messages!\n"), num_received, num_expected),
                          -1);
    }
  }
  ACE_CATCHALL
  {
    ACE_ERROR_RETURN ((LM_ERROR, "(%P|%t) Exception caught in read."), -1);
  }
  ACE_ENDTRY;

  ACE_DEBUG((LM_DEBUG,
              ACE_TEXT("(%P|%t) read done.\n")));

  return 0;
}

