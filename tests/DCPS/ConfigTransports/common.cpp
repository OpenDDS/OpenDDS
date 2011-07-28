// -*- C++ -*-
// ============================================================================
/**
 *  @file   common.cpp
 *
 *  $Id$
 *
 *
 */
// ============================================================================

#include "common.h"
#include "DDSTEST.h"
#include "DataWriterListenerImpl.h"
#include "DataReaderListener.h"

#include "../common/TestSupport.h"

#include "../FooType4/FooDefTypeSupportImpl.h"

#include "dds/DCPS/WaitSet.h"
#include "dds/DCPS/Marked_Default_Qos.h"
#include "dds/DCPS/transport/framework/TransportRegistry.h"

#include "ace/SString.h"
#include "ace/Arg_Shifter.h"

#include <stdexcept>
#include <string>

Options::Options() :
        test_duration(40),
        test_duration_str(ACE_TEXT("40")),
        LEASE_DURATION_STR(ACE_TEXT("infinite")),
        reliability_kind(::DDS::BEST_EFFORT_RELIABILITY_QOS),
        reliability_kind_str(ACE_TEXT("best_effort")),
        durability_kind(::DDS::TRANSIENT_LOCAL_DURABILITY_QOS),
        durability_kind_str(ACE_TEXT("transient_local")),
        liveliness_kind(::DDS::AUTOMATIC_LIVELINESS_QOS),
        liveliness_kind_str(ACE_TEXT("automatic")),
        compatible(false),
        entity_autoenable(true),
        entity_str(ACE_TEXT("none")),
        collocation_str(ACE_TEXT("none")),
        configuration_str(ACE_TEXT("none"))
{
  LEASE_DURATION.sec = ::DDS::DURATION_INFINITE_SEC;
  LEASE_DURATION.nanosec = ::DDS::DURATION_INFINITE_NSEC;
}

/// parse the command line arguments
/// Options:
///  -e entity                   defaults to none. may be used multiple times
///  -a collocation              defaults to none
///  -s transport configuration  defaults to none
///  -p effective transport protocol     defaults to none

///  -c expect compatibility     defaults to false
///  -t effective transport protocol     defaults to none. may be used multiple times
///  -d durability kind          defaults to TRANSIENT_LOCAL_DURABILITY_QOS
///  -k liveliness kind          defaults to AUTOMATIC_LIVELINESS_QOS
///  -r reliability kind         defaults to BEST_EFFORT_RELIABILITY_QOS
///  -l lease duration           no default
///  -x test duration in sec     defaults to 40

Options::Options(int argc, ACE_TCHAR *argv[]) :
        test_duration(40),
        test_duration_str(ACE_TEXT("40")),
        reliability_kind(::DDS::BEST_EFFORT_RELIABILITY_QOS),
        reliability_kind_str(ACE_TEXT("best_effort")),
        durability_kind(::DDS::TRANSIENT_LOCAL_DURABILITY_QOS),
        durability_kind_str(ACE_TEXT("transient_local")),
        liveliness_kind(::DDS::AUTOMATIC_LIVELINESS_QOS),
        liveliness_kind_str(ACE_TEXT("automatic")),
        compatible(false),
        entity_autoenable(true),
        entity_str(ACE_TEXT("none")),
        collocation_str(ACE_TEXT("none")),
        configuration_str(ACE_TEXT("none"))
{
  u_long mask = ACE_LOG_MSG->priority_mask(ACE_Log_Msg::PROCESS);
  ACE_LOG_MSG->priority_mask(mask | LM_TRACE | LM_DEBUG, ACE_Log_Msg::PROCESS);
  ACE_Arg_Shifter arg_shifter(argc, argv);

  // Swallow the executable name
  arg_shifter.consume_arg();

  LEASE_DURATION.sec = ::DDS::DURATION_INFINITE_SEC;
  LEASE_DURATION.nanosec = ::DDS::DURATION_INFINITE_NSEC;

  while (arg_shifter.is_anything_left())
    {
      const ACE_TCHAR *currentArg = 0;

      if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-t"))) != 0)
        {
          this->protocol_str.push_back(currentArg);
          arg_shifter.consume_arg();
        }
      else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-n"))) != 0)
        {
          this->entity_autoenable = this->get_entity_autoenable_kind(currentArg);
          arg_shifter.consume_arg();
        }
      else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-e"))) != 0)
        {
          this->entity_str = this->get_entity_kind(currentArg);
          arg_shifter.consume_arg();
        }
      else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-a"))) != 0)
        {
          this->collocation_str = this->get_collocation_kind(currentArg);
          arg_shifter.consume_arg();
        }
      else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-s"))) != 0)
        {
          this->configuration_str = currentArg;
          arg_shifter.consume_arg();
        }
      else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-c"))) != 0)
        {
          this->compatible = (ACE_TString(currentArg) == ACE_TEXT("true"));
          arg_shifter.consume_arg();
        }
      else if (0 == arg_shifter.cur_arg_strncasecmp(ACE_TEXT("-d")))
        {

          this->durability_kind_str = arg_shifter.get_the_parameter(ACE_TEXT("-d"));
          this->durability_kind = this->get_durability_kind(this->durability_kind_str);
          arg_shifter.consume_arg();
        }
      else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-k"))) != 0)
        {
          this->liveliness_kind_str = currentArg;
          this->liveliness_kind = this->get_liveliness_kind(this->liveliness_kind_str);
          arg_shifter.consume_arg();
        }
      else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-l"))) != 0)
        {
          this->LEASE_DURATION_STR = currentArg;
          this->LEASE_DURATION = this->get_lease_duration(this->LEASE_DURATION_STR);
          arg_shifter.consume_arg();
        }
      else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-r"))) != 0)
        {
          this->reliability_kind_str = currentArg;
          this->reliability_kind = this->get_reliability_kind(this->reliability_kind_str);
          arg_shifter.consume_arg();
        }
      else if ((currentArg = arg_shifter.get_the_parameter(ACE_TEXT("-x"))) != 0)
        {
          this->test_duration = ACE_OS::atoi(currentArg);
          arg_shifter.consume_arg();
        }
      else
        {
          ACE_ERROR((LM_WARNING,
                     ACE_TEXT("(%P|%t) Ignoring command line argument: %C\n"), arg_shifter.get_current()));
          arg_shifter.ignore_arg();
        }
    }
}

Options::~Options() { }

const std::string&
Options::get_collocation_kind(const std::string& argument)
{
  if (argument == ACE_TEXT("none"))
    {
      return argument;
    }
  else if (argument == ACE_TEXT("process"))
    {
      return argument;
    }
  else if (argument == ACE_TEXT("participant"))
    {
      return argument;
    }
  else if (argument == ACE_TEXT("pubsub"))
    {
      return argument;
    }
  throw std::runtime_error("invalid string for get_collocation_kind");
}

const std::string&
Options::get_entity_kind(const std::string& argument)
{
  if (argument == ACE_TEXT("none"))
    {
      return argument;
    }
  else if (argument == ACE_TEXT("rw"))
    {
      return argument;
    }
  else if (argument == ACE_TEXT("pubsub"))
    {
      return argument;
    }
  else if (argument == ACE_TEXT("participant"))
    {
      return argument;
    }
  throw std::runtime_error("invalid string for get_entity_kind");
}

bool
Options::get_entity_autoenable_kind(const std::string& argument)
{
  if (argument == ACE_TEXT("yes"))
    {
      return true;
    }
  else if (argument == ACE_TEXT("true"))
    {
      return true;
    }
  else if (argument == ACE_TEXT("no"))
    {
      return false;
    }
  else if (argument == ACE_TEXT("false"))
    {
      return false;
    }
  throw std::runtime_error("invalid string for get_entity_kind");
}

::DDS::LivelinessQosPolicyKind
Options::get_liveliness_kind(const std::string& argument)
{
  if (argument == ACE_TEXT("automatic"))
    {
      return ::DDS::AUTOMATIC_LIVELINESS_QOS;
    }
  else if (argument == ACE_TEXT("participant"))
    {
      return ::DDS::MANUAL_BY_PARTICIPANT_LIVELINESS_QOS;
    }
  else if (argument == ACE_TEXT("topic"))
    {
      return ::DDS::MANUAL_BY_TOPIC_LIVELINESS_QOS;
    }
  throw std::runtime_error("invalid string for get_liveliness_kind");
}

::DDS::DurabilityQosPolicyKind
Options::get_durability_kind(const std::string& argument)
{
  if (argument == ACE_TEXT("volatile"))
    {
      return ::DDS::VOLATILE_DURABILITY_QOS;
    }
  else if (argument == ACE_TEXT("transient_local"))
    {
      return ::DDS::TRANSIENT_LOCAL_DURABILITY_QOS;
    }
  else if (argument == ACE_TEXT("transient"))
    {
      return ::DDS::TRANSIENT_DURABILITY_QOS;
    }
  else if (argument == ACE_TEXT("persistent"))
    {
      return ::DDS::PERSISTENT_DURABILITY_QOS;
    }

  std::string message("invalid string for get_durability_kind: ");
  throw std::runtime_error(message + argument.c_str());
}

::DDS::ReliabilityQosPolicyKind
Options::get_reliability_kind(const std::string& argument)
{
  if (argument == ACE_TEXT("best_effort"))
    {
      return ::DDS::BEST_EFFORT_RELIABILITY_QOS;
    }
  else if (argument == ACE_TEXT("reliable"))
    {
      return ::DDS::RELIABLE_RELIABILITY_QOS;
    }
  throw std::runtime_error("invalid string for get_reliability_kind");
}

::DDS::Duration_t
Options::get_lease_duration(const std::string& argument)
{
  ::DDS::Duration_t lease;
  if (argument == ACE_TEXT("infinite"))
    {
      lease.sec = ::DDS::DURATION_INFINITE_SEC;
      lease.nanosec = ::DDS::DURATION_INFINITE_NSEC;
    }
  else
    {
      lease.sec = ACE_OS::atoi(argument.c_str());
      lease.nanosec = 0;
    }
  return lease;
}

DDS::DomainParticipant_ptr
create_plain_participant(DDS::DomainParticipantFactory_ptr dpf)
{
  DDS::DomainParticipant_var dp =
          dpf->create_participant(MY_DOMAIN,
                                  PARTICIPANT_QOS_DEFAULT,
                                  DDS::DomainParticipantListener::_nil(),
                                  OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  Xyz::FooTypeSupport_var fts(new Xyz::FooTypeSupportImpl);
  TEST_ASSERT(DDS::RETCODE_OK == fts->register_type(dp.in(), MY_TYPE));

  return dp._retn();

}

Factory::Factory(const Options& opts) : opts_(opts) { }

Factory::~Factory() { }

DDS::DomainParticipant_ptr
Factory::participant(DDS::DomainParticipantFactory_ptr dpf) const
{
  DDS::DomainParticipant_var dp =
          dpf->create_participant(MY_DOMAIN,
                                  PARTICIPANT_QOS_DEFAULT,
                                  DDS::DomainParticipantListener::_nil(),
                                  OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  // If there is a ini file-based configuration name initialize
  // the transport configuration for the corresponding Entity
  if (opts_.configuration_str != "none" && opts_.entity_str == "participant")
    {
      OpenDDS::DCPS::TransportRegistry::instance()->bind_config(opts_.configuration_str, dp.in());
    }


  Xyz::FooTypeSupport_var fts(new Xyz::FooTypeSupportImpl);
  TEST_ASSERT(DDS::RETCODE_OK == fts->register_type(dp.in(), MY_TYPE));

  return dp._retn();

}

DDS::Topic_ptr
Factory::topic(DDS::DomainParticipant_ptr dp) const
{
  TEST_CHECK(dp != 0);

  // When collocation doesn't matter we choose a topic name that will not match
  // the publisher's topic name
  std::string topicname((opts_.collocation_str == "none") ? MY_OTHER_TOPIC : MY_SAME_TOPIC);


  DDS::TopicQos topic_qos;
  TEST_CHECK(DDS::RETCODE_OK == dp->get_default_topic_qos(topic_qos));

  DDS::Topic_var p(dp->create_topic(topicname.c_str(),
                                    MY_TYPE,
                                    TOPIC_QOS_DEFAULT,
                                    DDS::TopicListener::_nil(),
                                    OpenDDS::DCPS::DEFAULT_STATUS_MASK));
  TEST_CHECK(!CORBA::is_nil(p.in()));
  return p._retn();
}

DDS::Publisher_ptr
Factory::publisher(DDS::DomainParticipant_ptr dp) const
{

  DDS::PublisherQos pub_qos;
  dp->get_default_publisher_qos(pub_qos);
  if (opts_.entity_str == "rw")
    {
      pub_qos.entity_factory.autoenable_created_entities = opts_.entity_autoenable;
    }

  // Create the publisher
  DDS::Publisher_var pub = dp->create_publisher(pub_qos,
                                                DDS::PublisherListener::_nil(),
                                                OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  TEST_ASSERT(!CORBA::is_nil(pub.in()));

  // If there is a ini file-based configuration name initialize
  // the transport configuration for the corresponding Entity
  if (opts_.configuration_str != "none" && opts_.entity_str == "pubsub")
    {
      OpenDDS::DCPS::TransportRegistry::instance()->bind_config(opts_.configuration_str, pub.in());
    }

  return pub._retn();
}

DDS::Subscriber_ptr
Factory::subscriber(DDS::DomainParticipant_ptr dp) const
{

  DDS::SubscriberQos sub_qos;
  dp->get_default_subscriber_qos(sub_qos);
  if (opts_.entity_str == "rw")
    {
      sub_qos.entity_factory.autoenable_created_entities = opts_.entity_autoenable;
    }

  // Create the subscriber
  DDS::Subscriber_var sub =
          dp->create_subscriber(sub_qos,
                                DDS::SubscriberListener::_nil(),
                                OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  TEST_ASSERT(!CORBA::is_nil(sub.in()));

  // If there is a ini file-based configuration name initialize
  // the transport configuration for the corresponding Entity
  if (opts_.configuration_str != "none" && opts_.entity_str == "pubsub")
    {
      OpenDDS::DCPS::TransportRegistry::instance()->bind_config(opts_.configuration_str, sub.in());
    }

  return sub._retn();
}

DDS::DataWriter_ptr
Factory::writer(DDS::Publisher_ptr pub, DDS::Topic_ptr topic, DDS::DataWriterListener_ptr dwl) const
{
  // Create the data writer
  DDS::DataWriterQos dw_qos;
  pub->get_default_datawriter_qos(dw_qos);

  dw_qos.durability.kind = opts_.durability_kind;
  dw_qos.liveliness.kind = opts_.liveliness_kind;
  dw_qos.liveliness.lease_duration = opts_.LEASE_DURATION;
  dw_qos.reliability.kind = opts_.reliability_kind;

  DDS::DataWriter_var dw = pub->create_datawriter(topic,
                                                  dw_qos,
                                                  dwl,
                                                  OpenDDS::DCPS::DEFAULT_STATUS_MASK);

  // Initialize the transport configuration for the appropriate entity
  if (opts_.configuration_str != "none" && opts_.entity_str == "rw")
    {
      OpenDDS::DCPS::TransportRegistry::instance()->bind_config(opts_.configuration_str,
                                                                dw.in());

      if (!opts_.entity_autoenable)
        {
          TEST_ASSERT(DDS::RETCODE_OK == dw->enable());
        }

    }

  return dw._retn();
}

DDS::DataReader_ptr
Factory::reader(DDS::Subscriber_ptr sub, DDS::Topic_ptr topic, DDS::DataReaderListener_ptr drl) const
{
  // Create the data readers
  DDS::DataReaderQos dr_qos;
  sub->get_default_datareader_qos(dr_qos);

  dr_qos.durability.kind = opts_.durability_kind;
  dr_qos.liveliness.kind = opts_.liveliness_kind;
  dr_qos.liveliness.lease_duration = opts_.LEASE_DURATION;
  dr_qos.reliability.kind = opts_.reliability_kind;

  DDS::TopicDescription_var description =
          sub->get_participant()->lookup_topicdescription(topic->get_name());
  TEST_ASSERT(!CORBA::is_nil(description.in()));

  DDS::DataReader_var rd(sub->create_datareader(description.in(),
                                                dr_qos,
                                                drl,
                                                ::OpenDDS::DCPS::DEFAULT_STATUS_MASK));

  // Initialize the transport configuration for the appropriate entity
  TEST_ASSERT(!opts_.configuration_str.empty());
  if (opts_.configuration_str != "none" && opts_.entity_str == "rw")
    {

      OpenDDS::DCPS::TransportRegistry::instance()->bind_config(opts_.configuration_str,
                                                                rd.in());
      if (!opts_.entity_autoenable)
        {
          TEST_ASSERT(DDS::RETCODE_OK == rd->enable());
        }
    }

  return rd._retn();
}

bool
assert_subscription_matched(const Options& opts, DDS::DataReaderListener_ptr drl)
{
  // Assert if pub/sub made a match ...

  DataReaderListenerImpl* drl_servant =
          dynamic_cast<DataReaderListenerImpl*> (drl);

  // there is an error if we matched when not compatible (or vice-versa)
  if (opts.compatible != drl_servant->subscription_matched())
    {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) Expected publication_matched to be %C, but it was %C [")
                        ACE_TEXT(" durability_kind=%s, liveliness_kind=%s, liveliness_duration=%s, ")
                        ACE_TEXT("reliability_kind=%s]\n"),
                        (opts.compatible) ? "true" : "false",
                        (drl_servant->subscription_matched()) ? "true" : "false",
                        opts.durability_kind_str.c_str(),
                        opts.liveliness_kind_str.c_str(),
                        opts.LEASE_DURATION_STR.c_str(),
                        opts.reliability_kind_str.c_str()),
                       false);
    }

  return true;
}

bool
assert_publication_matched(const Options& opts, DDS::DataWriterListener_ptr dwl)
{
  // Assert if pub/sub made a match ...
  DataWriterListenerImpl* dwl_servant =
          dynamic_cast<DataWriterListenerImpl*> (dwl);

  // check to see if the publisher worked
  if (opts.compatible != dwl_servant->publication_matched())
    {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) Expected publication_matched to be %C, but it was %C [")
                        ACE_TEXT(" durability_kind=%s, liveliness_kind=%s, liveliness_duration=%s, ")
                        ACE_TEXT("reliability_kind=%s]\n"),
                        (opts.compatible) ? "true" : "false",
                        (dwl_servant->publication_matched()) ? "true" : "false",
                        opts.durability_kind_str.c_str(),
                        opts.liveliness_kind_str.c_str(),
                        opts.LEASE_DURATION_STR.c_str(),
                        opts.reliability_kind_str.c_str()),
                       false);
    }

  return true;
}

bool
assert_supports_all(const Options& f, const DDS::Entity_ptr e)
{
  TEST_ASSERT(e != 0);

  const OpenDDS::DCPS::TransportClient* tc = dynamic_cast<const OpenDDS::DCPS::TransportClient*> (e);
  TEST_ASSERT(tc != 0);

  return assert_supports_all(f, tc, f.protocol_str);
}

bool
assert_supports_all(const Options& opts, const OpenDDS::DCPS::TransportClient* tc, const std::vector<std::string>& transporti)
{
  TEST_ASSERT(tc != 0);

  // Assert effective transport protocols
  size_t left = transporti.size();
  for (std::vector < std::string>::const_iterator proto = transporti.begin();
          proto < transporti.end(); proto++)
    {
      bool issupported = ::DDS_TEST::supports(tc, *proto);
      ACE_ERROR((LM_INFO,
                 ACE_TEXT("(%P|%t) Validating that '%C' entity supports protocol '%C': %C\n"),
                 opts.entity_str.c_str(),
                 proto->c_str(),
                 issupported ? "true" : "false"));

      if (issupported) left--;
    }

  return (0 == left);
}

bool
wait_publication_matched_status(const Options& opts, const DDS::Entity_ptr /*writer_*/)
{

  int duration = opts.test_duration;

  ACE_OS::sleep(duration);
  return true;


  // To check the match status?
  //          DDS::SubscriptionMatchedStatus matches = {0, 0, 0, 0, 0};
  //          TEST_ASSERT((r.reader_->get_subscription_matched_status(matches) == ::DDS::RETCODE_OK));
  //          TEST_ASSERT(matches.current_count > 0);
  //
  //          DDS::PublicationMatchedStatus matches = {0, 0, 0, 0, 0};
  //          TEST_ASSERT((w.writer_->get_publication_matched_status(matches) == ::DDS::RETCODE_OK));
  //          TEST_ASSERT(matches.current_count > 0);


  //  // Block until Subscriber is available
  //  DDS::StatusCondition_var condition = writer_->get_statuscondition();
  //  condition->set_enabled_statuses(DDS::PUBLICATION_MATCHED_STATUS
  //                                  | DDS::SUBSCRIPTION_MATCHED_STATUS
  //                                  //                                  | DDS::REQUESTED_INCOMPATIBLE_QOS_STATUS
  //                                  //                                  | DDS::OFFERED_INCOMPATIBLE_QOS_STATUS
  //                                  );
  //
  //  DDS::WaitSet_var ws = new DDS::WaitSet;
  //  ws->attach_condition(condition);
  //
  //  DDS::Duration_t timeout = {
  //    (duration < 0) ? DDS::DURATION_INFINITE_SEC : duration,
  //    DDS::DURATION_INFINITE_NSEC
  //  };
  //
  //  DDS::ConditionSeq conditions;
  //
  //  int status = ws->wait(conditions, timeout);
  //  ws->detach_condition(condition);
  //
  //  if (status != DDS::RETCODE_OK)
  //    {
  //      ACE_ERROR_RETURN((LM_ERROR,
  //                        ACE_TEXT("(%P|$t)")
  //                        ACE_TEXT(" ERROR: wait failed at %N:%l\n")), false);
  //    }
  //
  //  return true;
}


