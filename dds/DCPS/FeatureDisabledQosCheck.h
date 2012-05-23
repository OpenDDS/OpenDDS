/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_FEATURE_DISABLED_QOS_CHECK_H
#define OPENDDS_DCPS_FEATURE_DISABLED_QOS_CHECK_H

#ifdef  OPENDDS_NO_OWNERSHIP_PROFILE
#define OPENDDS_NO_OWNERSHIP_PROFILE_COMPATIBILITY_CHECK(qos) \
  if (qos.history.depth > 1) { \
    ACE_ERROR_RETURN((LM_ERROR, \
                      ACE_TEXT("(%P|%t) ERROR: ") \
                      ACE_TEXT("Feature ownership_profile disabled, ") \
                      ACE_TEXT("therefore history depth must be 1. \n")), \
                     DDS::RETCODE_UNSUPPORTED); \
  }
#else
#define OPENDDS_NO_OWNERSHIP_PROFILE_COMPATIBILITY_CHECK(qos)
#endif

#ifdef  OPENDDS_NO_OWNERSHIP_KIND_EXCLUSIVE
#define OPENDDS_NO_OWNERSHIP_KIND_EXCLUSIVE_COMPATIBILITY_CHECK(qos) \
  if (qos.ownership_strength != \
      TheServiceParticipant->initial_OwnershipStrengthQosPolicy()) { \
    ACE_ERROR_RETURN((LM_ERROR, \
                      ACE_TEXT("(%P|%t) ERROR: ") \
                      ACE_TEXT("Feature ownership_kind_exclusive disabled, ") \
                      ACE_TEXT("therefore ownership strength must be the default. \n")), \
                     DDS::RETCODE_UNSUPPORTED); \
  } \
  if (qos.ownership.kind == ::DDS::EXCLUSIVE_OWNERSHIP_QOS) { \
    ACE_ERROR_RETURN((LM_ERROR, \
                      ACE_TEXT("(%P|%t) ERROR: ") \
                      ACE_TEXT("Feature ownership_kind_exclusive disabled, ") \
                      ACE_TEXT("therefore ownership kind must be SHARED. \n")), \
                     DDS::RETCODE_UNSUPPORTED); \
  }
#else
#define OPENDDS_NO_OWNERSHIP_KIND_EXCLUSIVE_COMPATIBILITY_CHECK(qos)
#endif

#ifdef  OPENDDS_NO_OBJECT_MODEL_PROFILE
#define OPENDDS_NO_OBJECT_MODEL_PROFILE_COMPATIBILITY_CHECK(qos) \
  if (qos.presentation.access_scope == ::DDS::GROUP_PRESENTATION_QOS) { \
    ACE_ERROR_RETURN((LM_ERROR, \
                      ACE_TEXT("(%P|%t) ERROR: ") \
                      ACE_TEXT("Feature object_model_profile disabled, ") \
                      ACE_TEXT("therefore presentation access scope cannot be GROUP. \n")), \
                     DDS::RETCODE_UNSUPPORTED); \
  } \
  if (qos.presentation.access_scope == ::DDS::TOPIC_PRESENTATION_QOS) { \
    ACE_ERROR_RETURN((LM_ERROR, \
                      ACE_TEXT("(%P|%t) ERROR: ") \
                      ACE_TEXT("Feature object_model_profile disabled. ") \
                      ACE_TEXT("Although not part of the profile, currently ") \
                      ACE_TEXT("this means presentation access scope cannot be TOPIC. \n")), \
                     DDS::RETCODE_UNSUPPORTED); \
  }
#else  
#define OPENDDS_NO_OBJECT_MODEL_PROFILE_COMPATIBILITY_CHECK(qos)    
#endif

#endif
