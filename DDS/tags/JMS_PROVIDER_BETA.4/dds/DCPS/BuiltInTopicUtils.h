// -*- C++ -*-
// ============================================================================
/**
 *  @file   BuiltInTopicUtils.h
 *
 *  $Id$
 *
 *
 */
// ============================================================================

#ifndef BUILTINTOPICUTILS_H
#define BUILTINTOPICUTILS_H

#include "dcps_export.h"
#include "dds/DdsDcpsInfrastructureC.h"
#include "dds/DdsDcpsInfoUtilsC.h"
#include "dds/DdsDcpsSubscriptionC.h"
#include "Service_Participant.h"
#include "RepoIdBuilder.h"
#include "RepoIdConverter.h"
#include "dds/DCPS/DomainParticipantImpl.h"

#include <sstream>

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

    enum BuiltInTopicTransportTypeId
    {
      BIT_SIMPLE_TCP = 0xb17b17
    };

    enum BuiltInTopicTransportInstanceId
    {
      BIT_ALL_TRAFFIC = 0xb17b17
    };

    class DomainParticipantImpl;

    /**
     * Functor for ordering BuiltinKey_t.
     *
     * Use this like this:
     *   std::map< ::DDS::BuiltinTopicKey_t, int, OpenDDS::DCPS::BuiltinTopicKeyLess> MapType;
     */
    class BuiltinTopicKeyLess {
      public:
        bool operator()(
               const ::DDS::BuiltinTopicKey_t& lhs,
               const ::DDS::BuiltinTopicKey_t& rhs
             );
    };

    // changed from member function template to class template
    // to avoid VC++ v6 build problem.
    /*
     * Template method to retrieve the repository id by instance handle
     * from builtin topic.
     */
    template <class BIT_Reader, class BIT_Reader_var, class BIT_DataSeq>
    class BIT_Helper_1
    {
      public:
       ::DDS::ReturnCode_t instance_handle_to_repo_key (
           DomainParticipantImpl*         dp,
           const char*                    bit_name,
           const ::DDS::InstanceHandle_t& handle,
           ::OpenDDS::DCPS::RepoId&       repoId)
       {
         BIT_DataSeq data;
         ::DDS::ReturnCode_t ret
           = instance_handle_to_bit_data (dp, bit_name, handle, data);

         if (ret != ::DDS::RETCODE_OK)
           {
             ACE_ERROR_RETURN((LM_ERROR,
                 ACE_TEXT("(%P|%t) ERROR: BIT_Helper::instance_handle_to_repo_key: ")
                 ACE_TEXT("failed to find builtin topic data for instance 0x%x, ")
                 ACE_TEXT("error %d.\n"),
                 handle,
                 ret
             ), ret);
           }

         ::OpenDDS::DCPS::RepoIdBuilder builder(repoId);
         builder.from_BuiltinTopicKey(data[0].key);
         
         return ::DDS::RETCODE_OK;
       }

        ::DDS::ReturnCode_t instance_handle_to_bit_data (
            DomainParticipantImpl*   dp,
            const char*              bit_name,
            const ::DDS::InstanceHandle_t& handle,
            BIT_DataSeq&                   data)
        {
          ::DDS::Subscriber_var bit_subscriber
            = dp->get_builtin_subscriber () ;

          ::DDS::DataReader_var reader
            = bit_subscriber->lookup_datareader (bit_name) ;

          BIT_Reader_var bit_reader = BIT_Reader::_narrow (reader.in ());

          ACE_Time_Value due = ACE_OS::gettimeofday ()
            + ACE_Time_Value (TheServiceParticipant->bit_lookup_duration_msec () / 1000,
                             (TheServiceParticipant->bit_lookup_duration_msec () % 1000)*1000);

          DDS::ReturnCode_t ret = ::DDS::RETCODE_OK;

          // Look for the data from builtin topic datareader until we get results or
          // timeout.
          // This is to resolve the problem of lookup return nothing. This could happen
          // when the add_association is called before the builtin topic datareader got
          // the published data.
          while (1)
            {
              ::DDS::SampleInfoSeq the_info;
              BIT_DataSeq the_data;
              ret = bit_reader->read(the_data,
                                     the_info,
                                     ::DDS::LENGTH_UNLIMITED, // zero-copy
                                     ::DDS::ANY_SAMPLE_STATE,
                                     ::DDS::ANY_VIEW_STATE,
                                     ::DDS::ANY_INSTANCE_STATE);

              if (ret != ::DDS::RETCODE_OK && ret != ::DDS::RETCODE_NO_DATA)
                {
                  ACE_ERROR_RETURN ((LM_ERROR,
                                    ACE_TEXT("(%P|%t) ERROR: BIT_Helper::instance_handle_to_repo_id, ")
                                    ACE_TEXT(" read instance 0x%x returned error %d. \n"),
                                    handle, ret),
                                    ret);
                }
                
              // This is a temporary hack to work around the entity/data
              // instance handle mismatch when data handles are passed
              // to ignore_*. see: docs/design/instance-handles.txt
              for (CORBA::ULong i = 0; i < the_data.length(); ++i)
              {
                if (the_info[i].instance_handle == handle ||
                    the_data[i].key[2] == handle)
                {
                  data.length(1);
                  data[0] = the_data[i];
                  
                  return ::DDS::RETCODE_OK;
                }
              }

              ACE_Time_Value now = ACE_OS::gettimeofday ();
              if (now < due)
                {
                  if (DCPS_debug_level >= 10)
                    ACE_DEBUG((LM_DEBUG,
                         ACE_TEXT("(%P|%t) BIT_Helper::instance_handle_to_repo_id, ")
                         ACE_TEXT(" BIT reader read_instance failed - trying again. \n")));

                  ACE_Time_Value tv = due - now;
                  if (tv > ACE_Time_Value (0, 100000))
                    {
                      tv = ACE_Time_Value (0, 100000);
                    }
                  ACE_OS::sleep (tv);
                }
              else
                {
                  ACE_ERROR_RETURN ((LM_ERROR,
                            ACE_TEXT("(%P|%t) ERROR: BIT_Helper::instance_handle_to_repo_id, ")
                            ACE_TEXT(" timeout. \n")),
                            ::DDS::RETCODE_ERROR);
                  return ::DDS::RETCODE_TIMEOUT;
                }
            }
       }
    };


    template <class BIT_Reader, class BIT_Reader_var, class BIT_DataSeq, class IdSeq>
    class BIT_Helper_2
    {
      public:
        ::DDS::ReturnCode_t repo_ids_to_instance_handles (
            const IdSeq&                    repoids,
            ::DDS::InstanceHandleSeq&       handles)
        {
          ::CORBA::ULong repoids_len = repoids.length();
          handles.length(repoids_len);

          for (::CORBA::ULong i = 0; i < repoids_len; ++i)
          {
            ::OpenDDS::DCPS::RepoIdConverter converter(repoids[i]);
            handles[i] = ::DDS::InstanceHandle_t(converter);
          }
          return ::DDS::RETCODE_OK;
        }
    };

    inline
    bool
    BuiltinTopicKeyLess::operator()(
      const ::DDS::BuiltinTopicKey_t& lhs,
      const ::DDS::BuiltinTopicKey_t& rhs
    ) {
      // N.B.  This assumes that the MS index is 2 and the LS index is 0.
      return (lhs[2] < rhs[2])? true:
             (lhs[2] > rhs[2])? false:
             (lhs[1] < rhs[1])? true:
             (lhs[1] > rhs[1])? false:
             (lhs[0] < rhs[0])? true:
                                false;

    }

  } // End of namespace DCPS
} // End of namespace OpenDDS

#endif /* BUILTINTOPICUTILS_H  */
