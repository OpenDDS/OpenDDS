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

// changed from member function template to class template
// to avoid VC++ v6 build problem.
/*
 * Template method to retrieve the repository id by instance handle
 * from builtin topic.
 */
template <class BIT_Reader, class BIT_Reader_var, class BIT_DataSeq>
class BIT_Helper_1 {
public:
  DDS::ReturnCode_t instance_handle_to_bit_data(
    DomainParticipantImpl*       dp,
    const char*                  bit_name,
    const DDS::InstanceHandle_t& handle,
    BIT_DataSeq&                 data)
  {
    DDS::Subscriber_var bit_subscriber = dp->get_builtin_subscriber();

    DDS::DataReader_var reader =
      bit_subscriber->lookup_datareader(bit_name);

    BIT_Reader_var bit_reader = BIT_Reader::_narrow(reader.in());

    ACE_Time_Value due = ACE_OS::gettimeofday() +
      ACE_Time_Value(TheServiceParticipant->bit_lookup_duration_msec() / 1000,
                     (TheServiceParticipant->bit_lookup_duration_msec() % 1000)*1000);

    DDS::ReturnCode_t ret = DDS::RETCODE_OK;

    // Look for the data from builtin topic datareader until we get results or
    // timeout.
    // This is to resolve the problem of lookup return nothing. This could happen
    // when the add_association is called before the builtin topic datareader got
    // the published data.
    while (1) {
      DDS::SampleInfoSeq the_info;
      BIT_DataSeq the_data;
      ret = bit_reader->read_instance(the_data,
                                      the_info,
                                      DDS::LENGTH_UNLIMITED, // zero-copy
                                      handle,
                                      DDS::ANY_SAMPLE_STATE,
                                      DDS::ANY_VIEW_STATE,
                                      DDS::ANY_INSTANCE_STATE);

      if (ret != DDS::RETCODE_OK && ret != DDS::RETCODE_NO_DATA) {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: BIT_Helper::instance_handle_to_repo_id, ")
                          ACE_TEXT(" read instance 0x%x returned error %d. \n"),
                          handle, ret),
                         ret);
      }

      if (ret == DDS::RETCODE_OK) {
        data.length(1);
        data[0] = the_data[0];
        return ret;
      }

      ACE_Time_Value now = ACE_OS::gettimeofday();

      if (now < due) {
        if (DCPS_debug_level >= 10)
          ACE_DEBUG((LM_DEBUG,
                     ACE_TEXT("(%P|%t) BIT_Helper::instance_handle_to_repo_id, ")
                     ACE_TEXT(" BIT reader read_instance failed - trying again. \n")));

        ACE_Time_Value tv = due - now;

        if (tv > ACE_Time_Value(0, 100000)) {
          tv = ACE_Time_Value(0, 100000);
        }

        ACE_OS::sleep(tv);

      } else {
        ACE_ERROR_RETURN((LM_ERROR,
                          ACE_TEXT("(%P|%t) ERROR: BIT_Helper::instance_handle_to_repo_id, ")
                          ACE_TEXT(" timeout. \n")),
                         DDS::RETCODE_ERROR);
        return DDS::RETCODE_TIMEOUT;
      }
    }
  }
};

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
