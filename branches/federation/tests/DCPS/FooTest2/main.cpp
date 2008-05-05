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



#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/Marked_Default_Qos.h"

#include "tests/DCPS/FooType2/FooTypeSupportImpl.h"


#include  "dds/DCPS/TopicDescriptionImpl.h"
#include  "dds/DCPS/Service_Participant.h"
#include  "ace/Arg_Shifter.h"


const long  MY_DOMAIN   = 411;
const char* MY_TOPIC    = "foo";
const char* MY_TYPE     = "foo";
const ACE_Time_Value max_blocking_time(::DDS::DURATION_INFINITY_SEC);
const int max_samples_per_instance = 2;

int num_thread_to_write = 1;
int block_on_write = 0;
Foo foo;

class Writer : public ACE_Task_Base
{
public:
  Writer (::DDS::DataWriter_ptr writer, int num_thread_to_write = 1)
    : writer_ (::DDS::DataWriter::_duplicate(writer)),
      num_thread_to_write_ (num_thread_to_write)
  {
  };

  void start ()
  {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) Writer::start \n")));
    if (activate (THR_NEW_LWP | THR_JOINABLE, num_thread_to_write_) == -1)
    {
      ACE_ERROR ((LM_ERROR,
                  ACE_TEXT("(%P|%t) Writer::start, ")
                  ACE_TEXT ("%p."),
                  "activate"));
      exit (1);
    }
  }

  void end ()
  {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) Writer::end \n")));
    wait ();
  }

  /** Lanch a thread to write. **/
  virtual int svc ()
  {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) Writer::svc \n")));

    try
    {
      FooDataWriter_var foo_dw = FooDataWriter::_narrow(writer_);
      TEST_CHECK (! CORBA::is_nil (foo_dw.in ()));

      foo.key = 101010;
      foo.x = (float) 123.456;
      foo.y = (float) 987.654;

      ::DDS::InstanceHandle_t handle
        = foo_dw->_cxx_register (foo);

      Foo key_holder;
      ::DDS::ReturnCode_t ret
        = foo_dw->get_key_value(key_holder, handle);

      TEST_CHECK(ret == ::DDS::RETCODE_OK);
      // check for equality
      TEST_CHECK (foo.key == key_holder.key);
      TEST_CHECK (foo.x == key_holder.x);
      TEST_CHECK (foo.y == key_holder.y);

      foo_dw->write(foo,
                    handle);

    }
    catch (const CORBA::Exception& ex)
    {
      ex._tao_print_exception ("Exception caught in svc:");
    }

    return 0;
  };

private:
  ::DDS::DataWriter_var writer_;
  int num_thread_to_write_;
};


/// parse the command line arguments
int parse_args (int argc, char *argv[])
{
  ACE_Arg_Shifter arg_shifter (argc, argv);

  while (arg_shifter.is_anything_left ())
  {
    const char *currentArg = 0;

    if ((currentArg = arg_shifter.get_the_parameter("-t")) != 0)
    {
      num_thread_to_write = ACE_OS::atoi (currentArg);
      arg_shifter.consume_arg ();
    }
    if ((currentArg = arg_shifter.get_the_parameter("-b")) != 0)
    {
      block_on_write = ACE_OS::atoi (currentArg);
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
  try
    {
      parse_args (argc, argv);

      ::DDS::DomainParticipantFactory_var dpf = TheParticipantFactoryWithArgs(argc, argv);

      FooTypeSupport_var fts (new FooTypeSupportImpl);

      ::DDS::DomainParticipant_var dp =
        dpf->create_participant(MY_DOMAIN,
                                PARTICIPANT_QOS_DEFAULT,
                                ::DDS::DomainParticipantListener::_nil());
      TEST_CHECK (! CORBA::is_nil (dp.in ()));

      if (::DDS::RETCODE_OK != fts->register_type(dp.in (), MY_TYPE))
        {
          ACE_ERROR ((LM_ERROR,
            ACE_TEXT ("Failed to register the FooTypeSupport.")));
          return 1;
        }


      ::DDS::TopicQos topic_qos;
      dp->get_default_topic_qos(topic_qos);

      if (block_on_write)
      {
        topic_qos.reliability.kind  = ::DDS::RELIABLE_RELIABILITY_QOS;
        topic_qos.resource_limits.max_samples_per_instance = max_samples_per_instance;
        topic_qos.history.kind  = ::DDS::KEEP_ALL_HISTORY_QOS;
      }

      ::DDS::Topic_var topic =
        dp->create_topic (MY_TOPIC,
                          MY_TYPE,
                          topic_qos,
                          ::DDS::TopicListener::_nil());
      TEST_CHECK (! CORBA::is_nil (topic.in ()));

      ::DDS::Publisher_var pub =
        dp->create_publisher(PUBLISHER_QOS_DEFAULT,
                             ::DDS::PublisherListener::_nil());
      TEST_CHECK (! CORBA::is_nil (pub.in ()));

      ::DDS::DataWriterQos dw_qos;
      pub->get_default_datawriter_qos (dw_qos);

      if (block_on_write)
      {
        dw_qos.reliability.kind  = ::DDS::RELIABLE_RELIABILITY_QOS;
        dw_qos.resource_limits.max_samples_per_instance = max_samples_per_instance;
        dw_qos.history.kind  = ::DDS::KEEP_ALL_HISTORY_QOS;
      }

      ::DDS::DataWriter_var dw = pub->create_datawriter(topic.in (),
                                 dw_qos,
                                 ::DDS::DataWriterListener::_nil());

      TEST_CHECK (! CORBA::is_nil (dw.in ()));

      Writer writer(dw, num_thread_to_write);
      writer.start ();
      writer.end ();

      ::DDS::Subscriber_var sub =
        dp->create_subscriber(SUBSCRIBER_QOS_DEFAULT,
                              ::DDS::SubscriberListener::_nil());
      TEST_CHECK (! CORBA::is_nil (sub.in ()));

      ::DDS::TopicDescription_var description =
        dp->lookup_topicdescription(MY_TOPIC);
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
                               ::DDS::DataReaderListener::_nil());
      TEST_CHECK (! CORBA::is_nil (dr.in ()));

      FooDataReader_var foo_dr = FooDataReader::_narrow(dr.in ());
      TEST_CHECK (! CORBA::is_nil (foo_dr.in ()));

      for (int i = 0; i < num_thread_to_write; i ++)
      {
        Foo foo_read_data;
        foo_read_data.key = 22222;
        foo_read_data.x = (float) -795.295;
        foo_read_data.y = (float) 56987.2;

        ::DDS::SampleInfo si;
        foo_dr->read_next_sample(foo_read_data, si);

        // check for equality
        TEST_CHECK (foo.key == foo_read_data.key);
        TEST_CHECK (foo.x == foo_read_data.x);
        TEST_CHECK (foo.y == foo_read_data.y);
      }

      // clean up the objects
      pub->delete_datawriter(dw.in ());
      dp->delete_publisher(pub.in ());

      sub->delete_datareader(dr.in ());
      dp->delete_subscriber(sub.in ());

      dp->delete_topic(topic.in ());
      dpf->delete_participant(dp.in ());

      TheServiceParticipant->shutdown ();

    }
  catch (const CORBA::Exception& ex)
    {
      ex._tao_print_exception ("Exception caught in main.cpp:");
      return 1;
    }

  return 0;
}
