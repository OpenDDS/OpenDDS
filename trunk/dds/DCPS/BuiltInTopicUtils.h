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

#include  "dcps_export.h"
#include  "dds/DdsDcpsInfrastructureC.h"
#include  "dds/DdsDcpsInfoUtilsC.h"
#include  "dds/DdsDcpsSubscriptionC.h"
#include  "Service_Participant.h"
#include  "DomainParticipantImpl.h"


namespace TAO {
  namespace DCPS {

    TAO_DdsDcps_Export extern const char* BUILT_IN_PARTICIPANT_TOPIC;
    TAO_DdsDcps_Export extern const char* BUILT_IN_PARTICIPANT_TOPIC_TYPE;

    TAO_DdsDcps_Export extern const char* BUILT_IN_TOPIC_TOPIC;
    TAO_DdsDcps_Export extern const char* BUILT_IN_TOPIC_TOPIC_TYPE;

    TAO_DdsDcps_Export extern const char* BUILT_IN_SUBSCRIPTION_TOPIC;
    TAO_DdsDcps_Export extern const char* BUILT_IN_SUBSCRIPTION_TOPIC_TYPE;

    TAO_DdsDcps_Export extern const char* BUILT_IN_PUBLICATION_TOPIC;
    TAO_DdsDcps_Export extern const char* BUILT_IN_PUBLICATION_TOPIC_TYPE;

    enum BuiltInTopicTransportTypeId
    {
      BIT_SIMPLE_TCP = 0xb17b17
    };

    enum BuiltInTopicTransportInstanceId
    {
      BIT_ALL_TRAFFIC = 0xb17b17
    };

    class DomainParticipantImpl;

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
        ::DDS::ReturnCode_t instance_handle_to_repo_id (
            DomainParticipantImpl*   dp,
            const char*              bit_name,
            const ::DDS::InstanceHandle_t& handle,
            RepoId&                  repoid)
        {
          // The index in BuiltinTopicKey_t[3] for entity templatized in this function.
          // If the data is read from the participant BIT datareader then the key position
          // is 1; otherwise it's 2.
          int key_pos = 2;

          if (ACE_OS::strcmp (bit_name, BUILT_IN_PARTICIPANT_TOPIC) == 0)
            { 
              key_pos = 1;
            }

          BIT_DataSeq data;
          ::DDS::ReturnCode_t ret 
            = instance_handle_to_bit_data (dp, bit_name, handle, data);

          if (ret != ::DDS::RETCODE_OK)
            {
              ACE_ERROR_RETURN((LM_ERROR,
                                ACE_TEXT("(%P|%t) BIT_Helper::instance_handle_to_repo_id, ")
                                ACE_TEXT("failed to find builtin topic data for instance ")
                                ACE_TEXT("handle %d error %d\n"), handle, ret),
                                ret);
            }


          repoid = data[0].key[key_pos];  
 
          if (repoid == 0)
            {
              ACE_ERROR_RETURN ((LM_ERROR,
                                 ACE_TEXT("(%P|%t) ERROR: BIT_Helper::instance_handle_to_repo_id, ")
                                 ACE_TEXT(" got invalid repo id. \n")),
                                 ::DDS::RETCODE_ERROR);
            }
          return ::DDS::RETCODE_OK;
        }


        ::DDS::ReturnCode_t instance_handle_to_bit_data (
            DomainParticipantImpl*   dp,
            const char*              bit_name,
            const ::DDS::InstanceHandle_t& handle,
            BIT_DataSeq&                   data)
        {
          ::DDS::Subscriber_var bit_subscriber 
            = dp->get_builtin_subscriber (ACE_ENV_SINGLE_ARG_PARAMETER) ;
          ACE_CHECK_RETURN (::DDS::RETCODE_ERROR);

          ::DDS::DataReader_var reader 
            = bit_subscriber->lookup_datareader (bit_name ACE_ENV_SINGLE_ARG_PARAMETER) ;
          ACE_CHECK_RETURN (::DDS::RETCODE_ERROR);

          BIT_Reader_var bit_reader = BIT_Reader::_narrow (reader.in ()
                                                           ACE_ENV_ARG_PARAMETER);
          ACE_CHECK_RETURN (::DDS::RETCODE_ERROR);
          
          ACE_Time_Value due = ACE_OS::gettimeofday ()
            + ACE_Time_Value (TheServiceParticipant->bit_lookup_duration_sec ());

          DDS::ReturnCode_t ret = ::DDS::RETCODE_OK;

          // Look for the data from builtin topic datareader until we get results or
          // timeout.
          // This is to resolve the problem of lookup return nothing. This could happen
          // when the add_association is called before the builtin topic datareader got
          // the published data.
          while (1)
            {
              ::DDS::SampleInfoSeq the_info(1);
              BIT_DataSeq the_data(1);
              ret = bit_reader->read_instance (the_data, 
                                               the_info, 
                                               1, 
                                               handle, 
                                               ::DDS::ANY_SAMPLE_STATE, 
                                               ::DDS::ANY_VIEW_STATE, 
                                               ::DDS::ANY_INSTANCE_STATE
                                               ACE_ENV_ARG_PARAMETER);
              ACE_CHECK_RETURN (::DDS::RETCODE_ERROR);

              if (ret != ::DDS::RETCODE_OK && ret != ::DDS::RETCODE_NO_DATA)
                {
                  ACE_ERROR_RETURN ((LM_ERROR,
                                    ACE_TEXT("(%P|%t) ERROR: BIT_Helper::instance_handle_to_repo_id, ")
                                    ACE_TEXT(" read instance %d returned error %d. \n"),
                                    handle, ret),
                                    ret);
                }
              else if (the_data.length () != 1)  
                {
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
                                ACE_TEXT(" timout. \n")),
                                ::DDS::RETCODE_ERROR);
                      return ::DDS::RETCODE_TIMEOUT;
                    }
                }
              else 
                {
                  data = the_data;
                  return ::DDS::RETCODE_OK; // success
                }
            }
        }
    
    };


    template <class BIT_Reader, class BIT_Reader_var, class BIT_DataSeq, class IdSeq>
    class BIT_Helper_2 
    {
      public:

        ::DDS::ReturnCode_t repo_ids_to_instance_handles (
            DomainParticipantImpl*          dp,
            const char*                     bit_name,
            const IdSeq&                    repoids,
            ::DDS::InstanceHandleSeq&       handles)
        {
          // we should never have more than 1000 BIT values.  Right? :)
          const CORBA::ULong max_samples = 1000;

          // The index in BuiltinTopicKey_t[3] for entity templatized in this function.
          // If the data is read from the participant BIT datareader then the key position
          // is 1; otherwise it's 2.
          int key_pos = 2;

          if (ACE_OS::strcmp (bit_name, BUILT_IN_PARTICIPANT_TOPIC) == 0)
            { 
              key_pos = 1;
            }

          ::DDS::Subscriber_ptr bit_subscriber 
            = dp->get_builtin_subscriber (ACE_ENV_SINGLE_ARG_PARAMETER) ;
          ACE_CHECK_RETURN (::DDS::RETCODE_ERROR);

          ::DDS::DataReader_var reader 
            = bit_subscriber->lookup_datareader (bit_name
                                                 ACE_ENV_SINGLE_ARG_PARAMETER) ;
          ACE_CHECK_RETURN (::DDS::RETCODE_ERROR);
          if (CORBA::is_nil (reader.in ()))
            {
              ACE_ERROR_RETURN ((LM_DEBUG,
                   "ERROR: repo_ids_to_instance_handles() %s not found.\n",
                   bit_name),
                    1);
            }

          BIT_Reader_var bit_reader = BIT_Reader::_narrow (reader.in ()
                                                           ACE_ENV_ARG_PARAMETER);
          ACE_CHECK_RETURN (::DDS::RETCODE_ERROR);
          if (CORBA::is_nil (bit_reader.in ()))
            {
              ACE_ERROR_RETURN ((LM_DEBUG,
                   "ERROR: repo_ids_to_instance_handles() %s narrow failed.\n",
                   bit_name),
                    1);
            }
          
          BIT_DataSeq data(max_samples);
          ::DDS::SampleInfoSeq infos(max_samples);

          ACE_Time_Value due = ACE_OS::gettimeofday ()
            + ACE_Time_Value (TheServiceParticipant->bit_lookup_duration_sec());

          DDS::ReturnCode_t ret = ::DDS::RETCODE_OK;

          // Look for the data from builtin topic datareader until we get results or
          // timeout.
          // This is to resolve the problem of lookup return nothing. This could happen
          // when the add_association is called before the builtin topic datareader got
          // the published data.

          while (1)
            {
              ret = bit_reader->read (data, 
                                      infos, 
                                      max_samples, //TBD: should be UNLIMITED
                                      ::DDS::ANY_SAMPLE_STATE,
                                      ::DDS::ANY_VIEW_STATE, 
                                      ::DDS::ANY_INSTANCE_STATE
                                      ACE_ENV_ARG_PARAMETER);
              ACE_CHECK_RETURN (::DDS::RETCODE_ERROR);

              if (ret != ::DDS::RETCODE_OK && ret != ::DDS::RETCODE_NO_DATA)
                {
                  ACE_ERROR_RETURN ((LM_ERROR,
                                    ACE_TEXT("(%P|%t) ERROR: BIT_Helper::repo_ids_to_instance_handles, ")
                                    ACE_TEXT(" read BIT data returned error %d. \n"),
                                    ret),
                                    ret);
                }

              CORBA::ULong repoid_len = repoids.length ();
              CORBA::ULong data_len = data.length ();
              handles.length (repoid_len);

              CORBA::ULong count = 0;

              for (CORBA::ULong i = 0; i < repoid_len; i++)
                {
                  for (CORBA::ULong j = 0; j < data_len; j++)
                    {    
                      if (DCPS_debug_level >= 10)
                        ACE_DEBUG((LM_DEBUG,"%s BIT has [%d, %d, %d]\n",
                          bit_name, data[j].key[0], data[j].key[1], 
                          data[j].key[2] ));
                      if (data[j].key[key_pos] == repoids[i]) 
                        {
                          handles[i] = infos[j].instance_handle;
                          count ++;
                          break;
                        }
                    }
                }
              
              if (count < repoid_len)
                {

                  ACE_Time_Value now = ACE_OS::gettimeofday ();
                  if (now < due)
                    {
                      if (DCPS_debug_level >= 10)
                        ACE_DEBUG((LM_DEBUG,
                            ACE_TEXT("(%P|%t) BIT_Helper::repo_ids_to_instance_handles, ")
                            ACE_TEXT("missing some instance handles - trying again\n")));

                      ACE_Time_Value tv = due - now;
                      if (tv > ACE_Time_Value (0, 100000))
                        {
                          tv = ACE_Time_Value (0, 100000);
                        }
                      ACE_OS::sleep (tv);
                    }
                  else
                    {
                       ACE_ERROR((LM_ERROR,
                            ACE_TEXT("(%P|%t) BIT_Helper::repo_ids_to_instance_handles, ")
                            ACE_TEXT("timed out\n")));
                       return ::DDS::RETCODE_TIMEOUT;
                    }
                }
              else
                {
                  return ::DDS::RETCODE_OK;
                }
            }
        }

    };

  }; // End of namespace DCPS
}; // End of namespace TAO

#endif /* BUILTINTOPICUTILS_H  */
