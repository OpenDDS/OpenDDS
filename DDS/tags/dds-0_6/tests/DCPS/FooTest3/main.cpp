// -*- C++ -*-
// ============================================================================
/**
 *  @file   main.cpp
 *
 *  $Id$
 *
 *
 */
// ============================================================================


#include "Writer.h"
#include "InstanceDataMap.h"
#include "TestException.h"
#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/Marked_Default_Qos.h"
#include "dds/DCPS/TopicDescriptionImpl.h"
#include "tests/DCPS/FooType3/FooTypeSupportImpl.h"
#include "tests/DCPS/common/TestSupport.h"

#include "ace/Arg_Shifter.h"


const long  MY_DOMAIN   = 411;
const char* MY_TOPIC    = "foo";
const char* MY_TYPE     = "foo";
const ACE_Time_Value max_blocking_time(::DDS::DURATION_INFINITY_SEC);

int num_threads_to_write = 1;
int num_writes_per_thread = 1;
int multiple_instances = 0;
int num_datawriters = 1;
int block_on_write = 0;
int max_samples_per_instance = ::DDS::LENGTH_UNLIMITED;
int history_depth = 1;

/// parse the command line arguments
int parse_args (int argc, char *argv[])
{
  ACE_Arg_Shifter arg_shifter (argc, argv);
  
  while (arg_shifter.is_anything_left ()) 
  {
    // options:
    //  -t num_threads_to_write    defaults to 1 
    //  -i num_writes_per_thread   defaults to 1 
    //  -w num_datawriters         defaults to 1 
    //  -b block_on_write?1:0      defaults to 0 
    //  -m multiple_instances?1:0  defaults to 0
    //  -n max_samples_per_instance defaults to INFINITE
    //  -d history.depth           defaults to 1

    const char *currentArg = 0;
    
    if ((currentArg = arg_shifter.get_the_parameter("-t")) != 0) 
    {
      num_threads_to_write = ACE_OS::atoi (currentArg);
      arg_shifter.consume_arg ();
    }
    else if ((currentArg = arg_shifter.get_the_parameter("-b")) != 0) 
    {
      block_on_write = ACE_OS::atoi (currentArg);
      arg_shifter.consume_arg ();
    }
    else if ((currentArg = arg_shifter.get_the_parameter("-m")) != 0) 
    {
      multiple_instances = ACE_OS::atoi (currentArg);
      arg_shifter.consume_arg ();
    }
    else if ((currentArg = arg_shifter.get_the_parameter("-i")) != 0) 
    {
      num_writes_per_thread = ACE_OS::atoi (currentArg);
      arg_shifter.consume_arg ();
    }
    else if ((currentArg = arg_shifter.get_the_parameter("-w")) != 0) 
    {
      num_datawriters = ACE_OS::atoi (currentArg);
      arg_shifter.consume_arg ();
    }
    else if ((currentArg = arg_shifter.get_the_parameter("-n")) != 0) 
    {
      max_samples_per_instance = ACE_OS::atoi (currentArg);
      arg_shifter.consume_arg ();
    }
    else if ((currentArg = arg_shifter.get_the_parameter("-d")) != 0) 
    {
      history_depth = ACE_OS::atoi (currentArg);
      arg_shifter.consume_arg ();
    }
    else 
    {
      arg_shifter.ignore_arg ();
    }
  }
  // Indicates sucessful parsing of the command line
  return 0;
}


int main (int argc, char *argv[])
{
  ACE_TRY_NEW_ENV
    {
      parse_args (argc, argv);

      ::DDS::DomainParticipantFactory_var dpf = TheParticipantFactoryWithArgs(argc, argv);
      ACE_TRY_CHECK;

      ::Mine::FooTypeSupportImpl* fts_servant = new ::Mine::FooTypeSupportImpl();
      ::Mine::FooTypeSupport_var fts = 
        TAO::DCPS::servant_to_reference< ::Mine::FooTypeSupport,
                                         ::Mine::FooTypeSupportImpl, 
                                         ::Mine::FooTypeSupport_ptr >(fts_servant);
      ACE_TRY_CHECK;

      ::DDS::DomainParticipant_var dp = 
        dpf->create_participant(MY_DOMAIN, 
                                PARTICIPANT_QOS_DEFAULT, 
                                ::DDS::DomainParticipantListener::_nil() 
                                ACE_ENV_ARG_PARAMETER);
      ACE_TRY_CHECK;
      TEST_CHECK (! CORBA::is_nil (dp.in ()));

      if (::DDS::RETCODE_OK != fts->register_type(dp.in (), MY_TYPE))
        {
          ACE_ERROR ((LM_ERROR, 
            ACE_TEXT ("Failed to register the FooTypeSupport."))); 
          return 1;
        }

      ACE_TRY_CHECK;

      ::DDS::TopicQos topic_qos;
      dp->get_default_topic_qos(topic_qos);

      if (block_on_write)
      {
        topic_qos.reliability.kind  = ::DDS::RELIABLE_RELIABILITY_QOS;
        topic_qos.resource_limits.max_samples_per_instance = max_samples_per_instance;
        topic_qos.history.kind  = ::DDS::KEEP_ALL_HISTORY_QOS;
      }
      else
      {
        topic_qos.history.depth = history_depth;
      }

      ::DDS::Topic_var topic = 
        dp->create_topic (MY_TOPIC, 
                          MY_TYPE, 
                          topic_qos, 
                          ::DDS::TopicListener::_nil()
                          ACE_ENV_ARG_PARAMETER);
      ACE_TRY_CHECK;
      TEST_CHECK (! CORBA::is_nil (topic.in ()));

      
      ::DDS::Publisher_var pub =
        dp->create_publisher(PUBLISHER_QOS_DEFAULT,
                             ::DDS::PublisherListener::_nil()
                             ACE_ENV_ARG_PARAMETER);
      ACE_TRY_CHECK;
      TEST_CHECK (! CORBA::is_nil (pub.in ()));

      ::DDS::DataWriterQos dw_qos;
      pub->get_default_datawriter_qos (dw_qos);

      if (block_on_write)
      {
        dw_qos.reliability.kind  = ::DDS::RELIABLE_RELIABILITY_QOS;
        dw_qos.resource_limits.max_samples_per_instance = max_samples_per_instance;
        dw_qos.history.kind  = ::DDS::KEEP_ALL_HISTORY_QOS;
      }
      else
      {
        dw_qos.history.depth = history_depth;
      }

      ::DDS::DataWriter_var * dws = new ::DDS::DataWriter_var[num_datawriters];

      // Create one datawriter or multiple datawriters belong to the same 
      // publisher.
      for (int i = 0; i < num_datawriters; i ++)
      {
        dws[i] = pub->create_datawriter(topic.in (),
                                        dw_qos,
                                        ::DDS::DataWriterListener::_nil()
                                        ACE_ENV_ARG_PARAMETER);
        ACE_TRY_CHECK;
        TEST_CHECK (! CORBA::is_nil (dws[i].in ()));
      }

      Writer** writers = new Writer* [num_datawriters];

  { // make VC6 buid - avoid error C2374: 'i' : redefinition; multiple initialization
      // Each Writer/DataWriter launch threads to write samples
      // to the same instance or multiple instances. 
      // When writing to multiple instances, the instance key 
      // identifies instances is the thread id.
      for (int i = 0; i < num_datawriters; i ++)
      {
        writers[i] = new Writer(dws[i].in (), 
                                num_threads_to_write, 
                                num_writes_per_thread, 
                                multiple_instances,
                                i); 
        writers[i]->start ();
      }
  }
  { // make VC6 buid - avoid error C2374: 'i' : redefinition; multiple initialization

      // Record samples been written in the Writer's data map.
      // Verify the number of instances and the number of samples 
      // written to the datawriter.
      for (int i = 0; i < num_datawriters; i ++)
      {
        writers[i]->end ();
        InstanceDataMap& map = writers[i]->data_map ();
        if (multiple_instances == 0)
        {
          TEST_CHECK (map.num_instances() == 1);
        }
        else
        {
          TEST_CHECK (map.num_instances() == num_threads_to_write);
        }
        TEST_CHECK (map.num_samples() == num_threads_to_write * num_writes_per_thread);
      }
  }
      // Create the datareader to read the sample.
      ::DDS::Subscriber_var sub =
        dp->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
                              ::DDS::SubscriberListener::_nil()
                              ACE_ENV_ARG_PARAMETER);
      ACE_TRY_CHECK;
      TEST_CHECK (! CORBA::is_nil (sub.in ()));

      ::DDS::TopicDescription_var description =
        dp->lookup_topicdescription(MY_TOPIC ACE_ENV_ARG_PARAMETER);
      ACE_TRY_CHECK;
      TEST_CHECK (! CORBA::is_nil (description.in ())); 

      ::DDS::DataReaderQos dr_qos;
      sub->get_default_datareader_qos (dr_qos);

      if (block_on_write)
      {
        dr_qos.reliability.kind  = ::DDS::RELIABLE_RELIABILITY_QOS;
        dr_qos.resource_limits.max_samples_per_instance = max_samples_per_instance;
        dr_qos.history.kind  = ::DDS::KEEP_ALL_HISTORY_QOS;
      }

      ::DDS::DataReader_var dr =
        sub->create_datareader(description.in (),
                               dr_qos,
                               ::DDS::DataReaderListener::_nil()
                               ACE_ENV_ARG_PARAMETER);
      ACE_TRY_CHECK;
      TEST_CHECK (! CORBA::is_nil (dr.in ()));

      ::Mine::FooDataReader_var foo_dr 
        = ::Mine::FooDataReader::_narrow(dr.in () ACE_ENV_ARG_PARAMETER);
      TEST_CHECK (! CORBA::is_nil (foo_dr.in ()));

      int num_samples 
        = num_threads_to_write * num_writes_per_thread * num_datawriters;

  { // make VC6 buid - avoid error C2374: 'i' : redefinition; multiple initialization
      // Verify the delivered samples are the same as in the Writer's 
      // record.
      for (int i = 0; i < num_samples; i ++)
      {
        ::Xyz::Foo foo_read_data;
        foo_read_data.a_long_value = -22222; // This is the key of the Foo data.
        foo_read_data.handle_value = -1;
        foo_read_data.sample_sequence = -1;

        // Retrieve the delivered samples.

        ::DDS::SampleInfo si;
        foo_dr->read_next_sample(foo_read_data, si);
        ACE_TRY_CHECK;

        // Remove the delivered sample from the Writer's record.

        InstanceDataMap& map = writers[foo_read_data.writer_id]->data_map ();
        TEST_CHECK (map.remove (foo_read_data.handle_value, foo_read_data) == 0);
      }
  }

  { // make VC6 buid - avoid error C2374: 'i' : redefinition; multiple initialization

      // All delivered samples are removed from the Writer's record,
      // so the map should be empty.
      for (int i = 0; i < num_datawriters; i ++)
      {
        InstanceDataMap& map = writers[i]->data_map ();
        TEST_CHECK (map.is_empty () == true);
        pub->delete_datawriter(dws[i].in () ACE_ENV_ARG_PARAMETER);
      }
  }
      delete [] dws;
      delete [] writers;

      // clean up the objects
      dp->delete_publisher(pub.in () ACE_ENV_ARG_PARAMETER);

      sub->delete_datareader(dr.in () ACE_ENV_ARG_PARAMETER);
      dp->delete_subscriber(sub.in () ACE_ENV_ARG_PARAMETER);

      dp->delete_topic(topic.in () ACE_ENV_ARG_PARAMETER);
      dpf->delete_participant(dp.in () ACE_ENV_ARG_PARAMETER);

      TheServiceParticipant->shutdown (); 

    }
  ACE_CATCH (TestException,ex)
    {
      ACE_ERROR ((LM_ERROR,
                  ACE_TEXT("(%P|%t) TestException caught in main.cpp. ")));
      return 1;
    }
  ACE_CATCHANY
    {
      ACE_PRINT_EXCEPTION (ACE_ANY_EXCEPTION,
                           "Exception caught in main.cpp:");
      return 1;
    }
  ACE_ENDTRY;

  return 0;
}
