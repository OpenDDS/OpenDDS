#include "PubDriver.h"
#include "TestException.h"
#include "tests/DCPS/FooType3/FooTypeSupportC.h"
#include "tests/DCPS/FooType3/FooTypeSupportImpl.h"
#include "tests/DCPS/FooType3/FooDefC.h"
#include "dds/DCPS/transport/framework/TheTransportFactory.h"
#include "dds/DCPS/transport/simpleTCP/SimpleTcpConfiguration.h"
#include "dds/DCPS/transport/framework/NetworkAddress.h"
#include "dds/DCPS/AssociationData.h"
#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/PublisherImpl.h"
#include "dds/DCPS/Marked_Default_Qos.h"
#include "dds/DCPS/Qos_Helper.h"
#include "dds/DCPS/Service_Participant.h"
#include "dds/DCPS/Transient_Kludge.h"
#include "DomainParticipantListener.h"
#include "DataWriterListener.h"
#include "PublisherListener.h"
#include "tests/DCPS/common/TestSupport.h"

#include <ace/Arg_Shifter.h>
#include <string>

const long  MY_DOMAIN   = 411;
const char* MY_TOPIC    = "foo";
const char* MY_TYPE     = "foo";

long lease_duration_sec = 5;

using namespace ::TAO::DCPS;

int offered_incompatible_qos_called_on_dp = 0;
int offered_incompatible_qos_called_on_pub = 0;
int offered_incompatible_qos_called_on_dw = 0;


PubDriver::PubDriver()
: publisher_servant_ (0),
  datawriter_servant_ (0),
  foo_datawriter_servant_ (0),
  pub_id_fname_ ("pub_id.txt"),
  sub_id_ (0),
  history_depth_ (1),
  test_to_run_ (REGISTER_TEST),
  pub_driver_ior_ ("pubdriver.ior"),
  add_new_subscription_ (0),
  shutdown_ (0)
{
}


PubDriver::~PubDriver()
{
}


void
PubDriver::run(int& argc, char* argv[])
{
  parse_args(argc, argv);
  initialize(argc, argv);

  run();

  while (shutdown_ == 0)
  {
    if (add_new_subscription_ == 1)
    {
      TheTransientKludge->enable ();

      add_subscription (sub_id_, sub_addr_.c_str ());
      add_new_subscription_ = 0;
    }
    ACE_OS::sleep (1);
  }

  end ();
}

void
PubDriver::parse_args(int& argc, char* argv[])
{
  // Command-line arguments:
  //
  //  -p <pub_id_fname:pub_host:pub_port>
  //  -s <sub_id:sub_host:sub_port>
  //  -d history.depth             >=1  defaults to 1
  //  -t test_number
  //     0 - register test
  //     1 - unregister test
  //     2 - dispose test
  //     3 - resume test
  //     4 - listener test
  //     5 - allocator test
  ACE_Arg_Shifter arg_shifter(argc, argv);

  bool got_p = false;
  bool got_s = false;

  const char* current_arg = 0;

  while (arg_shifter.is_anything_left())
  {
    // The '-p' option
    if ((current_arg = arg_shifter.get_the_parameter("-p"))) {
      if (got_p) {
        ACE_ERROR((LM_ERROR,
                   "(%P|%t) Only one -p allowed on command-line.\n"));
        throw TestException();
      }

      int result = parse_pub_arg(current_arg);
      arg_shifter.consume_arg();

      if (result != 0) {
        ACE_ERROR((LM_ERROR,
                   "(%P|%t) Failed to parse -p command-line arg.\n"));
        throw TestException();
      }

      got_p = true;
    }
    // A '-s' option
    else if ((current_arg = arg_shifter.get_the_parameter("-s"))) {
      if (got_s) {
        ACE_ERROR((LM_ERROR,
                   "(%P|%t) Only one -s allowed on command-line.\n"));
        throw TestException();
      }

      int result = parse_sub_arg(current_arg);
      arg_shifter.consume_arg();

      if (result != 0) {
        ACE_ERROR((LM_ERROR,
                   "(%P|%t) Failed to parse -s command-line arg.\n"));
        throw TestException();
      }

      got_s = true;
    }
    else if (arg_shifter.cur_arg_strncasecmp("-DCPS") != -1)
    {
      // ignore -DCPSxxx options that will be handled by Service_Participant
      arg_shifter.ignore_arg();
    }
    else if ((current_arg = arg_shifter.get_the_parameter("-d")) != 0)
    {
      history_depth_ = ACE_OS::atoi (current_arg);
      arg_shifter.consume_arg ();
    }
    else if ((current_arg = arg_shifter.get_the_parameter("-t")) != 0)
    {
      test_to_run_ = ACE_OS::atoi (current_arg);
      arg_shifter.consume_arg ();
    }
    else if ((current_arg = arg_shifter.get_the_parameter("-v")) != 0)
    {
      pub_driver_ior_ = current_arg;
      arg_shifter.consume_arg ();
    }
    // The '-?' option
    else if (arg_shifter.cur_arg_strncasecmp("-?") == 0) {
      ACE_DEBUG((LM_DEBUG,
                 "usage: %s "
                 "-p pub_id:pub_host:pub_port -s sub_id:sub_host:sub_port\n",
                 argv[0]));

      arg_shifter.consume_arg();
      throw TestException();
    }
    // Anything else we just skip
    else {
      arg_shifter.ignore_arg();
    }
  }

  // Make sure we got the required arguments:
  if (!got_p) {
    ACE_ERROR((LM_ERROR,
               "(%P|%t) -p command-line option not specified (required).\n"));
    throw TestException();
  }

  if (!got_s) {
    ACE_ERROR((LM_ERROR,
               "(%P|%t) -s command-line option not specified (required).\n"));
    throw TestException();
  }
}


void
PubDriver::initialize(int& argc, char *argv[])
{
  ::DDS::DomainParticipantFactory_var dpf = TheParticipantFactoryWithArgs(argc, argv);
  ACE_CHECK;

  // Activate the PubDriver servant and write its ior to a file.

  PortableServer::POA_var poa = TheServiceParticipant->the_poa ();
  CORBA::ORB_var orb = TheServiceParticipant->get_ORB ();

  PortableServer::ObjectId_var id = poa->activate_object(this
                                                         ACE_ENV_ARG_PARAMETER);
  ACE_CHECK;

  CORBA::Object_var object = poa->id_to_reference(id.in()
                                                  ACE_ENV_ARG_PARAMETER);
  ACE_CHECK;

  CORBA::String_var ior_string = orb->object_to_string (object.in ()
                                                        ACE_ENV_ARG_PARAMETER);
  ACE_CHECK;

  //
  // Write the IOR to a file.
  //
  FILE *output_file= ACE_OS::fopen (pub_driver_ior_.c_str (), "w");
  if (output_file == 0)
  {
    ACE_ERROR ((LM_ERROR,
                ACE_TEXT("Cannot open output file for writing IOR\n")));
  }
  ACE_OS::fprintf (output_file, "%s", ior_string.in ());
  ACE_OS::fclose (output_file);

  ::DDS::ReturnCode_t ret = ::DDS::RETCODE_OK;

  ::Mine::FooTypeSupportImpl* fts_servant = new ::Mine::FooTypeSupportImpl();
   PortableServer::ServantBase_var safe_servant = fts_servant;

  ::Mine::FooTypeSupport_var fts =
    ::TAO::DCPS::servant_to_reference (fts_servant ACE_ENV_ARG_PARAMETER);
  ACE_CHECK;

  participant_ =
    dpf->create_participant(MY_DOMAIN,
                            PARTICIPANT_QOS_DEFAULT,
                            ::DDS::DomainParticipantListener::_nil()
                            ACE_ENV_ARG_PARAMETER);
  ACE_CHECK;
  TEST_CHECK (! CORBA::is_nil (participant_.in ()));

  if (::DDS::RETCODE_OK != fts->register_type(participant_.in (), MY_TYPE))
    {
      ACE_ERROR ((LM_ERROR,
                  ACE_TEXT ("Failed to register the FooTypeSupport.")));
    }

  ACE_CHECK;

  ::DDS::TopicQos default_topic_qos;
  participant_->get_default_topic_qos(default_topic_qos);

  ::DDS::TopicQos new_topic_qos = default_topic_qos;
  new_topic_qos.reliability.kind  = ::DDS::RELIABLE_RELIABILITY_QOS;

  //The SunOS compiler had problem resolving operator in a namespace.
  //To resolve the compilation errors, the operator is called explicitly.
  TEST_CHECK (! (new_topic_qos == default_topic_qos));

  participant_->set_default_topic_qos(new_topic_qos ACE_ENV_ARG_PARAMETER);

  topic_ = participant_->create_topic (MY_TOPIC,
                                       MY_TYPE,
                                       TOPIC_QOS_DEFAULT,
                                       ::DDS::TopicListener::_nil()
                                       ACE_ENV_ARG_PARAMETER);
  ACE_CHECK;
  TEST_CHECK (! CORBA::is_nil (topic_.in ()));

  publisher_ =
    participant_->create_publisher(PUBLISHER_QOS_DEFAULT,
                                   ::DDS::PublisherListener::_nil()
                                   ACE_ENV_ARG_PARAMETER);
  ACE_CHECK;
  TEST_CHECK (! CORBA::is_nil (publisher_.in ()));

  publisher_servant_
    = ::TAO::DCPS::reference_to_servant< ::TAO::DCPS::PublisherImpl, ::DDS::Publisher_ptr>
      (publisher_.in ());

  attach_to_transport ();

  ::DDS::PublisherQos pub_qos_got;
  publisher_->get_qos (pub_qos_got ACE_ENV_ARG_PARAMETER);

  ::DDS::PublisherQos default_pub_qos;
  participant_->get_default_publisher_qos (default_pub_qos
                                           ACE_ENV_ARG_PARAMETER);
  ACE_CHECK;
  TEST_CHECK (pub_qos_got == default_pub_qos);

  ::DDS::PublisherQos new_pub_qos = pub_qos_got;
  // This qos is not supported, so it's invalid qos.
  new_pub_qos.presentation.access_scope = ::DDS::GROUP_PRESENTATION_QOS;

  TEST_CHECK (! (new_pub_qos == default_pub_qos));

  ret = publisher_->set_qos (new_pub_qos ACE_ENV_ARG_PARAMETER);
  TEST_CHECK (ret == ::DDS::RETCODE_INCONSISTENT_POLICY);

  ::DDS::DomainParticipant_var participant
    = publisher_->get_participant (ACE_ENV_SINGLE_ARG_PARAMETER);
  ACE_CHECK;

  TEST_CHECK (participant.in () == participant_.in ());

  ::DDS::DataWriterQos default_dw_qos;
  publisher_->get_default_datawriter_qos (default_dw_qos ACE_ENV_ARG_PARAMETER);
  ACE_CHECK;

  ::DDS::DataWriterQos new_default_dw_qos = default_dw_qos;
  new_default_dw_qos.reliability.kind  = ::DDS::RELIABLE_RELIABILITY_QOS;

  TEST_CHECK (! (new_default_dw_qos == default_dw_qos));
  TEST_CHECK (publisher_->set_default_datawriter_qos (new_default_dw_qos)
              == ::DDS::RETCODE_OK);

  // Create datawriter to test copy_from_topic_qos.
  datawriter_
    = publisher_->create_datawriter(topic_.in (),
                                    DATAWRITER_QOS_USE_TOPIC_QOS,
                                    ::DDS::DataWriterListener::_nil()
                                    ACE_ENV_ARG_PARAMETER);
  ACE_CHECK;
  TEST_CHECK (! CORBA::is_nil (datawriter_.in ()));

  ::DDS::DataWriterQos dw_qos_use_topic_qos;
  datawriter_->get_qos (dw_qos_use_topic_qos ACE_ENV_ARG_PARAMETER);
  ACE_CHECK;

  ::DDS::DataWriterQos copied_from_topic = default_dw_qos;
  ret = publisher_->copy_from_topic_qos (copied_from_topic, new_topic_qos);
  ACE_CHECK;
  TEST_CHECK (ret == ::DDS::RETCODE_OK);

  TEST_CHECK (dw_qos_use_topic_qos == copied_from_topic);

  // Delete the datawriter.
  publisher_->delete_datawriter (datawriter_.in () ACE_ENV_ARG_PARAMETER);

  // Create datawriter to test DATAWRITER_QOS_DEFAULT/get_publisher
  // get_qos/set_qos/get_default_datawriter_qos.
  datawriter_
    = publisher_->create_datawriter(topic_.in (),
                                    DATAWRITER_QOS_DEFAULT,
                                    ::DDS::DataWriterListener::_nil()
                                    ACE_ENV_ARG_PARAMETER);
  ACE_CHECK;
  TEST_CHECK (! CORBA::is_nil (datawriter_.in ()));

  ::DDS::Topic_var topic_got
    = datawriter_->get_topic (ACE_ENV_SINGLE_ARG_PARAMETER);
  ACE_CHECK;

  // the topics should point to the same servant
  // but not the same Object Reference.
  TopicImpl* topic_got_servant
    = reference_to_servant<TopicImpl, ::DDS::Topic_ptr>
        (topic_got.in () ACE_ENV_ARG_PARAMETER);
  ACE_CHECK;
  TopicImpl* topic_servant
    = reference_to_servant<TopicImpl, ::DDS::Topic_ptr>
        (topic_.in () ACE_ENV_ARG_PARAMETER);
  ACE_CHECK;

  TEST_CHECK (topic_got_servant == topic_servant);

  ::DDS::Publisher_var pub_got
    = datawriter_->get_publisher (ACE_ENV_SINGLE_ARG_PARAMETER);
  ACE_CHECK;

  TEST_CHECK (pub_got.in () == publisher_.in ());

  ::DDS::DataWriterQos dw_qos_got;
  datawriter_->get_qos (dw_qos_got ACE_ENV_ARG_PARAMETER);
  ACE_CHECK;

  TEST_CHECK (dw_qos_got == new_default_dw_qos);

  ::DDS::DataWriterQos new_dw_qos = dw_qos_got;

  new_dw_qos.reliability.kind  = ::DDS::RELIABLE_RELIABILITY_QOS;
  new_dw_qos.resource_limits.max_samples_per_instance = 2;
  new_dw_qos.history.kind  = ::DDS::KEEP_ALL_HISTORY_QOS;

  TEST_CHECK (! (dw_qos_got == new_dw_qos));

  ret = datawriter_->set_qos (new_dw_qos ACE_ENV_ARG_PARAMETER);
  ACE_CHECK;

  TEST_CHECK (ret == ::DDS::RETCODE_IMMUTABLE_POLICY);

  // Delete the datawriter.
  publisher_->delete_datawriter (datawriter_.in () ACE_ENV_ARG_PARAMETER);

  // Create datawriter to test register/unregister/dispose and etc.
  ::DDS::DataWriterQos dw_qos;
  publisher_->get_default_datawriter_qos (dw_qos ACE_ENV_ARG_PARAMETER);
  ACE_CHECK;

  dw_qos.history.depth = history_depth_;

  if (test_to_run_ == LIVELINESS_TEST)
  {
    dw_qos.liveliness.lease_duration.sec = lease_duration_sec;
    dw_qos.liveliness.lease_duration.nanosec = 0;
  }

  datawriter_
    = publisher_->create_datawriter(topic_.in (),
                                    dw_qos,
                                    ::DDS::DataWriterListener::_nil()
                                    ACE_ENV_ARG_PARAMETER);
  ACE_CHECK;
  TEST_CHECK (! CORBA::is_nil (datawriter_.in ()));

  datawriter_servant_
    = ::TAO::DCPS::reference_to_servant< ::TAO::DCPS::DataWriterImpl, ::DDS::DataWriter_ptr>
    (datawriter_.in ());

  foo_datawriter_
    = ::Mine::FooDataWriter::_narrow(datawriter_.in () ACE_ENV_ARG_PARAMETER);
  ACE_CHECK;

  TEST_CHECK (! CORBA::is_nil (foo_datawriter_.in ()));

  foo_datawriter_servant_
    = ::TAO::DCPS::reference_to_servant < ::Mine::FooDataWriterImpl, ::Mine::FooDataWriter_ptr>
      (foo_datawriter_.in () ACE_ENV_ARG_PARAMETER);
  ACE_CHECK;

  TEST_CHECK (foo_datawriter_servant_ != 0);
}

void
PubDriver::end()
{
  // Record samples been written in the Writer's data map.
  // Verify the number of instances and the number of samples
  // written to the datawriter.

  publisher_->delete_contained_entities (ACE_ENV_SINGLE_ARG_PARAMETER);
  ACE_CHECK;
  // publisher_->delete_datawriter(datawriter_.in () ACE_ENV_ARG_PARAMETER);

  CORBA::String_var topic_name = topic_->get_name (ACE_ENV_SINGLE_ARG_PARAMETER);
  ACE_CHECK;

  ::DDS::DataWriter_var dw = publisher_->lookup_datawriter (topic_name.in ());

  TEST_CHECK (CORBA::is_nil (dw.in ()));

  // clean up the service objects
  participant_->delete_publisher(publisher_.in () ACE_ENV_ARG_PARAMETER);
  ACE_CHECK;

  participant_->delete_topic(topic_.in () ACE_ENV_ARG_PARAMETER);
  ACE_CHECK;

  ::DDS::DomainParticipantFactory_var dpf = TheParticipantFactory;
  dpf->delete_participant(participant_.in () ACE_ENV_ARG_PARAMETER);
  ACE_CHECK;

  TheServiceParticipant->shutdown ();
  // Tear-down the entire Transport Framework.
  TheTransportFactory->release();
}

void
PubDriver::run()
{
  FILE* fp = ACE_OS::fopen (pub_id_fname_.c_str (), ACE_LIB_TEXT("w"));
  if (fp == 0)
  {
    ACE_ERROR ((LM_ERROR,
                ACE_LIB_TEXT("Unable to open %s for writing:(%u) %p\n"),
                pub_id_fname_.c_str (),
                ACE_LIB_TEXT("PubDriver::run")));
    return;
  }

  TAO::DCPS::PublicationId pub_id = datawriter_servant_->get_publication_id ();

  // Write the publication id to a file.
  ACE_DEBUG ((LM_DEBUG,
              ACE_TEXT("(%P|%t) PubDriver::run, ")
              ACE_TEXT(" Write to %s: pub_id=%d. \n"),
              pub_id_fname_.c_str (),
              pub_id));

  ACE_OS::fprintf (fp, "%d\n", pub_id);
  fclose (fp);

  ACE_DEBUG ((LM_DEBUG,
              ACE_TEXT("(%P|%t) PubDriver::run, ")
              ACE_TEXT(" Wait for subscriber start. \n")));

  // Set up the subscriptions.
  add_subscription (this->sub_id_, this->sub_addr_.c_str ());

  // Let the subscriber catch up before we broadcast.
  ::DDS::InstanceHandleSeq handles;
  while (1)
    {
      foo_datawriter_->get_matched_subscriptions(handles);
      if (handles.length() > 0)
        break;
      else
        ACE_OS::sleep(ACE_Time_Value(0,200000));
    }

  run_test (test_to_run_);
}

void PubDriver::run_test (int test_to_run)
{
  // Only allow run one test at one time.
  switch (test_to_run)
  {
  case REGISTER_TEST :
    register_test ();
    break;
  case UNREGISTER_TEST :
    unregister_test ();
    break;
  case DISPOSE_TEST :
    dispose_test ();
    break;
  case RESUME_TEST :
    resume_test ();
    break;
  case LISTENER_TEST :
    listener_test ();
    break;
  case ALLOCATOR_TEST :
    allocator_test ();
    break;
  case LIVELINESS_TEST :
    liveliness_test ();
    break;
  default:
    ACE_ERROR((LM_ERROR,
      "(%P|%t) Invalid test kind %d\n", test_to_run_));
    throw TestException();
    break;
  }
}


// register test
void
PubDriver::register_test ()
{
  ::DDS::ReturnCode_t ret = ::DDS::RETCODE_OK;

  ::Xyz::Foo foo1;
  foo1.a_long_value = 101010;
  foo1.handle_value = -1;
  foo1.sample_sequence = -1;
  foo1.writer_id = -1;

  ::DDS::InstanceHandle_t handle1
    = foo_datawriter_->_cxx_register (foo1 ACE_ENV_ARG_PARAMETER);
  ACE_CHECK;

  ::Xyz::Foo foo2;
  foo2.a_long_value = 101010;
  foo2.handle_value = 99;
  foo2.sample_sequence = 99;
  foo2.writer_id = 99;

  ::DDS::InstanceHandle_t handle2
    = foo_datawriter_->_cxx_register (foo2 ACE_ENV_ARG_PARAMETER);
  ACE_CHECK;

  TEST_CHECK (handle1 == handle2);

  ::Xyz::Foo key_holder;
  ret = foo_datawriter_->get_key_value(key_holder, handle1 ACE_ENV_ARG_PARAMETER);
  ACE_CHECK;

  TEST_CHECK(ret == ::DDS::RETCODE_OK);
  TEST_CHECK(key_holder.a_long_value == foo1.a_long_value);
  TEST_CHECK(key_holder.handle_value == foo1.handle_value);
  TEST_CHECK(key_holder.sample_sequence == foo1.sample_sequence);
  TEST_CHECK(key_holder.writer_id == foo1.writer_id);

  for (size_t i = 1; i <= 2; i ++)
  {
    foo2.a_long_value = 101010;
    foo2.handle_value = handle1;
    foo2.sample_sequence = i;
    foo2.writer_id = 0;

    ret = foo_datawriter_->write(foo2,
                                 handle2
                                 ACE_ENV_ARG_PARAMETER);
    ACE_CHECK;

    TEST_CHECK (ret == ::DDS::RETCODE_OK);
  }
}


// dispose test
void
PubDriver::dispose_test ()
{
  ::DDS::ReturnCode_t ret = ::DDS::RETCODE_OK;

  ::Xyz::Foo foo1;
  foo1.a_long_value = 101010;
  foo1.handle_value = -1;
  foo1.sample_sequence = -1;
  foo1.writer_id = -1;

  ::DDS::InstanceHandle_t handle
    = foo_datawriter_->_cxx_register (foo1 ACE_ENV_ARG_PARAMETER);
  ACE_CHECK;

  ::Xyz::Foo foo2;
  foo2.a_long_value = 101010;
  foo2.handle_value = handle;
  foo2.sample_sequence = 0;
  foo2.writer_id = 0;

  for (int i = 1; i <= 10; i ++)
  {
    foo2.sample_sequence = i;

    ret = foo_datawriter_->write(foo2,
                        handle
                        ACE_ENV_ARG_PARAMETER);
    ACE_CHECK;

    TEST_CHECK (ret == ::DDS::RETCODE_OK);
  }

  ret
    = foo_datawriter_->dispose(foo1, handle ACE_ENV_ARG_PARAMETER);
  ACE_CHECK;

  TEST_CHECK (ret == ::DDS::RETCODE_OK);

  size_t sz = 0;
  ret = datawriter_servant_->num_samples (handle, sz);

  TEST_CHECK (ret == ::DDS::RETCODE_OK && sz == 0);
}

// unregister test
void
PubDriver::unregister_test ()
{
  ::DDS::ReturnCode_t ret = ::DDS::RETCODE_OK;

  ::Xyz::Foo foo1;
  foo1.a_long_value = 101010;
  foo1.handle_value = -1;
  foo1.sample_sequence = -1;
  foo1.writer_id = -1;

  ::DDS::InstanceHandle_t handle
    = foo_datawriter_->_cxx_register (foo1 ACE_ENV_ARG_PARAMETER);
  ACE_CHECK;

  ::Xyz::Foo foo2;
  foo2.a_long_value = 101010;
  foo2.handle_value = handle;
  foo2.sample_sequence = 1;
  foo2.writer_id = 0;

  ret = foo_datawriter_->write(foo2,
                               handle
                               ACE_ENV_ARG_PARAMETER);
  ACE_CHECK;

  TEST_CHECK (ret == ::DDS::RETCODE_OK);

  ret = foo_datawriter_->unregister (foo1,
                                     handle
                                     ACE_ENV_ARG_PARAMETER);
  ACE_CHECK;

  TEST_CHECK (ret == ::DDS::RETCODE_OK);

  foo2.sample_sequence = 2;

  ret = foo_datawriter_->write(foo2,
                               ::DDS::HANDLE_NIL
                               ACE_ENV_ARG_PARAMETER);
  ACE_CHECK;

  TEST_CHECK (ret == ::DDS::RETCODE_OK);

  foo2.sample_sequence = 3;

  ret = foo_datawriter_->write(foo2,
                               handle
                               ACE_ENV_ARG_PARAMETER);
  ACE_CHECK;

  TEST_CHECK (ret == ::DDS::RETCODE_OK);
}


// suspend/resume test
void
PubDriver::resume_test ()
{
  ::DDS::ReturnCode_t ret = ::DDS::RETCODE_OK;

  ::Xyz::Foo foo1;
  foo1.a_long_value = 101010;
  foo1.handle_value = -1;
  foo1.sample_sequence = -1;
  foo1.writer_id = -1;

  ::DDS::InstanceHandle_t handle
    = foo_datawriter_->_cxx_register (foo1 ACE_ENV_ARG_PARAMETER);
  ACE_CHECK;

  ::Xyz::Foo foo2;
  foo2.a_long_value = 101010;
  foo2.handle_value = handle;
  foo2.sample_sequence = 0;
  foo2.writer_id = 0;

  // fast way - call servant directly
  ret = publisher_servant_->suspend_publications (ACE_ENV_SINGLE_ARG_PARAMETER);
  TEST_CHECK (ret == ::DDS::RETCODE_OK);

  for (int i = 1; i <= 10; i ++)
  {
    foo2.sample_sequence = i;

    ret = foo_datawriter_->write(foo2,
                                 handle
                                 ACE_ENV_ARG_PARAMETER);
    ACE_CHECK;

    TEST_CHECK (ret == ::DDS::RETCODE_OK);
  }

  // fast way - call servant directly
  ret = publisher_servant_->resume_publications (ACE_ENV_SINGLE_ARG_PARAMETER);
  TEST_CHECK (ret == ::DDS::RETCODE_OK);
}


// listener and status test
void
PubDriver::listener_test ()
{
  // Create DomainParticipantListener, PublisherListener and
  // DataWriterListener.
  DomainParticipantListenerImpl* dpl_servant;
  ACE_NEW (dpl_servant,
           DomainParticipantListenerImpl());

  ::DDS::DomainParticipantListener_var dpl
    = ::TAO::DCPS::servant_to_reference (dpl_servant ACE_ENV_ARG_PARAMETER);
  ACE_CHECK;

  PublisherListenerImpl* pl_servant;
  ACE_NEW (pl_servant,
           PublisherListenerImpl());

  ::DDS::PublisherListener_var pl
    = ::TAO::DCPS::servant_to_reference (pl_servant ACE_ENV_ARG_PARAMETER);
  ACE_CHECK;

  DataWriterListenerImpl* dwl_servant;
  ACE_NEW (dwl_servant,
           DataWriterListenerImpl());

  ::DDS::DataWriterListener_var dwl
    = ::TAO::DCPS::servant_to_reference (dwl_servant ACE_ENV_ARG_PARAMETER);
  ACE_CHECK;

  // Test set_listener/get_listener for DomainParticipant.
  ::DDS::DomainParticipantListener_var dpl_got
    = participant_->get_listener (ACE_ENV_SINGLE_ARG_PARAMETER);

  TEST_CHECK (CORBA::is_nil (dpl_got.in ()));

  participant_->set_listener (dpl.in (), DEFAULT_STATUS_KIND_MASK ACE_ENV_ARG_PARAMETER);
  ACE_CHECK;

  dpl_got = participant_->get_listener (ACE_ENV_SINGLE_ARG_PARAMETER);

  TEST_CHECK (dpl_got.in () == dpl.in ());

  // Test set_listener/get_listener for Publisher.

  ::DDS::PublisherListener_var pl_got
    = publisher_->get_listener (ACE_ENV_SINGLE_ARG_PARAMETER);

  TEST_CHECK (CORBA::is_nil (pl_got.in ()));

  publisher_->set_listener (pl.in (), 0 ACE_ENV_ARG_PARAMETER);
  ACE_CHECK;

  pl_got = publisher_->get_listener (ACE_ENV_SINGLE_ARG_PARAMETER);

  TEST_CHECK (pl_got.in () == pl.in ());

  // Test set_listener/get_listener for DataWriter.

  ::DDS::DataWriterListener_var dwl_got
    = foo_datawriter_->get_listener (ACE_ENV_SINGLE_ARG_PARAMETER);

  TEST_CHECK (CORBA::is_nil (dwl_got.in ()));

  foo_datawriter_->set_listener (dwl.in (), 0 ACE_ENV_ARG_PARAMETER);
  ACE_CHECK;

  dwl_got = foo_datawriter_->get_listener (ACE_ENV_SINGLE_ARG_PARAMETER);

  TEST_CHECK (dwl_got.in () == dwl.in ());

  // Test update_incompatible_qos/get_offered_incompatible_qos_status
  // and listener for specific status kind.
  TAO::DCPS::IncompatibleQosStatus incomp_status;
  incomp_status.total_count = 20;
  incomp_status.count_since_last_send = 8;
  incomp_status.last_policy_id = 6;
  incomp_status.policies.length (2);
  incomp_status.policies[0].policy_id = 5;
  incomp_status.policies[0].count = 12;
  incomp_status.policies[1].policy_id = 6;
  incomp_status.policies[1].count = 10;

  // Call listener and update the status cached in datawriter.
  foo_datawriter_->update_incompatible_qos (incomp_status ACE_ENV_ARG_PARAMETER);
  ACE_CHECK;

  // Domainparticipant's listener is called to notify the
  // OFFERED_INCOMPATIBLE_QOS_STATUS change when the datawriter and
  // publisher do not have a listener for the status.
  TEST_CHECK (offered_incompatible_qos_called_on_dp  == 1);
  TEST_CHECK (offered_incompatible_qos_called_on_pub == 0);
  TEST_CHECK (offered_incompatible_qos_called_on_dw  == 0);

  ::DDS::OfferedIncompatibleQosStatus_var incomp_status_got
    = foo_datawriter_->get_offered_incompatible_qos_status (ACE_ENV_SINGLE_ARG_PARAMETER);

  TEST_CHECK (incomp_status_got.in ().total_count == incomp_status.total_count);
  // TBD: This should be reconfirmed when the DataWriterImpl::update_incompatible_qos is
  //      updated.
  TEST_CHECK (incomp_status_got.in ().total_count_change == 0);
  TEST_CHECK (incomp_status_got.in ().last_policy_id == incomp_status.last_policy_id);
  TEST_CHECK (incomp_status_got.in ().policies.length () == incomp_status.policies.length ());
  for (CORBA::ULong i = 0; i < 2; i ++)
  {
    TEST_CHECK (incomp_status_got.in ().policies[i].policy_id == incomp_status.policies[i].policy_id);
    TEST_CHECK (incomp_status_got.in ().policies[i].count == incomp_status.policies[i].count);
  }

  publisher_->set_listener (pl.in (),
                            ::DDS::OFFERED_INCOMPATIBLE_QOS_STATUS
                            ACE_ENV_ARG_PARAMETER);
  ACE_CHECK;

  // Call listener and update the status cached in datawriter.
  foo_datawriter_->update_incompatible_qos (incomp_status ACE_ENV_ARG_PARAMETER);
  ACE_CHECK;

  // Publisher's listener is called to notify the
  // OFFERED_INCOMPATIBLE_QOS_STATUS change when the datawriter
  // does not have a listener for the status.
  TEST_CHECK (offered_incompatible_qos_called_on_dp == 1);
  TEST_CHECK (offered_incompatible_qos_called_on_pub == 1);
  TEST_CHECK (offered_incompatible_qos_called_on_dw == 0);

  foo_datawriter_->set_listener (dwl.in (),
                                 ::DDS::OFFERED_INCOMPATIBLE_QOS_STATUS
                                 ACE_ENV_ARG_PARAMETER);
  ACE_CHECK;

  foo_datawriter_->update_incompatible_qos (incomp_status ACE_ENV_ARG_PARAMETER);
  ACE_CHECK;

  // DataWriter's listener is called to notify the
  // OFFERED_INCOMPATIBLE_QOS_STATUS change.
  TEST_CHECK (offered_incompatible_qos_called_on_dp == 1);
  TEST_CHECK (offered_incompatible_qos_called_on_pub == 1);
  TEST_CHECK (offered_incompatible_qos_called_on_dw == 1);

  ::DDS::StatusKindMask changed_status
    = foo_datawriter_->get_status_changes (ACE_ENV_SINGLE_ARG_PARAMETER);

  // Both OFFERED_INCOMPATIBLE_QOS_STATUS and PUBLICATION_MATCH_STATUS status
  // are changed.
  TEST_CHECK ((changed_status & ::DDS::OFFERED_INCOMPATIBLE_QOS_STATUS) != 0);
  TEST_CHECK ((changed_status & ::DDS::PUBLICATION_MATCH_STATUS) != 0);

  // Test get_matched_subscriptions.

  ::DDS::InstanceHandleSeq subscription_handles;
  ::DDS::ReturnCode_t ret
    = foo_datawriter_->get_matched_subscriptions (subscription_handles
                                                  ACE_ENV_ARG_PARAMETER);

  TEST_CHECK (ret == ::DDS::RETCODE_OK
              && subscription_handles.length () == 1
              && subscription_handles[0] == this->sub_id_);

  // Test get_publication_match_status.

  // The PUBLICATION_MATCH_STATUS status is updated when add_association.
  // One subscription is added already.
  ::DDS::PublicationMatchStatus match_status
    = foo_datawriter_->get_publication_match_status (ACE_ENV_SINGLE_ARG_PARAMETER);
  ACE_CHECK;

  TEST_CHECK (match_status.total_count == 1);
  // The listener is set after add_association, so the total_count_change
  // should be 1 since the datawriter is associated with one datareader.
  TEST_CHECK (match_status.total_count_change == 1);
  TEST_CHECK (match_status.last_subscription_handle == this->sub_id_);

  // Call register_test to send a few messages to remote subscriber.
  register_test ();

  //Test remove_associations

  ::TAO::DCPS::ReaderIdSeq reader_ids;
  reader_ids.length (1);
  reader_ids[0] = this->sub_id_;

  CORBA::Boolean dont_notify_lost = 0;
  foo_datawriter_->remove_associations (reader_ids, dont_notify_lost ACE_ENV_ARG_PARAMETER);

  ret = foo_datawriter_->get_matched_subscriptions (subscription_handles
                                                    ACE_ENV_ARG_PARAMETER);

  TEST_CHECK (ret == ::DDS::RETCODE_OK && subscription_handles.length () == 0);

  // The OfferedDeadlineMissedStatus and OfferedDeadlineMissedStatus are not
  // supported currently. The status got should be the same as initial status.
  ::DDS::OfferedDeadlineMissedStatus deadline_status
    = foo_datawriter_->get_offered_deadline_missed_status (ACE_ENV_SINGLE_ARG_PARAMETER);
  ::DDS::LivelinessLostStatus liveliness_status
    = foo_datawriter_->get_liveliness_lost_status (ACE_ENV_SINGLE_ARG_PARAMETER);

  ::DDS::OfferedDeadlineMissedStatus initial_deadline_status;
  initial_deadline_status.total_count = 0;
  initial_deadline_status.total_count_change = 0;
  initial_deadline_status.last_instance_handle = ::DDS::HANDLE_NIL;
  TEST_CHECK (deadline_status.total_count == initial_deadline_status.total_count);
  TEST_CHECK (deadline_status.total_count_change == initial_deadline_status.total_count_change);
  TEST_CHECK (deadline_status.last_instance_handle == initial_deadline_status.last_instance_handle);

  ::DDS::LivelinessLostStatus initial_liveliness_status;
  initial_liveliness_status.total_count = 0;
  initial_liveliness_status.total_count_change = 0;
  TEST_CHECK (liveliness_status.total_count == initial_liveliness_status.total_count);
  TEST_CHECK (liveliness_status.total_count_change == initial_liveliness_status.total_count_change);
}

void
PubDriver::allocator_test ()
{
  size_t n_chunks = TheServiceParticipant->n_chunks ();

  ::DDS::ReturnCode_t ret = ::DDS::RETCODE_OK;

  ::Xyz::Foo foo1;
  foo1.a_long_value = 101010;
  foo1.handle_value = -1;
  foo1.sample_sequence = -1;
  foo1.writer_id = -1;

  ::DDS::InstanceHandle_t handle
    = foo_datawriter_->_cxx_register (foo1 ACE_ENV_ARG_PARAMETER);
  ACE_CHECK;

  ::Xyz::Foo foo2;
  foo2.a_long_value = 101010;
  foo2.handle_value = handle;
  foo2.writer_id =0;

  // Allocate serialized foo data from pre-allocated pool
  for (size_t i = 1; i <= n_chunks; i ++)
  {
    foo2.sample_sequence = i;

    ret = foo_datawriter_->write(foo2,
                                 handle
                                 ACE_ENV_ARG_PARAMETER);
    ACE_CHECK;

    TEST_CHECK (ret == ::DDS::RETCODE_OK);

    if (_tao_is_bounded_size (foo1))
    {
      TEST_CHECK (foo_datawriter_servant_->data_allocator() != 0);
      TEST_CHECK (foo_datawriter_servant_->data_allocator()->allocs_from_heap_ == 0);
      TEST_CHECK (foo_datawriter_servant_->data_allocator()->allocs_from_pool_ == i);
    }
    else
    {
       TEST_CHECK (foo_datawriter_servant_->data_allocator() == 0);
    }
  }

  {
  // The pre-allocated pool is full, now the foo data is allocated from heap
  for (size_t i = 1; i <= 2; i ++)
  {
    foo2.sample_sequence = i + n_chunks;

    ret = foo_datawriter_->write(foo2,
                                 handle
                                 ACE_ENV_ARG_PARAMETER);
    ACE_CHECK;

    TEST_CHECK (ret == ::DDS::RETCODE_OK);

    if (_tao_is_bounded_size (foo1))
    {
      TEST_CHECK (foo_datawriter_servant_->data_allocator() != 0);
      TEST_CHECK (foo_datawriter_servant_->data_allocator()->allocs_from_heap_ == i);
      TEST_CHECK (foo_datawriter_servant_->data_allocator()->allocs_from_pool_ == n_chunks);
    }
    else
    {
       TEST_CHECK (foo_datawriter_servant_->data_allocator() == 0);
    }
  }
  }
}

// test liveliness
void
PubDriver::liveliness_test ()
{
  size_t  num_writes = 3;

  ::DDS::ReturnCode_t ret = ::DDS::RETCODE_OK;

  ::Xyz::Foo foo;
  foo.a_long_value = 101010;
  foo.handle_value = -1;
  foo.sample_sequence = -1;
  foo.writer_id = 0;

  ::DDS::InstanceHandle_t handle
    = foo_datawriter_->_cxx_register (foo ACE_ENV_ARG_PARAMETER);
  ACE_CHECK;

  foo.handle_value = handle;

  for (size_t i = 1; i < num_writes; i ++)
  {
    foo.sample_sequence = i;

    ret = foo_datawriter_->write(foo,
                                 handle
                                 ACE_ENV_ARG_PARAMETER);
    ACE_CHECK;

    TEST_CHECK (ret == ::DDS::RETCODE_OK);

    ACE_OS::sleep (ACE_Time_Value (lease_duration_sec));
  }

  // Some liveliness will be sent automatically.
  ACE_OS::sleep (lease_duration_sec * 3);

  // Writing this here makes this test deterministic.
  // The subscriber will wait for this message and both the
  // publish & subscriber close down simultaneously.
  foo.sample_sequence = num_writes;
  ret = foo_datawriter_->write(foo,
                               handle
                               ACE_ENV_ARG_PARAMETER);

  // Test control_dropped.
  // Unregister_all will request transport to remove_all_control_msgs.
  // which results the control_dropped call.
  datawriter_servant_->unregister_all ();

  // The subscriber will wait a while after telling the
  // publisher shutdown, so the unreigster message can still
  // be sent out.
  TEST_CHECK (datawriter_servant_->control_dropped_count_ == 0);
}

int
PubDriver::parse_pub_arg(const std::string& arg)
{
  std::string::size_type pos;

  // Find the first ':' character, and make sure it is in a legal spot.
  if ((pos = arg.find_first_of(':')) == std::string::npos) {
    ACE_ERROR((LM_ERROR,
               "(%P|%t) Bad -p command-line value (%s). Missing ':' char.\n",
               arg.c_str()));
    return -1;
  }

  if (pos == 0) {
    ACE_ERROR((LM_ERROR,
               "(%P|%t) Bad -p command-line value (%s). "
               "':' char cannot be first char.\n",
               arg.c_str()));
    return -1;
  }

  if (pos == (arg.length() - 1)) {
    ACE_ERROR((LM_ERROR,
               "(%P|%t) Bad -p command-line value  (%s) - "
               "':' char cannot be last char.\n",
               arg.c_str()));
    return -1;
  }

  // Parse the pub_id from left of ':' char, and remainder to right of ':'.
  std::string pub_id_str(arg,0,pos);
  std::string pub_addr_str(arg,pos+1,std::string::npos); //use 3-arg constructor to build with VC6

  this->pub_id_fname_ = pub_id_str.c_str();
  this->pub_addr_ = ACE_INET_Addr(pub_addr_str.c_str());

  return 0;
}

int
PubDriver::parse_sub_arg(const std::string& arg)
{
  std::string::size_type pos;

  // Find the first ':' character, and make sure it is in a legal spot.
  if ((pos = arg.find_first_of(':')) == std::string::npos) {
    ACE_ERROR((LM_ERROR,
               "(%P|%t) Bad -s command-line value (%s). Missing ':' char.\n",
               arg.c_str()));
    return -1;
  }

  if (pos == 0) {
    ACE_ERROR((LM_ERROR,
               "(%P|%t) Bad -s command-line value (%s). "
               "':' char cannot be first char.\n",
               arg.c_str()));
    return -1;
  }

  if (pos == (arg.length() - 1)) {
    ACE_ERROR((LM_ERROR,
               "(%P|%t) Bad -s command-line value  (%s) - "
               "':' char cannot be last char.\n",
               arg.c_str()));
    return -1;
  }

  // Parse the sub_id from left of ':' char, and remainder to right of ':'.
  std::string sub_id_str(arg,0,pos);
  std::string sub_addr_str(arg,pos+1,std::string::npos); //use 3-arg constructor to build with VC6

  this->sub_id_ = ACE_OS::atoi(sub_id_str.c_str());

  // Use the remainder as the "stringified" ACE_INET_Addr.
  this->sub_addr_ = sub_addr_str.c_str();

  return 0;
}

void PubDriver::shutdown (
    ACE_ENV_SINGLE_ARG_DECL
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ))
{
  shutdown_ = 1;
}


void PubDriver::add_new_subscription (
    CORBA::Long       reader_id,
    const char *      sub_addr
    ACE_ENV_ARG_DECL
  )
  ACE_THROW_SPEC ((
    CORBA::SystemException
  ))
{
  sub_id_ = reader_id;
  sub_addr_ = sub_addr;
  add_new_subscription_ = 1;
}


void PubDriver::add_subscription (
    CORBA::Long       reader_id,
    const char *      sub_addr
    )
{
  ::TAO::DCPS::ReaderAssociationSeq associations;
  associations.length (1);
  associations[0].readerTransInfo.transport_id = 1; // TBD - not right

  ACE_INET_Addr sub_inet_addr(sub_addr);
  TAO::DCPS::NetworkAddress network_order_address(sub_inet_addr);
  associations[0].readerTransInfo.data
    = TAO::DCPS::TransportInterfaceBLOB
                                   (sizeof(TAO::DCPS::NetworkAddress),
                                    sizeof(TAO::DCPS::NetworkAddress),
                                    (CORBA::Octet*)(&network_order_address));


  associations[0].readerId = reader_id;
  associations[0].subQos = TheServiceParticipant->initial_SubscriberQos ();
  associations[0].readerQos = TheServiceParticipant->initial_DataReaderQos ();

  TAO::DCPS::RepoId pub_id = foo_datawriter_servant_->get_publication_id();
  foo_datawriter_->add_associations (pub_id, associations);
}


void PubDriver::attach_to_transport ()
{
  TAO::DCPS::TransportImpl_rch transport_impl
    = TheTransportFactory->create_transport_impl (ALL_TRAFFIC, "SimpleTcp", TAO::DCPS::DONT_AUTO_CONFIG);

  TAO::DCPS::TransportConfiguration_rch config
    = TheTransportFactory->create_configuration (ALL_TRAFFIC, "SimpleTcp");

  TAO::DCPS::SimpleTcpConfiguration* tcp_config
    = static_cast <TAO::DCPS::SimpleTcpConfiguration*> (config.in ());

  tcp_config->local_address_ = this->pub_addr_;

  if (transport_impl->configure(config.in ()) != 0)
    {
      ACE_ERROR((LM_ERROR,
                 "(%P|%t) Failed to configure the transport impl\n"));
      throw TestException();
    }

  TAO::DCPS::AttachStatus status
    = publisher_servant_->attach_transport(transport_impl.in());

  if (status != TAO::DCPS::ATTACH_OK)
  {
    // We failed to attach to the transport for some reason.
    std::string status_str;

    switch (status)
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

    ACE_ERROR((LM_ERROR,
                "(%P|%t) Failed to attach to the transport. "
                "AttachStatus == %s\n", status_str.c_str()));
    throw TestException();
  }
}
