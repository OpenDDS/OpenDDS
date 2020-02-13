/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef BUILTINTOPICUTILS_H
#define BUILTINTOPICUTILS_H

#include "dcps_export.h"
#include "dds/DdsDcpsInfrastructureC.h"
#include "dds/DdsDcpsInfoUtilsC.h"
#include "dds/DdsDcpsSubscriptionC.h"
#include "dds/DdsDcpsCoreC.h"
#include "Service_Participant.h"
#include "DomainParticipantImpl.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

OpenDDS_Dcps_Export extern const char* const BUILT_IN_PARTICIPANT_TOPIC;
OpenDDS_Dcps_Export extern const char* const BUILT_IN_PARTICIPANT_TOPIC_TYPE;

OpenDDS_Dcps_Export extern const char* const BUILT_IN_TOPIC_TOPIC;
OpenDDS_Dcps_Export extern const char* const BUILT_IN_TOPIC_TOPIC_TYPE;

OpenDDS_Dcps_Export extern const char* const BUILT_IN_SUBSCRIPTION_TOPIC;
OpenDDS_Dcps_Export extern const char* const BUILT_IN_SUBSCRIPTION_TOPIC_TYPE;

OpenDDS_Dcps_Export extern const char* const BUILT_IN_PUBLICATION_TOPIC;
OpenDDS_Dcps_Export extern const char* const BUILT_IN_PUBLICATION_TOPIC_TYPE;

OpenDDS_Dcps_Export extern const char* const BUILT_IN_PARTICIPANT_LOCATION_TOPIC;
OpenDDS_Dcps_Export extern const char* const BUILT_IN_PARTICIPANT_LOCATION_TOPIC_TYPE;

/**
 * Returns true if the topic name and type pair matches one of the built-in
 * topic name and type pairs.
 */
inline bool
topicIsBIT(const char* name, const char* type)
{
  return (
    !ACE_OS::strcmp(name, BUILT_IN_PARTICIPANT_TOPIC) &&
    !ACE_OS::strcmp(type, BUILT_IN_PARTICIPANT_TOPIC_TYPE)
  ) || (
    !ACE_OS::strcmp(name, BUILT_IN_TOPIC_TOPIC) &&
    !ACE_OS::strcmp(type, BUILT_IN_TOPIC_TOPIC_TYPE)
  ) || (
    !ACE_OS::strcmp(name, BUILT_IN_SUBSCRIPTION_TOPIC) &&
    !ACE_OS::strcmp(type, BUILT_IN_SUBSCRIPTION_TOPIC_TYPE)
  ) || (
    !ACE_OS::strcmp(name, BUILT_IN_PUBLICATION_TOPIC) &&
    !ACE_OS::strcmp(type, BUILT_IN_PUBLICATION_TOPIC_TYPE)
  ) || (
    !ACE_OS::strcmp(name, BUILT_IN_PARTICIPANT_LOCATION_TOPIC) &&
    !ACE_OS::strcmp(type, BUILT_IN_PARTICIPANT_LOCATION_TOPIC_TYPE)
  );
}

class DomainParticipantImpl;

/**
 * Functor for ordering BuiltinKey_t.
 *
 * Use this like this:
 *   std::map<DDS::BuiltinTopicKey_t, int, OpenDDS::DCPS::BuiltinTopicKeyLess> MapType;
 */
class BuiltinTopicKeyLess {
public:
  bool operator()(
    const DDS::BuiltinTopicKey_t& lhs,
    const DDS::BuiltinTopicKey_t& rhs) const;
};

template<typename TopicType>
DDS::BuiltinTopicKey_t keyFromSample(TopicType* sample);

#if !defined (DDS_HAS_MINIMUM_BIT)

template<class BIT_Reader_var, class BIT_DataSeq>
DDS::ReturnCode_t instance_handle_to_bit_data(
  DomainParticipantImpl* dp,
  const char* bit_name,
  const DDS::InstanceHandle_t& handle,
  BIT_DataSeq& data)
{
  DDS::Subscriber_var bit_subscriber = dp->get_builtin_subscriber();

  DDS::DataReader_var reader = bit_subscriber->lookup_datareader(bit_name);

  typedef typename BIT_Reader_var::_obj_type BIT_Reader;
  BIT_Reader_var bit_reader = BIT_Reader::_narrow(reader.in());

  const MonotonicTimePoint due(MonotonicTimePoint::now() +
    TimeDuration::from_msec(TheServiceParticipant->bit_lookup_duration_msec()));

    // Look for the data from builtin topic datareader until we get results or
    // timeout.
    // This is to resolve the problem of lookup return nothing. This could happen
    // when the add_association is called before the builtin topic datareader got
    // the published data.
  while (true) {
    DDS::SampleInfoSeq the_info;
    BIT_DataSeq the_data;
    const DDS::ReturnCode_t ret =
      bit_reader->read_instance(the_data,
                                the_info,
                                DDS::LENGTH_UNLIMITED,
                                handle,
                                DDS::ANY_SAMPLE_STATE,
                                DDS::ANY_VIEW_STATE,
                                DDS::ANY_INSTANCE_STATE);

    if (ret == DDS::RETCODE_OK) {
      data.length(1);
      data[0] = the_data[0];
      return ret;
    }

    if (ret != DDS::RETCODE_BAD_PARAMETER && ret != DDS::RETCODE_NO_DATA) {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) ERROR: instance_handle_to_repo_id, ")
                        ACE_TEXT("read instance 0x%x returned error %d.\n"),
                        handle, ret),
                       ret);
    }

    const MonotonicTimePoint now = MonotonicTimePoint::now();
    if (now < due) {
      if (DCPS_debug_level >= 10) {
        ACE_DEBUG((LM_DEBUG,
                   ACE_TEXT("(%P|%t) instance_handle_to_repo_id, ")
                   ACE_TEXT("BIT reader read_instance failed - trying again.\n")));
      }
      ACE_OS::sleep(std::min(due - now, TimeDuration(0, 100000)).value());

    } else {
      ACE_ERROR_RETURN((LM_ERROR,
                        ACE_TEXT("(%P|%t) ERROR: instance_handle_to_repo_id,")
                        ACE_TEXT(" timeout. \n")),
                       DDS::RETCODE_ERROR);
      return DDS::RETCODE_TIMEOUT;
    }
  }
}
#endif

inline
bool
BuiltinTopicKeyLess::operator()(const DDS::BuiltinTopicKey_t& lhs,
                                const DDS::BuiltinTopicKey_t& rhs) const
{
  // N.B.  This assumes that the MS index is 2 and the LS index is 0.
  return (lhs.value[2] < rhs.value[2])? true:
         (lhs.value[2] > rhs.value[2])? false:
         (lhs.value[1] < rhs.value[1])? true:
         (lhs.value[1] > rhs.value[1])? false:
         (lhs.value[0] < rhs.value[0])? true:
         false;

}

#if !defined (DDS_HAS_MINIMUM_BIT)

template<>
inline
DDS::BuiltinTopicKey_t
keyFromSample<DDS::ParticipantBuiltinTopicData>(
  DDS::ParticipantBuiltinTopicData* sample)
{
  return sample->key;
}

template<>
inline
DDS::BuiltinTopicKey_t
keyFromSample<DDS::TopicBuiltinTopicData>(
  DDS::TopicBuiltinTopicData* sample)
{
  return sample->key;
}

template<>
inline
DDS::BuiltinTopicKey_t
keyFromSample<DDS::SubscriptionBuiltinTopicData>(
  DDS::SubscriptionBuiltinTopicData* sample)
{
  return sample->key;
}

template<>
inline
DDS::BuiltinTopicKey_t
keyFromSample<DDS::PublicationBuiltinTopicData>(
  DDS::PublicationBuiltinTopicData* sample)
{
  return sample->key;
}

#endif

template<typename TopicType>
inline
DDS::BuiltinTopicKey_t keyFromSample(TopicType*)
{
  DDS::BuiltinTopicKey_t value;
  value.value[0] = value.value[1] = value.value[2] = 0;
  return value;
}

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* BUILTINTOPICUTILS_H  */
