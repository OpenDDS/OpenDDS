#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/SendStateDataSampleList.h"
#include "dds/DCPS/InstanceDataSampleList.h"
#include "dds/DCPS/WriterDataSampleList.h"
#include "dds/DCPS/DataSampleElement.h"
#include "dds/DCPS/transport/framework/TransportSendElement.h"
#include "dds/DCPS/Marked_Default_Qos.h"
#include "dds/DCPS/RepoIdBuilder.h"
#include "dds/DCPS/Qos_Helper.h"
#include "dds/DCPS/TopicImpl.h"
#include "dds/DCPS/DomainParticipantImpl.h"
#include "dds/DdsDcpsDomainC.h"
#include "dds/DdsDcpsSubscriptionC.h"
#include "dds/DdsDcpsTopicC.h"
#include "MyTypeSupportImpl.h"
#include "tests/DCPS/common/TestSupport.h"

#include "dds/DCPS/StaticIncludes.h"

#include "ace/High_Res_Timer.h"
#include "ace/Arg_Shifter.h"
#include "ace/Reactor.h"
#include "ace/Argv_Type_Converter.h"

const long  MY_DOMAIN   = 911;
const char* MY_TOPIC    = "foo";
const char* OTHER_TOPIC = "other";
const char* MY_TYPE     = "foo";

const ACE_Time_Value find_topic_timeout(5, 0);
::DDS::DomainParticipantFactory_var dpf;

using namespace ::DDS;
using namespace ::OpenDDS::DCPS;

int run_domain_test ()
{
  ::DDS::ReturnCode_t ret;

  // create participant
  ::DDS::DomainParticipant_var new_dp
    = dpf->create_participant(MY_DOMAIN,
                              PARTICIPANT_QOS_DEFAULT,
                              ::DDS::DomainParticipantListener::_nil (),
                              ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  TEST_CHECK (! CORBA::is_nil (new_dp.in ()));
  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) run_domain_test: ")
    ACE_TEXT("(! CORBA::is_nil (new_dp.in ()))")
    ACE_TEXT("\n")
  ));

  ::DDS::DomainId_t domain_id
    = new_dp->get_domain_id ();

  TEST_CHECK (domain_id == MY_DOMAIN);
  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) run_domain_test: ")
    ACE_TEXT("(domain_id == MY_DOMAIN)")
    ACE_TEXT("\n")
  ));

  MyTypeSupport_var fts (new MyTypeSupportImpl);

  if (::DDS::RETCODE_OK != fts->register_type(new_dp.in (), MY_TYPE))
    {
      ACE_ERROR ((LM_ERROR,
      ACE_TEXT ("Failed to register the FooTypeSupport.")));
      return 1;
    }

  // lookup existent participant
  ::DDS::DomainParticipant_var looked_dp
    = dpf->lookup_participant(MY_DOMAIN);

  OpenDDS::DCPS::DomainParticipantImpl* new_dp_servant
    = dynamic_cast<OpenDDS::DCPS::DomainParticipantImpl*>(new_dp.in());

  OpenDDS::DCPS::DomainParticipantImpl* looked_dp_servant
    = dynamic_cast<OpenDDS::DCPS::DomainParticipantImpl*>(looked_dp.in ());

  TEST_CHECK (looked_dp_servant == new_dp_servant);
  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) run_domain_test: ")
    ACE_TEXT("(looked_dp_servant == new_dp_servant)")
    ACE_TEXT("\n")
  ));

  // create topic
  ::DDS::Topic_var new_topic
    = new_dp->create_topic(MY_TOPIC,
                           MY_TYPE,
                           TOPIC_QOS_DEFAULT,
                           ::DDS::TopicListener::_nil (),
                           ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  OpenDDS::DCPS::TopicImpl* new_topic_servant
    = dynamic_cast<OpenDDS::DCPS::TopicImpl*>(new_topic.in ());

  ::DDS::Duration_t timeout;
  timeout.sec = static_cast<long>(find_topic_timeout.sec ());
  timeout.nanosec = find_topic_timeout.usec ();

  // find existent topic
  ::DDS::Topic_var found_topic
    = new_dp->find_topic(MY_TOPIC, timeout);

  ::OpenDDS::DCPS::TopicImpl* found_topic_servant
    = dynamic_cast<OpenDDS::DCPS::TopicImpl*>
    (found_topic.in ());

  TEST_CHECK (new_topic_servant == found_topic_servant);
  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) run_domain_test: ")
    ACE_TEXT("(new_topic_servant == found_topic_servant)")
    ACE_TEXT("\n")
  ));

  // find existent topicdescription
  ::DDS::TopicDescription_var found_topicdescription
    = new_dp->lookup_topicdescription(MY_TOPIC);

  TEST_CHECK (! CORBA::is_nil (found_topicdescription.in ()));
  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) run_domain_test: ")
    ACE_TEXT("(! CORBA::is_nil (found_topicdescription.in ()))")
    ACE_TEXT("\n")
  ));

  // widen the topicdescription to topic
  ::DDS::Topic_var widened_topic
    = ::DDS::Topic::_narrow(found_topicdescription.in ());

  TEST_CHECK (! CORBA::is_nil (widened_topic.in ()));
  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) run_domain_test: ")
    ACE_TEXT("(! CORBA::is_nil (widened_topic.in ()))")
    ACE_TEXT("\n")
  ));

  ACE_ERROR((LM_ERROR,
    "We expect to see an error message from delete_participant\n"));
  ret = dpf->delete_participant(new_dp.in ());

  TEST_CHECK (ret == ::DDS::RETCODE_PRECONDITION_NOT_MET);
  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) run_domain_test: ")
    ACE_TEXT("(ret == ::DDS::RETCODE_PRECONDITION_NOT_MET)")
    ACE_TEXT("\n")
  ));

  // delete existent topic first time
  ret = new_dp->delete_topic(found_topic.in ());

  TEST_CHECK (ret == ::DDS::RETCODE_OK);
  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) run_domain_test: ")
    ACE_TEXT("(ret == ::DDS::RETCODE_OK)")
    ACE_TEXT("\n")
  ));

  // delete existent topic second time
  ret = new_dp->delete_topic(new_topic.in ());

  TEST_CHECK (ret == ::DDS::RETCODE_OK);
  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) run_domain_test: ")
    ACE_TEXT("(ret == ::DDS::RETCODE_OK)")
    ACE_TEXT("\n")
  ));

  // an extra delete existent topic
  ACE_ERROR((LM_ERROR,
    "We expect to see an error message from delete_topic\n"));
  ret = new_dp->delete_topic(new_topic.in ());

  TEST_CHECK (ret == ::DDS::RETCODE_ERROR);
  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) run_domain_test: ")
    ACE_TEXT("(ret == ::DDS::RETCODE_ERROR)")
    ACE_TEXT("\n")
  ));

  // Look up the topicdescription after the topic is deleted will
  // return nil.
  found_topicdescription
    = new_dp->lookup_topicdescription(MY_TOPIC);

  TEST_CHECK (CORBA::is_nil(found_topicdescription.in ()));
  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) run_domain_test: ")
    ACE_TEXT("(CORBA::is_nil(found_topicdescription.in ()))")
    ACE_TEXT("\n")
  ));

  // find a non-existent topic - return nil
  ACE_High_Res_Timer timer;
  ACE_Time_Value elapsedTime(0, 0);
  timer.start ();
  found_topic = new_dp->find_topic(OTHER_TOPIC, timeout);
  timer.stop();
  timer.elapsed_time(elapsedTime);
  ACE_Time_Value tenMillis (0, 10000);
  elapsedTime += tenMillis;
  // some systems can be short by up to 10 milliseconds
  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) run_domain_test: ")
    ACE_TEXT("(CORBA::is_nil(found_topic.in ()) && elapsedTime.msec() >= find_topic_timeout.msec())")
    ACE_TEXT("\n")
  ));

  // delete the existent participant
  ret = dpf->delete_participant(new_dp.in ());

  TEST_CHECK (ret == ::DDS::RETCODE_OK);
  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) run_domain_test: ")
    ACE_TEXT("(ret == ::DDS::RETCODE_OK)")
    ACE_TEXT("\n")
  ));

  // lookup the participant after it's deleted - return nil
  looked_dp = dpf->lookup_participant(MY_DOMAIN);

  TEST_CHECK (CORBA::is_nil(looked_dp.in ()));
  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) run_domain_test: ")
    ACE_TEXT("(CORBA::is_nil(looked_dp.in ()))")
    ACE_TEXT("\n")
  ));

  return 0;
}


void run_sample_list_test ()
{
  SendStateDataSampleList list;
  TEST_CHECK( list.begin() == list.end() );

  OpenDDS::DCPS::RepoId repoId(GUID_UNKNOWN);
  DataSampleElement* sample[3];
  ssize_t i;
  for (i = 0; i < 3; i ++)
  {
    repoId.entityId.entityKey[2] = static_cast<CORBA::Octet>(i);
    sample[i]
      = new DataSampleElement(repoId, 0, OpenDDS::DCPS::PublicationInstance_rch(), 0, 0);
    list.enqueue_tail(sample[i]);
  }
  TEST_CHECK( list.begin() != list.end() );
  SendStateDataSampleListIterator iter = list.begin();
  TEST_CHECK( (*iter).get_pub_id().entityId.entityKey[2] == 0 );
  TEST_CHECK( iter->get_pub_id().entityId.entityKey[2] == 0 );
  TEST_CHECK( ++iter != list.end() );
  TEST_CHECK( iter->get_pub_id().entityId.entityKey[2] == 1 );
  TEST_CHECK( ++iter != list.end() );
  TEST_CHECK( iter->get_pub_id().entityId.entityKey[2] == 2 );
  TEST_CHECK( ++iter == list.end() );
  TEST_CHECK( iter-- == list.end() );
  TEST_CHECK( iter != list.end() );
  TEST_CHECK( iter->get_pub_id().entityId.entityKey[2] == 2 );
  TEST_CHECK( --iter != list.end() );
  TEST_CHECK( iter->get_pub_id().entityId.entityKey[2] == 1 );
  TEST_CHECK( --iter != list.end() );
  TEST_CHECK( iter == list.begin() );
  TEST_CHECK( iter++ == list.begin() );
  TEST_CHECK( iter->get_pub_id().entityId.entityKey[2] == 1 );

  // document that SendStateDataSampleList::iterator == not based on list itself
  // but rather on the *send_sample_ pointers in the DataSampleElements themselves
  iter = list.begin();
  WriterDataSampleList sameHeadTailList;
  // calling enqueue_tail_next_sample will setup head and tail, but not mess with
  // send_sample params
  sameHeadTailList.enqueue_tail (sample[0]);
  sameHeadTailList.enqueue_tail (sample[2]);
  // will iterate the same, since sample 0-2 send_sample params were not changed
  SendStateDataSampleListIterator iter1 = SendStateDataSampleListIterator(sameHeadTailList.head(), sameHeadTailList.tail(), sameHeadTailList.head());
  TEST_CHECK( iter == iter1 );
  TEST_CHECK( ++iter == ++iter1 );
  TEST_CHECK( ++iter == ++iter1 );
  TEST_CHECK( ++iter == ++iter1 );

  // check same head, same current but different tail fails
  WriterDataSampleList tailDiffList;
  tailDiffList.enqueue_tail (sample[0]);
  tailDiffList.enqueue_tail (sample[1]);
  SendStateDataSampleListIterator iter_tailDiffList = SendStateDataSampleListIterator(tailDiffList.head(), tailDiffList.tail(), tailDiffList.head());
  TEST_CHECK( list.begin() != iter_tailDiffList );

  // check same tail, same current but different head fails
  WriterDataSampleList headDiffList;
  headDiffList.enqueue_tail (sample[1]);
  headDiffList.enqueue_tail (sample[2]);
  iter = list.begin();
  iter1 = SendStateDataSampleListIterator(headDiffList.head(), headDiffList.tail(), headDiffList.head());
  // verify both iters have same current
  TEST_CHECK( ++iter->get_pub_id().entityId.entityKey[2] == 1 );
  TEST_CHECK( iter1->get_pub_id().entityId.entityKey[2] == 1 );
  TEST_CHECK( iter != iter1 );


  list.reset();
  TEST_CHECK( list.begin() == list.end() );
  for (i = 0; i < 3; i ++)
  {
    delete sample[i];
  }
}

void run_next_sample_test (ssize_t size)
{
  WriterDataSampleList list;
  ssize_t pub_id_head = 0;
  ssize_t pub_id_tail = size - 1;
  ssize_t pub_id_middle = size/2;
  DataSampleElement* middle = 0;

  OpenDDS::DCPS::TransportSendElementAllocator trans_allocator(size, sizeof (OpenDDS::DCPS::TransportSendElement));

  // RepoIds are conventionally created and managed by the DCPSInfoRepo. Those
  // generated here are for the sole purpose of verifying internal behavior.
  OpenDDS::DCPS::RepoIdBuilder builder;

  builder.participantId(1);
  builder.entityKey(0);
  builder.entityKind(OpenDDS::DCPS::ENTITYKIND_USER_WRITER_WITH_KEY);

  OpenDDS::DCPS::RepoId repoId(builder);

  for (ssize_t i = 0; i < size; i ++)
  {
    repoId.entityId.entityKey[2] = static_cast<CORBA::Octet>(i);
    DataSampleElement* sample
      = new DataSampleElement(repoId, 0, OpenDDS::DCPS::PublicationInstance_rch(), &trans_allocator, 0);
    if (i == pub_id_middle)
    {
      middle = sample;
    }
    list.enqueue_tail (sample);
  }
  ssize_t current_size = list.size();
  bool ret = true;

  if (middle != 0)
  {
    ret = list.dequeue (middle);
    if (current_size == 0)
    {
      TEST_CHECK (ret == false);
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) run_next_sample_test: ")
        ACE_TEXT("(ret == false)")
        ACE_TEXT("\n")
      ));
    }
    else
    {
      TEST_CHECK (ret == true);
      delete middle;
    }
  }

  { // make VC6 buid - avoid error C2374: 'i' : redefinition; multiple initialization
  for (ssize_t i = pub_id_head;
       i <= pub_id_tail;
       i ++)
  {
    if (i == pub_id_middle)
    {
      continue;
    }
    DataSampleElement* sample;
    TEST_CHECK (list.dequeue_head (sample)
                == true);
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) run_next_sample_test: ")
      ACE_TEXT("(list.dequeue_head_next_sample (sample) == true)")
      ACE_TEXT("\n")
    ));
    repoId.entityId.entityKey[2] = static_cast<CORBA::Octet>(i);
    TEST_CHECK (sample->get_pub_id() == repoId);
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) run_next_sample_test: ")
      ACE_TEXT("(sample->publication_id_ == converter)")
      ACE_TEXT("\n")
    ));
    delete sample;
  }
  }

  TEST_CHECK (list.head() == 0
              && list.tail() == 0
              && list.size() == 0);
  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) run_next_sample_test: ")
    ACE_TEXT("(list.head() == 0 && list.tail() == 0 && list.size() == 0)")
    ACE_TEXT("\n")
  ));
}

void run_next_send_sample_test (ssize_t size)
{
  SendStateDataSampleList list;
  SendStateDataSampleList appended_list;
  ssize_t pub_id_head = 0;
  ssize_t pub_id_tail = size - 1;
  ssize_t pub_id_middle = size/2;
  DataSampleElement* middle = 0;

  OpenDDS::DCPS::TransportSendElementAllocator trans_allocator(size, sizeof (OpenDDS::DCPS::TransportSendElement));

  // RepoIds are conventionally created and managed by the DCPSInfoRepo. Those
  // generated here are for the sole purpose of verifying internal behavior.
  OpenDDS::DCPS::RepoIdBuilder builder;

  builder.participantId(1);
  builder.entityKey(0);
  builder.entityKind(OpenDDS::DCPS::ENTITYKIND_USER_WRITER_WITH_KEY);

  OpenDDS::DCPS::RepoId repoId(builder);

  for (ssize_t i = 0; i < pub_id_middle; i ++)
  {
    repoId.entityId.entityKey[2] = static_cast<CORBA::Octet>(i);
    DataSampleElement* sample
      = new DataSampleElement(repoId, 0, OpenDDS::DCPS::PublicationInstance_rch(), &trans_allocator, 0);
    list.enqueue_tail (sample);
  }

  for (ssize_t i = pub_id_middle; i < size; i ++)
  {
    repoId.entityId.entityKey[2] = static_cast<CORBA::Octet>(i);
    DataSampleElement* sample
      = new DataSampleElement(repoId, 0, OpenDDS::DCPS::PublicationInstance_rch(), &trans_allocator, 0);
    if (i == pub_id_middle)
    {
      middle = sample;
    }
    appended_list.enqueue_tail (sample);
  }
  list.enqueue_tail (appended_list);

  ssize_t current_size = list.size();
  bool ret = true;

  if (middle != 0)
  {
    ret = list.dequeue (middle);
    if (current_size == 0)
    {
      TEST_CHECK (ret == false);
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) run_next_send_sample_test: ")
        ACE_TEXT("(ret == false)")
        ACE_TEXT("\n")
      ));
    }
    else
    {
      TEST_CHECK (ret == true);
      delete middle;
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) run_next_send_sample_test: ")
        ACE_TEXT("(ret == true)")
        ACE_TEXT("\n")
      ));
    }
  }

  { // make VC6 buid - avoid error C2374: 'i' : redefinition; multiple initialization
  for (ssize_t i = pub_id_head;
       i <= pub_id_tail;
       i ++)
  {
    if (i == pub_id_middle)
    {
      continue;
    }
    DataSampleElement* sample;
    TEST_CHECK (list.dequeue_head (sample)
                == true);
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) run_next_send_sample_test: ")
      ACE_TEXT("(list.dequeue_head_next_send_sample (sample) == true)")
      ACE_TEXT("\n")
    ));
    repoId.entityId.entityKey[2] = static_cast<CORBA::Octet>(i);
    TEST_CHECK (sample->get_pub_id() == repoId);
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) run_next_send_sample_test: ")
      ACE_TEXT("(sample->publication_id_ == converter)")
      ACE_TEXT("\n")
    ));
    delete sample;
  }
  }
  TEST_CHECK (list.head() == 0
              && list.tail() == 0
              && list.size() == 0);
  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) run_next_send_sample_test: ")
    ACE_TEXT("(list.head() == 0 && list.tail() == 0 && list.size() == 0)")
    ACE_TEXT("\n")
  ));
}

void run_next_instance_sample_test (ssize_t size)
{
  InstanceDataSampleList list;
  ssize_t pub_id_head = 0;
  ssize_t pub_id_tail = size - 1;
  ssize_t pub_id_middle = size/2;
  DataSampleElement* middle = 0;

  OpenDDS::DCPS::TransportSendElementAllocator trans_allocator(size, sizeof (OpenDDS::DCPS::TransportSendElement));

  // RepoIds are conventionally created and managed by the DCPSInfoRepo. Those
  // generated here are for the sole purpose of verifying internal behavior.
  OpenDDS::DCPS::RepoIdBuilder builder;

  builder.participantId(1);
  builder.entityKey(0);
  builder.entityKind(OpenDDS::DCPS::ENTITYKIND_USER_WRITER_WITH_KEY);

  OpenDDS::DCPS::RepoId repoId(builder);

  for (ssize_t i = 0; i < size; i ++)
  {
    repoId.entityId.entityKey[2] = static_cast<CORBA::Octet>(i);
    DataSampleElement* sample
      = new DataSampleElement(repoId, 0, OpenDDS::DCPS::PublicationInstance_rch(), &trans_allocator, 0);
    if (i == pub_id_middle)
    {
      middle = sample;
    }
    list.enqueue_tail (sample);
  }

  ssize_t current_size = list.size();
  bool ret = true;

  if (middle != 0)
  {
    ret = list.dequeue (middle);
    if (current_size == 0)
    {
      TEST_CHECK (ret == false);
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) run_next_instance_sample_test: ")
        ACE_TEXT("(ret == false)")
        ACE_TEXT("\n")
      ));
    }
    else
    {
      TEST_CHECK (ret == true);
      delete middle;
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) run_next_instance_sample_test: ")
        ACE_TEXT("(ret == true)")
        ACE_TEXT("\n")
      ));
    }
  }

  { // make VC6 build - avoid error C2374: 'i' : redefinition; multiple initialization
  for (ssize_t i = pub_id_head;
       i <= pub_id_tail;
       i ++)
  {
    if (i == pub_id_middle)
    {
      continue;
    }
    DataSampleElement* sample;
    TEST_CHECK (list.dequeue_head (sample)
                == true);
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) run_next_instance_sample_test: ")
      ACE_TEXT("(list.dequeue_head_next_instance_sample (sample) == true)")
      ACE_TEXT("\n")
    ));
    repoId.entityId.entityKey[2] = static_cast<CORBA::Octet>(i);
    TEST_CHECK (sample->get_pub_id() == repoId);
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) run_next_instance_sample_test: ")
      ACE_TEXT("(sample->publication_id_ == converter)")
      ACE_TEXT("\n")
    ));
    delete sample;
  }
  }
  TEST_CHECK (list.head() == 0
              && list.tail() == 0
              && list.size() == 0);
  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) run_next_instance_sample_test: ")
    ACE_TEXT("")
    ACE_TEXT("\n")
  ));
}

int
ACE_TMAIN (int argc, ACE_TCHAR *argv[])
{
  try
    {
      dpf = TheParticipantFactoryWithArgs(argc, argv);

      int ret = run_domain_test ();
      TEST_CHECK (ret == 0);
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) main: ")
        ACE_TEXT("(ret == 0)")
        ACE_TEXT("\n")
      ));

      run_sample_list_test();

      for (ssize_t i = 0; i < 6; i ++)
      {
        run_next_sample_test (i);
        run_next_send_sample_test (i);
        run_next_instance_sample_test (i);
      }

      TheServiceParticipant->shutdown ();

    }
  catch (const CORBA::Exception& ex)
    {
      ex._tao_print_exception ("Exception caught:");
      return 1;
    }

  return 0;
}
