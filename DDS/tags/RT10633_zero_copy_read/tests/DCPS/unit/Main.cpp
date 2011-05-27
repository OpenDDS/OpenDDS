#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/DataSampleList.h"
#include "dds/DCPS/transport/framework/TransportSendElement.h"
#include "dds/DCPS/Marked_Default_Qos.h"
#include "dds/DCPS/Qos_Helper.h"
#include "dds/DCPS/TopicImpl.h"
#include "dds/DCPS/DomainParticipantImpl.h"
#include "dds/DdsDcpsDomainC.h"
#include "dds/DdsDcpsSubscriptionS.h"
#include "dds/DdsDcpsTopicC.h"
#include "MyTypeSupportImpl.h"
#include "tests/DCPS/common/TestSupport.h"

#include "tao/ORB_Core.h"
#include "ace/Get_Opt.h"
#include "ace/High_Res_Timer.h"
#include "ace/Arg_Shifter.h"
#include "ace/Reactor.h"

const long  MY_DOMAIN   = 911;
const char* MY_TOPIC    = "foo";
const char* OTHER_TOPIC = "other";
const char* MY_TYPE     = "foo";

int client_orb = 0;
const ACE_Time_Value find_topic_timeout(5, 0);
CORBA::ORB_var orb;
PortableServer::POA_var poa;
::DDS::DomainParticipantFactory_var dpf;
class ORB_Task;
ORB_Task* orb_task = 0;

using namespace ::DDS;
using namespace ::TAO::DCPS;

class ORB_Task : public ACE_Task_Base
{
public:
  ORB_Task (CORBA::ORB_ptr orb)
    : orb_(CORBA::ORB::_duplicate (orb))
  {
  };

  /** Lanch a thread to run the orb. **/
  virtual int svc ()
  {
    {
      bool done = false;
      while (! done)
        {
          try
            {
              if (orb_->orb_core()->has_shutdown () == false)
                {
                  orb_->run ();
                }
              done = true;
            }
          catch (const CORBA::SystemException& sysex)
            {
              sysex._tao_print_exception (
                "TAO_DCPS_Service_Participant::svc");
            }
          catch (const CORBA::UserException& userex)
            {
              userex._tao_print_exception (
                "TAO_DCPS_Service_Participant::svc");
            }
          catch (const CORBA::Exception& ex)
            {
              ex._tao_print_exception (
                "TAO_DCPS_Service_Participant::svc");
            }
          if (orb_->orb_core()->has_shutdown ())
            {
              done = true;
            }
          else
            {
              orb_->orb_core()->reactor()->reset_reactor_event_loop ();
            }
        }
    }

    return 0;
  };

private:
  CORBA::ORB_var orb_;
};

void
usage (const ACE_TCHAR * cmd)
{
  ACE_DEBUG ((LM_INFO,
              ACE_TEXT ("Usage:\n")
              ACE_TEXT ("  %s\n")
              ACE_TEXT ("    -c <client set_orb flag>\n")
              ACE_TEXT ("\n"),
              cmd));
}

void
parse_args (int argc,
            ACE_TCHAR *argv[])
{
  ACE_Arg_Shifter arg_shifter (argc, argv);

  while (arg_shifter.is_anything_left ())
  {
    const char *currentArg = 0;

    if ((currentArg = arg_shifter.get_the_parameter("-c")) != 0)
    {
      client_orb = ACE_OS::atoi (currentArg);
      arg_shifter.consume_arg ();
    }
    else
    {
      arg_shifter.ignore_arg ();
    }
  }
}


int run_domain_test ()
{
  ::DDS::ReturnCode_t ret;

  // create participant
  ::DDS::DomainParticipant_var new_dp
    = dpf->create_participant(MY_DOMAIN,
                              PARTICIPANT_QOS_DEFAULT,
                              ::DDS::DomainParticipantListener::_nil ());

  TEST_CHECK (! CORBA::is_nil (new_dp.in ()));

  ::DDS::DomainId_t domain_id
    = new_dp->get_domain_id ();

  TEST_CHECK (domain_id == MY_DOMAIN);

  MyTypeSupportImpl* fts_servant = new MyTypeSupportImpl();
  PortableServer::ServantBase_var safe_servant = fts_servant;


  MyTypeSupport_var fts =
    TAO::DCPS::servant_to_reference (fts_servant);

  if (::DDS::RETCODE_OK != fts->register_type(new_dp.in (), MY_TYPE))
    {
      ACE_ERROR ((LM_ERROR,
      ACE_TEXT ("Failed to register the FooTypeSupport.")));
      return 1;
    }

  // lookup existent participant
  ::DDS::DomainParticipant_var looked_dp
    = dpf->lookup_participant(MY_DOMAIN);

  ::TAO::DCPS::DomainParticipantImpl* new_dp_servant
    = ::TAO::DCPS::reference_to_servant< ::TAO::DCPS::DomainParticipantImpl,
                                ::DDS::DomainParticipant_ptr >
            (new_dp.in ());

  ::TAO::DCPS::DomainParticipantImpl* looked_dp_servant
    = ::TAO::DCPS::reference_to_servant< ::TAO::DCPS::DomainParticipantImpl,
                                ::DDS::DomainParticipant_ptr >
            (looked_dp.in ());

  TEST_CHECK (looked_dp_servant == new_dp_servant);

  // create topic
  ::DDS::Topic_var new_topic
    = new_dp->create_topic(MY_TOPIC,
                           MY_TYPE,
                           TOPIC_QOS_DEFAULT,
                           ::DDS::TopicListener::_nil ());

  ::TAO::DCPS::TopicImpl* new_topic_servant
    = ::TAO::DCPS::reference_to_servant< ::TAO::DCPS::TopicImpl,
                                ::DDS::Topic_ptr >
            (new_topic.in ());

  ::DDS::Duration_t timeout;
  timeout.sec = find_topic_timeout.sec ();
  timeout.nanosec = find_topic_timeout.usec ();

  // find existent topic
  ::DDS::Topic_var found_topic
    = new_dp->find_topic(MY_TOPIC, timeout);

  ::TAO::DCPS::TopicImpl* found_topic_servant
    = ::TAO::DCPS::reference_to_servant< ::TAO::DCPS::TopicImpl,
                                ::DDS::Topic_ptr >
            (found_topic.in ());

  TEST_CHECK (new_topic_servant == found_topic_servant);

  // find existent topicdescription
  ::DDS::TopicDescription_var found_topicdescription
    = new_dp->lookup_topicdescription(MY_TOPIC);

  TEST_CHECK (! CORBA::is_nil (found_topicdescription.in ()));

  // widen the topicdescription to topic
  ::DDS::Topic_var widened_topic
    = ::DDS::Topic::_narrow(found_topicdescription.in ());

  TEST_CHECK (! CORBA::is_nil (widened_topic.in ()));

  ACE_ERROR((LM_ERROR,
    "We expect to see an error message from delete_participant\n"));
  ret = dpf->delete_participant(new_dp.in ());

  TEST_CHECK (ret == ::DDS::RETCODE_PRECONDITION_NOT_MET);

  // delete existent topic first time
  ret = new_dp->delete_topic(found_topic.in ());

  TEST_CHECK (ret == ::DDS::RETCODE_OK);

  // delete existent topic second time
  ret = new_dp->delete_topic(new_topic.in ());

  TEST_CHECK (ret == ::DDS::RETCODE_OK);

  // an extra delete existent topic
  ACE_ERROR((LM_ERROR,
    "We expect to see an error message from delete_topic\n"));
  ret = new_dp->delete_topic(new_topic.in ());

  TEST_CHECK (ret == ::DDS::RETCODE_ERROR);

  // Look up the topicdescription after the topic is deleted will
  // return nil.
  found_topicdescription
    = new_dp->lookup_topicdescription(MY_TOPIC);

  TEST_CHECK (CORBA::is_nil(found_topicdescription.in ()));

  // find a non-existent topic - return nil
  ACE_High_Res_Timer timer;
  ACE_hrtime_t elapsedTime = 0;
  timer.start ();
  found_topic = new_dp->find_topic(OTHER_TOPIC, timeout);
  timer.stop();
  timer.elapsed_microseconds(elapsedTime);
  elapsedTime += 10000; // some systems can be short by up to 10 milliseconds
  TEST_CHECK (CORBA::is_nil(found_topic.in ())
    && long(elapsedTime) >= find_topic_timeout.sec () * 1000000 + find_topic_timeout.usec ());

  // delete the existent participant
  ret = dpf->delete_participant(new_dp.in ());

  TEST_CHECK (ret == ::DDS::RETCODE_OK);

  // lookup the participant after it's deleted - return nil
  looked_dp = dpf->lookup_participant(MY_DOMAIN);

  TEST_CHECK (CORBA::is_nil(looked_dp.in ()));

  return 0;
}


void run_next_sample_test (ssize_t size)
{
  DataSampleList list;
  ssize_t pub_id_head = 0;
  ssize_t pub_id_tail = size - 1;
  ssize_t pub_id_middle = size/2;
  DataSampleListElement* middle = 0;

  TAO::DCPS::TransportSendElementAllocator trans_allocator(size, sizeof (TAO::DCPS::TransportSendElement));

  { // make VC6 buid - avoid error C2374: 'i' : redefinition; multiple initialization
  for (ssize_t i = 0; i < size; i ++)
  {
    DataSampleListElement* sample
      = new DataSampleListElement (i, 0, 0, &trans_allocator);
    if (i == pub_id_middle)
    {
      middle = sample;
    }
    list.enqueue_tail_next_sample (sample);
  }
  }
  ssize_t current_size = list.size_;
  bool ret = true;

  if (middle != 0)
  {
    ret = list.dequeue_next_sample (middle);
    if (current_size == 0)
    {
      TEST_CHECK (ret == false);
    }
    else
    {
      TEST_CHECK (ret == true);
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
    DataSampleListElement* sample;
    TEST_CHECK (list.dequeue_head_next_sample (sample)
                == true);
    TEST_CHECK (sample->publication_id_ == i);
    delete sample;
  }
  }

  TEST_CHECK (list.head_ == 0
              && list.tail_ == 0
              && list.size_ == 0);
}

void run_next_send_sample_test (ssize_t size)
{
  DataSampleList list;
  DataSampleList appended_list;
  ssize_t pub_id_head = 0;
  ssize_t pub_id_tail = size - 1;
  ssize_t pub_id_middle = size/2;
  DataSampleListElement* middle = 0;

  TAO::DCPS::TransportSendElementAllocator trans_allocator(size, sizeof (TAO::DCPS::TransportSendElement));

  for (ssize_t i = 0; i < pub_id_middle; i ++)
  {
    DataSampleListElement* sample
      = new DataSampleListElement (i, 0, 0, &trans_allocator);
    list.enqueue_tail_next_send_sample (sample);
  }

  { // make VC6 buid - avoid error C2374: 'i' : redefinition; multiple initialization
  for (ssize_t i = pub_id_middle; i < size; i ++)
  {
    DataSampleListElement* sample
      = new DataSampleListElement (i, 0, 0, &trans_allocator);
    if (i == pub_id_middle)
    {
      middle = sample;
    }
    appended_list.enqueue_tail_next_send_sample (sample);
  }
  }
  list.enqueue_tail_next_send_sample (appended_list);

  ssize_t current_size = list.size_;
  bool ret = true;

  if (middle != 0)
  {
    ret = list.dequeue_next_send_sample (middle);
    if (current_size == 0)
    {
      TEST_CHECK (ret == false);
    }
    else
    {
      TEST_CHECK (ret == true);
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
    DataSampleListElement* sample;
    TEST_CHECK (list.dequeue_head_next_send_sample (sample)
                == true);
    TEST_CHECK (sample->publication_id_ == i);
    delete sample;
  }
  }
  TEST_CHECK (list.head_ == 0
              && list.tail_ == 0
              && list.size_ == 0);
}

void run_next_instance_sample_test (ssize_t size)
{
  DataSampleList list;
  ssize_t pub_id_head = 0;
  ssize_t pub_id_tail = size - 1;
  ssize_t pub_id_middle = size/2;
  DataSampleListElement* middle = 0;

  TAO::DCPS::TransportSendElementAllocator trans_allocator(size, sizeof (TAO::DCPS::TransportSendElement));

  for (ssize_t i = 0; i < size; i ++)
  {
    DataSampleListElement* sample
      = new DataSampleListElement (i, 0, 0, &trans_allocator);
    if (i == pub_id_middle)
    {
      middle = sample;
    }
    list.enqueue_tail_next_instance_sample (sample);
  }

  ssize_t current_size = list.size_;
  bool ret = true;

  if (middle != 0)
  {
    ret = list.dequeue_next_instance_sample (middle);
    if (current_size == 0)
    {
      TEST_CHECK (ret == false);
    }
    else
    {
      TEST_CHECK (ret == true);
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
    DataSampleListElement* sample;
    TEST_CHECK (list.dequeue_head_next_instance_sample (sample)
                == true);
    TEST_CHECK (sample->publication_id_ == i);
    delete sample;
  }
  }
  TEST_CHECK (list.head_ == 0
              && list.tail_ == 0
              && list.size_ == 0);
}

int
ACE_TMAIN (int argc, ACE_TCHAR *argv[])
{
  try
    {
      parse_args (argc, argv);

      if (client_orb)
        {
          // Client creates the orb.
          orb = CORBA::ORB_init (argc,
                                 argv,
                                 "TAO_DDS_DCPS");

          TheServiceParticipant->set_ORB(orb.in());

          // Client runs the orb.
          CORBA::Object_var obj =
            orb->resolve_initial_references ("RootPOA");

          poa = PortableServer::POA::_narrow (obj.in ());

          PortableServer::POAManager_var poa_manager =
            poa->the_POAManager ();

          poa_manager->activate ();

          orb_task = new ORB_Task (orb.in ());
          if (orb_task->activate (THR_NEW_LWP | THR_JOINABLE, 1) == -1)
            {
              ACE_ERROR ((LM_ERROR,
                          ACE_TEXT ("TAO_DCPS_Service_Participant::get_domain_participant_factory, ")
                          ACE_TEXT ("Failed to activate the orb task.")));
              return 1;
            }

          dpf = TheParticipantFactory;
       }
      else
        {
          dpf = TheParticipantFactoryWithArgs(argc, argv);
          poa = TheServiceParticipant->the_poa();
        }


      int ret = run_domain_test ();
      TEST_CHECK (ret == 0);

      for (ssize_t i = 0; i < 6; i ++)
      {
        run_next_sample_test (i);
        run_next_send_sample_test (i);
        run_next_instance_sample_test (i);
      }

      if (client_orb)
        {
          orb->shutdown (0);
          orb_task->wait ();
        }

      TheServiceParticipant->shutdown ();

      if (client_orb)
        {
          orb->destroy ();
        }

    }
  catch (const CORBA::Exception& ex)
    {
      ex._tao_print_exception ("Exception caught:");
      return 1;
    }

  return 0;
}



