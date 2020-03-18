// -*- C++ -*-
// ============================================================================
/**
 *  @file   main.cpp
 *
 *
 *  Test of view state.
 */
// ============================================================================


#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/Marked_Default_Qos.h"
#include "dds/DCPS/Qos_Helper.h"
#include "dds/DCPS/TopicDescriptionImpl.h"
#include "dds/DCPS/SubscriberImpl.h"
#include "dds/DCPS/PublisherImpl.h"
#include "SimpleTypeSupportImpl.h"
#include "dds/DCPS/transport/framework/EntryExit.h"

#include "dds/DCPS/StaticIncludes.h"

#include "ace/Arg_Shifter.h"
#include "ace/OS_NS_unistd.h"

#include <string>

class TestException
{
  public:

    TestException()  {}
    ~TestException() {}
};


const long  MY_DOMAIN   = 111;
const char* MY_TOPIC    = "foo";
const char* MY_TYPE     = "foo";

const ACE_Time_Value max_blocking_time(::DDS::DURATION_INFINITE_SEC);

int max_samples_per_instance = ::DDS::LENGTH_UNLIMITED;
int history_depth = 10 ;

int test_failed = 0;

int wait_for_data (::DDS::Subscriber_ptr sub,
                   int timeout_sec)
{
  const int factor = 10;
  ACE_Time_Value small_time(0,1000000/factor);
  int timeout_loops = timeout_sec * factor;

  ::DDS::DataReaderSeq_var discard = new ::DDS::DataReaderSeq(10);
  while (timeout_loops-- > 0)
    {
      sub->get_datareaders (
                    discard.inout (),
                    ::DDS::NOT_READ_SAMPLE_STATE,
                    ::DDS::ANY_VIEW_STATE,
                    ::DDS::ANY_INSTANCE_STATE );
      if (discard->length () > 0)
        return 1;

      ACE_OS::sleep (small_time);
    }
  return 0;
}

/// parse the command line arguments
int parse_args(int argc, ACE_TCHAR *argv[])
{

  u_long mask = ACE_LOG_MSG->priority_mask(ACE_Log_Msg::PROCESS);
  ACE_LOG_MSG->priority_mask(mask | LM_TRACE | LM_DEBUG, ACE_Log_Msg::PROCESS);
  ACE_Arg_Shifter arg_shifter(argc, argv);

  while (arg_shifter.is_anything_left())
  {
    // options:
    //  -n max_samples_per_instance defaults to INFINITE
    //  -d history.depth            defaults to 1
    //  -z                          verbose transport debug

    if (arg_shifter.cur_arg_strncasecmp(ACE_TEXT("-z")) == 0)
    {
      TURN_ON_VERBOSE_DEBUG;
      arg_shifter.consume_arg();
    }
    else
    {
      arg_shifter.ignore_arg ();
    }
  }
  // Indicates successful parsing of the command line
  return 0;
}

void check_read_status(DDS::ReturnCode_t status,
                       const Test::SimpleSeq& data,
                       CORBA::ULong expected,
                       const char* where)
{
  if (status == ::DDS::RETCODE_OK)
  {
      if (data.length() != expected)
      {
          ACE_ERROR ((LM_ERROR,
              ACE_TEXT("(%P|%t) %C ERROR: expected %d samples but got %d\n"),
              where, expected, data.length() ));
          test_failed = 1;
          throw TestException();
      }

  }
  else if (status == ::DDS::RETCODE_NO_DATA)
  {
    ACE_ERROR ((LM_ERROR,
      ACE_TEXT("(%P|%t) %C ERROR: reader received NO_DATA!\n"),
      where));
    test_failed = 1;
    throw TestException();
  }
  else if (status == ::DDS::RETCODE_PRECONDITION_NOT_MET)
  {
    ACE_ERROR ((LM_ERROR,
      ACE_TEXT("(%P|%t) %C ERROR: reader received PRECONDITION_NOT_MET!\n"),
      where));
    test_failed = 1;
    throw TestException();
  }
  else
  {
    ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) %C ERROR: unexpected status %d!\n"),
        where, status ));
    test_failed = 1;
    throw TestException();
  }
}


int ACE_TMAIN (int argc, ACE_TCHAR *argv[])
{
  u_long mask = ACE_LOG_MSG->priority_mask(ACE_Log_Msg::PROCESS);
  ACE_LOG_MSG->priority_mask(mask | LM_TRACE | LM_DEBUG, ACE_Log_Msg::PROCESS);

  ACE_DEBUG((LM_DEBUG,"(%P|%t) view_state test main\n"));
  try
    {
      ::DDS::DomainParticipantFactory_var dpf = TheParticipantFactoryWithArgs(argc, argv);
      if (CORBA::is_nil (dpf.in ()))
      {
        ACE_ERROR ((LM_ERROR,
                   ACE_TEXT("(%P|%t) creating the DomainParticipantFactory failed.\n")));
        return 1 ;
      }

      // let the Service_Participant (in above line) strip out -DCPSxxx parameters
      // and then get application specific parameters.
      parse_args (argc, argv);

      { //xxx dp scope
      ::DDS::DomainParticipant_var dp =
        dpf->create_participant(MY_DOMAIN,
                                PARTICIPANT_QOS_DEFAULT,
                                ::DDS::DomainParticipantListener::_nil(),
                                ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      //xxx obj rc = 3
      if (CORBA::is_nil (dp.in ()))
      {
        ACE_ERROR ((LM_ERROR,
                   ACE_TEXT("(%P|%t) create_participant failed.\n")));
        return 1 ;
      }

      Test::SimpleTypeSupport_var fts = new Test::SimpleTypeSupportImpl();

      if (::DDS::RETCODE_OK != fts->register_type(dp.in (), MY_TYPE))
        {
          ACE_ERROR ((LM_ERROR,
            ACE_TEXT ("Failed to register the SimpleTypeSupport.")));
          return 1;
        }


      ::DDS::TopicQos topic_qos;
      dp->get_default_topic_qos(topic_qos);

      topic_qos.history.kind = ::DDS::KEEP_LAST_HISTORY_QOS;
      topic_qos.history.depth = history_depth;
      topic_qos.resource_limits.max_samples_per_instance =
            max_samples_per_instance ;


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

      ::DDS::TopicDescription_var description =
        dp->lookup_topicdescription(MY_TOPIC);
      if (CORBA::is_nil (description.in ()))
      {
        ACE_ERROR_RETURN ((LM_ERROR,
                           ACE_TEXT("(%P|%t) lookup_topicdescription failed.\n")),
                           1);
      }



      // Create the subscriber
      ::DDS::Subscriber_var sub =
        dp->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
                             ::DDS::SubscriberListener::_nil(),
                             ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil (sub.in ()))
      {
        ACE_ERROR_RETURN ((LM_ERROR,
                           ACE_TEXT("(%P|%t) create_subscriber failed.\n")),
                           1);
      }

      // Create the publisher
      ::DDS::Publisher_var pub =
        dp->create_publisher(PUBLISHER_QOS_DEFAULT,
                             ::DDS::PublisherListener::_nil(),
                             ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      if (CORBA::is_nil (pub.in ()))
      {
        ACE_ERROR_RETURN ((LM_ERROR,
                          ACE_TEXT("(%P|%t) create_publisher failed.\n")),
                          1);
      }

      // Create the datawriter
      ::DDS::DataWriterQos dw_qos;
      pub->get_default_datawriter_qos (dw_qos);

      dw_qos.history.kind = ::DDS::KEEP_LAST_HISTORY_QOS;
      dw_qos.history.depth = history_depth  ;
      dw_qos.resource_limits.max_samples_per_instance =
            max_samples_per_instance ;
      dw_qos.liveliness.kind = ::DDS::AUTOMATIC_LIVELINESS_QOS;

      ::DDS::DataWriter_var dw = pub->create_datawriter(topic.in (),
                                        dw_qos,
                                        ::DDS::DataWriterListener::_nil(),
                                        ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

      if (CORBA::is_nil (dw.in ()))
      {
        ACE_ERROR ((LM_ERROR,
          ACE_TEXT("(%P|%t) create_datawriter failed.\n")));
        return 1 ;
      }

      // Create the Datareader
      ::DDS::DataReaderQos dr_qos;
      sub->get_default_datareader_qos (dr_qos);

      dr_qos.history.kind = ::DDS::KEEP_LAST_HISTORY_QOS;
      dr_qos.history.depth = history_depth  ;
      dr_qos.resource_limits.max_samples_per_instance =
            max_samples_per_instance ;

      ::DDS::DataReader_var dr
        = sub->create_datareader(description.in (),
                                 dr_qos,
                                 ::DDS::DataReaderListener::_nil(),
                                 ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

      if (CORBA::is_nil (dr.in ()))
        {
          ACE_ERROR_RETURN ((LM_ERROR,
            ACE_TEXT("(%P|%t) create_datareader failed.\n")),
            1);
        }

      Test::SimpleDataWriter_var foo_dw
        = Test::SimpleDataWriter::_narrow(dw.in ());
      if (CORBA::is_nil (foo_dw.in ()))
      {
        ACE_ERROR ((LM_ERROR,
          ACE_TEXT("(%P|%t) Test::SimpleDataWriter::_narrow failed.\n")));
        return 1; // failure
      }

      Test::SimpleDataReader_var foo_dr
        = Test::SimpleDataReader::_narrow(dr.in ());
      if (CORBA::is_nil (foo_dr.in ()))
      {
        ACE_ERROR ((LM_ERROR,
          ACE_TEXT("(%P|%t) Test::SimpleDataReader::_narrow failed.\n")));
        return 1; // failure
      }

      // wait for association establishement before writing.
      // -- replaced this sleep with the while loop below;
      //    waiting on the one association we expect.
      //  ACE_OS::sleep(5); //REMOVE if not needed
      ::DDS::InstanceHandleSeq handles;
      while (1)
      {
          foo_dw->get_matched_subscriptions(handles);
          if (handles.length() > 0)
              break;
          else
              ACE_OS::sleep(ACE_Time_Value(0,200000));
      }

      try { // the real testing.
          //=====================================================
          // show view state change in a single generation and
          // samples of an instance returned from a read/take
          // have same view state
          //=====================================================
          ACE_DEBUG((LM_INFO,"==== TEST case 1 : show view state change in a single generation.\n"));

          CORBA::Long max_samples = 5;

          Test::SimpleSeq     data1 (max_samples);
          ::DDS::SampleInfoSeq info1;

          ::Test::Simple foo1;
          foo1.key  = 1;
          foo1.count = 1;

          // register the data so we can use the handle.
          ::DDS::InstanceHandle_t handle
            = foo_dw->register_instance(foo1);

          // write first sample
          foo_dw->write(foo1, handle);

          // wait for write to propagate
          if (!wait_for_data(sub.in (), 5))
            ACE_ERROR_RETURN ((LM_ERROR,
            ACE_TEXT("(%P|%t) ERROR: timeout waiting for data.\n")),
            1);

          // read should return one sample with view state NEW.
          DDS::ReturnCode_t status  ;
          status = foo_dr->read(  data1
            , info1
            , max_samples
            , ::DDS::ANY_SAMPLE_STATE
            , ::DDS::ANY_VIEW_STATE
            , ::DDS::ANY_INSTANCE_STATE );


          check_read_status(status, data1, 1, "first read");

          if (info1[0].view_state != ::DDS::NEW_VIEW_STATE)
          {
            ACE_ERROR ((LM_ERROR,
              ACE_TEXT("(%P|%t) ERROR: expected NEW view state.\n") ));
            test_failed = 1;
          }

          if (data1[0].count != foo1.count)
          {
            // test to see the accessing the "lost" (because of history.depth)
            // but still held by zero-copy sequence value works.
            ACE_ERROR ((LM_ERROR,
              ACE_TEXT("(%P|%t) ERROR: unexpected value for data1.\n") ));
            test_failed = 1;

          }

          // since depth=10 the previous sample will be kept
          // in the instance container.
          ::Test::Simple foo2;
          foo2.key  = 1;
          foo2.count = 2;

          // write second sample
          foo_dw->write(foo2, handle);

          // wait for write to propagate
          if (!wait_for_data(sub.in (), 5))
            ACE_ERROR_RETURN ((LM_ERROR,
            ACE_TEXT("(%P|%t) ERROR: timeout waiting for data.\n")),
            1);

          // Read should return two samples with view state NOT NEW since
          // we read previously and the instance is not reborn.
          Test::SimpleSeq     data2 (max_samples);
          ::DDS::SampleInfoSeq info2;
          status = foo_dr->read(  data2
            , info2
            , max_samples
            , ::DDS::ANY_SAMPLE_STATE
            , ::DDS::ANY_VIEW_STATE
            , ::DDS::ANY_INSTANCE_STATE );

          check_read_status(status, data2, 2, "second read");

          if (info2[0].view_state != ::DDS::NOT_NEW_VIEW_STATE
            || info2[1].view_state != ::DDS::NOT_NEW_VIEW_STATE)
          {
            ACE_ERROR ((LM_ERROR,
              ACE_TEXT("(%P|%t) ERROR: expected NOT NEW view state.\n") ));
            test_failed = 1;
          }

          if (data2[0].count != foo1.count || data2[1].count != foo2.count)
          {
            ACE_ERROR ((LM_ERROR,
              ACE_TEXT("(%P|%t) ERROR: second read failed to provide same data.\n") ));
            test_failed = 1;
          }

          //=====================================================
          // show view state change in multiple generations and
          // samples of an instance returned from a read/take
          // have same view state.
          //=====================================================
          ACE_DEBUG((LM_INFO,"==== TEST case 2 : show view state change in multiple generations.\n"));

          // The datareader will receive the DISPOSE message and change the
          // instance state from ALIVE to NOT ALIVE.
          // The dispose sample is an invalid data sample which is used for notification.
          // Upon receiving dispose, a NEW sample is created which marks InstanceState
          // has NOT_READ_SAMPLE, but won't change the view state. The view state is
          // changed to NEW_VIEW when data sample is received.
          foo_dw->dispose(foo1, handle);

          //// wait for dispose sample to propagate
          if (!wait_for_data(sub.in (), 5))
            ACE_ERROR_RETURN ((LM_ERROR,
            ACE_TEXT("(%P|%t) ERROR: timeout waiting for data.\n")),
            1);

          // Read the dispose sample so the view state be NOT_NEW and
          // sample state be READ before next write/read
          max_samples = 1;

          Test::SimpleSeq     disposeData (max_samples);
          ::DDS::SampleInfoSeq disposeDataInfo;

          status = foo_dr->read(disposeData
            , disposeDataInfo
            , max_samples
            , ::DDS::NOT_READ_SAMPLE_STATE
            , ::DDS::ANY_VIEW_STATE
            , ::DDS::ANY_INSTANCE_STATE );

          // verify dispose sample
          if (disposeDataInfo[0].valid_data == 1
            || disposeDataInfo[0].instance_state != DDS::NOT_ALIVE_DISPOSED_INSTANCE_STATE)
            {
              ACE_ERROR ((LM_ERROR,
                ACE_TEXT("(%P|%t) ERROR: failed to verify DISPOSE sample.\n") ));
              test_failed = 1;
            }

          // Write sample after dispose. The datareader will change the
          // view state from NOT_NEW to NEW.
          ::Test::Simple foo3;
          foo3.key  = 1;
          foo3.count = 3;

          foo_dw->write (foo3, handle);

          //// wait for write to propagate
          if (!wait_for_data(sub.in (), 5))
            ACE_ERROR_RETURN ((LM_ERROR,
            ACE_TEXT("(%P|%t) ERROR: timeout waiting for data.\n")),
            1);

          // Read maximum 2 samples which should be from the old
          // generation (before dispose). The view state should not
          // be changed during this read. The view state should stay
          // NEW.
          max_samples = 2;

          Test::SimpleSeq     data3 (max_samples);
          ::DDS::SampleInfoSeq info3;
          status = foo_dr->read(  data3
            , info3
            , max_samples
            , ::DDS::ANY_SAMPLE_STATE
            , ::DDS::ANY_VIEW_STATE
            , ::DDS::ANY_INSTANCE_STATE );

          check_read_status(status, data3, 2, "read samples in old generation");

          if (info3[0].view_state != ::DDS::NEW_VIEW_STATE
            || info3[1].view_state != ::DDS::NEW_VIEW_STATE)
          {
            ACE_ERROR ((LM_ERROR,
              ACE_TEXT("(%P|%t) ERROR: expected NEW view state.\n") ));
            test_failed = 1;
          }

          if (data3[0].count != foo1.count || data3[1].count != foo2.count)
          {
            ACE_ERROR ((LM_ERROR,
              ACE_TEXT("(%P|%t) ERROR: old generation sample read failed to provide same data.\n") ));
            test_failed = 1;
          }

          // Extend max_sample to receive all samples from the instance.
          // This time read() should return 3 samples. The first and second
          // samples are from old generation and the third is from most
          // recent generation. These samples have NEW view state. Since
          // this read() reads samples from most recent generation then
          // the view state should be changed from NOT NEW.
          max_samples = 5;

          Test::SimpleSeq     data4 (max_samples);
          ::DDS::SampleInfoSeq info4;
          status = foo_dr->read(  data4
            , info4
            , max_samples
            , ::DDS::ANY_SAMPLE_STATE
            , ::DDS::ANY_VIEW_STATE
            , ::DDS::ANY_INSTANCE_STATE );

          check_read_status(status, data4, 4, "read samples from most recent generation");

          if (info4[0].view_state != ::DDS::NEW_VIEW_STATE
            || info4[1].view_state != ::DDS::NEW_VIEW_STATE
            || info4[3].view_state != ::DDS::NEW_VIEW_STATE)
          {
            ACE_ERROR ((LM_ERROR,
              ACE_TEXT("(%P|%t) ERROR: expected NEW view state.\n") ));
            test_failed = 1;
          }

          if (data4[0].count != foo1.count || data4[1].count != foo2.count
            || data4[3].count != foo3.count)
          {
            ACE_ERROR ((LM_ERROR,
              ACE_TEXT("(%P|%t) ERROR: read samples from most recent generation")
              ACE_TEXT(" failed to provide same data.\n") ));
            test_failed = 1;
          }

          if (info4[2].valid_data == 1)
          {
            ACE_ERROR ((LM_ERROR,
              ACE_TEXT("(%P|%t) ERROR: expected an invalid data sample (dispose notification), instance state is %C.\n"),
              OpenDDS::DCPS::InstanceState::instance_state_string(
                info4[2].instance_state)));
            test_failed = 1;
          }

          // Read again should get all samples in datareader data container.
          // The view state of the samples should be NOT NEW and this read()
          // will not change it.

          Test::SimpleSeq     data5 (max_samples);
          ::DDS::SampleInfoSeq info5;
          status = foo_dr->read(  data5
            , info5
            , max_samples
            , ::DDS::ANY_SAMPLE_STATE
            , ::DDS::ANY_VIEW_STATE
            , ::DDS::ANY_INSTANCE_STATE );


          check_read_status(status, data5, 4, "read after read most recent generation samples");

          if (info5[0].view_state != ::DDS::NOT_NEW_VIEW_STATE
            || info5[1].view_state != ::DDS::NOT_NEW_VIEW_STATE
            || info5[3].view_state != ::DDS::NOT_NEW_VIEW_STATE)
          {
            ACE_ERROR ((LM_ERROR,
              ACE_TEXT("(%P|%t) ERROR: expected NOT NEW view state.\n") ));
            test_failed = 1;
          }

          if (data5[0].count != foo1.count || data5[1].count != foo2.count
            || data5[3].count != foo3.count)
          {
            ACE_ERROR ((LM_ERROR,
              ACE_TEXT("(%P|%t) ERROR: read after read most recent generation samples ")
              ACE_TEXT("failed to provide same data.\n") ));
            test_failed = 1;
          }

          //=====================================================
          // show view state belongs to an instance and not be
          // affected by other instances in same reader.
          //=====================================================
          ACE_DEBUG((LM_INFO,"==== TEST case 3 : show view state and instance relation.\n"));

          // create another sample belong to a different instance.
          ::Test::Simple xfoo;
          xfoo.key  = 888;         // different key/instance.
          xfoo.count = 999;

          //Since it's a different instance, can not use previous handle.
          //Let it automatically register.
          foo_dw->write (xfoo, ::DDS::HANDLE_NIL);

          //// wait for write to propagate
          if (!wait_for_data(sub.in (), 5))
            ACE_ERROR_RETURN ((LM_ERROR,
            ACE_TEXT("(%P|%t) ERROR: timeout waiting for data.\n")),
            1);

          Test::SimpleSeq     data6 (max_samples);
          ::DDS::SampleInfoSeq info6;

          // read should return one sample with view state NEW.
          status = foo_dr->read(  data6
            , info6
            , max_samples
            , ::DDS::ANY_SAMPLE_STATE
            , ::DDS::ANY_VIEW_STATE
            , ::DDS::ANY_INSTANCE_STATE );


          check_read_status(status, data6, 5, "first read");

          if (info6[0].view_state != ::DDS::NOT_NEW_VIEW_STATE
            || info6[1].view_state != ::DDS::NOT_NEW_VIEW_STATE
            || info6[3].view_state != ::DDS::NOT_NEW_VIEW_STATE
            || info6[4].view_state != ::DDS::NEW_VIEW_STATE)
          {
            ACE_ERROR ((LM_ERROR,
              ACE_TEXT("(%P|%t) ERROR: un expected view state.\n") ));
            test_failed = 1;
          }

          if (data6[0].count != foo1.count || data6[1].count != foo2.count
            || data6[3].count != foo3.count || data6[4].count != xfoo.count)
          {
            // test to see the accessing the "lost" (because of history.depth)
            // but still held by zero-copy sequence value works.
            ACE_ERROR ((LM_ERROR,
              ACE_TEXT("(%P|%t) ERROR: unexpected value for data1.\n") ));
            test_failed = 1;
          }
      }//test scope
      catch (const TestException&)
      {
        ACE_ERROR ((LM_ERROR,
          ACE_TEXT("(%P|%t) TestException caught in main.cpp. ")));
        return 1;
      }
      //======== clean up ============

      // clean up common objects
      dp->delete_contained_entities();
      dpf->delete_participant(dp.in ());

      }//dp scope

      TheServiceParticipant->shutdown();

    } //xxx dp::Entity::Object::muxtex_refcount_ = 1
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

  return test_failed;
}
