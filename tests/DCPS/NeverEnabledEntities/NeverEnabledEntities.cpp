#include "NeverEnabledEntitiesTypeSupportImpl.h"

#include <dds/DCPS/Marked_Default_Qos.h>
#include <dds/DCPS/Service_Participant.h>
#ifdef ACE_AS_STATIC_LIBS
#  include <dds/DCPS/RTPS/RtpsDiscovery.h>
#  include <dds/DCPS/transport/rtps_udp/RtpsUdp.h>
#endif

#include <gtest/gtest.h>

using namespace NeverEnabled;
using OpenDDS::DCPS::DEFAULT_STATUS_MASK;

namespace {
  const DDS::DomainId_t domain = 47;
  const char topic_name[] = "NeverEnabledTopic";

  DDS::DomainParticipant_ptr create_participant(bool autoenable)
  {
    DDS::DomainParticipantFactory_var dpf = TheParticipantFactory;
    DDS::DomainParticipantFactoryQos factory_qos;
    dpf->get_qos(factory_qos);
    factory_qos.entity_factory.autoenable_created_entities = autoenable;
    dpf->set_qos(factory_qos);
    DDS::DomainParticipant_ptr dp =
      dpf->create_participant(domain, PARTICIPANT_QOS_DEFAULT, 0, DEFAULT_STATUS_MASK);
    factory_qos.entity_factory.autoenable_created_entities = true;
    dpf->set_qos(factory_qos);
    return dp;
  }

  DDS::Topic_ptr create_topic(DDS::DomainParticipant_ptr dp, const char* name = topic_name)
  {
    MessageTypeSupport_var ts = new MessageTypeSupportImpl;
    if (ts->register_type(dp, "") != DDS::RETCODE_OK) {
      return 0;
    }
    CORBA::String_var type_name = ts->get_type_name();
    return dp->create_topic(name, type_name, TOPIC_QOS_DEFAULT, 0, DEFAULT_STATUS_MASK);
  }

  DDS::Publisher_ptr create_disabled_publisher(DDS::DomainParticipant_ptr dp)
  {
    DDS::PublisherQos qos;
    dp->get_default_publisher_qos(qos);
    qos.entity_factory.autoenable_created_entities = false;
    return dp->create_publisher(qos, 0, DEFAULT_STATUS_MASK);
  }

  DDS::Subscriber_ptr create_disabled_subscriber(DDS::DomainParticipant_ptr dp)
  {
    DDS::SubscriberQos qos;
    dp->get_default_subscriber_qos(qos);
    qos.entity_factory.autoenable_created_entities = false;
    return dp->create_subscriber(qos, 0, DEFAULT_STATUS_MASK);
  }

  DDS::DataWriter_ptr create_writer(DDS::Publisher_ptr pub, DDS::Topic_ptr topic)
  {
    DDS::DataWriterQos qos;
    pub->get_default_datawriter_qos(qos);
    qos.reliability.kind = DDS::RELIABLE_RELIABILITY_QOS;
    qos.liveliness.kind = DDS::MANUAL_BY_TOPIC_LIVELINESS_QOS;
    return pub->create_datawriter(topic, qos, 0, DEFAULT_STATUS_MASK);
  }

  DDS::DataReader_ptr create_reader(DDS::Subscriber_ptr sub, DDS::TopicDescription_ptr topic)
  {
    return sub->create_datareader(topic, DATAREADER_QOS_DEFAULT, 0, DEFAULT_STATUS_MASK);
  }
}

class NeverEnabledEntities : public ::testing::Test {
protected:
  void SetUp()
  {
    participant_ = create_participant(true);
    ASSERT_TRUE(participant_);
    topic_ = create_topic(participant_);
    ASSERT_TRUE(topic_);
  }

  void TearDown()
  {
    if (participant_) {
      EXPECT_EQ(DDS::RETCODE_OK, participant_->delete_contained_entities());
      DDS::DomainParticipantFactory_var dpf = TheParticipantFactory;
      EXPECT_EQ(DDS::RETCODE_OK, dpf->delete_participant(participant_));
    }
  }

  DDS::DomainParticipant_var participant_;
  DDS::Topic_var topic_;
};

TEST_F(NeverEnabledEntities, DeleteWriter)
{
  DDS::Publisher_var pub = create_disabled_publisher(participant_);
  ASSERT_TRUE(pub);
  DDS::DataWriter_var dw = create_writer(pub, topic_);
  ASSERT_TRUE(dw);

  EXPECT_EQ(DDS::RETCODE_PRECONDITION_NOT_MET, participant_->delete_publisher(pub));
  EXPECT_EQ(DDS::RETCODE_OK, pub->delete_datawriter(dw));
  EXPECT_EQ(DDS::RETCODE_OK, participant_->delete_publisher(pub));
  EXPECT_EQ(DDS::RETCODE_OK, participant_->delete_topic(topic_));
}

TEST_F(NeverEnabledEntities, DeleteWriterWithoutCallerReference)
{
  DDS::Publisher_var pub = create_disabled_publisher(participant_);
  ASSERT_TRUE(pub);
  DDS::DataWriter_ptr dw = create_writer(pub, topic_);
  ASSERT_TRUE(dw);
  CORBA::release(dw);
  EXPECT_EQ(DDS::RETCODE_OK, pub->delete_datawriter(dw));
}

TEST_F(NeverEnabledEntities, DeleteReader)
{
  DDS::Subscriber_var sub = create_disabled_subscriber(participant_);
  ASSERT_TRUE(sub);
  DDS::DataReader_var dr = create_reader(sub, topic_);
  ASSERT_TRUE(dr);

  EXPECT_EQ(DDS::RETCODE_PRECONDITION_NOT_MET, participant_->delete_subscriber(sub));
  EXPECT_EQ(DDS::RETCODE_OK, sub->delete_datareader(dr));
  EXPECT_EQ(DDS::RETCODE_OK, participant_->delete_subscriber(sub));
  EXPECT_EQ(DDS::RETCODE_OK, participant_->delete_topic(topic_));
}

TEST_F(NeverEnabledEntities, Topic)
{
  DDS::DomainParticipantQos qos;
  participant_->get_qos(qos);
  qos.entity_factory.autoenable_created_entities = false;
  ASSERT_EQ(DDS::RETCODE_OK, participant_->set_qos(qos));

  DDS::Topic_var topic = create_topic(participant_, "NeverEnabledTopic2");
  ASSERT_TRUE(topic);
  DDS::TopicQos topic_qos;
  EXPECT_EQ(DDS::RETCODE_OK, topic->get_qos(topic_qos));
  topic_qos.topic_data.value.length(1);
  topic_qos.topic_data.value[0] = 1;
  EXPECT_EQ(DDS::RETCODE_OK, topic->set_qos(topic_qos));
  EXPECT_EQ(DDS::RETCODE_OK, participant_->delete_topic(topic));
}

TEST_F(NeverEnabledEntities, WriterOperations)
{
  DDS::Publisher_var pub = create_disabled_publisher(participant_);
  ASSERT_TRUE(pub);
  DDS::DataWriter_var dw = create_writer(pub, topic_);
  ASSERT_TRUE(dw);
  MessageDataWriter_var mdw = MessageDataWriter::_narrow(dw);
  ASSERT_TRUE(mdw);

  Message msg;
  msg.id = 1;
  msg.value = 2;
  const DDS::Duration_t max_wait = {1, 0};
  EXPECT_EQ(DDS::RETCODE_NOT_ENABLED, dw->assert_liveliness());
  EXPECT_EQ(DDS::RETCODE_NOT_ENABLED, dw->wait_for_acknowledgments(max_wait));
  EXPECT_EQ(DDS::HANDLE_NIL, mdw->register_instance(msg));
  EXPECT_EQ(DDS::RETCODE_NOT_ENABLED, mdw->write(msg, DDS::HANDLE_NIL));
  // A non-nil handle skips registration and goes straight to serialization.
  EXPECT_EQ(DDS::RETCODE_NOT_ENABLED, mdw->write(msg, 1));
  EXPECT_EQ(DDS::RETCODE_NOT_ENABLED, mdw->dispose(msg, DDS::HANDLE_NIL));
  EXPECT_EQ(DDS::RETCODE_NOT_ENABLED, mdw->unregister_instance(msg, DDS::HANDLE_NIL));
  EXPECT_EQ(DDS::HANDLE_NIL, mdw->lookup_instance(msg));
  EXPECT_EQ(DDS::RETCODE_NOT_ENABLED, mdw->get_key_value(msg, 1));
  DDS::InstanceHandleSeq handles;
  EXPECT_EQ(DDS::RETCODE_NOT_ENABLED, dw->get_matched_subscriptions(handles));

  DDS::PublicationMatchedStatus matched;
  EXPECT_EQ(DDS::RETCODE_OK, dw->get_publication_matched_status(matched));
  DDS::LivelinessLostStatus lost;
  EXPECT_EQ(DDS::RETCODE_OK, dw->get_liveliness_lost_status(lost));
  DDS::DataWriterQos qos;
  EXPECT_EQ(DDS::RETCODE_OK, dw->get_qos(qos));
  qos.user_data.value.length(1);
  qos.user_data.value[0] = 1;
  EXPECT_EQ(DDS::RETCODE_OK, dw->set_qos(qos));

  ASSERT_EQ(DDS::RETCODE_OK, pub->enable());
  ASSERT_EQ(DDS::RETCODE_OK, dw->enable());
  EXPECT_EQ(DDS::RETCODE_OK, dw->assert_liveliness());
  EXPECT_EQ(DDS::RETCODE_OK, mdw->write(msg, DDS::HANDLE_NIL));
  EXPECT_EQ(DDS::RETCODE_OK, dw->wait_for_acknowledgments(max_wait));
}

TEST_F(NeverEnabledEntities, ReaderOperations)
{
  DDS::Subscriber_var sub = create_disabled_subscriber(participant_);
  ASSERT_TRUE(sub);
  DDS::DataReader_var dr = create_reader(sub, topic_);
  ASSERT_TRUE(dr);
  MessageDataReader_var mdr = MessageDataReader::_narrow(dr);
  ASSERT_TRUE(mdr);

  MessageSeq data;
  DDS::SampleInfoSeq infos;
  EXPECT_EQ(DDS::RETCODE_NOT_ENABLED, mdr->read(data, infos, DDS::LENGTH_UNLIMITED,
    DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE));
  EXPECT_EQ(DDS::RETCODE_NOT_ENABLED, mdr->take(data, infos, DDS::LENGTH_UNLIMITED,
    DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE));
  Message msg;
  DDS::SampleInfo info;
  EXPECT_EQ(DDS::RETCODE_NOT_ENABLED, mdr->read_next_sample(msg, info));
  EXPECT_EQ(DDS::RETCODE_NOT_ENABLED, mdr->take_next_sample(msg, info));
  const DDS::Duration_t max_wait = {0, 0};
  EXPECT_EQ(DDS::RETCODE_NOT_ENABLED, dr->wait_for_historical_data(max_wait));
  DDS::InstanceHandleSeq handles;
  EXPECT_EQ(DDS::RETCODE_NOT_ENABLED, dr->get_matched_publications(handles));

  DDS::SubscriptionMatchedStatus matched;
  EXPECT_EQ(DDS::RETCODE_OK, dr->get_subscription_matched_status(matched));
  DDS::DataReaderQos qos;
  EXPECT_EQ(DDS::RETCODE_OK, dr->get_qos(qos));
  qos.user_data.value.length(1);
  qos.user_data.value[0] = 1;
  EXPECT_EQ(DDS::RETCODE_OK, dr->set_qos(qos));

  ASSERT_EQ(DDS::RETCODE_OK, sub->enable());
  ASSERT_EQ(DDS::RETCODE_OK, dr->enable());
  EXPECT_EQ(DDS::RETCODE_NO_DATA, mdr->read(data, infos, DDS::LENGTH_UNLIMITED,
    DDS::ANY_SAMPLE_STATE, DDS::ANY_VIEW_STATE, DDS::ANY_INSTANCE_STATE));
  EXPECT_EQ(DDS::RETCODE_OK, dr->wait_for_historical_data(max_wait));
}

TEST_F(NeverEnabledEntities, DeleteContainedEntities)
{
  DDS::Publisher_var pub = create_disabled_publisher(participant_);
  ASSERT_TRUE(pub);
  DDS::DataWriter_var dw1 = create_writer(pub, topic_);
  ASSERT_TRUE(dw1);
  DDS::DataWriter_var dw2 = create_writer(pub, topic_);
  ASSERT_TRUE(dw2);

  DDS::Subscriber_var sub = create_disabled_subscriber(participant_);
  ASSERT_TRUE(sub);
  DDS::DataReader_var dr = create_reader(sub, topic_);
  ASSERT_TRUE(dr);
#ifndef OPENDDS_NO_CONTENT_FILTERED_TOPIC
  DDS::ContentFilteredTopic_var cft = participant_->create_contentfilteredtopic(
    "NeverEnabledCft", topic_, "value > 0", DDS::StringSeq());
  ASSERT_TRUE(cft);
  DDS::DataReader_var cft_dr = create_reader(sub, cft);
  ASSERT_TRUE(cft_dr);
#endif

  EXPECT_EQ(DDS::RETCODE_PRECONDITION_NOT_MET, participant_->delete_publisher(pub));
  EXPECT_EQ(DDS::RETCODE_OK, pub->delete_contained_entities());
  EXPECT_EQ(DDS::RETCODE_OK, participant_->delete_publisher(pub));

  EXPECT_EQ(DDS::RETCODE_PRECONDITION_NOT_MET, participant_->delete_subscriber(sub));
  EXPECT_EQ(DDS::RETCODE_OK, sub->delete_contained_entities());
  EXPECT_EQ(DDS::RETCODE_OK, participant_->delete_subscriber(sub));

#ifndef OPENDDS_NO_CONTENT_FILTERED_TOPIC
  EXPECT_EQ(DDS::RETCODE_OK, participant_->delete_contentfilteredtopic(cft));
#endif
  EXPECT_EQ(DDS::RETCODE_OK, participant_->delete_topic(topic_));
}

#ifndef OPENDDS_NO_MULTI_TOPIC
TEST_F(NeverEnabledEntities, DeleteMultiTopicReader)
{
  CORBA::String_var type_name = topic_->get_type_name();
  DDS::MultiTopic_var mt = participant_->create_multitopic(
    "NeverEnabledMultiTopic", type_name, "SELECT * FROM NeverEnabledTopic", DDS::StringSeq());
  ASSERT_TRUE(mt);

  DDS::Subscriber_var sub = create_disabled_subscriber(participant_);
  ASSERT_TRUE(sub);
  DDS::DataReader_var dr = create_reader(sub, mt);
  ASSERT_TRUE(dr);

  EXPECT_EQ(DDS::RETCODE_OK, sub->delete_datareader(dr));
  EXPECT_EQ(DDS::RETCODE_OK, participant_->delete_subscriber(sub));
  EXPECT_EQ(DDS::RETCODE_OK, participant_->delete_multitopic(mt));
  EXPECT_EQ(DDS::RETCODE_OK, participant_->delete_topic(topic_));
}
#endif

TEST(NeverEnabledParticipant, DeleteParticipant)
{
  DDS::DomainParticipantFactory_var dpf = TheParticipantFactory;
  DDS::DomainParticipant_var dp = create_participant(false);
  ASSERT_TRUE(dp);
  EXPECT_EQ(DDS::RETCODE_OK, dpf->delete_participant(dp));
}

TEST(NeverEnabledParticipant, DeleteWithoutCallerReference)
{
  DDS::DomainParticipantFactory_var dpf = TheParticipantFactory;
  for (int autoenable = 0; autoenable < 2; ++autoenable) {
    DDS::DomainParticipant_ptr dp = create_participant(autoenable);
    ASSERT_TRUE(dp);
    // Only the factory holds a reference, as with the modeling SDK's Service.
    CORBA::release(dp);
    EXPECT_EQ(DDS::RETCODE_OK, dpf->delete_participant(dp));
  }
}

TEST(NeverEnabledParticipant, SetQos)
{
  DDS::DomainParticipantFactory_var dpf = TheParticipantFactory;
  DDS::DomainParticipant_var dp = create_participant(false);
  ASSERT_TRUE(dp);
  DDS::DomainParticipantQos qos;
  EXPECT_EQ(DDS::RETCODE_OK, dp->get_qos(qos));
  qos.user_data.value.length(1);
  qos.user_data.value[0] = 1;
  EXPECT_EQ(DDS::RETCODE_OK, dp->set_qos(qos));
  EXPECT_EQ(DDS::RETCODE_OK, dpf->delete_participant(dp));
}

TEST(NeverEnabledParticipant, Topic)
{
  DDS::DomainParticipantFactory_var dpf = TheParticipantFactory;
  DDS::DomainParticipant_var dp = create_participant(false);
  ASSERT_TRUE(dp);
  DDS::Topic_var topic = create_topic(dp);
  ASSERT_TRUE(topic);
  DDS::TopicQos qos;
  EXPECT_EQ(DDS::RETCODE_OK, topic->get_qos(qos));
  qos.topic_data.value.length(1);
  qos.topic_data.value[0] = 1;
  EXPECT_EQ(DDS::RETCODE_OK, topic->set_qos(qos));
  EXPECT_EQ(DDS::RETCODE_OK, dp->delete_topic(topic));
  EXPECT_EQ(DDS::RETCODE_OK, dpf->delete_participant(dp));
}

TEST(NeverEnabledParticipant, DeleteContainedEntities)
{
  DDS::DomainParticipantFactory_var dpf = TheParticipantFactory;
  DDS::DomainParticipant_var dp = create_participant(false);
  ASSERT_TRUE(dp);
  DDS::Topic_var topic = create_topic(dp);
  ASSERT_TRUE(topic);
  DDS::Publisher_var pub = dp->create_publisher(PUBLISHER_QOS_DEFAULT, 0, DEFAULT_STATUS_MASK);
  ASSERT_TRUE(pub);
  DDS::DataWriter_var dw = create_writer(pub, topic);
  ASSERT_TRUE(dw);
  DDS::Subscriber_var sub = dp->create_subscriber(SUBSCRIBER_QOS_DEFAULT, 0, DEFAULT_STATUS_MASK);
  ASSERT_TRUE(sub);
  DDS::DataReader_var dr = create_reader(sub, topic);
  ASSERT_TRUE(dr);

  EXPECT_EQ(DDS::RETCODE_PRECONDITION_NOT_MET, dpf->delete_participant(dp));
  EXPECT_EQ(DDS::RETCODE_OK, dp->delete_contained_entities());
  EXPECT_EQ(DDS::RETCODE_OK, dpf->delete_participant(dp));
}

int ACE_TMAIN(int argc, ACE_TCHAR* argv[])
{
  ::testing::InitGoogleTest(&argc, argv);
  DDS::DomainParticipantFactory_var dpf = TheParticipantFactoryWithArgs(argc, argv);
  const int result = RUN_ALL_TESTS();
  TheServiceParticipant->shutdown();
  return result;
}
