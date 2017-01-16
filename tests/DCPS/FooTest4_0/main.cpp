// -*- C++ -*-
// ============================================================================
/**
 *  @file   main.cpp
 *
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
#include "tests/DCPS/FooType4/FooDefTypeSupportImpl.h"
#include "dds/DCPS/transport/framework/TransportImpl.h"

#include "dds/DCPS/StaticIncludes.h"

#include "ace/Arg_Shifter.h"
#include "ace/OS_NS_unistd.h"

OpenDDS::DCPS::TransportImpl_rch reader_transport_impl;
OpenDDS::DCPS::TransportImpl_rch writer_transport_impl;

int max_samples_per_instance = 10 ;
int history_depth = 10 ;

ACE_Time_Value max_blocking_time(10);

/// parse the command line arguments
int parse_args (int argc, ACE_TCHAR *argv[])
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

    const ACE_TCHAR *currentArg = 0;

    if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-n"))) != 0)
    {
      max_samples_per_instance = ACE_OS::atoi (currentArg);
      arg_shifter.consume_arg ();
    }
    else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-d"))) != 0)
    {
      history_depth = ACE_OS::atoi (currentArg);
      arg_shifter.consume_arg ();
    }
    else
    {
      arg_shifter.ignore_arg ();
    }
  }
  // Indicates successful parsing of the command line
  return 0;
}


int ACE_TMAIN (int argc, ACE_TCHAR *argv[])
{
  try
    {
      ::DDS::DomainParticipantFactory_var dpf = TheParticipantFactoryWithArgs(argc, argv);

      // let the Service_Participant (in above line) strip out -DCPSxxx parameters
      // and then get application specific parameters.
      parse_args (argc, argv);

      ::Xyz::FooTypeSupport_var fts (new ::Xyz::FooTypeSupportImpl);

      ::DDS::DomainParticipant_var dp =
        dpf->create_participant(MY_DOMAIN,
                                PARTICIPANT_QOS_DEFAULT,
                                ::DDS::DomainParticipantListener::_nil(),
                                ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
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


      ::DDS::TopicQos topic_qos;
      dp->get_default_topic_qos(topic_qos);

      topic_qos.resource_limits.max_samples_per_instance =
            max_samples_per_instance ;

      topic_qos.history.depth = history_depth;

      ::DDS::Topic_var topic =
        dp->create_topic (MY_TOPIC,
                          MY_TYPE,
                          topic_qos,
                          ::DDS::TopicListener::_nil(),
                          ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil (topic.in ()))
      {
        return 1 ;
      }

      Reader* reader ;
      Writer* writer  ;

      SampleInfoMap si_map ;
      ::DDS::SampleInfo si ={::DDS::NOT_READ_SAMPLE_STATE, ::DDS::NOT_NEW_VIEW_STATE
                             , ::DDS::NOT_ALIVE_NO_WRITERS_INSTANCE_STATE
                             , {0, 0}, ::DDS::HANDLE_NIL
                             , 0, 0, 0, 0, 0, ::DDS::HANDLE_NIL, false, 0};

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

      // Test1: write I1 A, reg I2
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

      // Test2: write I2 X, write I1 B, reg I3, write I3 Q
      writer->test2 ();
      ACE_OS::sleep(1) ;
      reader->read (si_map);

      ACE_OS::sleep(1) ; // why???

      si_map['A'].sample_state = ::DDS::READ_SAMPLE_STATE ;
      si_map['A'].view_state = ::DDS::NEW_VIEW_STATE ;
      si_map['A'].instance_state = ::DDS::ALIVE_INSTANCE_STATE ;
      si_map['A'].disposed_generation_count = 2;
      si_map['A'].no_writers_generation_count = 0;
      // two addition dispose "sample" after 'A'.
      si_map['A'].sample_rank = 3 + 2;
      si_map['A'].generation_rank = 2;
      si_map['A'].absolute_generation_rank = 2;

      si_map['B'].sample_state = ::DDS::READ_SAMPLE_STATE ;
      // The samples of the same instance returned by
      // a single read() should have same view state
      // so samples 'B','C','D' should have same view
      // state as 'A'.
      si_map['B'].view_state = ::DDS::NEW_VIEW_STATE ;
      si_map['B'].instance_state = ::DDS::ALIVE_INSTANCE_STATE ;
      si_map['B'].disposed_generation_count = 2 ;
      si_map['B'].no_writers_generation_count = 0 ;
      // two addition dispose "sample" after 'B'.
      si_map['B'].sample_rank = 2 + 2;
      si_map['B'].generation_rank = 2 ;
      si_map['B'].absolute_generation_rank = 2 ;

      si.sample_state = ::DDS::NOT_READ_SAMPLE_STATE ;
      si.view_state = ::DDS::NEW_VIEW_STATE ;
      si.instance_state = ::DDS::ALIVE_INSTANCE_STATE ;
      si.disposed_generation_count = 2 ;
      si.no_writers_generation_count = 0 ;

      // one addition dispose "sample" after.
      si.sample_rank = 1 + 1;
      // disposed_generation_count = 1 when receiving "C"
      // while disposed_generation_count = 0 when receiving "A" and "B"
      // so
      //   S.disposed_generation_count = 1
      //   MRSIC.disposed_generation_count = 2
      //   MRSIC.no_writers_generation_count = S.no_writers_generation_count = 0
      //
      //
      //   si.generation_rank =
      //    (MRSIC.disposed_generation_count + MRSIC.no_writers_generation_count)
      //     - (S.disposed_generation_count + S.no_writers_generation_count)
      si.generation_rank = 1 ;
      si.absolute_generation_rank = 1 ;
      si_map['C'] = si ;

      si.sample_state = ::DDS::NOT_READ_SAMPLE_STATE ;
      si.view_state = ::DDS::NEW_VIEW_STATE ;
      si.instance_state  = ::DDS::ALIVE_INSTANCE_STATE ;
      si.disposed_generation_count = 2 ;
      si.no_writers_generation_count = 0 ;
      // no addition dispose "sample"
      si.sample_rank = 0;
      si.generation_rank =  0 ;
      si.absolute_generation_rank = 0 ;
      si_map['D'] = si ;

      si_map['X'].sample_state = ::DDS::READ_SAMPLE_STATE ;
      si_map['X'].view_state = ::DDS::NOT_NEW_VIEW_STATE ;
      si_map['X'].instance_state = ::DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE ;
      // The disposed_generation_count will not be changed if the datareader
      // has not received any new samples after dispose.
      si_map['X'].disposed_generation_count = 0 ;
      si_map['X'].no_writers_generation_count = 0 ;
      // one addition dispose "sample"
      si_map['X'].sample_rank = 1 + 1;
      si_map['X'].generation_rank = 0 ;
      si_map['X'].absolute_generation_rank = 0 ;

      si.sample_state = ::DDS::NOT_READ_SAMPLE_STATE ;
      si.view_state = ::DDS::NOT_NEW_VIEW_STATE ;
      si.instance_state = ::DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE ;
      si.disposed_generation_count = 0 ;
      si.no_writers_generation_count = 0 ;
      // one addition dispose "sample"
      si.sample_rank = 0 + 1;
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

      //Test3: Dispose I1, write I1 C, dispose I1, write I1 D, write I2 Y, dispose I2.
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
      si.sample_rank = 0;
      si.generation_rank = 0 ;
      si.absolute_generation_rank = 0 ;
      si_map['c'] = si ;

      //Test4: write I1 c.
      writer->test4 ();
      ACE_OS::sleep(1) ;
      reader->read (si_map);

      // Sleep to make no writer so the next read will get the instance
      // with NOT_ALIVE_NO_WRITERS instance state.
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
      // The samples of the same instance returned by
      // a single read() should have same view state
      // so sample 'd' should have same view state as
      // sample 'c'.
      si.view_state = ::DDS::NEW_VIEW_STATE ;
      si.instance_state = ::DDS::ALIVE_INSTANCE_STATE ;
      si.disposed_generation_count = 0 ;
      si.no_writers_generation_count = 1 ;
      si.sample_rank = 0 ;
      si.generation_rank =  0 ;
      si.absolute_generation_rank = 0 ;
      si_map['d'] = si ;

      //Test5: write I1 d
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

      //Test6: write I1 e.
      writer->test6 ();
      ACE_OS::sleep(1) ;
      reader->read (si_map);

      delete writer;
      delete reader;

//---------------------------------------------------------------------

      reader = new Reader(dp.in (), 1, 1) ;

      ACE_OS::sleep(5) ; // why???

      writer = new Writer(dp.in (), topic.in (), history_depth, max_samples_per_instance) ;

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

      dp->delete_topic(topic.in ());
      dpf->delete_participant(dp.in ());

      writer_transport_impl.reset();
      reader_transport_impl.reset();

      TheServiceParticipant->shutdown ();

    }
  catch (const TestException&)
    {
      ACE_ERROR ((LM_ERROR,
                  ACE_TEXT("(%P|%t) TestException caught in main.cpp. ")));
      return 1;
    }
  catch (const CORBA::Exception& ex)
    {
      ex._tao_print_exception ("Exception caught in main.cpp:");
      return 1;
    }

  return 0;
}
