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


#include "Reader.h"
#include "Writer.h"
#include "common.h"
#include "../common/TestException.h"
#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/Marked_Default_Qos.h"
#include "dds/DCPS/Qos_Helper.h"
#include "dds/DCPS/TopicDescriptionImpl.h"
#include "tests/DCPS/FooType4/FooTypeSupportImpl.h"

#include "ace/Arg_Shifter.h"

TAO::DCPS::TransportImpl_rch reader_transport_impl;
TAO::DCPS::TransportImpl_rch writer_transport_impl;

int max_samples_per_instance = 10 ;
int history_depth = 10 ;

ACE_Time_Value max_blocking_time(10);

/// parse the command line arguments
int parse_args (int argc, char *argv[])
{
  u_long mask =  ACE_LOG_MSG->priority_mask(ACE_Log_Msg::PROCESS) ;
  ACE_LOG_MSG->priority_mask(mask | LM_TRACE | LM_DEBUG, ACE_Log_Msg::PROCESS) ;
  ACE_Arg_Shifter arg_shifter (argc, argv);
  
  while (arg_shifter.is_anything_left ()) 
  {
    // options:
    //  -t use_take?1:0           defaults to 0 
    //  -i num_reads_per_thread   defaults to 1 
    //  -r num_datareaders         defaults to 1 
    //  -m multiple_instances?1:0  defaults to 0
    //  -n max_samples_per_instance defaults to INFINITE
    //  -d history.depth           defaults to 1

    const char *currentArg = 0;
    
    if ((currentArg = arg_shifter.get_the_parameter("-n")) != 0) 
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
      ::DDS::DomainParticipantFactory_var dpf = TheParticipantFactoryWithArgs(argc, argv);
      ACE_TRY_CHECK;

      // let the Service_Participant (in above line) strip out -DCPSxxx parameters
      // and then get application specific parameters.
      parse_args (argc, argv);


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
      if (CORBA::is_nil (dp.in ()))
      {
        ACE_ERROR ((LM_ERROR,
                   ACE_TEXT("(%P|%t) create_participant failed.\n")));
        return 1 ;
      }

      if (::DDS::RETCODE_OK != fts->register_type(dp.in (), MY_TYPE))
        {
          ACE_ERROR ((LM_ERROR, 
            ACE_TEXT ("Failed to register the FooTypeSupport."))); 
          return 1;
        }

      ACE_TRY_CHECK;

      TheTransportFactory->register_type(SIMPLE_TCP,
                                         new TAO::DCPS::SimpleTcpFactory());

      ::DDS::TopicQos topic_qos;
      dp->get_default_topic_qos(topic_qos);
      
      topic_qos.resource_limits.max_samples_per_instance =
            max_samples_per_instance ;

      topic_qos.history.depth = history_depth;

      ::DDS::Topic_var topic = 
        dp->create_topic (MY_TOPIC, 
                          MY_TYPE, 
                          topic_qos, 
                          ::DDS::TopicListener::_nil()
                          ACE_ENV_ARG_PARAMETER);
      ACE_TRY_CHECK;
      if (CORBA::is_nil (topic.in ()))
      {
        return 1 ;
      }
      
      Reader* reader ;
      Writer* writer  ;

      SampleInfoMap si_map ;
      ::DDS::SampleInfo si ;
        
      reader = new Reader(dp.in (), history_depth, max_samples_per_instance) ;
     
      ACE_OS::sleep(5) ; // why???

      writer = new Writer(dp.in (), topic.in (), history_depth, max_samples_per_instance) ;
     
      ACE_OS::sleep(5) ; // why???
      
      si.sample_state = ::DDS::NOT_READ_SAMPLE_STATE ;
      si.view_state = ::DDS::NEW_VIEW_STATE ;
      si.instance_state = ::DDS::ALIVE_INSTANCE_STATE;
      si.disposed_generation_count = 0 ;
      si.no_writers_generation_count = 0 ;
      si.sample_rank = 0 ;
      si.generation_rank = 0;
      si.absolute_generation_rank = 0;
      si_map['A'] = si ;

      writer->test1 ();
      reader->read (si_map);
     
      ACE_OS::sleep(1) ; // why???

      si_map['A'].sample_state = ::DDS::READ_SAMPLE_STATE ;
      si_map['A'].view_state = ::DDS::NOT_NEW_VIEW_STATE ;
      si_map['A'].instance_state = ::DDS::ALIVE_INSTANCE_STATE ;
      si_map['A'].disposed_generation_count = 0 ;
      si_map['A'].no_writers_generation_count = 0 ;
      si_map['A'].sample_rank = 1 ;
      si_map['A'].generation_rank = 0 ;
      si_map['A'].absolute_generation_rank = 0 ;

      si.sample_state = ::DDS::NOT_READ_SAMPLE_STATE ;
      si.view_state = ::DDS::NOT_NEW_VIEW_STATE ;
      si.instance_state = ::DDS::ALIVE_INSTANCE_STATE ;
      si.disposed_generation_count = 0 ;
      si.no_writers_generation_count = 0 ;
      si.sample_rank = 0 ;
      si.generation_rank = 0 ;
      si.absolute_generation_rank = 0 ;
      si_map['B'] = si ;

      si.sample_state = ::DDS::NOT_READ_SAMPLE_STATE ;
      si.view_state = ::DDS::NEW_VIEW_STATE ;
      si.instance_state = ::DDS::ALIVE_INSTANCE_STATE ;
      si.disposed_generation_count = 0 ;
      si.no_writers_generation_count = 0 ;
      si.sample_rank = 0 ;
      si.generation_rank = 0 ;
      si.absolute_generation_rank = 0 ;
      si_map['X'] = si ;

      si.sample_state =  ::DDS::NOT_READ_SAMPLE_STATE ;
      si.view_state = ::DDS::NEW_VIEW_STATE ;
      si.instance_state = ::DDS::ALIVE_INSTANCE_STATE ;
      si.disposed_generation_count = 0;
      si.no_writers_generation_count = 0;
      si.sample_rank = 0;
      si.generation_rank = 0;
      si.absolute_generation_rank = 0;
      si_map['Q'] = si ;
      
      writer->test2 ();
      ACE_OS::sleep(1) ;
      reader->read (si_map);
     
      ACE_OS::sleep(1) ; // why???

      si_map['A'].sample_state = ::DDS::READ_SAMPLE_STATE ;
      si_map['A'].view_state = ::DDS::NEW_VIEW_STATE ;
      si_map['A'].instance_state = ::DDS::ALIVE_INSTANCE_STATE ;
      si_map['A'].disposed_generation_count = 2;
      si_map['A'].no_writers_generation_count = 0;
      si_map['A'].sample_rank = 3;
      si_map['A'].generation_rank = 2;
      si_map['A'].absolute_generation_rank = 2;

      si_map['B'].sample_state = ::DDS::READ_SAMPLE_STATE ;
      si_map['B'].view_state = ::DDS::NOT_NEW_VIEW_STATE ;
      si_map['B'].instance_state = ::DDS::ALIVE_INSTANCE_STATE ;
      si_map['B'].disposed_generation_count = 2 ;
      si_map['B'].no_writers_generation_count = 0 ;
      si_map['B'].sample_rank = 2 ;
      si_map['B'].generation_rank = 2 ;
      si_map['B'].absolute_generation_rank = 2 ;

      si.sample_state = ::DDS::NOT_READ_SAMPLE_STATE ;
      si.view_state = ::DDS::NOT_NEW_VIEW_STATE ;
      si.instance_state = ::DDS::ALIVE_INSTANCE_STATE ;
      si.disposed_generation_count = 2 ;
      si.no_writers_generation_count = 0 ;
      si.sample_rank = 1 ;
      si.generation_rank = 1 ;
      si.absolute_generation_rank = 1 ;
      si_map['C'] = si ;

      si.sample_state = ::DDS::NOT_READ_SAMPLE_STATE ;
      si.view_state = ::DDS::NOT_NEW_VIEW_STATE ;
      si.instance_state  = ::DDS::ALIVE_INSTANCE_STATE ;
      si.disposed_generation_count = 2 ;
      si.no_writers_generation_count = 0 ;
      si.sample_rank = 0 ;
      si.generation_rank =  0 ;
      si.absolute_generation_rank = 0 ;
      si_map['D'] = si ;

      si_map['X'].sample_state = ::DDS::READ_SAMPLE_STATE ;
      si_map['X'].view_state = ::DDS::NOT_NEW_VIEW_STATE ;
      si_map['X'].instance_state = ::DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE ;
      si_map['X'].disposed_generation_count = 0 ;
      si_map['X'].no_writers_generation_count = 0 ;
      si_map['X'].sample_rank = 1 ;
      si_map['X'].generation_rank = 0 ;
      si_map['X'].absolute_generation_rank = 0 ;

      si.sample_state = ::DDS::NOT_READ_SAMPLE_STATE ;
      si.view_state = ::DDS::NOT_NEW_VIEW_STATE ;
      si.instance_state = ::DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE ;
      si.disposed_generation_count = 0 ;
      si.no_writers_generation_count = 0 ;
      si.sample_rank = 0 ;
      si.generation_rank = 0 ;
      si.absolute_generation_rank = 0 ;
      si_map['Y'] = si ;

      si_map['Q'].sample_state = ::DDS::READ_SAMPLE_STATE ;
      si_map['Q'].view_state = ::DDS::NOT_NEW_VIEW_STATE ;
      si_map['Q'].instance_state = ::DDS::ALIVE_INSTANCE_STATE ;
      si_map['Q'].disposed_generation_count = 0 ;
      si_map['Q'].no_writers_generation_count = 0 ;
      si_map['Q'].sample_rank = 0 ;
      si_map['Q'].generation_rank = 0 ;
      si_map['Q'].absolute_generation_rank = 0 ;
  
      writer->test3 ();
      ACE_OS::sleep(5) ;
      reader->read (si_map);

      delete writer;
      delete reader;

//---------------------------------------------------------------------
        
      reader = new Reader(dp.in (), history_depth, max_samples_per_instance) ;

      ACE_OS::sleep(5) ; // why???

      writer = new Writer(dp.in (), topic.in (), history_depth, max_samples_per_instance) ;
     
      ACE_OS::sleep(5) ; // why???

      si.sample_state = ::DDS::NOT_READ_SAMPLE_STATE ;
      si.view_state = ::DDS::NEW_VIEW_STATE ;
      si.instance_state = ::DDS::ALIVE_INSTANCE_STATE ;
      si.disposed_generation_count = 0 ;
      si.no_writers_generation_count = 0 ;
      si.sample_rank = 0 ;
      si.generation_rank = 0 ;
      si.absolute_generation_rank = 0 ;
      si_map['c'] = si ;
  
      writer->test4 ();
      ACE_OS::sleep(1) ;
      reader->read (si_map);

      ACE_OS::sleep(20) ;

      si_map['c'].sample_state = ::DDS::READ_SAMPLE_STATE ;
      si_map['c'].view_state = ::DDS::NOT_NEW_VIEW_STATE ;
      si_map['c'].instance_state = ::DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE ;
      si_map['c'].disposed_generation_count = 0 ;
      si_map['c'].no_writers_generation_count = 0 ;
      si_map['c'].sample_rank = 0 ;
      si_map['c'].generation_rank = 0 ;
      si_map['c'].absolute_generation_rank = 0 ;

      reader->read (si_map);

      si_map['c'].sample_state = ::DDS::READ_SAMPLE_STATE ;
      si_map['c'].view_state = ::DDS::NEW_VIEW_STATE ;
      si_map['c'].instance_state = ::DDS::ALIVE_INSTANCE_STATE ;
      si_map['c'].disposed_generation_count = 0 ;
      si_map['c'].no_writers_generation_count = 1 ;
      si_map['c'].sample_rank = 1 ;
      si_map['c'].generation_rank = 1 ;
      si_map['c'].absolute_generation_rank = 1 ;

      si.sample_state = ::DDS::NOT_READ_SAMPLE_STATE ;
      si.view_state = ::DDS::NOT_NEW_VIEW_STATE ;
      si.instance_state = ::DDS::ALIVE_INSTANCE_STATE ;
      si.disposed_generation_count = 0 ;
      si.no_writers_generation_count = 1 ;
      si.sample_rank = 0 ;
      si.generation_rank =  0 ;
      si.absolute_generation_rank = 0 ;
      si_map['d'] = si ;
     
      writer->test5 ();
      ACE_OS::sleep(1) ;
      reader->read (si_map);
     
      ACE_OS::sleep(1) ; // why???
                                    
      si_map['c'].sample_state = ::DDS::READ_SAMPLE_STATE ;
      si_map['c'].view_state = ::DDS::NOT_NEW_VIEW_STATE ;
      si_map['c'].instance_state = ::DDS::ALIVE_INSTANCE_STATE ;
      si_map['c'].disposed_generation_count = 0 ;
      si_map['c'].no_writers_generation_count = 1 ;
      si_map['c'].sample_rank = 2 ;
      si_map['c'].generation_rank = 1 ;
      si_map['c'].absolute_generation_rank = 1 ;

      si_map['d'].sample_state = ::DDS::READ_SAMPLE_STATE ;
      si_map['d'].view_state = ::DDS::NOT_NEW_VIEW_STATE ;
      si_map['d'].instance_state = ::DDS::ALIVE_INSTANCE_STATE ;
      si_map['d'].disposed_generation_count = 0 ;
      si_map['d'].no_writers_generation_count = 1 ;
      si_map['d'].sample_rank = 1 ;
      si_map['d'].generation_rank = 0 ;
      si_map['d'].absolute_generation_rank = 0 ;

      si.sample_state = ::DDS::NOT_READ_SAMPLE_STATE ;
      si.view_state = ::DDS::NOT_NEW_VIEW_STATE ;
      si.instance_state = ::DDS::ALIVE_INSTANCE_STATE ;
      si.disposed_generation_count = 0 ;
      si.no_writers_generation_count = 1 ;
      si.sample_rank = 0 ;
      si.generation_rank = 0 ;
      si.absolute_generation_rank = 0 ;
      si_map['e'] = si ;
 
      writer->test6 ();
      ACE_OS::sleep(1) ;
      reader->read (si_map);

      delete writer;
      delete reader;

//---------------------------------------------------------------------
        
      reader = new Reader(dp, 1, 1) ;
     
      ACE_OS::sleep(5) ; // why???

      writer = new Writer(dp, topic, history_depth, max_samples_per_instance) ;
     
      ACE_OS::sleep(5) ; // why???

      writer->test5 ();
      writer->test6 ();

      ACE_OS::sleep(1) ;
  
      si_map['d'].sample_state = ::DDS::NOT_READ_SAMPLE_STATE ;
      si_map['d'].view_state = ::DDS::NEW_VIEW_STATE ;
      si_map['d'].instance_state = ::DDS::ALIVE_INSTANCE_STATE ;
      si_map['d'].disposed_generation_count = 0 ;
      si_map['d'].no_writers_generation_count = 0 ;
      si_map['d'].sample_rank = 0 ;
      si_map['d'].generation_rank = 0 ;
      si_map['d'].absolute_generation_rank = 0 ;

      reader->read (si_map);

      delete writer;
      delete reader;

//---------------------------------------------------------------------

      dp->delete_topic(topic.in () ACE_ENV_ARG_PARAMETER);
      dpf->delete_participant(dp.in () ACE_ENV_ARG_PARAMETER);

      TheTransportFactory->release();
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
