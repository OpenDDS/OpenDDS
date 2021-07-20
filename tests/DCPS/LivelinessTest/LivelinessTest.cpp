#include "common.h"
#include "Writer.h"
#include "DataReaderListener.h"

#include <tests/DCPS/common/TestException.h>
#include <tests/DCPS/FooType4/FooDefTypeSupportImpl.h>
#include <tests/Utils/StatusMatching.h>

#include <dds/DCPS/Service_Participant.h>
#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/Qos_Helper.h>
#include <dds/DCPS/TopicDescriptionImpl.h>
#include <dds/DCPS/PublisherImpl.h>
#include <dds/DCPS/SubscriberImpl.h>
#include <dds/DCPS/transport/framework/TransportRegistry.h>
#include <dds/DCPS/StaticIncludes.h>
#include <dds/DCPS/WaitSet.h>
#if defined ACE_AS_STATIC_LIBS && !defined OPENDDS_SAFETY_PROFILE
#include <dds/DCPS/transport/udp/Udp.h>
#endif

#include <dds/DdsDcpsSubscriptionC.h>

#include <ace/Arg_Shifter.h>
#include <ace/OS_NS_unistd.h>

/// parse the command line arguments
int parse_args (int argc, ACE_TCHAR *argv[])
{
  u_long mask =  ACE_LOG_MSG->priority_mask(ACE_Log_Msg::PROCESS);
  ACE_LOG_MSG->priority_mask(mask | LM_TRACE | LM_DEBUG, ACE_Log_Msg::PROCESS);
  ACE_Arg_Shifter arg_shifter (argc, argv);

  while (arg_shifter.is_anything_left())
  {
    // options:
    //  -i num_ops_per_thread       defaults to 1
    //  -l num_unlively_periods     defaults to 10
    //  -w num_datawriters          defaults to 1
    //  -n max_samples_per_instance defaults to INFINITE
    //  -d history.depth            defaults to 1
    //  -z                          verbose transport debug
    //  -r reliable                 0 for udp transport 1 for others

    const ACE_TCHAR *currentArg = 0;

    if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-i"))) != 0) {
      num_ops_per_thread = ACE_OS::atoi (currentArg);
      arg_shifter.consume_arg();
    } else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-l"))) != 0) {
      num_unlively_periods = ACE_OS::atoi (currentArg);
      arg_shifter.consume_arg();
    } else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-n"))) != 0) {
      max_samples_per_instance = ACE_OS::atoi (currentArg);
      arg_shifter.consume_arg();
    } else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-d"))) != 0) {
      history_depth = ACE_OS::atoi (currentArg);
      arg_shifter.consume_arg();
    } else if (arg_shifter.cur_arg_strncasecmp(ACE_TEXT("-z")) == 0) {
      TURN_ON_VERBOSE_DEBUG;
      arg_shifter.consume_arg();
    } else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-t"))) != 0) {
      use_take = ACE_OS::atoi (currentArg);
      arg_shifter.consume_arg();
    } else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-r"))) != 0) {
      use_reliable = (ACE_OS::atoi (currentArg));
      arg_shifter.consume_arg();
    } else {
      arg_shifter.ignore_arg();
    }
  }
  // Indicates successful parsing of the command line
  return 0;
}


int ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  int status = 0;
  DDS::DomainParticipantFactory_var dpf = TheParticipantFactoryWithArgs(argc, argv);
  // let the Service_Participant (in above line) strip out -DCPSxxx parameters
  // and then get application specific parameters.
  parse_args (argc, argv);

  ::Xyz::FooTypeSupport_var fts(new ::Xyz::FooTypeSupportImpl);
  ::Xyz::FooTypeSupport_var fts2(new ::Xyz::FooTypeSupportImpl);

  DDS::DomainParticipant_var dp =
      dpf->create_participant(MY_DOMAIN,
                              PARTICIPANT_QOS_DEFAULT,
                              DDS::DomainParticipantListener::_nil(),
                              OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  DDS::DomainParticipant_var dp2 =
      dpf->create_participant(MY_DOMAIN,
                              PARTICIPANT_QOS_DEFAULT,
                              DDS::DomainParticipantListener::_nil(),
                              OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  OpenDDS::DCPS::TransportConfig_rch cfg = TheTransportRegistry->get_config("rtps");
  if (!cfg.is_nil()) {
    TheTransportRegistry->bind_config(cfg, dp);
  }
  cfg = TheTransportRegistry->get_config("rtps2");
  if (!cfg.is_nil()) {
    TheTransportRegistry->bind_config(cfg, dp2);
  }

  fts->register_type(dp, MY_TYPE);
  fts2->register_type(dp2, MY_TYPE);

  DDS::TopicQos topic_qos;
  dp->get_default_topic_qos(topic_qos);
  topic_qos.resource_limits.max_samples_per_instance = max_samples_per_instance;
  topic_qos.history.depth = history_depth;

  DDS::Topic_var automatic_topic =
      dp->create_topic (AUTOMATIC_TOPIC, MY_TYPE, topic_qos,
                        DDS::TopicListener::_nil(), OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  DDS::Topic_var manual_topic =
      dp->create_topic (MANUAL_TOPIC, MY_TYPE, topic_qos,
                        DDS::TopicListener::_nil(), OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  DDS::Topic_var automatic_topic2 =
      dp2->create_topic (AUTOMATIC_TOPIC, MY_TYPE, topic_qos,
                         DDS::TopicListener::_nil(), OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  DDS::Topic_var manual_topic2 =
      dp2->create_topic (MANUAL_TOPIC, MY_TYPE, topic_qos,
                         DDS::TopicListener::_nil(), OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  // Create the publisher
  DDS::Publisher_var pub =
      dp->create_publisher(PUBLISHER_QOS_DEFAULT,
                           DDS::PublisherListener::_nil(), OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  // Create the datawriters
  DDS::DataWriterQos automatic_dw_qos;
  pub->get_default_datawriter_qos (automatic_dw_qos);
  automatic_dw_qos.history.depth = history_depth;
  automatic_dw_qos.resource_limits.max_samples_per_instance = max_samples_per_instance;
  automatic_dw_qos.liveliness.kind = DDS::AUTOMATIC_LIVELINESS_QOS;
  automatic_dw_qos.liveliness.lease_duration.sec = LEASE_DURATION_SEC;
  automatic_dw_qos.liveliness.lease_duration.nanosec = 0;
  if (use_reliable) {
    automatic_dw_qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;
  }
  DDS::DataWriterQos manual_dw_qos;
  pub->get_default_datawriter_qos (manual_dw_qos);
  manual_dw_qos.history.depth = history_depth;
  manual_dw_qos.resource_limits.max_samples_per_instance = max_samples_per_instance;
  manual_dw_qos.liveliness.kind = DDS::MANUAL_BY_PARTICIPANT_LIVELINESS_QOS;
  manual_dw_qos.liveliness.lease_duration.sec = LEASE_DURATION_SEC;
  manual_dw_qos.liveliness.lease_duration.nanosec = 0;
  if (use_reliable) {
    manual_dw_qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;
  }

  DDS::DataWriter_var dw_automatic =
      pub->create_datawriter(automatic_topic, automatic_dw_qos,
                             DDS::DataWriterListener::_nil(), OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  DDS::DataWriter_var dw_manual =
      pub->create_datawriter(manual_topic, manual_dw_qos,
                             DDS::DataWriterListener::_nil(), OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  DDS::TopicDescription_var manual_description = dp->lookup_topicdescription(MANUAL_TOPIC);
  DDS::TopicDescription_var automatic_description2 = dp2->lookup_topicdescription(AUTOMATIC_TOPIC);
  DDS::TopicDescription_var manual_description2 = dp2->lookup_topicdescription(MANUAL_TOPIC);

  // Create the subscriber
  DDS::Subscriber_var remote_sub =
      dp2->create_subscriber(SUBSCRIBER_QOS_DEFAULT, DDS::SubscriberListener::_nil(),
                             OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  DDS::Subscriber_var local_sub =
      dp->create_subscriber(SUBSCRIBER_QOS_DEFAULT, DDS::SubscriberListener::_nil(),
                            OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  // Create the Datareaders
  DDS::DataReaderQos automatic_dr_qos;
  remote_sub->get_default_datareader_qos (automatic_dr_qos);
  automatic_dr_qos.history.depth = history_depth;
  automatic_dr_qos.resource_limits.max_samples_per_instance = max_samples_per_instance;
  automatic_dr_qos.liveliness.kind = DDS::AUTOMATIC_LIVELINESS_QOS;
  automatic_dr_qos.liveliness.lease_duration.sec = LEASE_DURATION_SEC;
  automatic_dr_qos.liveliness.lease_duration.nanosec = 0;
  if (use_reliable) {
    automatic_dr_qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;
  }
  DDS::DataReaderQos remote_manual_dr_qos;
  remote_sub->get_default_datareader_qos (remote_manual_dr_qos);
  remote_manual_dr_qos.history.depth = history_depth;
  remote_manual_dr_qos.resource_limits.max_samples_per_instance = max_samples_per_instance;
  remote_manual_dr_qos.liveliness.kind = DDS::MANUAL_BY_PARTICIPANT_LIVELINESS_QOS;
  remote_manual_dr_qos.liveliness.lease_duration.sec = LEASE_DURATION_SEC;
  remote_manual_dr_qos.liveliness.lease_duration.nanosec = 0;
  if (use_reliable) {
    remote_manual_dr_qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;
  }
  DDS::DataReaderQos local_manual_dr_qos;
  local_sub->get_default_datareader_qos (local_manual_dr_qos);
  local_manual_dr_qos.history.depth = history_depth;
  local_manual_dr_qos.resource_limits.max_samples_per_instance = max_samples_per_instance;
  local_manual_dr_qos.liveliness.kind = DDS::MANUAL_BY_PARTICIPANT_LIVELINESS_QOS;
  local_manual_dr_qos.liveliness.lease_duration.sec = LEASE_DURATION_SEC;
  local_manual_dr_qos.liveliness.lease_duration.nanosec = 0;
  if (use_reliable) {
    local_manual_dr_qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;
  }

  DistributedConditionSet_rch dcs = OpenDDS::DCPS::make_rch<InMemoryDistributedConditionSet>();

  DataReaderListenerImpl* automatic_drl_servant = new DataReaderListenerImpl(dcs, AUTOMATIC_DATAREADER);
  DDS::DataReaderListener_var automatic_drl(automatic_drl_servant);
  DataReaderListenerImpl* remote_manual_drl_servant = new DataReaderListenerImpl(dcs, REMOTE_MANUAL_DATAREADER);
  DDS::DataReaderListener_var remote_manual_drl(remote_manual_drl_servant);
  DataReaderListenerImpl* local_manual_drl_servant = new DataReaderListenerImpl(dcs, LOCAL_MANUAL_DATAREADER);
  DDS::DataReaderListener_var local_manual_drl (local_manual_drl_servant);

  DDS::DataReader_var automatic_dr =
    remote_sub->create_datareader(automatic_description2,
                                  automatic_dr_qos,
                                  automatic_drl,
                                  OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  if (CORBA::is_nil (automatic_dr.in ())) {
    ACE_ERROR_RETURN ((LM_ERROR,
      ACE_TEXT("(%P|%t) create_datareader failed.\n")), 1);
  }
  DDS::DataReader_var remote_manual_dr =
    remote_sub->create_datareader(manual_description2,
                                  remote_manual_dr_qos,
                                  remote_manual_drl,
                                  OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  DDS::DataReader_var local_manual_dr =
    local_sub->create_datareader(manual_description,
                                 local_manual_dr_qos,
                                 local_manual_drl,
                                 OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  if (Utils::wait_match(dw_automatic, 1, Utils::EQ)) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("Error waiting for match for dw_automatic\n")));
    return 1;
  }

  if (Utils::wait_match(automatic_dr, 1, Utils::EQ)) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("Error waiting for match for automatic_dr\n")));
    return 1;
  }

  if (Utils::wait_match(dw_manual, 2, Utils::EQ)) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("Error waiting for match for dw_manual\n")));
    return 1;
  }

  if (Utils::wait_match(remote_manual_dr, 1, Utils::EQ)) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("Error waiting for match for remote_manual_dr\n")));
    return 1;
  }

  if (Utils::wait_match(local_manual_dr, 1, Utils::EQ)) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("Error waiting for match for local_manual_dr\n")));
    return 1;
  }

  // send an automatic message to show manual readers are not
  // notified of liveliness.
  ::Xyz::Foo foo;
  foo.x = -1;
  foo.y = -1;
  foo.key = 101010;
  ::Xyz::FooDataWriter_var foo_dw = ::Xyz::FooDataWriter::_narrow(dw_automatic);
  DDS::InstanceHandle_t handle = foo_dw->register_instance(foo);
  foo_dw->write(foo, handle);

  Writer* writer = new Writer(dw_manual, num_ops_per_thread);

  for (int i = 1; i <= num_unlively_periods; ++i) {
    // Write a sample.
    writer->run_test(i - 1);
    // Wait for liveliness gained.
    dcs->wait_for("driver", REMOTE_MANUAL_DATAREADER, "LIVELINESS_GAINED_" + OpenDDS::DCPS::to_dds_string(i));
    dcs->wait_for("driver", LOCAL_MANUAL_DATAREADER, "LIVELINESS_GAINED_" + OpenDDS::DCPS::to_dds_string(i));
    // Wait for liveliness lost.
    dcs->wait_for("driver", REMOTE_MANUAL_DATAREADER, "LIVELINESS_LOST_" + OpenDDS::DCPS::to_dds_string(i));
    dcs->wait_for("driver", LOCAL_MANUAL_DATAREADER, "LIVELINESS_LOST_" + OpenDDS::DCPS::to_dds_string(i));
  }

  delete writer;
  pub->delete_contained_entities();
  dp->delete_publisher(pub);

  if (Utils::wait_match(automatic_dr, 0, Utils::EQ)) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("Error waiting for match for automatic_dr\n")));
    return 1;
  }

  if (Utils::wait_match(remote_manual_dr, 0, Utils::EQ)) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("Error waiting for match for remote_manual_dr\n")));
    return 1;
  }

  if (Utils::wait_match(local_manual_dr, 0, Utils::EQ)) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("Error waiting for match for local_manual_dr\n")));
    return 1;
  }

  int expected_manual_liveliness_changes = 3 + (2 * (num_unlively_periods));
  int expected_no_writers_generation_count = (use_take ? 0 : num_unlively_periods - 1);
  // Determine the test status at this point.

  ACE_OS::fprintf(stderr, "**********\n");
  ACE_OS::fprintf(stderr, "automatic_drl_servant->liveliness_changed_count() = %d\n",
                  automatic_drl_servant->liveliness_changed_count());
  ACE_OS::fprintf(stderr, "automatic_drl_servant->no_writers_generation_count() = %d\n",
                  automatic_drl_servant->no_writers_generation_count());
  ACE_OS::fprintf(stderr, "********** use_take=%d\n", use_take);

  //automatic should stay alive due to reactor,
  //going up at start then coming down at end
  if (automatic_drl_servant->liveliness_changed_count() != 3) {
    status = 1;
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: automatic_drl_servant->liveliness_changed_count is %d - ")
               ACE_TEXT("expected 3\n"), automatic_drl_servant->liveliness_changed_count()
               ));
  } else if (automatic_drl_servant->verify_last_liveliness_status() == false) {
    status = 1;
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: automatic_drl_servant - ")
               ACE_TEXT("test shut down while alive.\n")
               ));
  } else if (automatic_drl_servant->no_writers_generation_count() != 0) {
    status = 1;
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: automatic_drl_servant->no_writers_generation_count is %d - ")
               ACE_TEXT("expected %d\n"), automatic_drl_servant->no_writers_generation_count(), 0
               ));
  }
  ACE_OS::fprintf(stderr, "**********\n");
  ACE_OS::fprintf(stderr, "remote_manual_drl_servant->liveliness_changed_count() = %d\n",
                  remote_manual_drl_servant->liveliness_changed_count());
  ACE_OS::fprintf(stderr, "remote_manual_drl_servant->no_writers_generation_count() = %d\n",
                  remote_manual_drl_servant->no_writers_generation_count());
  ACE_OS::fprintf(stderr, "********** use_take=%d\n", use_take);

  if (remote_manual_drl_servant->liveliness_changed_count() != expected_manual_liveliness_changes) {
    status = 1;
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: remote_manual_drl_servant->liveliness_changed_count is %d - ")
               ACE_TEXT("expected %d\n"), remote_manual_drl_servant->liveliness_changed_count(), expected_manual_liveliness_changes
               ));
  } else if (remote_manual_drl_servant->verify_last_liveliness_status() == false) {
    status = 1;
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: remote_manual_drl_servant - ")
               ACE_TEXT("test shut down while alive.\n")
               ));
  } else if (remote_manual_drl_servant->no_writers_generation_count() != expected_no_writers_generation_count) {
    status = 1;
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: remote_manual_drl_servant->no_writers_generation_count is %d - ")
               ACE_TEXT("expected %d\n"), remote_manual_drl_servant->no_writers_generation_count(), num_unlively_periods - 1
               ));
  }
  ACE_OS::fprintf(stderr, "**********\n");
  ACE_OS::fprintf(stderr, "local_manual_drl_servant->liveliness_changed_count() = %d\n",
                  local_manual_drl_servant->liveliness_changed_count());
  ACE_OS::fprintf(stderr, "local_manual_drl_servant->no_writers_generation_count() = %d\n",
                  local_manual_drl_servant->no_writers_generation_count());
  ACE_OS::fprintf(stderr, "********** use_take=%d\n", use_take);

  if (local_manual_drl_servant->liveliness_changed_count() < expected_manual_liveliness_changes) {
    status = 1;
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: local_manual_drl_servant->liveliness_changed_count is %d - ")
               ACE_TEXT("expected %d\n"), local_manual_drl_servant->liveliness_changed_count(), expected_manual_liveliness_changes
               ));
  } else if (local_manual_drl_servant->verify_last_liveliness_status() == false) {
    status = 1;
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: local_manual_drl_servant - ")
               ACE_TEXT("test shut down while alive.\n")
               ));
  } else if (local_manual_drl_servant->no_writers_generation_count() != expected_no_writers_generation_count) {
    status = 1;
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("(%P|%t) ERROR: local_manual_drl_servant->no_writers_generation_count is %d - ")
               ACE_TEXT("expected %d\n"), local_manual_drl_servant->no_writers_generation_count(), num_unlively_periods - 1
               ));
  }
  local_sub->delete_contained_entities();
  remote_sub->delete_contained_entities();
  dp->delete_subscriber(local_sub);
  dp->delete_subscriber(remote_sub);
  dp->delete_topic(automatic_topic);
  dp->delete_topic(manual_topic);
  dpf->delete_participant(dp);
  dpf->delete_participant(dp2);
  TheServiceParticipant->shutdown();

  return status;
}
