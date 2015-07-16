/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_FEATURE_DISABLED_QOS_CHECK_H
#define OPENDDS_DCPS_FEATURE_DISABLED_QOS_CHECK_H

#include "dds/DCPS/Qos_Helper.h"

#ifdef  OPENDDS_NO_OWNERSHIP_PROFILE
#define OPENDDS_NO_OWNERSHIP_PROFILE_COMPATIBILITY_CHECK(qos, error_rtn_value) \
  if (qos.history.kind == ::DDS::KEEP_ALL_HISTORY_QOS || qos.history.depth > 1) { \
    ACE_ERROR((LM_ERROR, \
              ACE_TEXT("(%P|%t) ERROR: ") \
              ACE_TEXT("Feature ownership_profile disabled, ") \
              ACE_TEXT("therefore history must be KEEP_LAST, depth 1.\n"))); \
    return error_rtn_value; \
  }
#else
#define OPENDDS_NO_OWNERSHIP_PROFILE_COMPATIBILITY_CHECK(qos, error_rtn_value)
#endif

#ifdef  OPENDDS_NO_OWNERSHIP_KIND_EXCLUSIVE
#define OPENDDS_NO_OWNERSHIP_KIND_EXCLUSIVE_COMPATIBILITY_CHECK(qos, error_rtn_value) \
  if (qos.ownership.kind == ::DDS::EXCLUSIVE_OWNERSHIP_QOS) { \
    ACE_ERROR((LM_ERROR, \
              ACE_TEXT("(%P|%t) ERROR: ") \
              ACE_TEXT("Feature ownership_kind_exclusive disabled, ") \
              ACE_TEXT("therefore ownership kind must be SHARED. \n"))); \
    return error_rtn_value; \
  }
#define OPENDDS_NO_OWNERSHIP_STRENGTH_COMPATIBILITY_CHECK(qos, error_rtn_value) \
  if (qos.ownership_strength.value != 0) { \
    ACE_ERROR((LM_ERROR, \
              ACE_TEXT("(%P|%t) ERROR: ") \
              ACE_TEXT("Feature ownership_kind_exclusive disabled, ") \
              ACE_TEXT("therefore ownership strength must be the default. \n"))); \
    return error_rtn_value; \
  }
#else
#define OPENDDS_NO_OWNERSHIP_KIND_EXCLUSIVE_COMPATIBILITY_CHECK(qos, error_rtn_value)
#define OPENDDS_NO_OWNERSHIP_STRENGTH_COMPATIBILITY_CHECK(qos, error_rtn_value)
#endif

#ifdef  OPENDDS_NO_OBJECT_MODEL_PROFILE
#define OPENDDS_NO_OBJECT_MODEL_PROFILE_COMPATIBILITY_CHECK(qos, error_rtn_value) \
  if (qos.presentation.access_scope == ::DDS::GROUP_PRESENTATION_QOS) { \
    ACE_ERROR((LM_ERROR, \
              ACE_TEXT("(%P|%t) ERROR: ") \
              ACE_TEXT("Feature object_model_profile disabled, ") \
              ACE_TEXT("therefore presentation access scope cannot be GROUP. \n"))); \
    return error_rtn_value; \
  } \
  if (qos.presentation.access_scope == ::DDS::TOPIC_PRESENTATION_QOS) { \
    ACE_ERROR((LM_ERROR, \
              ACE_TEXT("(%P|%t) ERROR: ") \
              ACE_TEXT("Feature object_model_profile disabled. ") \
              ACE_TEXT("Although not part of the profile, currently ") \
              ACE_TEXT("this means presentation access scope cannot be TOPIC. \n"))); \
    return error_rtn_value; \
  }
#else
#define OPENDDS_NO_OBJECT_MODEL_PROFILE_COMPATIBILITY_CHECK(qos, error_rtn_value)
#endif

#ifdef OPENDDS_NO_PERSISTENCE_PROFILE

#define OPENDDS_NO_DURABILITY_SERVICE_COMPATIBILITY_CHECK(qos, error_rtn_value) \
  if (qos.durability_service != TheServiceParticipant->initial_DurabilityServiceQosPolicy()) { \
    ACE_ERROR((LM_ERROR, \
              ACE_TEXT("(%P|%t) ERROR: ") \
              ACE_TEXT("Feature persistence_profile disabled, ") \
              ACE_TEXT("therefore durability service must be the default. \n"))); \
    return error_rtn_value; \
  }

#define OPENDDS_NO_DURABILITY_KIND_TRANSIENT_PERSISTENT_COMPATIBILITY_CHECK(qos, error_rtn_value) \
  if (qos.durability.kind == ::DDS::TRANSIENT_DURABILITY_QOS || \
      qos.durability.kind == ::DDS::PERSISTENT_DURABILITY_QOS) { \
    ACE_ERROR((LM_ERROR, \
              ACE_TEXT("(%P|%t) ERROR: ") \
              ACE_TEXT("Feature persistence_profile disabled, ") \
              ACE_TEXT("therefore durability kind must not ") \
              ACE_TEXT("be TRANSIENT_DURABILITY_QOS or PERSISTENT_DURABILITY_QOS. \n"))); \
    return error_rtn_value; \
  }

#else

#define OPENDDS_NO_DURABILITY_SERVICE_COMPATIBILITY_CHECK(qos, error_rtn_value)
#define OPENDDS_NO_DURABILITY_KIND_TRANSIENT_PERSISTENT_COMPATIBILITY_CHECK(qos, error_rtn_value)

#endif

#endif
