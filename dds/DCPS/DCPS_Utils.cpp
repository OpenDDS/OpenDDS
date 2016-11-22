/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h"
#include "dds/DCPS/DCPS_Utils.h"
#include "dds/DCPS/Qos_Helper.h"
#include "dds/DCPS/Definitions.h"

#include "ace/ACE.h"  /* For ACE::wild_match() */
#include "ace/OS_NS_string.h"

#include <cstring>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

bool
is_wildcard(const char *str)
{
  static const char wild[] = "?*[";

  while (*str) {
    size_t i = ACE_OS::strcspn(str, wild);

    if (!str[i]) return false; // no wildcard

    if (i > 0 && str[i-1] == '\\') str += i + 1; // escaped wildcard

    else return true;
  }

  return false;
}

class PartitionName {
public:
  PartitionName(const char* name)
    : name_(name),
      wildcard_(is_wildcard(name)) {}

  bool matches(const PartitionName& n) {
    if (wildcard_ && n.wildcard_)
      return false; // wildcards never match

    if (wildcard_)
      return ACE::wild_match(n.name_, name_, true, true);

    else if (n.wildcard_)
      return ACE::wild_match(name_, n.name_, true, true);

    else
      return ACE_OS::strcmp(name_, n.name_) == 0;
  }

private:
  const char* name_;
  bool wildcard_;
};

bool
matches_name(const DDS::PartitionQosPolicy& qos, const PartitionName& name)
{
  for (CORBA::ULong i = 0; i < qos.name.length(); ++i) {
    PartitionName qos_name(qos.name[i]);

    if (qos_name.matches(name))
      return true;
  }

  return false;
}

bool
matches_default(const DDS::PartitionQosPolicy& qos)
{
  if (qos.name.length() == 0)
    return true; // default

  for (CORBA::ULong i = 0; i < qos.name.length(); ++i) {
    if (*qos.name[i] == 0)
      return true; // default (empty string)
  }

  return false;
}

bool
matching_partitions(const DDS::PartitionQosPolicy& pub,
                    const DDS::PartitionQosPolicy& sub)
{
  if (matches_default(pub)) {
    if (matches_default(sub))
      return true;

    // Zero-length sequences should be treated the same as a
    // sequence of length 1 that contains an empty string:
    if (pub.name.length() == 0)
      return matches_name(sub, "");
  }

  for (CORBA::ULong i = 0; i < pub.name.length(); ++i) {
    const char* name = pub.name[i];

    if (matches_name(sub, name))
      return true;
  }

  return false;
}

void
increment_incompatibility_count(OpenDDS::DCPS::IncompatibleQosStatus* status,
                                DDS::QosPolicyId_t incompatible_policy)
{
  ++status->total_count;
  ++status->count_since_last_send;
  status->last_policy_id = incompatible_policy;
  CORBA::ULong const size = status->policies.length();
  CORBA::ULong count = 0;
  bool updated = false;

  for (; !updated && count < size; ++count) {
    if (status->policies[count].policy_id == incompatible_policy) {
      ++status->policies[count].count;
      updated = true;
    }
  }

  if (!updated) {
    DDS::QosPolicyCount policy;
    policy.policy_id = incompatible_policy;
    policy.count = 1;
    status->policies.length(count + 1);
    status->policies[count] = policy;
  }
}

bool
compatibleTransports(const OpenDDS::DCPS::TransportLocatorSeq& s1,
                     const OpenDDS::DCPS::TransportLocatorSeq& s2)
{
  for (CORBA::ULong i = 0; i < s1.length(); ++i) {
    for (CORBA::ULong j = 0; j < s2.length(); ++j) {
      if (0 == std::strcmp(s1[i].transport_type, s2[j].transport_type)) {
        return true;
      }
    }
  }
  return false;
}

bool
compatibleQOS(OpenDDS::DCPS::IncompatibleQosStatus* writerStatus,
              OpenDDS::DCPS::IncompatibleQosStatus* readerStatus,
              const OpenDDS::DCPS::TransportLocatorSeq& pubTLS,
              const OpenDDS::DCPS::TransportLocatorSeq& subTLS,
              DDS::DataWriterQos const * const writerQos,
              DDS::DataReaderQos const * const readerQos,
              DDS::PublisherQos const * const pubQos,
              DDS::SubscriberQos const * const subQos)
{
  bool compatible = true;

  // Check transport-type compatibility
  if (!compatibleTransports(pubTLS, subTLS)) {
    compatible = false;
    increment_incompatibility_count(writerStatus,
                                    OpenDDS::TRANSPORTTYPE_QOS_POLICY_ID);
    increment_incompatibility_count(readerStatus,
                                    OpenDDS::TRANSPORTTYPE_QOS_POLICY_ID);
  }

  // Verify compatibility of DataWriterQos and DataReaderQos
  compatible = compatible && compatibleQOS(writerQos, readerQos,
                                           writerStatus, readerStatus);

  // Verify compatibility of PublisherQos and SubscriberQos
  compatible = compatible && compatibleQOS(pubQos, subQos,
                                           writerStatus, readerStatus);

  // Verify publisher and subscriber are in a matching partition.
  //
  // According to the DDS spec:
  //
  //   Failure to match partitions is not considered an incompatible
  //   QoS and does not trigger any listeners nor conditions.
  //
  // Don't increment the incompatibity count.
  compatible = compatible && matching_partitions(pubQos->partition,
                                                 subQos->partition);

  return compatible;
}

bool
compatibleQOS(const DDS::PublisherQos*  pubQos,
              const DDS::SubscriberQos* subQos,
              OpenDDS::DCPS::IncompatibleQosStatus* writerStatus,
              OpenDDS::DCPS::IncompatibleQosStatus* readerStatus)
{
  bool compatible = true;

  // PARTITION, GROUP_DATA, and ENTITY_FACTORY are RxO==no.

  // Check the PRESENTATION_QOS_POLICY_ID
  if ((pubQos->presentation.access_scope < subQos->presentation.access_scope)
      || ((pubQos->presentation.coherent_access == false)
          &&(subQos->presentation.coherent_access == true))
      || ((pubQos->presentation.ordered_access  == false)
          &&(subQos->presentation.ordered_access  == true))) {
    compatible = false;

    increment_incompatibility_count(writerStatus,
                                    DDS::PRESENTATION_QOS_POLICY_ID);
    increment_incompatibility_count(readerStatus,
                                    DDS::PRESENTATION_QOS_POLICY_ID);
  }

  return compatible;
}

bool
compatibleQOS(const DDS::DataWriterQos * writerQos,
              const DDS::DataReaderQos * readerQos,
              OpenDDS::DCPS::IncompatibleQosStatus* writerStatus,
              OpenDDS::DCPS::IncompatibleQosStatus* readerStatus)
{
  bool compatible = true;

  // Check the RELIABILITY_QOS_POLICY_ID
  if (writerQos->reliability.kind < readerQos->reliability.kind) {
    compatible = false;

    increment_incompatibility_count(writerStatus,
                                    DDS::RELIABILITY_QOS_POLICY_ID);
    increment_incompatibility_count(readerStatus,
                                    DDS::RELIABILITY_QOS_POLICY_ID);
  }

  // Check the DURABILITY_QOS_POLICY_ID
  if (writerQos->durability.kind < readerQos->durability.kind) {
    compatible = false;

    increment_incompatibility_count(writerStatus,
                                    DDS::DURABILITY_QOS_POLICY_ID);
    increment_incompatibility_count(readerStatus,
                                    DDS::DURABILITY_QOS_POLICY_ID);
  }

  // Check the LIVELINESS_QOS_POLICY_ID
  // invalid if offered kind is less than requested kind OR
  //         if offered liveliness duration greater than requested
  //         liveliness duration
  using OpenDDS::DCPS::operator>;
  if (writerQos->liveliness.kind < readerQos->liveliness.kind
      || writerQos->liveliness.lease_duration
      > readerQos->liveliness.lease_duration) {

    compatible = false;

    increment_incompatibility_count(writerStatus,
                                    DDS::LIVELINESS_QOS_POLICY_ID);
    increment_incompatibility_count(readerStatus,
                                    DDS::LIVELINESS_QOS_POLICY_ID);
  }

  // Check the DEADLINE_QOS_POLICY_ID
  //   Offered deadline must be less than or equal to the requested
  //   deadline.
  if (writerQos->deadline.period > readerQos->deadline.period) {

    compatible = false;

    increment_incompatibility_count(writerStatus,
                                    DDS::DEADLINE_QOS_POLICY_ID);
    increment_incompatibility_count(readerStatus,
                                    DDS::DEADLINE_QOS_POLICY_ID);
  }

  // Check the LATENCY_BUDGET
  //   The reader's duration must be greater than or equal to the writer's
  using OpenDDS::DCPS::operator<;
  if (readerQos->latency_budget.duration < writerQos->latency_budget.duration) {

    compatible = false;

    increment_incompatibility_count(writerStatus,
                                    DDS::LATENCYBUDGET_QOS_POLICY_ID);
    increment_incompatibility_count(readerStatus,
                                    DDS::LATENCYBUDGET_QOS_POLICY_ID);
  }

  // The value of the OWNERSHIP kind offered must exactly match the one
  // requested or else they are considered incompatible.
  if (writerQos->ownership.kind != readerQos->ownership.kind) {
    compatible = false;

    increment_incompatibility_count(writerStatus,
                                    DDS::OWNERSHIP_QOS_POLICY_ID);
    increment_incompatibility_count(readerStatus,
                                    DDS::OWNERSHIP_QOS_POLICY_ID);
  }

  return compatible;
}

#ifndef OPENDDS_SAFETY_PROFILE
using OpenDDS::DCPS::operator==;
#endif
bool should_check_association_upon_change(const DDS::DataReaderQos & qos1,
                                          const DDS::DataReaderQos & qos2)
{
  return !(
           (qos1.deadline == qos2.deadline) &&
           (qos1.latency_budget == qos2.latency_budget));
}

bool should_check_association_upon_change(const DDS::DataWriterQos & qos1,
                                          const DDS::DataWriterQos & qos2)
{
  return !(
           (qos1.deadline == qos2.deadline) &&
           (qos1.latency_budget == qos2.latency_budget));
}

bool should_check_association_upon_change(const DDS::SubscriberQos & qos1,
                                          const DDS::SubscriberQos & qos2)
{
  return !(qos1.partition == qos2.partition);
}

bool should_check_association_upon_change(const DDS::PublisherQos & qos1,
                                          const DDS::PublisherQos & qos2)
{
  return !(qos1.partition == qos2.partition);
}

bool should_check_association_upon_change(const DDS::TopicQos & /*qos1*/,
                                          const DDS::TopicQos & /*qos2*/)
{
  return false;
}

bool should_check_association_upon_change(const DDS::DomainParticipantQos & /*qos1*/,
                                          const DDS::DomainParticipantQos & /*qos2*/)
{
  return false;
}

}}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
