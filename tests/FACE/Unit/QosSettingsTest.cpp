#include "dds/FACE/config/QosSettings.h"

unsigned int assertions = 0;

#define TEST_CHECK(COND) \
  ++assertions; \
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

void test_set_datawriter_deadline_sec() {
  QosSettings settings;
  QosSettingsAccessor accessor(settings);

  accessor.set_qos(QosSettings::data_writer, "deadline.period.sec", "2");
  DDS::DataWriterQos qos;
  settings.apply_to(qos);

  TEST_CHECK(2 == qos.deadline.period.sec);
  TEST_CHECK(0 == qos.deadline.period.nanosec);
}

void test_set_datawriter_deadline_nanosec() {
  QosSettings settings;
  QosSettingsAccessor accessor(settings);

  accessor.set_qos(QosSettings::data_writer, "deadline.period.nanosec", "200");
  DDS::DataWriterQos qos;
  settings.apply_to(qos);

  TEST_CHECK(0 == qos.deadline.period.sec);
  TEST_CHECK(200 == qos.deadline.period.nanosec);
}

void test_set_datawriter_deadline_both() {
  QosSettings settings;
  QosSettingsAccessor accessor(settings);

  accessor.set_qos(QosSettings::data_writer, "deadline.period.sec", "3");
  accessor.set_qos(QosSettings::data_writer, "deadline.period.nanosec", "500");
  DDS::DataWriterQos qos;
  settings.apply_to(qos);

  TEST_CHECK(3 == qos.deadline.period.sec);
  TEST_CHECK(500 == qos.deadline.period.nanosec);
}

void test_set_datawriter_latency_budget_sec() {
  QosSettings settings;
  QosSettingsAccessor accessor(settings);

  accessor.set_qos(QosSettings::data_writer, "latency_budget.duration.sec", "2");
  DDS::DataWriterQos qos;
  settings.apply_to(qos);

  TEST_CHECK(2 == qos.latency_budget.duration.sec);
  TEST_CHECK(0 == qos.latency_budget.duration.nanosec);
}

void test_set_datawriter_latency_budget_nanosec() {
  QosSettings settings;
  QosSettingsAccessor accessor(settings);

  accessor.set_qos(QosSettings::data_writer, "latency_budget.duration.nanosec", "350");
  DDS::DataWriterQos qos;
  settings.apply_to(qos);

  TEST_CHECK(0 == qos.latency_budget.duration.sec);
  TEST_CHECK(350 == qos.latency_budget.duration.nanosec);
}

void test_set_datawriter_latency_budget_both() {
  QosSettings settings;
  QosSettingsAccessor accessor(settings);

  accessor.set_qos(QosSettings::data_writer, "latency_budget.duration.sec", "1");
  accessor.set_qos(QosSettings::data_writer, "latency_budget.duration.nanosec", "150");
  DDS::DataWriterQos qos;
  settings.apply_to(qos);

  TEST_CHECK(1 == qos.latency_budget.duration.sec);
  TEST_CHECK(150 == qos.latency_budget.duration.nanosec);
}

void
test_set_datawriter_liveliness_lease_duration_sec()
{
  QosSettings settings;
  QosSettingsAccessor accessor(settings);

  accessor.set_qos(QosSettings::data_writer, "liveliness.lease_duration.sec", "2");
  DDS::DataWriterQos qos;
  settings.apply_to(qos);

  TEST_CHECK(2 == qos.liveliness.lease_duration.sec);
  TEST_CHECK(0 == qos.liveliness.lease_duration.nanosec);
}

void
test_set_datawriter_liveliness_lease_duration_nanosec()
{
  QosSettings settings;
  QosSettingsAccessor accessor(settings);

  accessor.set_qos(QosSettings::data_writer, "liveliness.lease_duration.nanosec", "333");
  DDS::DataWriterQos qos;
  settings.apply_to(qos);

  TEST_CHECK(0 == qos.liveliness.lease_duration.sec);
  TEST_CHECK(333 == qos.liveliness.lease_duration.nanosec);
}

void
test_set_datawriter_liveliness_lease_duration_both()
{
  QosSettings settings;
  QosSettingsAccessor accessor(settings);

  accessor.set_qos(QosSettings::data_writer, "liveliness.lease_duration.sec", "1");
  accessor.set_qos(QosSettings::data_writer, "liveliness.lease_duration.nanosec", "333");
  DDS::DataWriterQos qos;
  settings.apply_to(qos);

  TEST_CHECK(1 == qos.liveliness.lease_duration.sec);
  TEST_CHECK(333 == qos.liveliness.lease_duration.nanosec);
}

void test_set_datawriter_reliability_kind_best_effort()
{
  QosSettings settings;
  QosSettingsAccessor accessor(settings);

  accessor.set_qos(QosSettings::data_writer, "reliability.kind", "BEST_EFFORT");
  DDS::DataWriterQos qos;
  settings.apply_to(qos);

  TEST_CHECK(DDS::BEST_EFFORT_RELIABILITY_QOS == qos.reliability.kind);
}

void test_set_datawriter_reliability_kind_reliable()
{
  QosSettings settings;
  QosSettingsAccessor accessor(settings);

  accessor.set_qos(QosSettings::data_writer, "reliability.kind", "RELIABLE");
  DDS::DataWriterQos qos;
  settings.apply_to(qos);

  TEST_CHECK(DDS::RELIABLE_RELIABILITY_QOS == qos.reliability.kind);
}

void test_set_datawriter_reliability_max_blocking_time_sec()
{
  QosSettings settings;
  QosSettingsAccessor accessor(settings);

  accessor.set_qos(QosSettings::data_writer, "reliability.max_blocking_time.sec", "2");
  DDS::DataWriterQos qos;
  settings.apply_to(qos);

  TEST_CHECK(2 == qos.reliability.max_blocking_time.sec);
  TEST_CHECK(0 == qos.reliability.max_blocking_time.nanosec);
}

void test_set_datawriter_reliability_max_blocking_time_nanosec()
{
  QosSettings settings;
  QosSettingsAccessor accessor(settings);

  accessor.set_qos(QosSettings::data_writer, "reliability.max_blocking_time.nanosec", "175");
  DDS::DataWriterQos qos;
  settings.apply_to(qos);

  TEST_CHECK(0 == qos.reliability.max_blocking_time.sec);
  TEST_CHECK(175 == qos.reliability.max_blocking_time.nanosec);
}

void test_set_datawriter_reliability_max_blocking_time_both()
{
  QosSettings settings;
  QosSettingsAccessor accessor(settings);

  accessor.set_qos(QosSettings::data_writer, "reliability.max_blocking_time.sec", "1");
  accessor.set_qos(QosSettings::data_writer, "reliability.max_blocking_time.nanosec", "175");
  DDS::DataWriterQos qos;
  settings.apply_to(qos);

  TEST_CHECK(1 == qos.reliability.max_blocking_time.sec);
  TEST_CHECK(175 == qos.reliability.max_blocking_time.nanosec);
}

void test_set_datawriter_destination_order_kind_by_source_timestamp()
{
  QosSettings settings;
  QosSettingsAccessor accessor(settings);

  accessor.set_qos(QosSettings::data_writer, "destination_order.kind", "BY_SOURCE_TIMESTAMP");
  DDS::DataWriterQos qos;
  settings.apply_to(qos);

  TEST_CHECK(DDS::BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS ==
             qos.destination_order.kind);
}

void test_set_datawriter_destination_order_kind_by_reception_timestamp()
{
  QosSettings settings;
  QosSettingsAccessor accessor(settings);

  accessor.set_qos(QosSettings::data_writer, "destination_order.kind", "BY_RECEPTION_TIMESTAMP");
  DDS::DataWriterQos qos;
  settings.apply_to(qos);

  TEST_CHECK(DDS::BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS ==
             qos.destination_order.kind);
}

void test_set_datawriter_history_kind_keep_all()
{
  QosSettings settings;
  QosSettingsAccessor accessor(settings);

  accessor.set_qos(QosSettings::data_writer, "history.kind", "KEEP_ALL");
  DDS::DataWriterQos qos;
  settings.apply_to(qos);

  TEST_CHECK(DDS::KEEP_ALL_HISTORY_QOS == qos.history.kind);
}

void test_set_datawriter_history_kind_keep_last()
{
  QosSettings settings;
  QosSettingsAccessor accessor(settings);

  accessor.set_qos(QosSettings::data_writer, "history.kind", "KEEP_LAST");
  DDS::DataWriterQos qos;
  settings.apply_to(qos);

  TEST_CHECK(DDS::KEEP_LAST_HISTORY_QOS == qos.history.kind);
}

void test_set_datawriter_history_depth()
{
  QosSettings settings;
  QosSettingsAccessor accessor(settings);

  accessor.set_qos(QosSettings::data_writer, "history.depth", "14");
  DDS::DataWriterQos qos;
  settings.apply_to(qos);

  TEST_CHECK(14 == qos.history.depth);
}

void test_set_datawriter_resource_limits_max_samples()
{
  QosSettings settings;
  QosSettingsAccessor accessor(settings);

  accessor.set_qos(QosSettings::data_writer, "resource_limits.max_samples", "14");
  DDS::DataWriterQos qos;
  settings.apply_to(qos);

  TEST_CHECK(14 == qos.resource_limits.max_samples);
}

void test_set_datawriter_resource_limits_max_instances()
{
  QosSettings settings;
  QosSettingsAccessor accessor(settings);

  accessor.set_qos(QosSettings::data_writer, "resource_limits.max_instances", "14");
  DDS::DataWriterQos qos;
  settings.apply_to(qos);

  TEST_CHECK(14 == qos.resource_limits.max_instances);
}

void test_set_datawriter_resource_limits_max_samples_per_instance()
{
  QosSettings settings;
  QosSettingsAccessor accessor(settings);

  accessor.set_qos(QosSettings::data_writer, "resource_limits.max_samples_per_instance", "14");
  DDS::DataWriterQos qos;
  settings.apply_to(qos);

  TEST_CHECK(14 == qos.resource_limits.max_samples_per_instance);
}

/////// DataReader tests 
void test_set_datareader_durability_volatile()
{
  QosSettings settings;
  QosSettingsAccessor accessor(settings);

  accessor.set_qos(QosSettings::data_reader, "durability.kind", "VOLATILE");
  DDS::DataReaderQos qos;
  settings.apply_to(qos);

  TEST_CHECK(DDS::VOLATILE_DURABILITY_QOS == qos.durability.kind);
}

void test_set_datareader_durability_transient_local()
{
  QosSettings settings;
  QosSettingsAccessor accessor(settings);

  accessor.set_qos(QosSettings::data_reader, "durability.kind", "TRANSIENT_LOCAL");
  DDS::DataReaderQos qos;
  settings.apply_to(qos);

  TEST_CHECK(DDS::TRANSIENT_LOCAL_DURABILITY_QOS == qos.durability.kind);
}

void test_set_datareader_durability_transient()
{
  QosSettings settings;
  QosSettingsAccessor accessor(settings);

  accessor.set_qos(QosSettings::data_reader, "durability.kind", "TRANSIENT");
  DDS::DataReaderQos qos;
  settings.apply_to(qos);

  TEST_CHECK(DDS::TRANSIENT_DURABILITY_QOS == qos.durability.kind);
}

void test_set_datareader_durability_persistent()
{
  QosSettings settings;
  QosSettingsAccessor accessor(settings);

  accessor.set_qos(QosSettings::data_reader, "durability.kind", "PERSISTENT");
  DDS::DataReaderQos qos;
  settings.apply_to(qos);

  TEST_CHECK(DDS::PERSISTENT_DURABILITY_QOS == qos.durability.kind);
}

void test_set_datareader_deadline_sec() {
  QosSettings settings;
  QosSettingsAccessor accessor(settings);

  accessor.set_qos(QosSettings::data_reader, "deadline.period.sec", "2");
  DDS::DataReaderQos qos;
  settings.apply_to(qos);

  TEST_CHECK(2 == qos.deadline.period.sec);
  TEST_CHECK(0 == qos.deadline.period.nanosec);
}

void test_set_datareader_deadline_nanosec() {
  QosSettings settings;
  QosSettingsAccessor accessor(settings);

  accessor.set_qos(QosSettings::data_reader, "deadline.period.nanosec", "200");
  DDS::DataReaderQos qos;
  settings.apply_to(qos);

  TEST_CHECK(0 == qos.deadline.period.sec);
  TEST_CHECK(200 == qos.deadline.period.nanosec);
}

void test_set_datareader_deadline_both() {
  QosSettings settings;
  QosSettingsAccessor accessor(settings);

  accessor.set_qos(QosSettings::data_reader, "deadline.period.sec", "3");
  accessor.set_qos(QosSettings::data_reader, "deadline.period.nanosec", "500");
  DDS::DataReaderQos qos;
  settings.apply_to(qos);

  TEST_CHECK(3 == qos.deadline.period.sec);
  TEST_CHECK(500 == qos.deadline.period.nanosec);
}

void
test_set_datareader_liveliness_lease_duration_sec()
{
  QosSettings settings;
  QosSettingsAccessor accessor(settings);

  accessor.set_qos(QosSettings::data_reader, "liveliness.lease_duration.sec", "2");
  DDS::DataReaderQos qos;
  settings.apply_to(qos);

  TEST_CHECK(2 == qos.liveliness.lease_duration.sec);
  TEST_CHECK(0 == qos.liveliness.lease_duration.nanosec);
}

void
test_set_datareader_liveliness_lease_duration_nanosec()
{
  QosSettings settings;
  QosSettingsAccessor accessor(settings);

  accessor.set_qos(QosSettings::data_reader, "liveliness.lease_duration.nanosec", "333");
  DDS::DataReaderQos qos;
  settings.apply_to(qos);

  TEST_CHECK(0 == qos.liveliness.lease_duration.sec);
  TEST_CHECK(333 == qos.liveliness.lease_duration.nanosec);
}

void
test_set_datareader_liveliness_lease_duration_both()
{
  QosSettings settings;
  QosSettingsAccessor accessor(settings);

  accessor.set_qos(QosSettings::data_reader, "liveliness.lease_duration.sec", "1");
  accessor.set_qos(QosSettings::data_reader, "liveliness.lease_duration.nanosec", "333");
  DDS::DataReaderQos qos;
  settings.apply_to(qos);

  TEST_CHECK(1 == qos.liveliness.lease_duration.sec);
  TEST_CHECK(333 == qos.liveliness.lease_duration.nanosec);
}

void test_set_datareader_reliability_kind_best_effort()
{
  QosSettings settings;
  QosSettingsAccessor accessor(settings);

  accessor.set_qos(QosSettings::data_reader, "reliability.kind", "BEST_EFFORT");
  DDS::DataReaderQos qos;
  settings.apply_to(qos);

  TEST_CHECK(DDS::BEST_EFFORT_RELIABILITY_QOS == qos.reliability.kind);
}

void test_set_datareader_reliability_kind_reliable()
{
  QosSettings settings;
  QosSettingsAccessor accessor(settings);

  accessor.set_qos(QosSettings::data_reader, "reliability.kind", "RELIABLE");
  DDS::DataReaderQos qos;
  settings.apply_to(qos);

  TEST_CHECK(DDS::RELIABLE_RELIABILITY_QOS == qos.reliability.kind);
}

void test_set_datareader_reliability_max_blocking_time_sec()
{
  QosSettings settings;
  QosSettingsAccessor accessor(settings);

  accessor.set_qos(QosSettings::data_reader, "reliability.max_blocking_time.sec", "2");
  DDS::DataReaderQos qos;
  settings.apply_to(qos);

  TEST_CHECK(2 == qos.reliability.max_blocking_time.sec);
  TEST_CHECK(0 == qos.reliability.max_blocking_time.nanosec);
}

void test_set_datareader_reliability_max_blocking_time_nanosec()
{
  QosSettings settings;
  QosSettingsAccessor accessor(settings);

  accessor.set_qos(QosSettings::data_reader, "reliability.max_blocking_time.nanosec", "175");
  DDS::DataReaderQos qos;
  settings.apply_to(qos);

  TEST_CHECK(0 == qos.reliability.max_blocking_time.sec);
  TEST_CHECK(175 == qos.reliability.max_blocking_time.nanosec);
}

void test_set_datareader_reliability_max_blocking_time_both()
{
  QosSettings settings;
  QosSettingsAccessor accessor(settings);

  accessor.set_qos(QosSettings::data_reader, "reliability.max_blocking_time.sec", "1");
  accessor.set_qos(QosSettings::data_reader, "reliability.max_blocking_time.nanosec", "175");
  DDS::DataReaderQos qos;
  settings.apply_to(qos);

  TEST_CHECK(1 == qos.reliability.max_blocking_time.sec);
  TEST_CHECK(175 == qos.reliability.max_blocking_time.nanosec);
}

void test_set_datareader_destination_order_kind_by_source_timestamp()
{
  QosSettings settings;
  QosSettingsAccessor accessor(settings);

  accessor.set_qos(QosSettings::data_reader, "destination_order.kind", "BY_SOURCE_TIMESTAMP");
  DDS::DataReaderQos qos;
  settings.apply_to(qos);

  TEST_CHECK(DDS::BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS ==
             qos.destination_order.kind);
}

void test_set_datareader_destination_order_kind_by_reception_timestamp()
{
  QosSettings settings;
  QosSettingsAccessor accessor(settings);

  accessor.set_qos(QosSettings::data_reader, "destination_order.kind", "BY_RECEPTION_TIMESTAMP");
  DDS::DataReaderQos qos;
  settings.apply_to(qos);

  TEST_CHECK(DDS::BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS ==
             qos.destination_order.kind);
}

void test_set_datareader_history_kind_keep_all()
{
  QosSettings settings;
  QosSettingsAccessor accessor(settings);

  accessor.set_qos(QosSettings::data_reader, "history.kind", "KEEP_ALL");
  DDS::DataReaderQos qos;
  settings.apply_to(qos);

  TEST_CHECK(DDS::KEEP_ALL_HISTORY_QOS == qos.history.kind);
}

void test_set_datareader_history_kind_keep_last()
{
  QosSettings settings;
  QosSettingsAccessor accessor(settings);

  accessor.set_qos(QosSettings::data_reader, "history.kind", "KEEP_LAST");
  DDS::DataReaderQos qos;
  settings.apply_to(qos);

  TEST_CHECK(DDS::KEEP_LAST_HISTORY_QOS == qos.history.kind);
}

void test_set_datareader_history_depth()
{
  QosSettings settings;
  QosSettingsAccessor accessor(settings);

  accessor.set_qos(QosSettings::data_reader, "history.depth", "21");
  DDS::DataReaderQos qos;
  settings.apply_to(qos);

  TEST_CHECK(21 == qos.history.depth);
}

void test_set_datareader_resource_limits_max_samples()
{
  QosSettings settings;
  QosSettingsAccessor accessor(settings);

  accessor.set_qos(QosSettings::data_reader, "resource_limits.max_samples", "14");
  DDS::DataReaderQos qos;
  settings.apply_to(qos);

  TEST_CHECK(14 == qos.resource_limits.max_samples);
}

void test_set_datareader_resource_limits_max_instances()
{
  QosSettings settings;
  QosSettingsAccessor accessor(settings);

  accessor.set_qos(QosSettings::data_reader, "resource_limits.max_instances", "14");
  DDS::DataReaderQos qos;
  settings.apply_to(qos);

  TEST_CHECK(14 == qos.resource_limits.max_instances);
}

void test_set_datareader_resource_limits_max_samples_per_instance()
{
  QosSettings settings;
  QosSettingsAccessor accessor(settings);

  accessor.set_qos(QosSettings::data_reader, "resource_limits.max_samples_per_instance", "14");
  DDS::DataReaderQos qos;
  settings.apply_to(qos);

  TEST_CHECK(14 == qos.resource_limits.max_samples_per_instance);
}

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
  test_set_datawriter_deadline_sec();
  test_set_datawriter_deadline_nanosec();
  test_set_datawriter_deadline_both();
  test_set_datawriter_latency_budget_sec();
  test_set_datawriter_latency_budget_nanosec();
  test_set_datawriter_latency_budget_both();
  test_set_datawriter_liveliness_lease_duration_sec();
  test_set_datawriter_liveliness_lease_duration_nanosec();
  test_set_datawriter_liveliness_lease_duration_both();
  test_set_datawriter_reliability_kind_best_effort();
  test_set_datawriter_reliability_kind_reliable();
  test_set_datawriter_reliability_max_blocking_time_sec();
  test_set_datawriter_reliability_max_blocking_time_nanosec();
  test_set_datawriter_reliability_max_blocking_time_both();
  test_set_datawriter_destination_order_kind_by_source_timestamp();
  test_set_datawriter_destination_order_kind_by_reception_timestamp();
  test_set_datawriter_history_kind_keep_all();
  test_set_datawriter_history_kind_keep_last();
  test_set_datawriter_history_depth();
  test_set_datawriter_resource_limits_max_samples();
  test_set_datawriter_resource_limits_max_instances();
  test_set_datawriter_resource_limits_max_samples_per_instance();

  // DataReader QOS
  test_set_datareader_durability_volatile();
  test_set_datareader_durability_transient_local();
  test_set_datareader_durability_transient();
  test_set_datareader_durability_persistent();
  test_set_datareader_deadline_sec();
  test_set_datareader_deadline_nanosec();
  test_set_datareader_deadline_both();
  test_set_datareader_liveliness_lease_duration_sec();
  test_set_datareader_liveliness_lease_duration_nanosec();
  test_set_datareader_liveliness_lease_duration_both();
  test_set_datareader_reliability_kind_best_effort();
  test_set_datareader_reliability_kind_reliable();
  test_set_datareader_reliability_max_blocking_time_sec();
  test_set_datareader_reliability_max_blocking_time_nanosec();
  test_set_datareader_reliability_max_blocking_time_both();
  test_set_datareader_destination_order_kind_by_source_timestamp();
  test_set_datareader_destination_order_kind_by_reception_timestamp();
  test_set_datareader_history_kind_keep_all();
  test_set_datareader_history_kind_keep_last();
  test_set_datareader_history_depth();
  test_set_datareader_resource_limits_max_samples();
  test_set_datareader_resource_limits_max_instances();
  test_set_datareader_resource_limits_max_samples_per_instance();

  printf("%d assertions passed\n", assertions);
  return 0;
}

