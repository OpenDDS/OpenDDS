#include "common.h"
#include "Factory.h"
#include "Options.h"
#include "../common/TestSupport.h"

#include "dds/DCPS/Marked_Default_Qos.h"
#include "dds/DCPS/transport/framework/TransportRegistry.h"

Factory::Factory(const Options& opts, const OpenDDS::DCPS::TypeSupport_var& typsup) :
        opts_(opts),
        typsup_(typsup)
{
  TEST_CHECK(typsup != 0);
}

Factory::~Factory() { }

DDS::DomainParticipant_var
Factory::participant(const DDS::DomainParticipantFactory_var& dpf) const
{
  DDS::DomainParticipant_var dp =
          dpf->create_participant(MY_DOMAIN,
                                  PARTICIPANT_QOS_DEFAULT,
                                  DDS::DomainParticipantListener::_nil(),
                                  OpenDDS::DCPS::DEFAULT_STATUS_MASK);
  TEST_CHECK(!CORBA::is_nil(dp.in()));

  // If there is a ini file-based configuration name initialize
  // the transport configuration for the corresponding Entity
  if (opts_.configuration_str != "none" && opts_.entity_str == "participant")
    {
      OpenDDS::DCPS::TransportRegistry::instance()->bind_config(opts_.configuration_str, dp.in());
    }


  // Register TypeSupport (Messenger::Message)
  CORBA::String_var tn = typsup_->get_type_name();
  TEST_CHECK(DDS::RETCODE_OK == typsup_->register_type(dp.in(), tn)); // Use the default name for the type

  return dp;

}

DDS::Topic_var
Factory::topic(const DDS::DomainParticipant_var& dp) const
{

  // When collocation doesn't matter we choose a topic name that will not match
  // the publisher's topic name
  std::string topicname((opts_.collocation_str == "none") ? MY_OTHER_TOPIC : MY_SAME_TOPIC);

  DDS::TopicQos topic_qos;
  TEST_CHECK(DDS::RETCODE_OK == dp->get_default_topic_qos(topic_qos));

  CORBA::String_var tn = typsup_->get_type_name();
  ACE_DEBUG((LM_DEBUG, ACE_TEXT("(%P|%t) Topic name: %C\n"), tn.in()));
  DDS::Topic_var p(dp->create_topic(topicname.c_str(),
                                    tn,
                                    TOPIC_QOS_DEFAULT,
                                    DDS::TopicListener::_nil(),
                                    OpenDDS::DCPS::DEFAULT_STATUS_MASK));
  TEST_CHECK(!CORBA::is_nil(p.in()));
  return p;
}

DDS::Publisher_var
Factory::publisher(const DDS::DomainParticipant_var& dp) const
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

  return pub;
}

DDS::Subscriber_var
Factory::subscriber(const DDS::DomainParticipant_var& dp) const
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

  return sub;
}

DDS::DataWriter_var
Factory::writer(const DDS::Publisher_var& pub, const DDS::Topic_var& topic, const DDS::DataWriterListener_var& dwl) const
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
                                                  dwl.in(),
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

  return dw;
}

DDS::DataReader_var
Factory::reader(const DDS::Subscriber_var& sub, const DDS::Topic_var& topic, const DDS::DataReaderListener_var& drl) const
{
  // Create the data readers
  DDS::DataReaderQos dr_qos;
  sub->get_default_datareader_qos(dr_qos);

  dr_qos.durability.kind = opts_.durability_kind;
  dr_qos.liveliness.kind = opts_.liveliness_kind;
  dr_qos.liveliness.lease_duration = opts_.LEASE_DURATION;
  dr_qos.reliability.kind = opts_.reliability_kind;

  DDS::DomainParticipant_var dp = sub->get_participant();
  CORBA::String_var tn = topic->get_name();
  DDS::TopicDescription_var description = dp->lookup_topicdescription(tn);
  TEST_ASSERT(!CORBA::is_nil(description.in()));

  DDS::DataReader_var rd(sub->create_datareader(description.in(),
                                                dr_qos,
                                                drl.in(),
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

  return rd;
}
