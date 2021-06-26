#include "dds/DCPS/Definitions.h"

#ifndef DDS_HAS_WCHAR
#ifndef DDS_HAS_MINIMUM_BIT

#include <gtest/gtest.h>

#include "dds/FACE/config/QosSettings.h"

#include <string.h>
#include <iostream>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL
namespace OpenDDS { namespace FaceTSS { namespace config {

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
using namespace OpenDDS::FaceTSS::config;
OPENDDS_END_VERSIONED_NAMESPACE_DECL


/////// Publisher tests
void test_set_publisher_single_partition() {
  QosSettings settings;

  settings.set_qos(QosSettings::publisher, "partition.name", "Foo123");
  DDS::PublisherQos qos;
  settings.apply_to(qos);

  EXPECT_TRUE(1 == qos.partition.name.length());
  EXPECT_TRUE(!strcmp(qos.partition.name[0], "Foo123"));
}

void test_set_publisher_multiple_partitions() {
  QosSettings settings;

  settings.set_qos(QosSettings::publisher, "partition.name", "Foo123,Bar234");

  DDS::PublisherQos qos;
  settings.apply_to(qos);

  EXPECT_TRUE(2 == qos.partition.name.length());
  EXPECT_TRUE(!strcmp(qos.partition.name[0], "Foo123"));
  EXPECT_TRUE(!strcmp(qos.partition.name[1], "Bar234"));
}

void test_set_publisher_presentation_access_scope_instance() {
  QosSettings settings;

  settings.set_qos(QosSettings::publisher, "presentation.access_scope", "INSTANCE");
  DDS::PublisherQos qos;
  settings.apply_to(qos);

  EXPECT_TRUE(DDS::INSTANCE_PRESENTATION_QOS == qos.presentation.access_scope);
}

void test_set_publisher_presentation_access_scope_topic() {
  QosSettings settings;

  settings.set_qos(QosSettings::publisher, "presentation.access_scope", "TOPIC");
  DDS::PublisherQos qos;
  settings.apply_to(qos);

  EXPECT_TRUE(DDS::TOPIC_PRESENTATION_QOS == qos.presentation.access_scope);
}

void test_set_publisher_presentation_access_scope_group() {
  QosSettings settings;

  settings.set_qos(QosSettings::publisher, "presentation.access_scope", "GROUP");
  DDS::PublisherQos qos;
  settings.apply_to(qos);

  EXPECT_TRUE(DDS::GROUP_PRESENTATION_QOS == qos.presentation.access_scope);
}

void test_set_publisher_presentation_coherent_access_true() {
  QosSettings settings;

  settings.set_qos(QosSettings::publisher, "presentation.coherent_access", "true");
  DDS::PublisherQos qos;
  settings.apply_to(qos);

  EXPECT_TRUE(qos.presentation.coherent_access);
}

void test_set_publisher_presentation_coherent_access_false() {
  QosSettings settings;

  settings.set_qos(QosSettings::publisher, "presentation.coherent_access", "false");
  DDS::PublisherQos qos;
  settings.apply_to(qos);

  EXPECT_TRUE(!qos.presentation.coherent_access);
}

void test_set_publisher_presentation_ordered_access_true() {
  QosSettings settings;

  settings.set_qos(QosSettings::publisher, "presentation.ordered_access", "true");
  DDS::PublisherQos qos;
  settings.apply_to(qos);

  EXPECT_TRUE(qos.presentation.ordered_access);
}

void test_set_publisher_presentation_ordered_access_false() {
  QosSettings settings;

  settings.set_qos(QosSettings::publisher, "presentation.ordered_access", "false");
  DDS::PublisherQos qos;
  settings.apply_to(qos);

  EXPECT_TRUE(!qos.presentation.ordered_access);
}

/////// Subscriber tests
void test_set_subscriber_single_partition() {
  QosSettings settings;

  settings.set_qos(QosSettings::subscriber, "partition.name", "Foo123");
  DDS::SubscriberQos qos;
  settings.apply_to(qos);

  EXPECT_TRUE(1 == qos.partition.name.length());
  EXPECT_TRUE(!strcmp(qos.partition.name[0], "Foo123"));
}

void test_set_subscriber_multiple_partitions() {
  QosSettings settings;

  settings.set_qos(QosSettings::subscriber, "partition.name", "Foo123");
  settings.set_qos(QosSettings::subscriber, "partition.name", "Bar234");

  DDS::SubscriberQos qos;
  settings.apply_to(qos);

  EXPECT_TRUE(2 == qos.partition.name.length());
  EXPECT_TRUE(!strcmp(qos.partition.name[0], "Foo123"));
  EXPECT_TRUE(!strcmp(qos.partition.name[1], "Bar234"));
}

void test_set_subscriber_presentation_access_scope_instance() {
  QosSettings settings;

  settings.set_qos(QosSettings::subscriber, "presentation.access_scope", "INSTANCE");
  DDS::SubscriberQos qos;
  settings.apply_to(qos);

  EXPECT_TRUE(DDS::INSTANCE_PRESENTATION_QOS == qos.presentation.access_scope);
}

void test_set_subscriber_presentation_access_scope_topic() {
  QosSettings settings;

  settings.set_qos(QosSettings::subscriber, "presentation.access_scope", "TOPIC");
  DDS::SubscriberQos qos;
  settings.apply_to(qos);

  EXPECT_TRUE(DDS::TOPIC_PRESENTATION_QOS == qos.presentation.access_scope);
}

void test_set_subscriber_presentation_access_scope_group() {
  QosSettings settings;

  settings.set_qos(QosSettings::subscriber, "presentation.access_scope", "GROUP");
  DDS::SubscriberQos qos;
  settings.apply_to(qos);

  EXPECT_TRUE(DDS::GROUP_PRESENTATION_QOS == qos.presentation.access_scope);
}

void test_set_subscriber_presentation_coherent_access_true() {
  QosSettings settings;

  settings.set_qos(QosSettings::subscriber, "presentation.coherent_access", "true");
  DDS::SubscriberQos qos;
  settings.apply_to(qos);

  EXPECT_TRUE(qos.presentation.coherent_access);
}

void test_set_subscriber_presentation_coherent_access_false() {
  QosSettings settings;

  settings.set_qos(QosSettings::subscriber, "presentation.coherent_access", "false");
  DDS::SubscriberQos qos;
  settings.apply_to(qos);

  EXPECT_TRUE(!qos.presentation.coherent_access);
}

void test_set_subscriber_presentation_ordered_access_true() {
  QosSettings settings;

  settings.set_qos(QosSettings::subscriber, "presentation.ordered_access", "true");
  DDS::SubscriberQos qos;
  settings.apply_to(qos);

  EXPECT_TRUE(qos.presentation.ordered_access);
}

void test_set_subscriber_presentation_ordered_access_false() {
  QosSettings settings;

  settings.set_qos(QosSettings::subscriber, "presentation.ordered_access", "false");
  DDS::SubscriberQos qos;
  settings.apply_to(qos);

  EXPECT_TRUE(!qos.presentation.ordered_access);
}

/////// DataWriter tests
void test_set_datawriter_durability_volatile()
{
  QosSettings settings;

  settings.set_qos(QosSettings::datawriter, "durability.kind", "VOLATILE");
  DDS::DataWriterQos qos;
  settings.apply_to(qos);

  EXPECT_TRUE(DDS::VOLATILE_DURABILITY_QOS == qos.durability.kind);
}

void test_set_datawriter_durability_transient_local()
{
  QosSettings settings;

  settings.set_qos(QosSettings::datawriter, "durability.kind", "TRANSIENT_LOCAL");
  DDS::DataWriterQos qos;
  settings.apply_to(qos);

  EXPECT_TRUE(DDS::TRANSIENT_LOCAL_DURABILITY_QOS == qos.durability.kind);
}

void test_set_datawriter_durability_transient()
{
  QosSettings settings;

  settings.set_qos(QosSettings::datawriter, "durability.kind", "TRANSIENT");
  DDS::DataWriterQos qos;
  settings.apply_to(qos);

  EXPECT_TRUE(DDS::TRANSIENT_DURABILITY_QOS == qos.durability.kind);
}

void test_set_datawriter_durability_persistent()
{
  QosSettings settings;

  settings.set_qos(QosSettings::datawriter, "durability.kind", "PERSISTENT");
  DDS::DataWriterQos qos;
  settings.apply_to(qos);

  EXPECT_TRUE(DDS::PERSISTENT_DURABILITY_QOS == qos.durability.kind);
}

void test_set_datawriter_deadline_sec() {
  QosSettings settings;

  settings.set_qos(QosSettings::datawriter, "deadline.period.sec", "2");
  settings.set_qos(QosSettings::datawriter, "deadline.period.nanosec", "0");
  DDS::DataWriterQos qos;
  settings.apply_to(qos);

  EXPECT_TRUE(2 == qos.deadline.period.sec);
  EXPECT_TRUE(0 == qos.deadline.period.nanosec);
}

void test_set_datawriter_deadline_nanosec() {
  QosSettings settings;

  settings.set_qos(QosSettings::datawriter, "deadline.period.sec", "0");
  settings.set_qos(QosSettings::datawriter, "deadline.period.nanosec", "200");
  DDS::DataWriterQos qos;
  settings.apply_to(qos);

  EXPECT_TRUE(0 == qos.deadline.period.sec);
  EXPECT_TRUE(200 == qos.deadline.period.nanosec);
}

void test_set_datawriter_deadline_both() {
  QosSettings settings;

  settings.set_qos(QosSettings::datawriter, "deadline.period.sec", "3");
  settings.set_qos(QosSettings::datawriter, "deadline.period.nanosec", "500");
  DDS::DataWriterQos qos;
  settings.apply_to(qos);

  EXPECT_TRUE(3 == qos.deadline.period.sec);
  EXPECT_TRUE(500 == qos.deadline.period.nanosec);
}

void test_set_datawriter_latency_budget_sec() {
  QosSettings settings;

  settings.set_qos(QosSettings::datawriter, "latency_budget.duration.sec", "2");
  DDS::DataWriterQos qos;
  settings.apply_to(qos);

  EXPECT_TRUE(2 == qos.latency_budget.duration.sec);
  EXPECT_TRUE(0 == qos.latency_budget.duration.nanosec);
}

void test_set_datawriter_latency_budget_nanosec() {
  QosSettings settings;

  settings.set_qos(QosSettings::datawriter, "latency_budget.duration.nanosec", "350");
  DDS::DataWriterQos qos;
  settings.apply_to(qos);

  EXPECT_TRUE(0 == qos.latency_budget.duration.sec);
  EXPECT_TRUE(350 == qos.latency_budget.duration.nanosec);
}

void test_set_datawriter_latency_budget_both() {
  QosSettings settings;

  settings.set_qos(QosSettings::datawriter, "latency_budget.duration.sec", "1");
  settings.set_qos(QosSettings::datawriter, "latency_budget.duration.nanosec", "150");
  DDS::DataWriterQos qos;
  settings.apply_to(qos);

  EXPECT_TRUE(1 == qos.latency_budget.duration.sec);
  EXPECT_TRUE(150 == qos.latency_budget.duration.nanosec);
}

void
test_set_datawriter_liveliness_kind_automatic()
{
  QosSettings settings;

  settings.set_qos(QosSettings::datawriter, "liveliness.kind", "AUTOMATIC");
  DDS::DataWriterQos qos;
  settings.apply_to(qos);

  EXPECT_TRUE(DDS::AUTOMATIC_LIVELINESS_QOS  == qos.liveliness.kind);
}

void
test_set_datawriter_liveliness_kind_manual_by_topic()
{
  QosSettings settings;

  settings.set_qos(QosSettings::datawriter, "liveliness.kind", "MANUAL_BY_TOPIC");
  DDS::DataWriterQos qos;
  settings.apply_to(qos);

  EXPECT_TRUE(DDS::MANUAL_BY_TOPIC_LIVELINESS_QOS  == qos.liveliness.kind);
}

void
test_set_datawriter_liveliness_kind_manual_by_participant()
{
  QosSettings settings;

  settings.set_qos(QosSettings::datawriter, "liveliness.kind", "MANUAL_BY_PARTICIPANT");
  DDS::DataWriterQos qos;
  settings.apply_to(qos);

  EXPECT_TRUE(DDS::MANUAL_BY_PARTICIPANT_LIVELINESS_QOS  == qos.liveliness.kind);
}

void
test_set_datawriter_liveliness_lease_duration_sec()
{
  QosSettings settings;

  settings.set_qos(QosSettings::datawriter, "liveliness.lease_duration.sec", "2");
  settings.set_qos(QosSettings::datawriter, "liveliness.lease_duration.nanosec", "0");
  DDS::DataWriterQos qos;
  settings.apply_to(qos);

  EXPECT_TRUE(2 == qos.liveliness.lease_duration.sec);
  EXPECT_TRUE(0 == qos.liveliness.lease_duration.nanosec);
}

void
test_set_datawriter_liveliness_lease_duration_nanosec()
{
  QosSettings settings;

  settings.set_qos(QosSettings::datawriter, "liveliness.lease_duration.sec", "0");
  settings.set_qos(QosSettings::datawriter, "liveliness.lease_duration.nanosec", "333");
  DDS::DataWriterQos qos;
  settings.apply_to(qos);

  EXPECT_TRUE(0 == qos.liveliness.lease_duration.sec);
  EXPECT_TRUE(333 == qos.liveliness.lease_duration.nanosec);
}

void
test_set_datawriter_liveliness_lease_duration_both()
{
  QosSettings settings;

  settings.set_qos(QosSettings::datawriter, "liveliness.lease_duration.sec", "1");
  settings.set_qos(QosSettings::datawriter, "liveliness.lease_duration.nanosec", "333");
  DDS::DataWriterQos qos;
  settings.apply_to(qos);

  EXPECT_TRUE(1 == qos.liveliness.lease_duration.sec);
  EXPECT_TRUE(333 == qos.liveliness.lease_duration.nanosec);
}

void test_set_datawriter_reliability_kind_best_effort()
{
  QosSettings settings;

  settings.set_qos(QosSettings::datawriter, "reliability.kind", "BEST_EFFORT");
  DDS::DataWriterQos qos;
  settings.apply_to(qos);

  EXPECT_TRUE(DDS::BEST_EFFORT_RELIABILITY_QOS == qos.reliability.kind);
}

void test_set_datawriter_reliability_kind_reliable()
{
  QosSettings settings;

  settings.set_qos(QosSettings::datawriter, "reliability.kind", "RELIABLE");
  DDS::DataWriterQos qos;
  settings.apply_to(qos);

  EXPECT_TRUE(DDS::RELIABLE_RELIABILITY_QOS == qos.reliability.kind);
}

void test_set_datawriter_reliability_max_blocking_time_sec()
{
  QosSettings settings;

  settings.set_qos(QosSettings::datawriter, "reliability.max_blocking_time.sec", "2");
  settings.set_qos(QosSettings::datawriter, "reliability.max_blocking_time.nanosec", "0");
  DDS::DataWriterQos qos;
  settings.apply_to(qos);

  EXPECT_TRUE(2 == qos.reliability.max_blocking_time.sec);
  EXPECT_TRUE(0 == qos.reliability.max_blocking_time.nanosec);
}

void test_set_datawriter_reliability_max_blocking_time_nanosec()
{
  QosSettings settings;

  settings.set_qos(QosSettings::datawriter, "reliability.max_blocking_time.nanosec", "175");
  DDS::DataWriterQos qos;
  settings.apply_to(qos);

  EXPECT_TRUE(0 == qos.reliability.max_blocking_time.sec);
  EXPECT_TRUE(175 == qos.reliability.max_blocking_time.nanosec);
}

void test_set_datawriter_reliability_max_blocking_time_both()
{
  QosSettings settings;

  settings.set_qos(QosSettings::datawriter, "reliability.max_blocking_time.sec", "1");
  settings.set_qos(QosSettings::datawriter, "reliability.max_blocking_time.nanosec", "175");
  DDS::DataWriterQos qos;
  settings.apply_to(qos);

  EXPECT_TRUE(1 == qos.reliability.max_blocking_time.sec);
  EXPECT_TRUE(175 == qos.reliability.max_blocking_time.nanosec);
}

void test_set_datawriter_destination_order_kind_by_source_timestamp()
{
  QosSettings settings;

  settings.set_qos(QosSettings::datawriter, "destination_order.kind", "BY_SOURCE_TIMESTAMP");
  DDS::DataWriterQos qos;
  settings.apply_to(qos);

  EXPECT_TRUE(DDS::BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS ==
             qos.destination_order.kind);
}

void test_set_datawriter_destination_order_kind_by_reception_timestamp()
{
  QosSettings settings;

  settings.set_qos(QosSettings::datawriter, "destination_order.kind", "BY_RECEPTION_TIMESTAMP");
  DDS::DataWriterQos qos;
  settings.apply_to(qos);

  EXPECT_TRUE(DDS::BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS ==
             qos.destination_order.kind);
}

void test_set_datawriter_history_kind_keep_all()
{
  QosSettings settings;

  settings.set_qos(QosSettings::datawriter, "history.kind", "KEEP_ALL");
  DDS::DataWriterQos qos;
  settings.apply_to(qos);

  EXPECT_TRUE(DDS::KEEP_ALL_HISTORY_QOS == qos.history.kind);
}

void test_set_datawriter_history_kind_keep_last()
{
  QosSettings settings;

  settings.set_qos(QosSettings::datawriter, "history.kind", "KEEP_LAST");
  DDS::DataWriterQos qos;
  settings.apply_to(qos);

  EXPECT_TRUE(DDS::KEEP_LAST_HISTORY_QOS == qos.history.kind);
}

void test_set_datawriter_history_depth()
{
  QosSettings settings;

  settings.set_qos(QosSettings::datawriter, "history.depth", "14");
  DDS::DataWriterQos qos;
  settings.apply_to(qos);

  EXPECT_TRUE(14 == qos.history.depth);
}

void test_set_datawriter_resource_limits_max_samples()
{
  QosSettings settings;

  settings.set_qos(QosSettings::datawriter, "resource_limits.max_samples", "14");
  DDS::DataWriterQos qos;
  settings.apply_to(qos);

  EXPECT_TRUE(14 == qos.resource_limits.max_samples);
}

void test_set_datawriter_resource_limits_max_instances()
{
  QosSettings settings;

  settings.set_qos(QosSettings::datawriter, "resource_limits.max_instances", "14");
  DDS::DataWriterQos qos;
  settings.apply_to(qos);

  EXPECT_TRUE(14 == qos.resource_limits.max_instances);
}

void test_set_datawriter_resource_limits_max_samples_per_instance()
{
  QosSettings settings;

  settings.set_qos(QosSettings::datawriter, "resource_limits.max_samples_per_instance", "14");
  DDS::DataWriterQos qos;
  settings.apply_to(qos);

  EXPECT_TRUE(14 == qos.resource_limits.max_samples_per_instance);
}

void test_set_datawriter_transport_priority_value()
{
  QosSettings settings;

  settings.set_qos(QosSettings::datawriter, "transport_priority.value", "3");
  DDS::DataWriterQos qos;
  settings.apply_to(qos);

  EXPECT_TRUE(3 == qos.transport_priority.value);
}

void test_set_datawriter_lifespan_duration_sec()
{
  QosSettings settings;

  settings.set_qos(QosSettings::datawriter, "lifespan.duration.sec", "3");
  DDS::DataWriterQos qos;
  settings.apply_to(qos);

  EXPECT_TRUE(3 == qos.lifespan.duration.sec);
}

void test_set_datawriter_lifespan_duration_nanosec()
{
  QosSettings settings;

  settings.set_qos(QosSettings::datawriter, "lifespan.duration.nanosec", "100");
  DDS::DataWriterQos qos;
  settings.apply_to(qos);

  EXPECT_TRUE(100 == qos.lifespan.duration.nanosec);
}

void test_set_datawriter_lifespan_duration_both()
{
  QosSettings settings;

  settings.set_qos(QosSettings::datawriter, "lifespan.duration.sec", "1");
  settings.set_qos(QosSettings::datawriter, "lifespan.duration.nanosec", "100");
  DDS::DataWriterQos qos;
  settings.apply_to(qos);

  EXPECT_TRUE(1 == qos.lifespan.duration.sec);
  EXPECT_TRUE(100 == qos.lifespan.duration.nanosec);
}

void test_set_datawriter_ownership_kind_shared()
{
  QosSettings settings;

  settings.set_qos(QosSettings::datawriter, "ownership.kind", "SHARED");
  DDS::DataWriterQos qos;
  settings.apply_to(qos);

  EXPECT_TRUE(DDS::SHARED_OWNERSHIP_QOS == qos.ownership.kind);
}

void test_set_datawriter_ownership_kind_exclusive()
{
  QosSettings settings;

  settings.set_qos(QosSettings::datawriter, "ownership.kind", "EXCLUSIVE");
  DDS::DataWriterQos qos;
  settings.apply_to(qos);

  EXPECT_TRUE(DDS::EXCLUSIVE_OWNERSHIP_QOS == qos.ownership.kind);
}

void test_set_datawriter_ownership_strength_value()
{
  QosSettings settings;

  settings.set_qos(QosSettings::datawriter, "ownership_strength.value", "32");
  DDS::DataWriterQos qos;
  settings.apply_to(qos);

  EXPECT_TRUE(32 == qos.ownership_strength.value);
}

/////// DataReader tests
void test_set_datareader_durability_volatile()
{
  QosSettings settings;

  settings.set_qos(QosSettings::datareader, "durability.kind", "VOLATILE");
  DDS::DataReaderQos qos;
  settings.apply_to(qos);

  EXPECT_TRUE(DDS::VOLATILE_DURABILITY_QOS == qos.durability.kind);
}

void test_set_datareader_durability_transient_local()
{
  QosSettings settings;

  settings.set_qos(QosSettings::datareader, "durability.kind", "TRANSIENT_LOCAL");
  DDS::DataReaderQos qos;
  settings.apply_to(qos);

  EXPECT_TRUE(DDS::TRANSIENT_LOCAL_DURABILITY_QOS == qos.durability.kind);
}

void test_set_datareader_durability_transient()
{
  QosSettings settings;

  settings.set_qos(QosSettings::datareader, "durability.kind", "TRANSIENT");
  DDS::DataReaderQos qos;
  settings.apply_to(qos);

  EXPECT_TRUE(DDS::TRANSIENT_DURABILITY_QOS == qos.durability.kind);
}

void test_set_datareader_durability_persistent()
{
  QosSettings settings;

  settings.set_qos(QosSettings::datareader, "durability.kind", "PERSISTENT");
  DDS::DataReaderQos qos;
  settings.apply_to(qos);

  EXPECT_TRUE(DDS::PERSISTENT_DURABILITY_QOS == qos.durability.kind);
}

void test_set_datareader_deadline_sec() {
  QosSettings settings;

  settings.set_qos(QosSettings::datareader, "deadline.period.sec", "2");
  settings.set_qos(QosSettings::datareader, "deadline.period.nanosec", "0");
  DDS::DataReaderQos qos;
  settings.apply_to(qos);

  EXPECT_TRUE(2 == qos.deadline.period.sec);
  EXPECT_TRUE(0 == qos.deadline.period.nanosec);
}

void test_set_datareader_deadline_nanosec() {
  QosSettings settings;

  settings.set_qos(QosSettings::datareader, "deadline.period.sec", "0");
  settings.set_qos(QosSettings::datareader, "deadline.period.nanosec", "200");
  DDS::DataReaderQos qos;
  settings.apply_to(qos);

  EXPECT_TRUE(0 == qos.deadline.period.sec);
  EXPECT_TRUE(200 == qos.deadline.period.nanosec);
}

void test_set_datareader_deadline_both() {
  QosSettings settings;

  settings.set_qos(QosSettings::datareader, "deadline.period.sec", "3");
  settings.set_qos(QosSettings::datareader, "deadline.period.nanosec", "500");
  DDS::DataReaderQos qos;
  settings.apply_to(qos);

  EXPECT_TRUE(3 == qos.deadline.period.sec);
  EXPECT_TRUE(500 == qos.deadline.period.nanosec);
}

void
test_set_datareader_liveliness_kind_automatic()
{
  QosSettings settings;

  settings.set_qos(QosSettings::datareader, "liveliness.kind", "AUTOMATIC");
  DDS::DataReaderQos qos;
  settings.apply_to(qos);

  EXPECT_TRUE(DDS::AUTOMATIC_LIVELINESS_QOS  == qos.liveliness.kind);
}

void
test_set_datareader_liveliness_kind_manual_by_topic()
{
  QosSettings settings;

  settings.set_qos(QosSettings::datareader, "liveliness.kind", "MANUAL_BY_TOPIC");
  DDS::DataReaderQos qos;
  settings.apply_to(qos);

  EXPECT_TRUE(DDS::MANUAL_BY_TOPIC_LIVELINESS_QOS  == qos.liveliness.kind);
}

void
test_set_datareader_liveliness_kind_manual_by_participant()
{
  QosSettings settings;

  settings.set_qos(QosSettings::datareader, "liveliness.kind", "MANUAL_BY_PARTICIPANT");
  DDS::DataReaderQos qos;
  settings.apply_to(qos);

  EXPECT_TRUE(DDS::MANUAL_BY_PARTICIPANT_LIVELINESS_QOS  == qos.liveliness.kind);
}

void
test_set_datareader_liveliness_lease_duration_sec()
{
  QosSettings settings;

  settings.set_qos(QosSettings::datareader, "liveliness.lease_duration.sec", "2");
  settings.set_qos(QosSettings::datareader, "liveliness.lease_duration.nanosec", "0");
  DDS::DataReaderQos qos;
  settings.apply_to(qos);

  EXPECT_TRUE(2 == qos.liveliness.lease_duration.sec);
  EXPECT_TRUE(0 == qos.liveliness.lease_duration.nanosec);
}

void
test_set_datareader_liveliness_lease_duration_nanosec()
{
  QosSettings settings;

  settings.set_qos(QosSettings::datareader, "liveliness.lease_duration.sec", "0");
  settings.set_qos(QosSettings::datareader, "liveliness.lease_duration.nanosec", "333");
  DDS::DataReaderQos qos;
  settings.apply_to(qos);

  EXPECT_TRUE(0 == qos.liveliness.lease_duration.sec);
  EXPECT_TRUE(333 == qos.liveliness.lease_duration.nanosec);
}

void
test_set_datareader_liveliness_lease_duration_both()
{
  QosSettings settings;

  settings.set_qos(QosSettings::datareader, "liveliness.lease_duration.sec", "1");
  settings.set_qos(QosSettings::datareader, "liveliness.lease_duration.nanosec", "333");
  DDS::DataReaderQos qos;
  settings.apply_to(qos);

  EXPECT_TRUE(1 == qos.liveliness.lease_duration.sec);
  EXPECT_TRUE(333 == qos.liveliness.lease_duration.nanosec);
}

void test_set_datareader_reliability_kind_best_effort()
{
  QosSettings settings;

  settings.set_qos(QosSettings::datareader, "reliability.kind", "BEST_EFFORT");
  DDS::DataReaderQos qos;
  settings.apply_to(qos);

  EXPECT_TRUE(DDS::BEST_EFFORT_RELIABILITY_QOS == qos.reliability.kind);
}

void test_set_datareader_reliability_kind_reliable()
{
  QosSettings settings;

  settings.set_qos(QosSettings::datareader, "reliability.kind", "RELIABLE");
  DDS::DataReaderQos qos;
  settings.apply_to(qos);

  EXPECT_TRUE(DDS::RELIABLE_RELIABILITY_QOS == qos.reliability.kind);
}

void test_set_datareader_reliability_max_blocking_time_sec()
{
  QosSettings settings;

  settings.set_qos(QosSettings::datareader, "reliability.max_blocking_time.sec", "2");
  settings.set_qos(QosSettings::datareader, "reliability.max_blocking_time.nanosec", "0");
  DDS::DataReaderQos qos;
  settings.apply_to(qos);

  EXPECT_TRUE(2 == qos.reliability.max_blocking_time.sec);
  EXPECT_TRUE(0 == qos.reliability.max_blocking_time.nanosec);
}

void test_set_datareader_reliability_max_blocking_time_nanosec()
{
  QosSettings settings;

  settings.set_qos(QosSettings::datareader, "reliability.max_blocking_time.sec", "0");
  settings.set_qos(QosSettings::datareader, "reliability.max_blocking_time.nanosec", "175");
  DDS::DataReaderQos qos;
  settings.apply_to(qos);

  EXPECT_TRUE(0 == qos.reliability.max_blocking_time.sec);
  EXPECT_TRUE(175 == qos.reliability.max_blocking_time.nanosec);
}

void test_set_datareader_reliability_max_blocking_time_both()
{
  QosSettings settings;

  settings.set_qos(QosSettings::datareader, "reliability.max_blocking_time.sec", "1");
  settings.set_qos(QosSettings::datareader, "reliability.max_blocking_time.nanosec", "175");
  DDS::DataReaderQos qos;
  settings.apply_to(qos);

  EXPECT_TRUE(1 == qos.reliability.max_blocking_time.sec);
  EXPECT_TRUE(175 == qos.reliability.max_blocking_time.nanosec);
}

void test_set_datareader_destination_order_kind_by_source_timestamp()
{
  QosSettings settings;

  settings.set_qos(QosSettings::datareader, "destination_order.kind", "BY_SOURCE_TIMESTAMP");
  DDS::DataReaderQos qos;
  settings.apply_to(qos);

  EXPECT_TRUE(DDS::BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS ==
             qos.destination_order.kind);
}

void test_set_datareader_destination_order_kind_by_reception_timestamp()
{
  QosSettings settings;

  settings.set_qos(QosSettings::datareader, "destination_order.kind", "BY_RECEPTION_TIMESTAMP");
  DDS::DataReaderQos qos;
  settings.apply_to(qos);

  EXPECT_TRUE(DDS::BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS ==
             qos.destination_order.kind);
}

void test_set_datareader_history_kind_keep_all()
{
  QosSettings settings;

  settings.set_qos(QosSettings::datareader, "history.kind", "KEEP_ALL");
  DDS::DataReaderQos qos;
  settings.apply_to(qos);

  EXPECT_TRUE(DDS::KEEP_ALL_HISTORY_QOS == qos.history.kind);
}

void test_set_datareader_history_kind_keep_last()
{
  QosSettings settings;

  settings.set_qos(QosSettings::datareader, "history.kind", "KEEP_LAST");
  DDS::DataReaderQos qos;
  settings.apply_to(qos);

  EXPECT_TRUE(DDS::KEEP_LAST_HISTORY_QOS == qos.history.kind);
}

void test_set_datareader_history_depth()
{
  QosSettings settings;

  settings.set_qos(QosSettings::datareader, "history.depth", "21");
  DDS::DataReaderQos qos;
  settings.apply_to(qos);

  EXPECT_TRUE(21 == qos.history.depth);
}

void test_set_datareader_resource_limits_max_samples()
{
  QosSettings settings;

  settings.set_qos(QosSettings::datareader, "resource_limits.max_samples", "14");
  DDS::DataReaderQos qos;
  settings.apply_to(qos);

  EXPECT_TRUE(14 == qos.resource_limits.max_samples);
}

void test_set_datareader_resource_limits_max_instances()
{
  QosSettings settings;

  settings.set_qos(QosSettings::datareader, "resource_limits.max_instances", "14");
  DDS::DataReaderQos qos;
  settings.apply_to(qos);

  EXPECT_TRUE(14 == qos.resource_limits.max_instances);
}

void test_set_datareader_resource_limits_max_samples_per_instance()
{
  QosSettings settings;

  settings.set_qos(QosSettings::datareader, "resource_limits.max_samples_per_instance", "14");
  DDS::DataReaderQos qos;
  settings.apply_to(qos);

  EXPECT_TRUE(14 == qos.resource_limits.max_samples_per_instance);
}

void test_set_datareader_ownership_kind_shared()
{
  QosSettings settings;

  settings.set_qos(QosSettings::datareader, "ownership.kind", "SHARED");
  DDS::DataReaderQos qos;
  settings.apply_to(qos);

  EXPECT_TRUE(DDS::SHARED_OWNERSHIP_QOS == qos.ownership.kind);
}

void test_set_datareader_ownership_kind_exclusive()
{
  QosSettings settings;

  settings.set_qos(QosSettings::datareader, "ownership.kind", "EXCLUSIVE");
  DDS::DataReaderQos qos;
  settings.apply_to(qos);

  EXPECT_TRUE(DDS::EXCLUSIVE_OWNERSHIP_QOS == qos.ownership.kind);
}

void test_set_datareader_time_based_filter_minimum_separation_sec()
{
  QosSettings settings;

  settings.set_qos(QosSettings::datareader, "time_based_filter.minimum_separation.sec", "2");
  DDS::DataReaderQos qos;
  settings.apply_to(qos);

  EXPECT_TRUE(2 == qos.time_based_filter.minimum_separation.sec);
  EXPECT_TRUE(0 == qos.time_based_filter.minimum_separation.nanosec);
}

void test_set_datareader_time_based_filter_minimum_separation_nanosec()
{
  QosSettings settings;

  settings.set_qos(QosSettings::datareader, "time_based_filter.minimum_separation.nanosec", "170");
  DDS::DataReaderQos qos;
  settings.apply_to(qos);

  EXPECT_TRUE(0 == qos.time_based_filter.minimum_separation.sec);
  EXPECT_TRUE(170 == qos.time_based_filter.minimum_separation.nanosec);
}

void test_set_datareader_time_based_filter_minimum_separation_both()
{
  QosSettings settings;

  settings.set_qos(QosSettings::datareader, "time_based_filter.minimum_separation.sec", "1");
  settings.set_qos(QosSettings::datareader, "time_based_filter.minimum_separation.nanosec", "100");
  DDS::DataReaderQos qos;
  settings.apply_to(qos);

  EXPECT_TRUE(1 == qos.time_based_filter.minimum_separation.sec);
  EXPECT_TRUE(100 == qos.time_based_filter.minimum_separation.nanosec);
}

void test_set_datareader_reader_data_lifecycle_autopurge_nowriter_samples_delay_sec()
{
  QosSettings settings;

  settings.set_qos(QosSettings::datareader, "reader_data_lifecycle.autopurge_nowriter_samples_delay.sec", "5");
  settings.set_qos(QosSettings::datareader, "reader_data_lifecycle.autopurge_nowriter_samples_delay.nanosec", "0");
  DDS::DataReaderQos qos;
  settings.apply_to(qos);

  EXPECT_TRUE(5 == qos.reader_data_lifecycle.autopurge_nowriter_samples_delay.sec);
  EXPECT_TRUE(0 == qos.reader_data_lifecycle.autopurge_nowriter_samples_delay.nanosec);
}

void test_set_datareader_reader_data_lifecycle_autopurge_nowriter_samples_delay_nanosec()
{
  QosSettings settings;

  settings.set_qos(QosSettings::datareader, "reader_data_lifecycle.autopurge_nowriter_samples_delay.sec", "0");
  settings.set_qos(QosSettings::datareader, "reader_data_lifecycle.autopurge_nowriter_samples_delay.nanosec", "5000");
  DDS::DataReaderQos qos;
  settings.apply_to(qos);

  EXPECT_TRUE(0 == qos.reader_data_lifecycle.autopurge_nowriter_samples_delay.sec);
  EXPECT_TRUE(5000 == qos.reader_data_lifecycle.autopurge_nowriter_samples_delay.nanosec);
}

void test_set_datareader_reader_data_lifecycle_autopurge_nowriter_samples_delay_both()
{
  QosSettings settings;

  settings.set_qos(QosSettings::datareader, "reader_data_lifecycle.autopurge_nowriter_samples_delay.nanosec", "1000");
  settings.set_qos(QosSettings::datareader, "reader_data_lifecycle.autopurge_nowriter_samples_delay.sec", "1");
  DDS::DataReaderQos qos;
  settings.apply_to(qos);

  EXPECT_TRUE(1 == qos.reader_data_lifecycle.autopurge_nowriter_samples_delay.sec);
  EXPECT_TRUE(1000 == qos.reader_data_lifecycle.autopurge_nowriter_samples_delay.nanosec);
}

void test_set_datareader_reader_data_lifecycle_autopurge_disposed_samples_delay_sec()
{
  QosSettings settings;

  settings.set_qos(QosSettings::datareader, "reader_data_lifecycle.autopurge_disposed_samples_delay.sec", "5");
  settings.set_qos(QosSettings::datareader, "reader_data_lifecycle.autopurge_disposed_samples_delay.nanosec", "0");
  DDS::DataReaderQos qos;
  settings.apply_to(qos);

  EXPECT_TRUE(5 == qos.reader_data_lifecycle.autopurge_disposed_samples_delay.sec);
  EXPECT_TRUE(0 == qos.reader_data_lifecycle.autopurge_disposed_samples_delay.nanosec);
}

void test_set_datareader_reader_data_lifecycle_autopurge_disposed_samples_delay_nanosec()
{
  QosSettings settings;

  settings.set_qos(QosSettings::datareader, "reader_data_lifecycle.autopurge_disposed_samples_delay.sec", "0");
  settings.set_qos(QosSettings::datareader, "reader_data_lifecycle.autopurge_disposed_samples_delay.nanosec", "5000");
  DDS::DataReaderQos qos;
  settings.apply_to(qos);

  EXPECT_TRUE(0 == qos.reader_data_lifecycle.autopurge_disposed_samples_delay.sec);
  EXPECT_TRUE(5000 == qos.reader_data_lifecycle.autopurge_disposed_samples_delay.nanosec);
}

void test_set_datareader_reader_data_lifecycle_autopurge_disposed_samples_delay_both()
{
  QosSettings settings;

  settings.set_qos(QosSettings::datareader, "reader_data_lifecycle.autopurge_disposed_samples_delay.nanosec", "1000");
  settings.set_qos(QosSettings::datareader, "reader_data_lifecycle.autopurge_disposed_samples_delay.sec", "1");
  DDS::DataReaderQos qos;
  settings.apply_to(qos);

  EXPECT_TRUE(1 == qos.reader_data_lifecycle.autopurge_disposed_samples_delay.sec);
  EXPECT_TRUE(1000 == qos.reader_data_lifecycle.autopurge_disposed_samples_delay.nanosec);
}

TEST(QosSettings, maintest)
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
#ifndef OPENDDS_NO_PERSISTENCE_PROFILE
  test_set_datawriter_durability_transient();
  test_set_datawriter_durability_persistent();
#endif // OPENDDS_NO_PERSISTENCE_PROFILE
  test_set_datawriter_deadline_sec();
  test_set_datawriter_deadline_nanosec();
  test_set_datawriter_deadline_both();
  test_set_datawriter_latency_budget_sec();
  test_set_datawriter_latency_budget_nanosec();
  test_set_datawriter_latency_budget_both();
  test_set_datawriter_liveliness_kind_automatic();
  test_set_datawriter_liveliness_kind_manual_by_topic();
  test_set_datawriter_liveliness_kind_manual_by_participant();
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
  test_set_datawriter_transport_priority_value();
  test_set_datawriter_lifespan_duration_sec();
  test_set_datawriter_lifespan_duration_nanosec();
  test_set_datawriter_lifespan_duration_both();
  test_set_datawriter_ownership_kind_shared();
  test_set_datawriter_ownership_kind_exclusive();
  test_set_datawriter_ownership_strength_value();

  // DataReader QOS
  test_set_datareader_durability_volatile();
  test_set_datareader_durability_transient_local();
#ifndef OPENDDS_NO_PERSISTENCE_PROFILE
  test_set_datareader_durability_transient();
  test_set_datareader_durability_persistent();
#endif // OPENDDS_NO_PERSISTENCE_PROFILE
  test_set_datareader_deadline_sec();
  test_set_datareader_deadline_nanosec();
  test_set_datareader_deadline_both();
  test_set_datareader_liveliness_kind_automatic();
  test_set_datareader_liveliness_kind_manual_by_topic();
  test_set_datareader_liveliness_kind_manual_by_participant();
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
  test_set_datareader_ownership_kind_shared();
  test_set_datareader_ownership_kind_exclusive();
  test_set_datareader_time_based_filter_minimum_separation_sec();
  test_set_datareader_time_based_filter_minimum_separation_nanosec();
  test_set_datareader_time_based_filter_minimum_separation_both();
  test_set_datareader_reader_data_lifecycle_autopurge_nowriter_samples_delay_sec();
  test_set_datareader_reader_data_lifecycle_autopurge_nowriter_samples_delay_nanosec();
  test_set_datareader_reader_data_lifecycle_autopurge_nowriter_samples_delay_both();
  test_set_datareader_reader_data_lifecycle_autopurge_disposed_samples_delay_sec();
  test_set_datareader_reader_data_lifecycle_autopurge_disposed_samples_delay_nanosec();
  test_set_datareader_reader_data_lifecycle_autopurge_disposed_samples_delay_both();
}

#endif
#endif
