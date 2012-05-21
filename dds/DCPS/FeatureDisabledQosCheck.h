/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_FEATURE_DISABLED_QOS_CHECK_H
#define OPENDDS_DCPS_FEATURE_DISABLED_QOS_CHECK_H

#ifdef OPENDDS_NO_OBJECT_MODEL_PROFILE
#define OPENDDS_NO_OWNERSHIP_PROFILE_COMPATIBILITY_CHECK(qos) \
  if (qos.history.depth > 1) { \
    ACE_ERROR_RETURN((LM_ERROR, \
                      ACE_TEXT("(%P|%t) ERROR: ") \
                      ACE_TEXT("Feature object_model_profile disabled, ") \
                      ACE_TEXT("therefore history depth must be 1. \n")), \
                     DDS::RETCODE_NOT_SUPPORTED) \
  }
#else
#define OPENDDS_NO_OWNERSHIP_PROFILE_COMPATIBILITY_CHECK(qos)
#endif

#ifdef OPENDDS_NO_OWNERSHIP_KIND_EXCLUSIVE
#define OPENDDS_NO_OWNERSHIP_KIND_EXCLUSIVE_COMPATIBILITY_CHECK(qos) \
  if (qos.ownership_strength != \
      TheServiceParticipant->initial_OwnershipStrengthQosPolicy()) { \
    ACE_ERROR_RETURN((LM_ERROR, \
                      ACE_TEXT("(%P|%t) ERROR: ") \
                      ACE_TEXT("Feature ownership_profile disabled, ") \
                      ACE_TEXT("therefore ownership strength must be the default. \n")), \
                     DDS::RETCODE_NOT_SUPPORTED) \
  } \
  if (qos.ownership.kind == ::DDS::EXCLUSIVE_OWNERSHIP_QOS) { \
    ACE_ERROR_RETURN((LM_ERROR, \
                      ACE_TEXT("(%P|%t) ERROR: ") \
                      ACE_TEXT("Feature ownership_profile disabled, ") \
                      ACE_TEXT("therefore ownership kind must be SHARED. \n")), \
                     DDS::RETCODE_NOT_SUPPORTED) \
  }
#else
#define OPENDDS_NO_OWNERSHIP_KIND_EXCLUSIVE_COMPATIBILITY_CHECK(qos)
#endif

#endif
