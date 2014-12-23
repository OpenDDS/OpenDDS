#include "dds/FACE/config/QosSettings.h"

#define TEST_CHECK(COND) \
  if (!( COND )) \
      ACE_ERROR((LM_ERROR,"(%P|%t) TEST_CHECK(%C) FAILED at %N:%l %a\n",\
        #COND , -1));

namespace OpenDDS { namespace FACE { namespace config {

class QosSettingsAccessor {
public:
  QosSettingsAccessor(QosSettings& qs) : qs_(qs) {}
  void set_qos(
    QosSettings::QosLevel level, const char* name, const char* value)
  {
    qs_.set_qos(level, name, value);
  }

private:
  QosSettings& qs_;
};

} } }

using namespace OpenDDS::FACE::config;

/////// Publisher tests 
void test_set_publisher_single_partition() {
  QosSettings settings;
  QosSettingsAccessor accessor(settings);

  accessor.set_qos(QosSettings::publisher, "partition.name", "Foo123");
  DDS::PublisherQos qos;
  settings.apply_to(qos);

  TEST_CHECK(1 == qos.partition.name.length());
  TEST_CHECK(!strcmp(qos.partition.name[0], "Foo123"));
}

void test_set_publisher_multiple_partitions() {
  QosSettings settings;
  QosSettingsAccessor accessor(settings);

  accessor.set_qos(QosSettings::publisher, "partition.name", "Foo123");
  accessor.set_qos(QosSettings::publisher, "partition.name", "Bar234");

  DDS::PublisherQos qos;
  settings.apply_to(qos);

  TEST_CHECK(2 == qos.partition.name.length());
  TEST_CHECK(!strcmp(qos.partition.name[0], "Foo123"));
  TEST_CHECK(!strcmp(qos.partition.name[1], "Bar234"));
}

void test_set_publisher_presentation_access_scope_instance() {
  QosSettings settings;
  QosSettingsAccessor accessor(settings);

  accessor.set_qos(QosSettings::publisher, "presentation.access_scope", "INSTANCE");
  DDS::PublisherQos qos;
  settings.apply_to(qos);

  TEST_CHECK(DDS::INSTANCE_PRESENTATION_QOS == qos.presentation.access_scope);
}

void test_set_publisher_presentation_access_scope_topic() {
  QosSettings settings;
  QosSettingsAccessor accessor(settings);

  accessor.set_qos(QosSettings::publisher, "presentation.access_scope", "TOPIC");
  DDS::PublisherQos qos;
  settings.apply_to(qos);

  TEST_CHECK(DDS::TOPIC_PRESENTATION_QOS == qos.presentation.access_scope);
}

void test_set_publisher_presentation_access_scope_group() {
  QosSettings settings;
  QosSettingsAccessor accessor(settings);

  accessor.set_qos(QosSettings::publisher, "presentation.access_scope", "GROUP");
  DDS::PublisherQos qos;
  settings.apply_to(qos);

  TEST_CHECK(DDS::GROUP_PRESENTATION_QOS == qos.presentation.access_scope);
}

void test_set_publisher_presentation_coherent_access_true() {
  QosSettings settings;
  QosSettingsAccessor accessor(settings);

  accessor.set_qos(QosSettings::publisher, "presentation.coherent_access", "true");
  DDS::PublisherQos qos;
  settings.apply_to(qos);

  TEST_CHECK(qos.presentation.coherent_access);
}

void test_set_publisher_presentation_coherent_access_false() {
  QosSettings settings;
  QosSettingsAccessor accessor(settings);

  accessor.set_qos(QosSettings::publisher, "presentation.coherent_access", "false");
  DDS::PublisherQos qos;
  settings.apply_to(qos);

  TEST_CHECK(!qos.presentation.coherent_access);
}

void test_set_publisher_presentation_ordered_access_true() {
  QosSettings settings;
  QosSettingsAccessor accessor(settings);

  accessor.set_qos(QosSettings::publisher, "presentation.ordered_access", "true");
  DDS::PublisherQos qos;
  settings.apply_to(qos);

  TEST_CHECK(qos.presentation.ordered_access);
}

void test_set_publisher_presentation_ordered_access_false() {
  QosSettings settings;
  QosSettingsAccessor accessor(settings);

  accessor.set_qos(QosSettings::publisher, "presentation.ordered_access", "false");
  DDS::PublisherQos qos;
  settings.apply_to(qos);

  TEST_CHECK(!qos.presentation.ordered_access);
}

/////// Subscriber tests 
void test_set_subscriber_single_partition() {
  QosSettings settings;
  QosSettingsAccessor accessor(settings);

  accessor.set_qos(QosSettings::subscriber, "partition.name", "Foo123");
  DDS::SubscriberQos qos;
  settings.apply_to(qos);

  TEST_CHECK(1 == qos.partition.name.length());
  TEST_CHECK(!strcmp(qos.partition.name[0], "Foo123"));
}

void test_set_subscriber_multiple_partitions() {
  QosSettings settings;
  QosSettingsAccessor accessor(settings);

  accessor.set_qos(QosSettings::subscriber, "partition.name", "Foo123");
  accessor.set_qos(QosSettings::subscriber, "partition.name", "Bar234");

  DDS::SubscriberQos qos;
  settings.apply_to(qos);

  TEST_CHECK(2 == qos.partition.name.length());
  TEST_CHECK(!strcmp(qos.partition.name[0], "Foo123"));
  TEST_CHECK(!strcmp(qos.partition.name[1], "Bar234"));
}

void test_set_subscriber_presentation_access_scope_instance() {
  QosSettings settings;
  QosSettingsAccessor accessor(settings);

  accessor.set_qos(QosSettings::subscriber, "presentation.access_scope", "INSTANCE");
  DDS::SubscriberQos qos;
  settings.apply_to(qos);

  TEST_CHECK(DDS::INSTANCE_PRESENTATION_QOS == qos.presentation.access_scope);
}

void test_set_subscriber_presentation_access_scope_topic() {
  QosSettings settings;
  QosSettingsAccessor accessor(settings);

  accessor.set_qos(QosSettings::subscriber, "presentation.access_scope", "TOPIC");
  DDS::SubscriberQos qos;
  settings.apply_to(qos);

  TEST_CHECK(DDS::TOPIC_PRESENTATION_QOS == qos.presentation.access_scope);
}

void test_set_subscriber_presentation_access_scope_group() {
  QosSettings settings;
  QosSettingsAccessor accessor(settings);

  accessor.set_qos(QosSettings::subscriber, "presentation.access_scope", "GROUP");
  DDS::SubscriberQos qos;
  settings.apply_to(qos);

  TEST_CHECK(DDS::GROUP_PRESENTATION_QOS == qos.presentation.access_scope);
}

void test_set_subscriber_presentation_coherent_access_true() {
  QosSettings settings;
  QosSettingsAccessor accessor(settings);

  accessor.set_qos(QosSettings::subscriber, "presentation.coherent_access", "true");
  DDS::SubscriberQos qos;
  settings.apply_to(qos);

  TEST_CHECK(qos.presentation.coherent_access);
}

void test_set_subscriber_presentation_coherent_access_false() {
  QosSettings settings;
  QosSettingsAccessor accessor(settings);

  accessor.set_qos(QosSettings::subscriber, "presentation.coherent_access", "false");
  DDS::SubscriberQos qos;
  settings.apply_to(qos);

  TEST_CHECK(!qos.presentation.coherent_access);
}

void test_set_subscriber_presentation_ordered_access_true() {
  QosSettings settings;
  QosSettingsAccessor accessor(settings);

  accessor.set_qos(QosSettings::subscriber, "presentation.ordered_access", "true");
  DDS::SubscriberQos qos;
  settings.apply_to(qos);

  TEST_CHECK(qos.presentation.ordered_access);
}

void test_set_subscriber_presentation_ordered_access_false() {
  QosSettings settings;
  QosSettingsAccessor accessor(settings);

  accessor.set_qos(QosSettings::subscriber, "presentation.ordered_access", "false");
  DDS::SubscriberQos qos;
  settings.apply_to(qos);

  TEST_CHECK(!qos.presentation.ordered_access);
}

/////// DataWriter tests 
void test_set_datawriter_durability_volatile()
{
  QosSettings settings;
  QosSettingsAccessor accessor(settings);

  accessor.set_qos(QosSettings::data_writer, "durability.kind", "VOLATILE");
  DDS::DataWriterQos qos;
  settings.apply_to(qos);

  TEST_CHECK(DDS::VOLATILE_DURABILITY_QOS == qos.durability.kind);
}

void test_set_datawriter_durability_transient_local()
{
  QosSettings settings;
  QosSettingsAccessor accessor(settings);

  accessor.set_qos(QosSettings::data_writer, "durability.kind", "TRANSIENT_LOCAL");
  DDS::DataWriterQos qos;
  settings.apply_to(qos);

  TEST_CHECK(DDS::TRANSIENT_LOCAL_DURABILITY_QOS == qos.durability.kind);
}

void test_set_datawriter_durability_transient()
{
  QosSettings settings;
  QosSettingsAccessor accessor(settings);

  accessor.set_qos(QosSettings::data_writer, "durability.kind", "TRANSIENT");
  DDS::DataWriterQos qos;
  settings.apply_to(qos);

  TEST_CHECK(DDS::TRANSIENT_DURABILITY_QOS == qos.durability.kind);
}

void test_set_datawriter_durability_persistent()
{
  QosSettings settings;
  QosSettingsAccessor accessor(settings);

  accessor.set_qos(QosSettings::data_writer, "durability.kind", "PERSISTENT");
  DDS::DataWriterQos qos;
  settings.apply_to(qos);

  TEST_CHECK(DDS::PERSISTENT_DURABILITY_QOS == qos.durability.kind);
}

/////// DataReader tests 
int main(int, const char** )
{
  // Publisher QOS
  test_set_publisher_single_partition();
  test_set_publisher_multiple_partitions();
  test_set_publisher_presentation_access_scope_instance();
  test_set_publisher_presentation_access_scope_topic();
  test_set_publisher_presentation_access_scope_group();
  test_set_publisher_presentation_coherent_access_true();
  test_set_publisher_presentation_coherent_access_false();
  test_set_publisher_presentation_ordered_access_true();
  test_set_publisher_presentation_ordered_access_false();
  // Subscriber QOS
  test_set_subscriber_single_partition();
  test_set_subscriber_multiple_partitions();
  test_set_subscriber_presentation_access_scope_instance();
  test_set_subscriber_presentation_access_scope_topic();
  test_set_subscriber_presentation_access_scope_group();
  test_set_subscriber_presentation_coherent_access_true();
  test_set_subscriber_presentation_coherent_access_false();
  test_set_subscriber_presentation_ordered_access_true();
  test_set_subscriber_presentation_ordered_access_false();
  // DataWriter QOS
  test_set_datawriter_durability_volatile();
  test_set_datawriter_durability_transient_local();
  test_set_datawriter_durability_transient();
  test_set_datawriter_durability_persistent();


  return 0;
}

