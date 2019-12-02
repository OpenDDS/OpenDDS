// -*- C++ -*-
//

#include "common.h"
#include "tests/DCPS/common/TestSupport.h"
#include "dds/DCPS/BuiltInTopicUtils.h"
#include "tests/DCPS/FooType4/FooDefTypeSupportImpl.h"
#include "ace/OS_NS_unistd.h"
#include <string>

#include <sstream>

const long  TEST_DOMAIN   = 42;
const char* TEST_TOPIC    = "foo";
const char* TEST_TOPIC_TYPE     = "foo";
const ACE_TCHAR* reader_address_str = ACE_TEXT("localhost:0");
const ACE_TCHAR* writer_address_str = ACE_TEXT("localhost:0");
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
      // test knows everything and can use the discovery RepoID.
      ::OpenDDS::DCPS::RepoId part_id = participant_servant->get_id ();
      //SHH one of these should be the subscriber participant and the other should be the publisher participant.
      ::OpenDDS::DCPS::RepoId ignore_id = participant_servant->get_id ();

      std::stringstream participantBuffer;

      participantBuffer << ::OpenDDS::DCPS::to_string(part_id);

      std::stringstream ignoreBuffer;
      ignoreBuffer << ::OpenDDS::DCPS::to_string(ignore_id);

      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) IGNORE_PARTICIPANT,  participant %C ignore participant %C .\n"),
                 participantBuffer.str().c_str(), ignoreBuffer.str().c_str()));

      InstanceHandle_t handle = participant_servant->id_to_handle(ignore_id);

      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) IGNORE_PARTICIPANT, ignored participant %C has handle 0x%x.\n"),
        ignoreBuffer.str().c_str(),
        handle
      ));

      DDS::ReturnCode_t ret = participant->ignore_participant(handle);

      if (ret != ::DDS::RETCODE_OK)
      {
        ACE_ERROR ((LM_ERROR,
                    ACE_TEXT("(%P|%t) IGNORE_PARTICIPANT, ")
                    ACE_TEXT(" participant %C ignore participant %C returned error %d\n"),
                    participantBuffer.str().c_str(), ignoreBuffer.str().c_str(), ret));
        return -1;
      }
    }
    break;

  case IGNORE_TOPIC:
    {
      ::OpenDDS::DCPS::RepoId part_id = participant_servant->get_id ();
      ::OpenDDS::DCPS::RepoId ignore_id = topic_servant->get_id ();

      std::stringstream participantBuffer;
      participantBuffer << ::OpenDDS::DCPS::to_string(part_id);

      std::stringstream ignoreBuffer;
      ignoreBuffer << ::OpenDDS::DCPS::to_string(ignore_id);

      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) IGNORE_TOPIC, participant %C ignore topic %C .\n"),
                 participantBuffer.str().c_str(), ignoreBuffer.str().c_str()));

      InstanceHandle_t handle = participant_servant->id_to_handle(ignore_id);
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) IGNORE_TOPIC,  ignored topic %C has handle 0x%x.\n"),
        ignoreBuffer.str().c_str(),
        handle
      ));

      DDS::ReturnCode_t ret = participant->ignore_topic(handle);

      if (ret != ::DDS::RETCODE_OK)
      {
        ACE_ERROR ((LM_ERROR,
                    ACE_TEXT("(%P|%t) IGNORE_TOPIC, ")
                    ACE_TEXT(" ignore_topic 0x%x return error %d\n"),
                    handle, ret));
        return -1;
      }
    }
    break;
  case IGNORE_PUBLICATION:
    {
      ::OpenDDS::DCPS::RepoId part_id = participant_servant->get_id ();
      ::OpenDDS::DCPS::RepoId ignore_id = datawriter_servant->get_publication_id ();

      std::stringstream participantBuffer;
      participantBuffer << ::OpenDDS::DCPS::to_string(part_id);

      std::stringstream ignoreBuffer;
      ignoreBuffer << ::OpenDDS::DCPS::to_string(ignore_id);

      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) IGNORE_PUBLICATION, participant %C ignore publication %C .\n"),
                 participantBuffer.str().c_str(), ignoreBuffer.str().c_str()));

      InstanceHandle_t handle = participant_servant->id_to_handle(ignore_id);
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) IGNORE_PUBLICATION,  ignored topic %C has handle 0x%x.\n"),
        ignoreBuffer.str().c_str(),
        handle
      ));

      DDS::ReturnCode_t ret = participant->ignore_publication(handle);

      if (ret != ::DDS::RETCODE_OK)
      {
        ACE_ERROR ((LM_ERROR,
                    ACE_TEXT("(%P|%t) IGNORE_PUBLICATION, ")
                    ACE_TEXT(" ignore_publication 0x%x return error %d\n"),
                    handle, ret));
        return -1;
      }
    }
    break;
  case IGNORE_SUBSCRIPTION:
    {
      ::OpenDDS::DCPS::RepoId part_id = participant_servant->get_id ();
      ::OpenDDS::DCPS::RepoId ignore_id = datareader_servant->get_subscription_id ();

      std::stringstream participantBuffer;
      participantBuffer << ::OpenDDS::DCPS::to_string(part_id);

      std::stringstream ignoreBuffer;
      ignoreBuffer << ::OpenDDS::DCPS::to_string(ignore_id);

      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) IGNORE_SUBSCRIPTION, participant %C ignore subscription %C .\n"),
                 participantBuffer.str().c_str(), ignoreBuffer.str().c_str()));

      InstanceHandle_t handle = participant_servant->id_to_handle(ignore_id);
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) IGNORE_SUBSCRIPTION,  ignored topic %C has handle 0x%x.\n"),
        ignoreBuffer.str().c_str(),
        handle
      ));

      DDS::ReturnCode_t ret = participant->ignore_subscription(handle);

      if (ret != ::DDS::RETCODE_OK)
      {
        ACE_ERROR ((LM_ERROR,
                    ACE_TEXT("(%P|%t) IGNORE_SUBSCRIPTION, ")
                    ACE_TEXT(" ignore_subscription 0x%x returned error %d\n"),
                    handle, ret));
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

  try
  {
    ::Xyz::Foo foo;
    foo.x = -1;
    foo.y = -1;
    foo.key = default_key;

    ::Xyz::FooDataWriter_var foo_dw
      = ::Xyz::FooDataWriter::_narrow(datawriter.in ());
    TEST_CHECK (! CORBA::is_nil (foo_dw.in ()));

    ::DDS::InstanceHandle_t handle
        = foo_dw->register_instance(foo);

    for (int i = 0; i< num_writes; i ++)
    {

      foo.x = (float)i;

      foo_dw->write(foo,
                    handle);
    }

    ACE_DEBUG((LM_DEBUG,
             ACE_TEXT("(%P|%t) write  done\n")));
  }
  catch (...)
  {
    ACE_ERROR_RETURN ((LM_ERROR, "(%P|%t) Exception caught in write."), -1);
  }

  return 0;
}

int read (int expect_success)
{
  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) read begins. \n")));

  try
  {
    ::Xyz::FooDataReader_var foo_dr
      = ::Xyz::FooDataReader::_narrow(datareader.in ());
    if (CORBA::is_nil (foo_dr.in ()))
    {
      ACE_ERROR_RETURN ((LM_ERROR,
                 ACE_TEXT("(%P|%t) ::Xyz::FooDataReader::_narrow failed.\n")),
                 -1);
    }

    int num_reads = 0;
    int num_received = 0;

    bool init_val = false;
    ACE_Array<bool> messages(num_writes, init_val);

    while ( num_reads < num_writes)
    {

      ::Xyz::Foo foo;
      ::DDS::SampleInfo si ;

      DDS::ReturnCode_t status = foo_dr->read_next_sample(foo, si) ;
      num_reads ++;

      if (status == ::DDS::RETCODE_OK)
      {
        if (!expect_success)
        {
          ACE_ERROR_RETURN ((LM_ERROR,
                             ACE_TEXT("(%P|%t) ERROR: received an unexpected sample!\n"))
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
  catch (...)
  {
    ACE_ERROR_RETURN ((LM_ERROR, "(%P|%t) Exception caught in read."), -1);
  }

  ACE_DEBUG((LM_DEBUG,
              ACE_TEXT("(%P|%t) read done.\n")));

  return 0;
}

