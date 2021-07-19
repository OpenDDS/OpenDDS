/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h"
#include "DCPS_Utils.h"

#include "Qos_Helper.h"
#include "Definitions.h"

#include <ace/ACE.h> /* For ACE::wild_match() */
#include <ace/OS_NS_string.h>

#ifdef OPENDDS_SECURITY
#  include "dds/DdsSecurityCoreC.h"
#endif

#include <cstring>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

const char* retcode_to_string(DDS::ReturnCode_t value)
{
  switch (value) {
  case DDS::RETCODE_OK:
    return "OK";
  case DDS::RETCODE_ERROR:
    return "Error";
  case DDS::RETCODE_UNSUPPORTED:
    return "Unsupported";
  case DDS::RETCODE_BAD_PARAMETER:
    return "Bad parameter";
  case DDS::RETCODE_PRECONDITION_NOT_MET:
    return "Precondition not met";
  case DDS::RETCODE_OUT_OF_RESOURCES:
    return "Out of resources";
  case DDS::RETCODE_NOT_ENABLED:
    return "Not enabled";
  case DDS::RETCODE_IMMUTABLE_POLICY:
    return "Immutable policy";
  case DDS::RETCODE_INCONSISTENT_POLICY:
    return "Inconsistent policy";
  case DDS::RETCODE_ALREADY_DELETED:
    return "Already deleted";
  case DDS::RETCODE_TIMEOUT:
    return "Timeout";
  case DDS::RETCODE_NO_DATA:
    return "No data";
  case DDS::RETCODE_ILLEGAL_OPERATION:
    return "Illegal operation";
#ifdef OPENDDS_SECURITY
  case DDS::Security::RETCODE_NOT_ALLOWED_BY_SECURITY:
    return "Not allowed by security";
#endif
  default:
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: retcode_to_string: ")
      ACE_TEXT("%d is either invalid or not recognized.\n"),
      value));
    return "Invalid return code";
  }
}

const char* topicstatus_to_string(TopicStatus value)
{
  switch (value) {
  case CREATED:
    return "Created";
  case ENABLED:
    return "Enabled";
  case FOUND:
    return "Found";
  case NOT_FOUND:
    return "Not found";
  case REMOVED:
    return "Removed";
  case CONFLICTING_TYPENAME:
    return "Conflicting typename";
  case PRECONDITION_NOT_MET:
    return "Precondition not met";
  case INTERNAL_ERROR:
    return "Internal error";
  case TOPIC_DISABLED:
    return "Topic disabled";
  default:
    ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: topicstatus_to_string: ")
      ACE_TEXT("%d is either invalid or not recognized.\n"),
      value));
    return "Invalid topic status";
  }
}

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
  if (!status) return;

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
      || (!pubQos->presentation.coherent_access && subQos->presentation.coherent_access)
      || (!pubQos->presentation.ordered_access && subQos->presentation.ordered_access)) {
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

  {
    // Find a common data representation
    bool found = false;
    DDS::DataRepresentationIdSeq readerIds =
      get_effective_data_rep_qos(readerQos->representation.value, true);
    DDS::DataRepresentationIdSeq writerIds =
      get_effective_data_rep_qos(writerQos->representation.value, false);
    const CORBA::ULong reader_count = readerIds.length();
    const CORBA::ULong writer_count = writerIds.length();
    for (CORBA::ULong wi = 0; !found && wi < writer_count; ++wi) {
      for (CORBA::ULong ri = 0; !found && ri < reader_count; ++ri) {
        if (readerIds[ri] == writerIds[wi]) {
          found = true;
          break;
        }
      }
    }

    if (!found) {
      increment_incompatibility_count(writerStatus,
        DDS::DATA_REPRESENTATION_QOS_POLICY_ID);
      increment_incompatibility_count(readerStatus,
        DDS::DATA_REPRESENTATION_QOS_POLICY_ID);
      compatible = false;
    }
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

bool repr_to_encoding_kind(DDS::DataRepresentationId_t repr, Encoding::Kind& kind)
{
  switch(repr) {
  case DDS::XCDR_DATA_REPRESENTATION:
    kind = Encoding::KIND_XCDR1;
    break;
  case DDS::XCDR2_DATA_REPRESENTATION:
    kind = Encoding::KIND_XCDR2;
    break;
  default:
    return false;
  }
  return true;
}

DDS::DataRepresentationIdSeq get_effective_data_rep_qos(const DDS::DataRepresentationIdSeq& qos, bool reader)
{
  if (qos.length() == 0) {
    DDS::DataRepresentationIdSeq ids;
    ids.length(reader ? 2 : 1);
    ids[0] = DDS::XCDR2_DATA_REPRESENTATION;
    if (reader) {
      ids[1] = DDS::XCDR_DATA_REPRESENTATION;
    }
    return ids;
  }
  return qos;
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
