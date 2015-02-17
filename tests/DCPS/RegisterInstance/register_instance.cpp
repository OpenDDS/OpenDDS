#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/Marked_Default_Qos.h"

#include "tests/DCPS/FooType/FooTypeTypeSupportImpl.h"
#include "tests/DCPS/common/TestSupport.h"

#ifdef ACE_AS_STATIC_LIBS
#include "dds/DCPS/RTPS/RtpsDiscovery.h"
#include "dds/DCPS/transport/rtps_udp/RtpsUdp.h"
#endif

#include "ace/Arg_Shifter.h"

// const data declarations
const long  TEST_DOMAIN_NUMBER   = 51;
const char* TEST_TOPIC_NAME    = "foo-name";
const char* TEST_TYPE_NAME     = "foo-type";

Foo foo1;
Foo foo2;

int ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  try
    {

      ::DDS::DomainParticipantFactory_var dpf = TheParticipantFactoryWithArgs(argc, argv);

      FooTypeSupport_var fts (new FooTypeSupportImpl);

      ::DDS::DomainParticipant_var dp =
        dpf->create_participant(TEST_DOMAIN_NUMBER,
                                PARTICIPANT_QOS_DEFAULT,
                                ::DDS::DomainParticipantListener::_nil(),
                                ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      TEST_CHECK (! CORBA::is_nil (dp.in ()));

      if (::DDS::RETCODE_OK != fts->register_type(dp.in (), TEST_TYPE_NAME))
        {
          ACE_ERROR ((LM_ERROR,
            ACE_TEXT ("Failed to register the FooTypeSupport.")));
          return 1;
        }


      ::DDS::TopicQos topic_qos;
      dp->get_default_topic_qos(topic_qos);

      ::DDS::Topic_var topic =
        dp->create_topic (TEST_TOPIC_NAME,
                          TEST_TYPE_NAME,
                          topic_qos,
                          ::DDS::TopicListener::_nil(),
                          ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      TEST_CHECK (! CORBA::is_nil (topic.in ()));

      ::DDS::Publisher_var pub =
        dp->create_publisher(PUBLISHER_QOS_DEFAULT,
                             ::DDS::PublisherListener::_nil(),
                             ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);
      TEST_CHECK (! CORBA::is_nil (pub.in ()));

      ::DDS::DataWriterQos dw_qos;
      pub->get_default_datawriter_qos (dw_qos);

      dw_qos.reliability.kind  = ::DDS::RELIABLE_RELIABILITY_QOS;
      dw_qos.reliability.max_blocking_time.sec = 10;
      dw_qos.reliability.max_blocking_time.nanosec = 0;
      dw_qos.resource_limits.max_instances = 1;

      ::DDS::DataWriter_var dw = pub->create_datawriter(topic.in (),
                                 dw_qos,
                                 ::DDS::DataWriterListener::_nil(),
                                 ::OpenDDS::DCPS::DEFAULT_STATUS_MASK);

      TEST_CHECK (! CORBA::is_nil (dw.in ()));

      FooDataWriter_var foo_dw = FooDataWriter::_narrow(dw.in ());
      TEST_CHECK (! CORBA::is_nil (foo_dw.in ()));


      foo1.key = 101010;
      foo1.x = (float) 123.456;
      foo1.y = (float) 987.654;

      ::DDS::InstanceHandle_t handle1
        = foo_dw->register_instance(foo1);

      TEST_CHECK (handle1 != ::DDS::HANDLE_NIL);

      foo2.key = 202020;
      foo2.x = (float) 123.456;
      foo2.y = (float) 987.654;

      ::DDS::ReturnCode_t ret = foo_dw->write (foo2, ::DDS::HANDLE_NIL);

      // write an unregistered instance. This should fail
      // (spec 1.2 section 7.1.2.4.2.11)
      TEST_CHECK (ret == ::DDS::RETCODE_OUT_OF_RESOURCES);

      // Check whether the instance resides in DDS.
      ::DDS::InstanceHandle_t hnd = foo_dw->lookup_instance (foo2);

      TEST_CHECK (hnd == ::DDS::HANDLE_NIL);

      ::DDS::InstanceHandle_t handle2
        = foo_dw->register_instance(foo2);

      // Assume that this handle (handl2) is NIL since
      // max_instances is set to 1.
      TEST_CHECK (handle2 == ::DDS::HANDLE_NIL);

      // clean up the objects
      pub->delete_datawriter(dw.in ());
      dp->delete_publisher(pub.in ());

      dp->delete_topic(topic.in ());
      dpf->delete_participant(dp.in ());

      TheServiceParticipant->shutdown ();

    }
  catch (const CORBA::Exception& ex)
    {
      ex._tao_print_exception ("Exception caught in client.cpp:");
      return 1;
    }

  return 0;
}
