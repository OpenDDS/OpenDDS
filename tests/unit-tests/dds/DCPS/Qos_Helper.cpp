// -*- C++ -*-

#include <gtest/gtest.h>

#include "dds/DCPS/Qos_Helper.h"

#include "MockTopic.h"
#include "MockPublisher.h"
#include "MockLogger.h"

#include "dds/DdsDcpsInfrastructureC.h"
#include "dds/DCPS/Time_Helper.h"
#include "dds/DCPS/Definitions.h"


const DDS::Duration_t zero = { DDS::DURATION_ZERO_SEC, DDS::DURATION_ZERO_NSEC };
const DDS::Duration_t infinite = { DDS::DURATION_INFINITE_SEC, DDS::DURATION_INFINITE_NSEC };
const DDS::Duration_t ms100 = { 0, 100000000 };

bool
lease_greater_than (::DDS::LivelinessQosPolicy const & qos1,
                    ::DDS::LivelinessQosPolicy const & qos2)
{
  using OpenDDS::DCPS::operator>;
  return qos1.lease_duration > qos2.lease_duration;
}

TEST(dds_DCPS_Qos_Helper, maintest)
{
  // verify that if dw is infinite, that it is always greater
  // (except when dr is also infinite, covered next)
  {
    ::DDS::LivelinessQosPolicy dw_liveliness;
    dw_liveliness.lease_duration.sec = ::DDS::DURATION_INFINITE_SEC;
    dw_liveliness.lease_duration.nanosec = ::DDS::DURATION_INFINITE_NSEC;

    ::DDS::LivelinessQosPolicy dr_liveliness;
    dr_liveliness.lease_duration.sec = ::DDS::DURATION_INFINITE_SEC;
    dr_liveliness.lease_duration.nanosec = ::DDS::DURATION_INFINITE_NSEC + 1;

    // verify that the datawriter liveliness is greater, since it is infinite
    EXPECT_TRUE(lease_greater_than(dw_liveliness, dr_liveliness));
    EXPECT_TRUE(!lease_greater_than(dr_liveliness, dw_liveliness));

    dr_liveliness.lease_duration.sec = ::DDS::DURATION_INFINITE_NSEC;
    dr_liveliness.lease_duration.nanosec = ::DDS::DURATION_INFINITE_NSEC - 1;

    // verify that the datawriter liveliness is greater, since it is infinite
    EXPECT_TRUE(lease_greater_than(dw_liveliness, dr_liveliness));
    EXPECT_TRUE(!lease_greater_than(dr_liveliness, dw_liveliness));

    dr_liveliness.lease_duration.sec = 0;
    dr_liveliness.lease_duration.nanosec = 0;

    // verify that the datawriter liveliness is greater, since it is infinite
    EXPECT_TRUE(lease_greater_than(dw_liveliness, dr_liveliness));
    EXPECT_TRUE(!lease_greater_than(dr_liveliness, dw_liveliness));
  }

  // verify that if dr is infinite, that dw is never greater
  {
    ::DDS::LivelinessQosPolicy dw_liveliness;
    dw_liveliness.lease_duration.sec = ::DDS::DURATION_INFINITE_SEC;
    dw_liveliness.lease_duration.nanosec = ::DDS::DURATION_INFINITE_NSEC;

    ::DDS::LivelinessQosPolicy dr_liveliness;
    dr_liveliness.lease_duration.sec = ::DDS::DURATION_INFINITE_SEC;
    dr_liveliness.lease_duration.nanosec = ::DDS::DURATION_INFINITE_NSEC;

    // verify that the datawriter liveliness isn't greater, since they are equal
    EXPECT_TRUE(!lease_greater_than(dw_liveliness, dr_liveliness));
    EXPECT_TRUE(!lease_greater_than(dr_liveliness, dw_liveliness));

    dw_liveliness.lease_duration.sec = ::DDS::DURATION_INFINITE_SEC;
    dw_liveliness.lease_duration.nanosec = ::DDS::DURATION_INFINITE_NSEC + 1;

    // verify that the datawriter liveliness isn't greater, since dr is infinite
    EXPECT_TRUE(!lease_greater_than(dw_liveliness, dr_liveliness));
    EXPECT_TRUE(lease_greater_than(dr_liveliness, dw_liveliness));

    dw_liveliness.lease_duration.sec = ::DDS::DURATION_INFINITE_SEC;
    dw_liveliness.lease_duration.nanosec = ::DDS::DURATION_INFINITE_NSEC - 1;

    // verify that the datawriter liveliness isn't greater, since dr is infinite
    EXPECT_TRUE(!lease_greater_than(dw_liveliness, dr_liveliness));
    EXPECT_TRUE(lease_greater_than(dr_liveliness, dw_liveliness));

    dw_liveliness.lease_duration.sec = 0;
    dw_liveliness.lease_duration.nanosec = 0;

    // verify that the datawriter liveliness isn't greater, since dr is infinite
    EXPECT_TRUE(!lease_greater_than(dw_liveliness, dr_liveliness));
    EXPECT_TRUE(lease_greater_than(dr_liveliness, dw_liveliness));
  }

  // neither is infinite
  {
    ::DDS::LivelinessQosPolicy dw_liveliness;
    dw_liveliness.lease_duration.sec = 5;
    dw_liveliness.lease_duration.nanosec = 0;

    ::DDS::LivelinessQosPolicy dr_liveliness;
    dr_liveliness.lease_duration.sec = 5;
    dr_liveliness.lease_duration.nanosec = 0;

    // verify that the datawriter liveliness isn't greater
    EXPECT_TRUE(!lease_greater_than(dw_liveliness, dr_liveliness));
    EXPECT_TRUE(!lease_greater_than(dr_liveliness, dw_liveliness));

    dw_liveliness.lease_duration.sec += 1;

    // verify that the datawriter liveliness is greater
    EXPECT_TRUE(lease_greater_than(dw_liveliness, dr_liveliness));
    EXPECT_TRUE(!lease_greater_than(dr_liveliness, dw_liveliness));

    dw_liveliness.lease_duration.sec -= 2;

    // verify that the datawriter liveliness isn't greater
    EXPECT_TRUE(!lease_greater_than(dw_liveliness, dr_liveliness));
    EXPECT_TRUE(lease_greater_than(dr_liveliness, dw_liveliness));

    dw_liveliness.lease_duration.sec = 15;
    dw_liveliness.lease_duration.nanosec = 16;

    dr_liveliness.lease_duration.sec = 15;
    dr_liveliness.lease_duration.nanosec = 15;

    // verify that the datawriter liveliness is greater
    EXPECT_TRUE(lease_greater_than(dw_liveliness, dr_liveliness));
    EXPECT_TRUE(!lease_greater_than(dr_liveliness, dw_liveliness));

    dw_liveliness.lease_duration.nanosec -= 2;

    // verify that the datawriter liveliness isn't greater
    EXPECT_TRUE(!lease_greater_than(dw_liveliness, dr_liveliness));
    EXPECT_TRUE(lease_greater_than(dr_liveliness, dw_liveliness));
  }
}

TEST(dds_DCPS_Qos_Helper, TransportPriorityQosPolicyBuilder_default_ctor)
{
  const OpenDDS::DCPS::TransportPriorityQosPolicyBuilder uut;
  const DDS::TransportPriorityQosPolicy& qos = uut;
  EXPECT_EQ(qos.value, 0);
}

TEST(dds_DCPS_Qos_Helper, TransportPriorityQosPolicyBuilder_ctor)
{
  OpenDDS::DCPS::TransportPriorityQosPolicyBuilder uut;
  uut.qos().value = 5;
  DDS::TransportPriorityQosPolicy& qos = uut;
  qos.value = 3;

  EXPECT_EQ(uut.qos().value, 3);
}

TEST(dds_DCPS_Qos_Helper, TransportPriorityQosPolicyBuilder_value)
{
  OpenDDS::DCPS::TransportPriorityQosPolicyBuilder uut;
  uut.value(3);
  EXPECT_EQ(uut.qos().value, 3);
}

TEST(dds_DCPS_Qos_Helper, LifespanQosPolicyBuilder_default_ctor)
{
  using OpenDDS::DCPS::operator==;

  const OpenDDS::DCPS::LifespanQosPolicyBuilder uut;
  const DDS::LifespanQosPolicy& qos = uut;
  EXPECT_TRUE(qos.duration == infinite);
}

TEST(dds_DCPS_Qos_Helper, LifespanQosPolicyBuilder_ctor)
{
  using OpenDDS::DCPS::operator==;

  OpenDDS::DCPS::LifespanQosPolicyBuilder uut;
  uut.qos().duration = zero;
  DDS::LifespanQosPolicy& qos = uut;
  qos.duration = ms100;

  EXPECT_TRUE(uut.qos().duration == ms100);
}

TEST(dds_DCPS_Qos_Helper, LifespanQosPolicyBuilder_duration)
{
  using OpenDDS::DCPS::operator==;

  OpenDDS::DCPS::LifespanQosPolicyBuilder uut;
  uut.duration(zero);
  EXPECT_TRUE(uut.qos().duration == zero);
}

TEST(dds_DCPS_Qos_Helper, DurabilityQosPolicyBuilder_default_ctor)
{
  const OpenDDS::DCPS::DurabilityQosPolicyBuilder uut;
  const DDS::DurabilityQosPolicy& qos = uut;
  EXPECT_EQ(qos.kind, DDS::VOLATILE_DURABILITY_QOS);
}

TEST(dds_DCPS_Qos_Helper, DurabilityQosPolicyBuilder_ctor)
{
  OpenDDS::DCPS::DurabilityQosPolicyBuilder uut;
  uut.qos().kind = DDS::TRANSIENT_LOCAL_DURABILITY_QOS;
  DDS::DurabilityQosPolicy& qos = uut;
  qos.kind = DDS::TRANSIENT_DURABILITY_QOS;

  EXPECT_EQ(uut.qos().kind, DDS::TRANSIENT_DURABILITY_QOS);
}

TEST(dds_DCPS_Qos_Helper, DurabilityQosPolicyBuilder_kind)
{
  OpenDDS::DCPS::DurabilityQosPolicyBuilder uut;
  uut.kind(DDS::TRANSIENT_DURABILITY_QOS);
  EXPECT_EQ(uut.qos().kind, DDS::TRANSIENT_DURABILITY_QOS);;
}

TEST(dds_DCPS_Qos_Helper, DurabilityQosPolicyBuilder_volatile)
{
  OpenDDS::DCPS::DurabilityQosPolicyBuilder uut;
  uut._volatile();
  EXPECT_EQ(uut.qos().kind, DDS::VOLATILE_DURABILITY_QOS);;
}

TEST(dds_DCPS_Qos_Helper, DurabilityQosPolicyBuilder_transient_local)
{
  OpenDDS::DCPS::DurabilityQosPolicyBuilder uut;
  uut.transient_local();
  EXPECT_EQ(uut.qos().kind, DDS::TRANSIENT_LOCAL_DURABILITY_QOS);;
}

TEST(dds_DCPS_Qos_Helper, DurabilityQosPolicyBuilder_transient)
{
  OpenDDS::DCPS::DurabilityQosPolicyBuilder uut;
  uut.transient();
  EXPECT_EQ(uut.qos().kind, DDS::TRANSIENT_DURABILITY_QOS);;
}

TEST(dds_DCPS_Qos_Helper, DurabilityQosPolicyBuilder_persistent)
{
  OpenDDS::DCPS::DurabilityQosPolicyBuilder uut;
  uut.persistent();
  EXPECT_EQ(uut.qos().kind, DDS::PERSISTENT_DURABILITY_QOS);;
}

TEST(dds_DCPS_Qos_Helper, DurabilityServiceQosPolicyBuilder_default_ctor)
{
  using OpenDDS::DCPS::operator==;

  const OpenDDS::DCPS::DurabilityServiceQosPolicyBuilder uut;
  const DDS::DurabilityServiceQosPolicy& qos = uut;
  EXPECT_TRUE(qos.service_cleanup_delay == zero);
  EXPECT_EQ(qos.history_kind, DDS::KEEP_LAST_HISTORY_QOS);
  EXPECT_EQ(qos.history_depth, 1);
  EXPECT_EQ(qos.max_samples, DDS::LENGTH_UNLIMITED);
  EXPECT_EQ(qos.max_instances, DDS::LENGTH_UNLIMITED);
  EXPECT_EQ(qos.max_samples_per_instance, DDS::LENGTH_UNLIMITED);
}

TEST(dds_DCPS_Qos_Helper, DurabilityServiceQosPolicyBuilder_ctor)
{
  using OpenDDS::DCPS::operator==;

  OpenDDS::DCPS::DurabilityServiceQosPolicyBuilder uut;
  uut.qos().history_kind = DDS::KEEP_ALL_HISTORY_QOS;
  DDS::DurabilityServiceQosPolicy& qos = uut;
  qos.history_depth = 3;

  EXPECT_TRUE(uut.qos().service_cleanup_delay == zero);
  EXPECT_EQ(uut.qos().history_kind, DDS::KEEP_ALL_HISTORY_QOS);
  EXPECT_EQ(uut.qos().history_depth, 3);
  EXPECT_EQ(uut.qos().max_samples, DDS::LENGTH_UNLIMITED);
  EXPECT_EQ(uut.qos().max_instances, DDS::LENGTH_UNLIMITED);
  EXPECT_EQ(uut.qos().max_samples_per_instance, DDS::LENGTH_UNLIMITED);
}

TEST(dds_DCPS_Qos_Helper, DurabilityServiceQosPolicyBuilder_service_cleanup_delay)
{
  using OpenDDS::DCPS::operator==;
  OpenDDS::DCPS::DurabilityServiceQosPolicyBuilder uut;
  uut.service_cleanup_delay(ms100);
  EXPECT_TRUE(uut.qos().service_cleanup_delay == ms100);
}

TEST(dds_DCPS_Qos_Helper, DurabilityServiceQosPolicyBuilder_history_kind)
{
  OpenDDS::DCPS::DurabilityServiceQosPolicyBuilder uut;
  uut.history_kind(DDS::KEEP_ALL_HISTORY_QOS);
  EXPECT_EQ(uut.qos().history_kind, DDS::KEEP_ALL_HISTORY_QOS);
}

TEST(dds_DCPS_Qos_Helper, DurabilityServiceQosPolicyBuilder_keep_last)
{
  OpenDDS::DCPS::DurabilityServiceQosPolicyBuilder uut;
  uut.keep_last(5);
  EXPECT_EQ(uut.qos().history_kind, DDS::KEEP_LAST_HISTORY_QOS);
  EXPECT_EQ(uut.qos().history_depth, 5);
}

TEST(dds_DCPS_Qos_Helper, DurabilityServiceQosPolicyBuilder_keep_all)
{
  OpenDDS::DCPS::DurabilityServiceQosPolicyBuilder uut;
  uut.keep_all();
  EXPECT_EQ(uut.qos().history_kind, DDS::KEEP_ALL_HISTORY_QOS);
}

TEST(dds_DCPS_Qos_Helper, DurabilityServiceQosPolicyBuilder_history_depth)
{
  OpenDDS::DCPS::DurabilityServiceQosPolicyBuilder uut;
  uut.history_depth(5);
  EXPECT_EQ(uut.qos().history_depth, 5);
}

TEST(dds_DCPS_Qos_Helper, DurabilityServiceQosPolicyBuilder_max_samples)
{
  OpenDDS::DCPS::DurabilityServiceQosPolicyBuilder uut;
  uut.max_samples(5);
  EXPECT_EQ(uut.qos().max_samples, 5);
}

TEST(dds_DCPS_Qos_Helper, DurabilityServiceQosPolicyBuilder_max_instances)
{
  OpenDDS::DCPS::DurabilityServiceQosPolicyBuilder uut;
  uut.max_instances(5);
  EXPECT_EQ(uut.qos().max_instances, 5);
}

TEST(dds_DCPS_Qos_Helper, DurabilityServiceQosPolicyBuilder_max_samples_per_instance)
{
  OpenDDS::DCPS::DurabilityServiceQosPolicyBuilder uut;
  uut.max_samples_per_instance(5);
  EXPECT_EQ(uut.qos().max_samples_per_instance, 5);
}

TEST(dds_DCPS_Qos_Helper, DeadlineQosPolicyBuilder_default_ctor)
{
  using OpenDDS::DCPS::operator==;

  const OpenDDS::DCPS::DeadlineQosPolicyBuilder uut;
  const DDS::DeadlineQosPolicy& qos = uut;
  EXPECT_TRUE(qos.period == infinite);
}

TEST(dds_DCPS_Qos_Helper, DeadlineQosPolicyBuilder_ctor)
{
  using OpenDDS::DCPS::operator==;

  OpenDDS::DCPS::DeadlineQosPolicyBuilder uut;
  uut.qos().period = zero;
  DDS::DeadlineQosPolicy& qos = uut;
  qos.period = ms100;

  EXPECT_TRUE(uut.qos().period == ms100);
}

TEST(dds_DCPS_Qos_Helper, DeadlineQosPolicyBuilder_period)
{
  using OpenDDS::DCPS::operator==;

  OpenDDS::DCPS::DeadlineQosPolicyBuilder uut;
  uut.period(zero);
  EXPECT_TRUE(uut.qos().period == zero);
}

TEST(dds_DCPS_Qos_Helper, LatencyBudgetQosPolicyBuilder_default_ctor)
{
  using OpenDDS::DCPS::operator==;

  const OpenDDS::DCPS::LatencyBudgetQosPolicyBuilder uut;
  const DDS::LatencyBudgetQosPolicy& qos = uut;
  EXPECT_TRUE(qos.duration == zero);
}

TEST(dds_DCPS_Qos_Helper, LatencyBudgetQosPolicyBuilder_ctor)
{
  using OpenDDS::DCPS::operator==;

  OpenDDS::DCPS::LatencyBudgetQosPolicyBuilder uut;
  uut.qos().duration = infinite;
  DDS::LatencyBudgetQosPolicy& qos = uut;
  qos.duration = ms100;

  EXPECT_TRUE(uut.qos().duration == ms100);
}

TEST(dds_DCPS_Qos_Helper, LatencyBudgetQosPolicyBuilder_duration)
{
  using OpenDDS::DCPS::operator==;

  OpenDDS::DCPS::LatencyBudgetQosPolicyBuilder uut;
  uut.duration(zero);
  EXPECT_TRUE(uut.qos().duration == zero);
}

TEST(dds_DCPS_Qos_Helper, OwnershipQosPolicyBuilder_default_ctor)
{
  const OpenDDS::DCPS::OwnershipQosPolicyBuilder uut;
  const DDS::OwnershipQosPolicy& qos = uut;
  EXPECT_EQ(qos.kind, DDS::SHARED_OWNERSHIP_QOS);
}

TEST(dds_DCPS_Qos_Helper, OwnershipQosPolicyBuilder_ctor)
{
  OpenDDS::DCPS::OwnershipQosPolicyBuilder uut;
  uut.qos().kind = DDS::SHARED_OWNERSHIP_QOS;
  DDS::OwnershipQosPolicy& qos = uut;
  qos.kind = DDS::EXCLUSIVE_OWNERSHIP_QOS;

  EXPECT_EQ(uut.qos().kind, DDS::EXCLUSIVE_OWNERSHIP_QOS);
}

TEST(dds_DCPS_Qos_Helper, OwnershipQosPolicyBuilder_kind)
{
  OpenDDS::DCPS::OwnershipQosPolicyBuilder uut;
  uut.kind(DDS::EXCLUSIVE_OWNERSHIP_QOS);
  EXPECT_EQ(uut.qos().kind, DDS::EXCLUSIVE_OWNERSHIP_QOS);
}

TEST(dds_DCPS_Qos_Helper, OwnershipQosPolicyBuilder_shared)
{
  OpenDDS::DCPS::OwnershipQosPolicyBuilder uut;
  uut.shared();
  EXPECT_EQ(uut.qos().kind, DDS::SHARED_OWNERSHIP_QOS);;
}

TEST(dds_DCPS_Qos_Helper, OwnershipQosPolicyBuilder_exclusive)
{
  OpenDDS::DCPS::OwnershipQosPolicyBuilder uut;
  uut.exclusive();
  EXPECT_EQ(uut.qos().kind, DDS::EXCLUSIVE_OWNERSHIP_QOS);;
}

TEST(dds_DCPS_Qos_Helper, OwnershipStrengthQosPolicyBuilder_default_ctor)
{
  const OpenDDS::DCPS::OwnershipStrengthQosPolicyBuilder uut;
  const DDS::OwnershipStrengthQosPolicy& qos = uut;
  EXPECT_EQ(qos.value, 0);
}

TEST(dds_DCPS_Qos_Helper, OwnershipStrengthQosPolicyBuilder_ctor)
{
  OpenDDS::DCPS::OwnershipStrengthQosPolicyBuilder uut;
  uut.qos().value = 5;
  DDS::OwnershipStrengthQosPolicy& qos = uut;
  qos.value = 3;

  EXPECT_EQ(uut.qos().value, 3);
}

TEST(dds_DCPS_Qos_Helper, OwnershipStrengthQosPolicyBuilder_value)
{
  OpenDDS::DCPS::OwnershipStrengthQosPolicyBuilder uut;
  uut.value(3);
  EXPECT_EQ(uut.qos().value, 3);
}

TEST(dds_DCPS_Qos_Helper, LivelinessQosPolicyBuilder_default_ctor)
{
  using OpenDDS::DCPS::operator==;
  const OpenDDS::DCPS::LivelinessQosPolicyBuilder uut;
  const DDS::LivelinessQosPolicy& qos = uut;
  EXPECT_EQ(qos.kind, DDS::AUTOMATIC_LIVELINESS_QOS);
  EXPECT_TRUE(qos.lease_duration == infinite);
}

TEST(dds_DCPS_Qos_Helper, LivelinessQosPolicyBuilder_ctor)
{
  using OpenDDS::DCPS::operator==;
  OpenDDS::DCPS::LivelinessQosPolicyBuilder uut;
  uut.qos().kind = DDS::MANUAL_BY_PARTICIPANT_LIVELINESS_QOS;
  DDS::LivelinessQosPolicy& qos = uut;
  qos.lease_duration = ms100;

  EXPECT_EQ(uut.qos().kind, DDS::MANUAL_BY_PARTICIPANT_LIVELINESS_QOS);
  EXPECT_TRUE(qos.lease_duration == ms100);
}

TEST(dds_DCPS_Qos_Helper, LivelinessQosPolicyBuilder_kind)
{
  OpenDDS::DCPS::LivelinessQosPolicyBuilder uut;
  uut.kind(DDS::MANUAL_BY_PARTICIPANT_LIVELINESS_QOS);
  EXPECT_EQ(uut.qos().kind, DDS::MANUAL_BY_PARTICIPANT_LIVELINESS_QOS);
}

TEST(dds_DCPS_Qos_Helper, LivelinessQosPolicyBuilder_automatic)
{
  OpenDDS::DCPS::LivelinessQosPolicyBuilder uut;
  uut.automatic();
  EXPECT_EQ(uut.qos().kind, DDS::AUTOMATIC_LIVELINESS_QOS);;
}

TEST(dds_DCPS_Qos_Helper, LivelinessQosPolicyBuilder_manual_by_participant)
{
  OpenDDS::DCPS::LivelinessQosPolicyBuilder uut;
  uut.manual_by_participant();
  EXPECT_EQ(uut.qos().kind, DDS::MANUAL_BY_PARTICIPANT_LIVELINESS_QOS);;
}

TEST(dds_DCPS_Qos_Helper, LivelinessQosPolicyBuilder_manual_by_topic)
{
  OpenDDS::DCPS::LivelinessQosPolicyBuilder uut;
  uut.manual_by_topic();
  EXPECT_EQ(uut.qos().kind, DDS::MANUAL_BY_TOPIC_LIVELINESS_QOS);;
}

TEST(dds_DCPS_Qos_Helper, LivelinessQosPolicyBuilder_lease_duration)
{
  using OpenDDS::DCPS::operator==;
  OpenDDS::DCPS::LivelinessQosPolicyBuilder uut;
  uut.lease_duration(ms100);
  EXPECT_TRUE(uut.qos().lease_duration == ms100);
}

TEST(dds_DCPS_Qos_Helper, ResourceLimitsQosPolicyBuilder_default_ctor)
{
  const OpenDDS::DCPS::ResourceLimitsQosPolicyBuilder uut;
  const DDS::ResourceLimitsQosPolicy& qos = uut;
  EXPECT_EQ(qos.max_samples, DDS::LENGTH_UNLIMITED);
  EXPECT_EQ(qos.max_instances, DDS::LENGTH_UNLIMITED);
  EXPECT_EQ(qos.max_samples_per_instance, DDS::LENGTH_UNLIMITED);
}

TEST(dds_DCPS_Qos_Helper, ResourceLimitsQosPolicyBuilder_ctor)
{
  using OpenDDS::DCPS::operator==;

  OpenDDS::DCPS::ResourceLimitsQosPolicyBuilder uut;
  uut.qos().max_samples = 1;
  DDS::ResourceLimitsQosPolicy& qos = uut;
  qos.max_instances = 2;

  EXPECT_EQ(uut.qos().max_samples, 1);
  EXPECT_EQ(uut.qos().max_instances, 2);
  EXPECT_EQ(uut.qos().max_samples_per_instance, DDS::LENGTH_UNLIMITED);
}

TEST(dds_DCPS_Qos_Helper, ResourceLimitsQosPolicyBuilder_max_samples)
{
  OpenDDS::DCPS::ResourceLimitsQosPolicyBuilder uut;
  uut.max_samples(5);
  EXPECT_EQ(uut.qos().max_samples, 5);
}

TEST(dds_DCPS_Qos_Helper, ResourceLimitsQosPolicyBuilder_max_instances)
{
  OpenDDS::DCPS::ResourceLimitsQosPolicyBuilder uut;
  uut.max_instances(5);
  EXPECT_EQ(uut.qos().max_instances, 5);
}

TEST(dds_DCPS_Qos_Helper, ResourceLimitsQosPolicyBuilder_max_samples_per_instance)
{
  OpenDDS::DCPS::ResourceLimitsQosPolicyBuilder uut;
  uut.max_samples_per_instance(5);
  EXPECT_EQ(uut.qos().max_samples_per_instance, 5);
}

TEST(dds_DCPS_Qos_Helper, WriterDataLifecycleQosPolicyBuilder_default_ctor)
{
  const OpenDDS::DCPS::WriterDataLifecycleQosPolicyBuilder uut;
  const DDS::WriterDataLifecycleQosPolicy& qos = uut;
  EXPECT_EQ(qos.autodispose_unregistered_instances, true);
}

TEST(dds_DCPS_Qos_Helper, WriterDataLifecycleQosPolicyBuilder_ctor)
{
  OpenDDS::DCPS::WriterDataLifecycleQosPolicyBuilder uut;
  uut.qos().autodispose_unregistered_instances = false;
  DDS::WriterDataLifecycleQosPolicy& qos = uut;
  qos.autodispose_unregistered_instances = true;

  EXPECT_EQ(uut.qos().autodispose_unregistered_instances, true);
}

TEST(dds_DCPS_Qos_Helper, WriterDataLifecycleQosPolicyBuilder_autodispose_unregistered_instances)
{
  OpenDDS::DCPS::WriterDataLifecycleQosPolicyBuilder uut;
  uut.autodispose_unregistered_instances(false);
  EXPECT_EQ(uut.qos().autodispose_unregistered_instances, false);
}

TEST(dds_DCPS_Qos_Helper, TopicQosBuilder_default_ctor)
{
  using OpenDDS::DCPS::operator==;

  const OpenDDS::DCPS::TopicQosBuilder uut;
  const DDS::TopicQos& qos = uut;
  EXPECT_EQ(qos.topic_data.value.length(), 0u);
  EXPECT_EQ(qos.durability.kind, DDS::VOLATILE_DURABILITY_QOS);
  EXPECT_TRUE(qos.durability_service.service_cleanup_delay == zero);
  EXPECT_EQ(qos.durability_service.history_kind, DDS::KEEP_LAST_HISTORY_QOS);
  EXPECT_EQ(qos.durability_service.history_depth, 1);
  EXPECT_EQ(qos.durability_service.max_samples, DDS::LENGTH_UNLIMITED);
  EXPECT_EQ(qos.durability_service.max_instances, DDS::LENGTH_UNLIMITED);
  EXPECT_EQ(qos.durability_service.max_samples_per_instance, DDS::LENGTH_UNLIMITED);
  EXPECT_TRUE(qos.deadline.period == infinite);
  EXPECT_TRUE(qos.latency_budget.duration == zero);
  EXPECT_EQ(qos.liveliness.kind, DDS::AUTOMATIC_LIVELINESS_QOS);
  EXPECT_TRUE(qos.liveliness.lease_duration == infinite);
  EXPECT_EQ(qos.reliability.kind, DDS::BEST_EFFORT_RELIABILITY_QOS);
  EXPECT_TRUE(qos.reliability.max_blocking_time == infinite);
  EXPECT_EQ(qos.destination_order.kind, DDS::BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS);
  EXPECT_EQ(qos.history.kind, DDS::KEEP_LAST_HISTORY_QOS);
  EXPECT_EQ(qos.history.depth, 1);
  EXPECT_EQ(qos.resource_limits.max_samples, DDS::LENGTH_UNLIMITED);
  EXPECT_EQ(qos.resource_limits.max_instances, DDS::LENGTH_UNLIMITED);
  EXPECT_EQ(qos.resource_limits.max_samples_per_instance, DDS::LENGTH_UNLIMITED);
  EXPECT_EQ(qos.transport_priority.value, 0);
  EXPECT_TRUE(qos.lifespan.duration == infinite);
  EXPECT_EQ(qos.ownership.kind, DDS::SHARED_OWNERSHIP_QOS);
}

TEST(dds_DCPS_Qos_Helper, TopicQosBuilder_ctor)
{
  using OpenDDS::DCPS::operator==;

  OpenDDS::DCPS::TopicQosBuilder uut;
  uut.qos().durability.kind = DDS::TRANSIENT_LOCAL_DURABILITY_QOS;
  DDS::TopicQos& qos = uut;
  qos.history.depth = 3;

  EXPECT_EQ(uut.qos().topic_data.value.length(), 0u);
  EXPECT_EQ(uut.qos().durability.kind, DDS::TRANSIENT_LOCAL_DURABILITY_QOS);
  EXPECT_TRUE(uut.qos().durability_service.service_cleanup_delay == zero);
  EXPECT_EQ(uut.qos().durability_service.history_kind, DDS::KEEP_LAST_HISTORY_QOS);
  EXPECT_EQ(uut.qos().durability_service.history_depth, 1);
  EXPECT_EQ(uut.qos().durability_service.max_samples, DDS::LENGTH_UNLIMITED);
  EXPECT_EQ(uut.qos().durability_service.max_instances, DDS::LENGTH_UNLIMITED);
  EXPECT_EQ(uut.qos().durability_service.max_samples_per_instance, DDS::LENGTH_UNLIMITED);
  EXPECT_TRUE(uut.qos().deadline.period == infinite);
  EXPECT_TRUE(uut.qos().latency_budget.duration == zero);
  EXPECT_EQ(uut.qos().liveliness.kind, DDS::AUTOMATIC_LIVELINESS_QOS);
  EXPECT_TRUE(uut.qos().liveliness.lease_duration == infinite);
  EXPECT_EQ(uut.qos().reliability.kind, DDS::BEST_EFFORT_RELIABILITY_QOS);
  EXPECT_TRUE(uut.qos().reliability.max_blocking_time == infinite);
  EXPECT_EQ(uut.qos().destination_order.kind, DDS::BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS);
  EXPECT_EQ(uut.qos().history.kind, DDS::KEEP_LAST_HISTORY_QOS);
  EXPECT_EQ(uut.qos().history.depth, 3);
  EXPECT_EQ(uut.qos().resource_limits.max_samples, DDS::LENGTH_UNLIMITED);
  EXPECT_EQ(uut.qos().resource_limits.max_instances, DDS::LENGTH_UNLIMITED);
  EXPECT_EQ(uut.qos().resource_limits.max_samples_per_instance, DDS::LENGTH_UNLIMITED);
  EXPECT_EQ(uut.qos().transport_priority.value, 0);
  EXPECT_TRUE(uut.qos().lifespan.duration == infinite);
  EXPECT_EQ(uut.qos().ownership.kind, DDS::SHARED_OWNERSHIP_QOS);
}

TEST(dds_DCPS_Qos_Helper, TopicQosBuilder_topic_data_value)
{
  using OpenDDS::DCPS::operator==;
  DDS::OctetSeq topic_data;
  topic_data.length(1);
  topic_data[0] = 'A';
  OpenDDS::DCPS::TopicQosBuilder uut;
  uut.topic_data_value(topic_data);
  EXPECT_TRUE(uut.qos().topic_data.value == topic_data);
}

TEST(dds_DCPS_Qos_Helper, TopicQosBuilder_durability_kind)
{
  OpenDDS::DCPS::TopicQosBuilder uut;
  uut.durability_kind(DDS::TRANSIENT_LOCAL_DURABILITY_QOS);
  EXPECT_EQ(uut.qos().durability.kind, DDS::TRANSIENT_LOCAL_DURABILITY_QOS);
}

TEST(dds_DCPS_Qos_Helper, TopicQosBuilder_durability_volatile)
{
  OpenDDS::DCPS::TopicQosBuilder uut;
  uut.durability_transient_local().durability_volatile();
  EXPECT_EQ(uut.qos().durability.kind, DDS::VOLATILE_DURABILITY_QOS);
}

TEST(dds_DCPS_Qos_Helper, TopicQosBuilder_durability_transient_local)
{
  OpenDDS::DCPS::TopicQosBuilder uut;
  uut.durability_transient_local();
  EXPECT_EQ(uut.qos().durability.kind, DDS::TRANSIENT_LOCAL_DURABILITY_QOS);
}

TEST(dds_DCPS_Qos_Helper, TopicQosBuilder_durability_transient)
{
  OpenDDS::DCPS::TopicQosBuilder uut;
  uut.durability_transient();
  EXPECT_EQ(uut.qos().durability.kind, DDS::TRANSIENT_DURABILITY_QOS);
}

TEST(dds_DCPS_Qos_Helper, TopicQosBuilder_durability_persistent)
{
  OpenDDS::DCPS::TopicQosBuilder uut;
  uut.durability_persistent();
  EXPECT_EQ(uut.qos().durability.kind, DDS::PERSISTENT_DURABILITY_QOS);
}

TEST(dds_DCPS_Qos_Helper, TopicQosBuilder_durability_service_service_cleanup_delay)
{
  using OpenDDS::DCPS::operator==;
  OpenDDS::DCPS::TopicQosBuilder uut;
  uut.durability_service_service_cleanup_delay(ms100);
  EXPECT_TRUE(uut.qos().durability_service.service_cleanup_delay == ms100);
}

TEST(dds_DCPS_Qos_Helper, TopicQosBuilder_durability_service_history_kind)
{
  OpenDDS::DCPS::TopicQosBuilder uut;
  uut.durability_service_history_kind(DDS::KEEP_ALL_HISTORY_QOS);
  EXPECT_EQ(uut.qos().durability_service.history_kind, DDS::KEEP_ALL_HISTORY_QOS);
}

TEST(dds_DCPS_Qos_Helper, TopicQosBuilder_durability_service_keep_last)
{
  OpenDDS::DCPS::TopicQosBuilder uut;
  uut.durability_service_keep_last(5);
  EXPECT_EQ(uut.qos().durability_service.history_kind, DDS::KEEP_LAST_HISTORY_QOS);
  EXPECT_EQ(uut.qos().durability_service.history_depth, 5);
}

TEST(dds_DCPS_Qos_Helper, TopicQosBuilder_durability_service_keep_all)
{
  OpenDDS::DCPS::TopicQosBuilder uut;
  uut.durability_service_keep_all();
  EXPECT_EQ(uut.qos().durability_service.history_kind, DDS::KEEP_ALL_HISTORY_QOS);
}

TEST(dds_DCPS_Qos_Helper, TopicQosBuilder_durability_service_history_depth)
{
  OpenDDS::DCPS::TopicQosBuilder uut;
  uut.durability_service_history_depth(5);
  EXPECT_EQ(uut.qos().durability_service.history_depth, 5);
}

TEST(dds_DCPS_Qos_Helper, TopicQosBuilder_durability_service_max_samples)
{
  OpenDDS::DCPS::TopicQosBuilder uut;
  uut.durability_service_max_samples(5);
  EXPECT_EQ(uut.qos().durability_service.max_samples, 5);
}

TEST(dds_DCPS_Qos_Helper, TopicQosBuilder_durability_service_max_instances)
{
  OpenDDS::DCPS::TopicQosBuilder uut;
  uut.durability_service_max_instances(5);
  EXPECT_EQ(uut.qos().durability_service.max_instances, 5);
}

TEST(dds_DCPS_Qos_Helper, TopicQosBuilder_durability_service_max_samples_per_instance)
{
  OpenDDS::DCPS::TopicQosBuilder uut;
  uut.durability_service_max_samples_per_instance(5);
  EXPECT_EQ(uut.qos().durability_service.max_samples_per_instance, 5);
}

TEST(dds_DCPS_Qos_Helper, TopicQosBuilder_deadline_period)
{
  using OpenDDS::DCPS::operator==;
  OpenDDS::DCPS::TopicQosBuilder uut;
  uut.deadline_period(ms100);
  EXPECT_TRUE(uut.qos().deadline.period == ms100);
}

TEST(dds_DCPS_Qos_Helper, TopicQosBuilder_latency_budget_duration)
{
  using OpenDDS::DCPS::operator==;
  OpenDDS::DCPS::TopicQosBuilder uut;
  uut.latency_budget_duration(ms100);
  EXPECT_TRUE(uut.qos().latency_budget.duration == ms100);
}

TEST(dds_DCPS_Qos_Helper, TopicQosBuilder_liveliness_kind)
{
  OpenDDS::DCPS::TopicQosBuilder uut;
  uut.liveliness_kind(DDS::MANUAL_BY_PARTICIPANT_LIVELINESS_QOS);
  EXPECT_EQ(uut.qos().liveliness.kind, DDS::MANUAL_BY_PARTICIPANT_LIVELINESS_QOS);
}

TEST(dds_DCPS_Qos_Helper, TopicQosBuilder_liveliness_automatic)
{
  OpenDDS::DCPS::TopicQosBuilder uut;
  uut.liveliness_manual_by_participant().liveliness_automatic();
  EXPECT_EQ(uut.qos().liveliness.kind, DDS::AUTOMATIC_LIVELINESS_QOS);
}

TEST(dds_DCPS_Qos_Helper, TopicQosBuilder_liveliness_manual_by_participant)
{
  OpenDDS::DCPS::TopicQosBuilder uut;
  uut.liveliness_manual_by_participant().liveliness_manual_by_participant();
  EXPECT_EQ(uut.qos().liveliness.kind, DDS::MANUAL_BY_PARTICIPANT_LIVELINESS_QOS);
}

TEST(dds_DCPS_Qos_Helper, TopicQosBuilder_liveliness_manual_by_topic)
{
  OpenDDS::DCPS::TopicQosBuilder uut;
  uut.liveliness_manual_by_topic().liveliness_manual_by_topic();
  EXPECT_EQ(uut.qos().liveliness.kind, DDS::MANUAL_BY_TOPIC_LIVELINESS_QOS);
}

TEST(dds_DCPS_Qos_Helper, TopicQosBuilder_liveliness_lease_duration)
{
  using OpenDDS::DCPS::operator==;
  OpenDDS::DCPS::TopicQosBuilder uut;
  uut.liveliness_lease_duration(ms100);
  EXPECT_TRUE(uut.qos().liveliness.lease_duration == ms100);
}

TEST(dds_DCPS_Qos_Helper, TopicQosBuilder_reliability_kind)
{
  OpenDDS::DCPS::TopicQosBuilder uut;
  uut.reliability_kind(DDS::BEST_EFFORT_RELIABILITY_QOS);
  EXPECT_EQ(uut.qos().reliability.kind, DDS::BEST_EFFORT_RELIABILITY_QOS);
}

TEST(dds_DCPS_Qos_Helper, TopicQosBuilder_reliability_best_effort)
{
  OpenDDS::DCPS::TopicQosBuilder uut;
  uut.reliability_best_effort();
  EXPECT_EQ(uut.qos().reliability.kind, DDS::BEST_EFFORT_RELIABILITY_QOS);
}

TEST(dds_DCPS_Qos_Helper, TopicQosBuilder_reliability_reliable)
{
  OpenDDS::DCPS::TopicQosBuilder uut;
  uut.reliability_best_effort().reliability_reliable();
  EXPECT_EQ(uut.qos().reliability.kind, DDS::RELIABLE_RELIABILITY_QOS);
}

TEST(dds_DCPS_Qos_Helper, TopicQosBuilder_reliability_max_blocking_time)
{
  using OpenDDS::DCPS::operator==;
  OpenDDS::DCPS::TopicQosBuilder uut;
  uut.reliability_max_blocking_time(zero);
  EXPECT_TRUE(uut.qos().reliability.max_blocking_time == zero);
}

TEST(dds_DCPS_Qos_Helper, TopicQosBuilder_destination_order_kind)
{
  OpenDDS::DCPS::TopicQosBuilder uut;
  uut.destination_order_kind(DDS::BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS);
  EXPECT_EQ(uut.qos().destination_order.kind, DDS::BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS);
}

TEST(dds_DCPS_Qos_Helper, TopicQosBuilder_destination_order_by_source_timestamp)
{
  OpenDDS::DCPS::TopicQosBuilder uut;
  uut.destination_order_by_source_timestamp();
  EXPECT_EQ(uut.qos().destination_order.kind, DDS::BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS);
}

TEST(dds_DCPS_Qos_Helper, TopicQosBuilder_destination_order_by_reception_timestamp)
{
  OpenDDS::DCPS::TopicQosBuilder uut;
  uut.destination_order_by_source_timestamp().destination_order_by_reception_timestamp();
  EXPECT_EQ(uut.qos().destination_order.kind, DDS::BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS);
}

TEST(dds_DCPS_Qos_Helper, TopicQosBuilder_history_kind)
{
  OpenDDS::DCPS::TopicQosBuilder uut;
  uut.history_kind(DDS::KEEP_ALL_HISTORY_QOS);
  EXPECT_EQ(uut.qos().history.kind, DDS::KEEP_ALL_HISTORY_QOS);
}

TEST(dds_DCPS_Qos_Helper, TopicQosBuilder_history_keep_last)
{
  OpenDDS::DCPS::TopicQosBuilder uut;
  uut.history_keep_last(5);
  EXPECT_EQ(uut.qos().history.kind, DDS::KEEP_LAST_HISTORY_QOS);
  EXPECT_EQ(uut.qos().history.depth, 5);
}

TEST(dds_DCPS_Qos_Helper, TopicQosBuilder_history_keep_all)
{
  OpenDDS::DCPS::TopicQosBuilder uut;
  uut.history_keep_all();
  EXPECT_EQ(uut.qos().history.kind, DDS::KEEP_ALL_HISTORY_QOS);
}

TEST(dds_DCPS_Qos_Helper, TopicQosBuilder_history_depth)
{
  OpenDDS::DCPS::TopicQosBuilder uut;
  uut.history_depth(5);
  EXPECT_EQ(uut.qos().history.depth, 5);
}

TEST(dds_DCPS_Qos_Helper, TopicQosBuilder_resource_limits_max_samples)
{
  OpenDDS::DCPS::TopicQosBuilder uut;
  uut.resource_limits_max_samples(5);
  EXPECT_EQ(uut.qos().resource_limits.max_samples, 5);
}

TEST(dds_DCPS_Qos_Helper, TopicQosBuilder_resource_limits_max_instances)
{
  OpenDDS::DCPS::TopicQosBuilder uut;
  uut.resource_limits_max_instances(5);
  EXPECT_EQ(uut.qos().resource_limits.max_instances, 5);
}

TEST(dds_DCPS_Qos_Helper, TopicQosBuilder_resource_limits_max_samples_per_instance)
{
  OpenDDS::DCPS::TopicQosBuilder uut;
  uut.resource_limits_max_samples_per_instance(5);
  EXPECT_EQ(uut.qos().resource_limits.max_samples_per_instance, 5);
}

TEST(dds_DCPS_Qos_Helper, TopicQosBuilder_transport_priority_value)
{
  OpenDDS::DCPS::TopicQosBuilder uut;
  uut.transport_priority_value(5);
  EXPECT_EQ(uut.qos().transport_priority.value, 5);
}

TEST(dds_DCPS_Qos_Helper, TopicQosBuilder_lifespan_duration)
{
  using OpenDDS::DCPS::operator==;
  OpenDDS::DCPS::TopicQosBuilder uut;
  uut.lifespan_duration(ms100);
  EXPECT_TRUE(uut.qos().lifespan.duration == ms100);
}

TEST(dds_DCPS_Qos_Helper, TopicQosBuilder_ownership_kind)
{
  OpenDDS::DCPS::TopicQosBuilder uut;
  uut.ownership_kind(DDS::EXCLUSIVE_OWNERSHIP_QOS);
  EXPECT_EQ(uut.qos().ownership.kind, DDS::EXCLUSIVE_OWNERSHIP_QOS);
}

TEST(dds_DCPS_Qos_Helper, TopicQosBuilder_ownership_shared)
{
  OpenDDS::DCPS::TopicQosBuilder uut;
  uut.ownership_shared();
  EXPECT_EQ(uut.qos().ownership.kind, DDS::SHARED_OWNERSHIP_QOS);
}

TEST(dds_DCPS_Qos_Helper, TopicQosBuilder_ownership_exclusive)
{
  OpenDDS::DCPS::TopicQosBuilder uut;
  uut.ownership_exclusive();
  EXPECT_EQ(uut.qos().ownership.kind, DDS::EXCLUSIVE_OWNERSHIP_QOS);
}

TEST(dds_DCPS_Qos_Helper, DataWriterQosBuilder_default_ctor)
{
  using OpenDDS::DCPS::operator==;

  const OpenDDS::DCPS::DataWriterQosBuilder uut;
  const DDS::DataWriterQos& qos = uut;
  EXPECT_EQ(qos.durability.kind, DDS::VOLATILE_DURABILITY_QOS);
  EXPECT_TRUE(qos.durability_service.service_cleanup_delay == zero);
  EXPECT_EQ(qos.durability_service.history_kind, DDS::KEEP_LAST_HISTORY_QOS);
  EXPECT_EQ(qos.durability_service.history_depth, 1);
  EXPECT_EQ(qos.durability_service.max_samples, DDS::LENGTH_UNLIMITED);
  EXPECT_EQ(qos.durability_service.max_instances, DDS::LENGTH_UNLIMITED);
  EXPECT_EQ(qos.durability_service.max_samples_per_instance, DDS::LENGTH_UNLIMITED);
  EXPECT_TRUE(qos.deadline.period == infinite);
  EXPECT_TRUE(qos.latency_budget.duration == zero);
  EXPECT_EQ(qos.liveliness.kind, DDS::AUTOMATIC_LIVELINESS_QOS);
  EXPECT_TRUE(qos.liveliness.lease_duration == infinite);
  EXPECT_EQ(qos.reliability.kind, DDS::RELIABLE_RELIABILITY_QOS);
  EXPECT_TRUE(qos.reliability.max_blocking_time == ms100);
  EXPECT_EQ(qos.destination_order.kind, DDS::BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS);
  EXPECT_EQ(qos.history.kind, DDS::KEEP_LAST_HISTORY_QOS);
  EXPECT_EQ(qos.history.depth, 1);
  EXPECT_EQ(qos.resource_limits.max_samples, DDS::LENGTH_UNLIMITED);
  EXPECT_EQ(qos.resource_limits.max_instances, DDS::LENGTH_UNLIMITED);
  EXPECT_EQ(qos.resource_limits.max_samples_per_instance, DDS::LENGTH_UNLIMITED);
  EXPECT_EQ(qos.transport_priority.value, 0);
  EXPECT_TRUE(qos.lifespan.duration == infinite);
  EXPECT_EQ(qos.user_data.value.length(), 0u);
  EXPECT_EQ(qos.ownership.kind, DDS::SHARED_OWNERSHIP_QOS);
  EXPECT_EQ(qos.ownership_strength.value, 0);
  EXPECT_EQ(qos.writer_data_lifecycle.autodispose_unregistered_instances, true);
}

TEST(dds_DCPS_Qos_Helper, DataWriterQosBuilder_ctor)
{
  using OpenDDS::DCPS::operator==;

  OpenDDS::DCPS::DataWriterQosBuilder uut;
  uut.qos().durability.kind = DDS::TRANSIENT_LOCAL_DURABILITY_QOS;
  DDS::DataWriterQos& qos = uut;
  qos.history.depth = 3;

  EXPECT_EQ(uut.qos().durability.kind, DDS::TRANSIENT_LOCAL_DURABILITY_QOS);
  EXPECT_TRUE(uut.qos().durability_service.service_cleanup_delay == zero);
  EXPECT_EQ(uut.qos().durability_service.history_kind, DDS::KEEP_LAST_HISTORY_QOS);
  EXPECT_EQ(uut.qos().durability_service.history_depth, 1);
  EXPECT_EQ(uut.qos().durability_service.max_samples, DDS::LENGTH_UNLIMITED);
  EXPECT_EQ(uut.qos().durability_service.max_instances, DDS::LENGTH_UNLIMITED);
  EXPECT_EQ(uut.qos().durability_service.max_samples_per_instance, DDS::LENGTH_UNLIMITED);
  EXPECT_TRUE(uut.qos().deadline.period == infinite);
  EXPECT_TRUE(uut.qos().latency_budget.duration == zero);
  EXPECT_EQ(uut.qos().liveliness.kind, DDS::AUTOMATIC_LIVELINESS_QOS);
  EXPECT_TRUE(uut.qos().liveliness.lease_duration == infinite);
  EXPECT_EQ(uut.qos().reliability.kind, DDS::RELIABLE_RELIABILITY_QOS);
  EXPECT_TRUE(uut.qos().reliability.max_blocking_time == ms100);
  EXPECT_EQ(uut.qos().destination_order.kind, DDS::BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS);
  EXPECT_EQ(uut.qos().history.kind, DDS::KEEP_LAST_HISTORY_QOS);
  EXPECT_EQ(uut.qos().history.depth, 3);
  EXPECT_EQ(uut.qos().resource_limits.max_samples, DDS::LENGTH_UNLIMITED);
  EXPECT_EQ(uut.qos().resource_limits.max_instances, DDS::LENGTH_UNLIMITED);
  EXPECT_EQ(uut.qos().resource_limits.max_samples_per_instance, DDS::LENGTH_UNLIMITED);
  EXPECT_EQ(uut.qos().transport_priority.value, 0);
  EXPECT_TRUE(uut.qos().lifespan.duration == infinite);
  EXPECT_EQ(uut.qos().user_data.value.length(), 0u);
  EXPECT_EQ(uut.qos().ownership.kind, DDS::SHARED_OWNERSHIP_QOS);
  EXPECT_EQ(uut.qos().ownership_strength.value, 0);
  EXPECT_EQ(uut.qos().writer_data_lifecycle.autodispose_unregistered_instances, true);
}

TEST(dds_DCPS_Qos_Helper, DataWriterQosBuilder_durability_kind)
{
  OpenDDS::DCPS::DataWriterQosBuilder uut;
  uut.durability_kind(DDS::TRANSIENT_LOCAL_DURABILITY_QOS);
  EXPECT_EQ(uut.qos().durability.kind, DDS::TRANSIENT_LOCAL_DURABILITY_QOS);
}

TEST(dds_DCPS_Qos_Helper, DataWriterQosBuilder_durability_volatile)
{
  OpenDDS::DCPS::DataWriterQosBuilder uut;
  uut.durability_transient_local().durability_volatile();
  EXPECT_EQ(uut.qos().durability.kind, DDS::VOLATILE_DURABILITY_QOS);
}

TEST(dds_DCPS_Qos_Helper, DataWriterQosBuilder_durability_transient_local)
{
  OpenDDS::DCPS::DataWriterQosBuilder uut;
  uut.durability_transient_local();
  EXPECT_EQ(uut.qos().durability.kind, DDS::TRANSIENT_LOCAL_DURABILITY_QOS);
}

TEST(dds_DCPS_Qos_Helper, DataWriterQosBuilder_durability_transient)
{
  OpenDDS::DCPS::DataWriterQosBuilder uut;
  uut.durability_transient();
  EXPECT_EQ(uut.qos().durability.kind, DDS::TRANSIENT_DURABILITY_QOS);
}

TEST(dds_DCPS_Qos_Helper, DataWriterQosBuilder_durability_persistent)
{
  OpenDDS::DCPS::DataWriterQosBuilder uut;
  uut.durability_persistent();
  EXPECT_EQ(uut.qos().durability.kind, DDS::PERSISTENT_DURABILITY_QOS);
}

TEST(dds_DCPS_Qos_Helper, DataWriterQosBuilder_durability_service_service_cleanup_delay)
{
  using OpenDDS::DCPS::operator==;
  OpenDDS::DCPS::DataWriterQosBuilder uut;
  uut.durability_service_service_cleanup_delay(ms100);
  EXPECT_TRUE(uut.qos().durability_service.service_cleanup_delay == ms100);
}

TEST(dds_DCPS_Qos_Helper, DataWriterQosBuilder_durability_service_history_kind)
{
  OpenDDS::DCPS::DataWriterQosBuilder uut;
  uut.durability_service_history_kind(DDS::KEEP_ALL_HISTORY_QOS);
  EXPECT_EQ(uut.qos().durability_service.history_kind, DDS::KEEP_ALL_HISTORY_QOS);
}

TEST(dds_DCPS_Qos_Helper, DataWriterQosBuilder_durability_service_keep_last)
{
  OpenDDS::DCPS::DataWriterQosBuilder uut;
  uut.durability_service_keep_last(5);
  EXPECT_EQ(uut.qos().durability_service.history_kind, DDS::KEEP_LAST_HISTORY_QOS);
  EXPECT_EQ(uut.qos().durability_service.history_depth, 5);
}

TEST(dds_DCPS_Qos_Helper, DataWriterQosBuilder_durability_service_keep_all)
{
  OpenDDS::DCPS::DataWriterQosBuilder uut;
  uut.durability_service_keep_all();
  EXPECT_EQ(uut.qos().durability_service.history_kind, DDS::KEEP_ALL_HISTORY_QOS);
}

TEST(dds_DCPS_Qos_Helper, DataWriterQosBuilder_durability_service_history_depth)
{
  OpenDDS::DCPS::DataWriterQosBuilder uut;
  uut.durability_service_history_depth(5);
  EXPECT_EQ(uut.qos().durability_service.history_depth, 5);
}

TEST(dds_DCPS_Qos_Helper, DataWriterQosBuilder_durability_service_max_samples)
{
  OpenDDS::DCPS::DataWriterQosBuilder uut;
  uut.durability_service_max_samples(5);
  EXPECT_EQ(uut.qos().durability_service.max_samples, 5);
}

TEST(dds_DCPS_Qos_Helper, DataWriterQosBuilder_durability_service_max_instances)
{
  OpenDDS::DCPS::DataWriterQosBuilder uut;
  uut.durability_service_max_instances(5);
  EXPECT_EQ(uut.qos().durability_service.max_instances, 5);
}

TEST(dds_DCPS_Qos_Helper, DataWriterQosBuilder_durability_service_max_samples_per_instance)
{
  OpenDDS::DCPS::DataWriterQosBuilder uut;
  uut.durability_service_max_samples_per_instance(5);
  EXPECT_EQ(uut.qos().durability_service.max_samples_per_instance, 5);
}

TEST(dds_DCPS_Qos_Helper, DataWriterQosBuilder_deadline_period)
{
  using OpenDDS::DCPS::operator==;
  OpenDDS::DCPS::DataWriterQosBuilder uut;
  uut.deadline_period(ms100);
  EXPECT_TRUE(uut.qos().deadline.period == ms100);
}

TEST(dds_DCPS_Qos_Helper, DataWriterQosBuilder_latency_budget_duration)
{
  using OpenDDS::DCPS::operator==;
  OpenDDS::DCPS::DataWriterQosBuilder uut;
  uut.latency_budget_duration(ms100);
  EXPECT_TRUE(uut.qos().latency_budget.duration == ms100);
}

TEST(dds_DCPS_Qos_Helper, DataWriterQosBuilder_liveliness_kind)
{
  OpenDDS::DCPS::DataWriterQosBuilder uut;
  uut.liveliness_kind(DDS::MANUAL_BY_PARTICIPANT_LIVELINESS_QOS);
  EXPECT_EQ(uut.qos().liveliness.kind, DDS::MANUAL_BY_PARTICIPANT_LIVELINESS_QOS);
}

TEST(dds_DCPS_Qos_Helper, DataWriterQosBuilder_liveliness_automatic)
{
  OpenDDS::DCPS::DataWriterQosBuilder uut;
  uut.liveliness_manual_by_participant().liveliness_automatic();
  EXPECT_EQ(uut.qos().liveliness.kind, DDS::AUTOMATIC_LIVELINESS_QOS);
}

TEST(dds_DCPS_Qos_Helper, DataWriterQosBuilder_liveliness_manual_by_participant)
{
  OpenDDS::DCPS::DataWriterQosBuilder uut;
  uut.liveliness_manual_by_participant().liveliness_manual_by_participant();
  EXPECT_EQ(uut.qos().liveliness.kind, DDS::MANUAL_BY_PARTICIPANT_LIVELINESS_QOS);
}

TEST(dds_DCPS_Qos_Helper, DataWriterQosBuilder_liveliness_manual_by_topic)
{
  OpenDDS::DCPS::DataWriterQosBuilder uut;
  uut.liveliness_manual_by_topic().liveliness_manual_by_topic();
  EXPECT_EQ(uut.qos().liveliness.kind, DDS::MANUAL_BY_TOPIC_LIVELINESS_QOS);
}

TEST(dds_DCPS_Qos_Helper, DataWriterQosBuilder_liveliness_lease_duration)
{
  using OpenDDS::DCPS::operator==;
  OpenDDS::DCPS::DataWriterQosBuilder uut;
  uut.liveliness_lease_duration(ms100);
  EXPECT_TRUE(uut.qos().liveliness.lease_duration == ms100);
}

TEST(dds_DCPS_Qos_Helper, DataWriterQosBuilder_reliability_kind)
{
  OpenDDS::DCPS::DataWriterQosBuilder uut;
  uut.reliability_kind(DDS::BEST_EFFORT_RELIABILITY_QOS);
  EXPECT_EQ(uut.qos().reliability.kind, DDS::BEST_EFFORT_RELIABILITY_QOS);
}

TEST(dds_DCPS_Qos_Helper, DataWriterQosBuilder_reliability_best_effort)
{
  OpenDDS::DCPS::DataWriterQosBuilder uut;
  uut.reliability_best_effort();
  EXPECT_EQ(uut.qos().reliability.kind, DDS::BEST_EFFORT_RELIABILITY_QOS);
}

TEST(dds_DCPS_Qos_Helper, DataWriterQosBuilder_reliability_reliable)
{
  OpenDDS::DCPS::DataWriterQosBuilder uut;
  uut.reliability_best_effort().reliability_reliable();
  EXPECT_EQ(uut.qos().reliability.kind, DDS::RELIABLE_RELIABILITY_QOS);
}

TEST(dds_DCPS_Qos_Helper, DataWriterQosBuilder_reliability_max_blocking_time)
{
  using OpenDDS::DCPS::operator==;
  OpenDDS::DCPS::DataWriterQosBuilder uut;
  uut.reliability_max_blocking_time(zero);
  EXPECT_TRUE(uut.qos().reliability.max_blocking_time == zero);
}

TEST(dds_DCPS_Qos_Helper, DataWriterQosBuilder_destination_order_kind)
{
  OpenDDS::DCPS::DataWriterQosBuilder uut;
  uut.destination_order_kind(DDS::BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS);
  EXPECT_EQ(uut.qos().destination_order.kind, DDS::BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS);
}

TEST(dds_DCPS_Qos_Helper, DataWriterQosBuilder_destination_order_by_source_timestamp)
{
  OpenDDS::DCPS::DataWriterQosBuilder uut;
  uut.destination_order_by_source_timestamp();
  EXPECT_EQ(uut.qos().destination_order.kind, DDS::BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS);
}

TEST(dds_DCPS_Qos_Helper, DataWriterQosBuilder_destination_order_by_reception_timestamp)
{
  OpenDDS::DCPS::DataWriterQosBuilder uut;
  uut.destination_order_by_source_timestamp().destination_order_by_reception_timestamp();
  EXPECT_EQ(uut.qos().destination_order.kind, DDS::BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS);
}

TEST(dds_DCPS_Qos_Helper, DataWriterQosBuilder_history_kind)
{
  OpenDDS::DCPS::DataWriterQosBuilder uut;
  uut.history_kind(DDS::KEEP_ALL_HISTORY_QOS);
  EXPECT_EQ(uut.qos().history.kind, DDS::KEEP_ALL_HISTORY_QOS);
}

TEST(dds_DCPS_Qos_Helper, DataWriterQosBuilder_history_keep_last)
{
  OpenDDS::DCPS::DataWriterQosBuilder uut;
  uut.history_keep_last(5);
  EXPECT_EQ(uut.qos().history.kind, DDS::KEEP_LAST_HISTORY_QOS);
  EXPECT_EQ(uut.qos().history.depth, 5);
}

TEST(dds_DCPS_Qos_Helper, DataWriterQosBuilder_history_keep_all)
{
  OpenDDS::DCPS::DataWriterQosBuilder uut;
  uut.history_keep_all();
  EXPECT_EQ(uut.qos().history.kind, DDS::KEEP_ALL_HISTORY_QOS);
}

TEST(dds_DCPS_Qos_Helper, DataWriterQosBuilder_history_depth)
{
  OpenDDS::DCPS::DataWriterQosBuilder uut;
  uut.history_depth(5);
  EXPECT_EQ(uut.qos().history.depth, 5);
}

TEST(dds_DCPS_Qos_Helper, DataWriterQosBuilder_resource_limits_max_samples)
{
  OpenDDS::DCPS::DataWriterQosBuilder uut;
  uut.resource_limits_max_samples(5);
  EXPECT_EQ(uut.qos().resource_limits.max_samples, 5);
}

TEST(dds_DCPS_Qos_Helper, DataWriterQosBuilder_resource_limits_max_instances)
{
  OpenDDS::DCPS::DataWriterQosBuilder uut;
  uut.resource_limits_max_instances(5);
  EXPECT_EQ(uut.qos().resource_limits.max_instances, 5);
}

TEST(dds_DCPS_Qos_Helper, DataWriterQosBuilder_resource_limits_max_samples_per_instance)
{
  OpenDDS::DCPS::DataWriterQosBuilder uut;
  uut.resource_limits_max_samples_per_instance(5);
  EXPECT_EQ(uut.qos().resource_limits.max_samples_per_instance, 5);
}

TEST(dds_DCPS_Qos_Helper, DataWriterQosBuilder_transport_priority_value)
{
  OpenDDS::DCPS::DataWriterQosBuilder uut;
  uut.transport_priority_value(5);
  EXPECT_EQ(uut.qos().transport_priority.value, 5);
}

TEST(dds_DCPS_Qos_Helper, DataWriterQosBuilder_lifespan_duration)
{
  using OpenDDS::DCPS::operator==;
  OpenDDS::DCPS::DataWriterQosBuilder uut;
  uut.lifespan_duration(ms100);
  EXPECT_TRUE(uut.qos().lifespan.duration == ms100);
}

TEST(dds_DCPS_Qos_Helper, DataWriterQosBuilder_user_data_value)
{
  using OpenDDS::DCPS::operator==;
  DDS::OctetSeq user_data;
  user_data.length(1);
  user_data[0] = 'A';
  OpenDDS::DCPS::DataWriterQosBuilder uut;
  uut.user_data_value(user_data);
  EXPECT_TRUE(uut.qos().user_data.value == user_data);
}

TEST(dds_DCPS_Qos_Helper, DataWriterQosBuilder_ownership_kind)
{
  OpenDDS::DCPS::DataWriterQosBuilder uut;
  uut.ownership_kind(DDS::EXCLUSIVE_OWNERSHIP_QOS);
  EXPECT_EQ(uut.qos().ownership.kind, DDS::EXCLUSIVE_OWNERSHIP_QOS);
}

TEST(dds_DCPS_Qos_Helper, DataWriterQosBuilder_ownership_shared)
{
  OpenDDS::DCPS::DataWriterQosBuilder uut;
  uut.ownership_shared();
  EXPECT_EQ(uut.qos().ownership.kind, DDS::SHARED_OWNERSHIP_QOS);
}

TEST(dds_DCPS_Qos_Helper, DataWriterQosBuilder_ownership_exclusive)
{
  OpenDDS::DCPS::DataWriterQosBuilder uut;
  uut.ownership_exclusive();
  EXPECT_EQ(uut.qos().ownership.kind, DDS::EXCLUSIVE_OWNERSHIP_QOS);
}

TEST(dds_DCPS_Qos_Helper, DataWriterQosBuilder_ownership_strength_value)
{
  OpenDDS::DCPS::DataWriterQosBuilder uut;
  uut.ownership_strength_value(5);
  EXPECT_EQ(uut.qos().ownership_strength.value, 5);
}


TEST(dds_DCPS_Qos_Helper, DataWriterQosBuilder_writer_data_lifecycle_autodispose_unregistered_instances)
{
  OpenDDS::DCPS::DataWriterQosBuilder uut;
  uut.writer_data_lifecycle_autodispose_unregistered_instances(false);
  EXPECT_EQ(uut.qos().writer_data_lifecycle.autodispose_unregistered_instances, false);
}

TEST(dds_DCPS_Qos_Helper, DataWriterQosBuilder_Publisher_ctor)
{
  OpenDDS::DCPS::DataWriterQosBuilder qos;
  qos.reliability_best_effort();
  OpenDDS::DCPS::RcHandle<OpenDDS::Test::MockPublisher> publisher = OpenDDS::DCPS::make_rch<OpenDDS::Test::MockPublisher>();
  EXPECT_CALL(*publisher, get_default_datawriter_qos(testing::_))
    .WillOnce(DoAll(testing::SetArgReferee<0>(qos.qos()), testing::Return(DDS::RETCODE_OK)));
  DDS::Publisher_var publisher_var = DDS::Publisher::_duplicate(publisher.in());
  OpenDDS::DCPS::DataWriterQosBuilder uut(publisher_var);
  EXPECT_EQ(qos, uut);
}

TEST(dds_DCPS_Qos_Helper, DataWriterQosBuilder_Publisher_ctor_error)
{
  OpenDDS::DCPS::LogRestore restore;
  OpenDDS::DCPS::log_level.set(OpenDDS::DCPS::LogLevel::Warning);

  OpenDDS::Test::MockLogger logger;
  EXPECT_CALL(logger, log(testing::_))
    .Times(1)
    .WillOnce(testing::Return(0));

  OpenDDS::DCPS::DataWriterQosBuilder qos;
  qos.reliability_best_effort();
  OpenDDS::DCPS::RcHandle<OpenDDS::Test::MockPublisher> publisher = OpenDDS::DCPS::make_rch<OpenDDS::Test::MockPublisher>();
  EXPECT_CALL(*publisher, get_default_datawriter_qos(testing::_))
    .WillOnce(testing::Return(DDS::RETCODE_ERROR));

  DDS::Publisher_var publisher_var = DDS::Publisher::_duplicate(publisher.in());
  OpenDDS::DCPS::DataWriterQosBuilder uut(publisher_var);
  EXPECT_NE(qos, uut);
}

namespace {
  void copy_from_topic(DDS::DataWriterQos& dwqos,
                       const DDS::TopicQos& tqos)
  {
    dwqos.durability = tqos.durability;
    dwqos.durability_service = tqos.durability_service;
    dwqos.deadline = tqos.deadline;
    dwqos.latency_budget = tqos.latency_budget;
    dwqos.liveliness = tqos.liveliness;
    dwqos.reliability = tqos.reliability;
    dwqos.destination_order = tqos.destination_order;
    dwqos.history = tqos.history;
    dwqos.resource_limits = tqos.resource_limits;
    dwqos.transport_priority = tqos.transport_priority;
    dwqos.lifespan = tqos.lifespan;
  }
}


TEST(dds_DCPS_Qos_Helper, DataWriterQosBuilder_Topic_ctor)
{
  OpenDDS::DCPS::TopicQosBuilder topic_qos;
  topic_qos.durability_transient_local();

  OpenDDS::DCPS::DataWriterQosBuilder datawriter_qos;
  datawriter_qos.writer_data_lifecycle_autodispose_unregistered_instances(false);

  OpenDDS::DCPS::RcHandle<OpenDDS::Test::MockTopic> topic = OpenDDS::DCPS::make_rch<OpenDDS::Test::MockTopic>();
  EXPECT_CALL(*topic, get_qos(testing::_))
    .WillOnce(DoAll(testing::SetArgReferee<0>(topic_qos.qos()), testing::Return(DDS::RETCODE_OK)));
  DDS::Topic_var topic_var = DDS::Topic::_duplicate(topic.in());

  OpenDDS::DCPS::RcHandle<OpenDDS::Test::MockPublisher> publisher = OpenDDS::DCPS::make_rch<OpenDDS::Test::MockPublisher>();
  EXPECT_CALL(*publisher, get_default_datawriter_qos(testing::_))
    .WillOnce(DoAll(testing::SetArgReferee<0>(datawriter_qos.qos()), testing::Return(DDS::RETCODE_OK)));
  EXPECT_CALL(*publisher, copy_from_topic_qos(testing::_, testing::_))
    .WillOnce(testing::DoAll(testing::Invoke(copy_from_topic), testing::Return(DDS::RETCODE_OK)));
  DDS::Publisher_var publisher_var = DDS::Publisher::_duplicate(publisher.in());

  datawriter_qos.durability_transient_local();
  datawriter_qos.qos().reliability = OpenDDS::DCPS::ReliabilityQosPolicyBuilder();

  OpenDDS::DCPS::DataWriterQosBuilder uut(topic_var, publisher_var);
  EXPECT_EQ(datawriter_qos, uut);
}

TEST(dds_DCPS_Qos_Helper, DataWriterQosBuilder_Topic_ctor_error)
{
  OpenDDS::DCPS::LogRestore restore;
  OpenDDS::DCPS::log_level.set(OpenDDS::DCPS::LogLevel::Warning);

  OpenDDS::Test::MockLogger logger;
  EXPECT_CALL(logger, log(testing::_))
    .Times(3)
    .WillRepeatedly(testing::Return(0));

  OpenDDS::DCPS::RcHandle<OpenDDS::Test::MockTopic> topic = OpenDDS::DCPS::make_rch<OpenDDS::Test::MockTopic>();
  EXPECT_CALL(*topic, get_qos(testing::_))
    .WillOnce(testing::Return(DDS::RETCODE_ERROR));
  DDS::Topic_var topic_var = DDS::Topic::_duplicate(topic.in());

  OpenDDS::DCPS::RcHandle<OpenDDS::Test::MockPublisher> publisher = OpenDDS::DCPS::make_rch<OpenDDS::Test::MockPublisher>();
  EXPECT_CALL(*publisher, get_default_datawriter_qos(testing::_))
    .WillOnce(testing::Return(DDS::RETCODE_ERROR));
  EXPECT_CALL(*publisher, copy_from_topic_qos(testing::_, testing::_))
    .WillOnce(testing::Return(DDS::RETCODE_ERROR));
  DDS::Publisher_var publisher_var = DDS::Publisher::_duplicate(publisher.in());

  OpenDDS::DCPS::DataWriterQosBuilder uut(topic_var, publisher_var);
}
