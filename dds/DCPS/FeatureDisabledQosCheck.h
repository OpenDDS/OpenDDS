/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_FEATURE_DISABLED_QOS_CHECK_H
#define OPENDDS_DCPS_FEATURE_DISABLED_QOS_CHECK_H

#include "dds/DCPS/Qos_Helper.h"

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
  if (qos.ownership.kind == ::DDS::EXCLUSIVE_OWNERSHIP_QOS) { \
    ACE_ERROR_RETURN((LM_ERROR, \
                      ACE_TEXT("(%P|%t) ERROR: ") \
                      ACE_TEXT("Feature ownership_kind_exclusive disabled, ") \
                      ACE_TEXT("therefore ownership kind must be SHARED. \n")), \
                     DDS::RETCODE_UNSUPPORTED); \
  }
#define OPENDDS_NO_OWNERSHIP_STRENGTH_COMPATIBILITY_CHECK(qos) \
  if (qos.ownership_strength.value != 0) { \
    ACE_ERROR_RETURN((LM_ERROR, \
                      ACE_TEXT("(%P|%t) ERROR: ") \
                      ACE_TEXT("Feature ownership_kind_exclusive disabled, ") \
                      ACE_TEXT("therefore ownership strength must be the default. \n")), \
                     DDS::RETCODE_UNSUPPORTED); \
  }
#else
#define OPENDDS_NO_OWNERSHIP_KIND_EXCLUSIVE_COMPATIBILITY_CHECK(qos)
#define OPENDDS_NO_OWNERSHIP_STRENGTH_COMPATIBILITY_CHECK(qos)
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

#ifdef OPENDDS_NO_PERSISTENCE_PROFILE

#define OPENDDS_NO_DURABILITY_SERVICE_COMPATIBILITY_CHECK(qos) \
  if (qos.durability_service != TheServiceParticipant->initial_DurabilityServiceQosPolicy()) { \
    ACE_ERROR_RETURN((LM_ERROR, \
                      ACE_TEXT("(%P|%t) ERROR: ") \
                      ACE_TEXT("Feature persistence_profile disabled, ") \
                      ACE_TEXT("therefore durability service must be the default. \n")), \
                     DDS::RETCODE_UNSUPPORTED); \
  }

#define OPENDDS_NO_DURABILITY_KIND_TRANSIENT_PERSISTENT(qos) \
  if (qos.durability.kind == ::DDS::TRANSIENT_DURABILITY_QOS || \
      qos.durability.kind == ::DDS::PERSISTENT_DURABILITY_QOS) { \
    ACE_ERROR_RETURN((LM_ERROR, \
                      ACE_TEXT("(%P|%t) ERROR: ") \
                      ACE_TEXT("Feature persistence_profile disabled, ") \
                      ACE_TEXT("therefore durability kind must not ") \
                      ACE_TEXT("be TRANSIENT_DURABILITY_QOS or PERSISTENT_DURABILITY_QOS. \n")), \
                     DDS::RETCODE_UNSUPPORTED); \
  }
  
#else

#define OPENDDS_NO_DURABILITY_SERVICE_COMPATIBILITY_CHECK(qos)
#define OPENDDS_NO_DURABILITY_KIND_TRANSIENT_PERSISTENT(qos)

#endif

#endif
