/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "dds/DCPS/BuiltInTopicUtils.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

const char* const BUILT_IN_PARTICIPANT_TOPIC = "DCPSParticipant";
const char* const BUILT_IN_PARTICIPANT_TOPIC_TYPE = "PARTICIPANT_BUILT_IN_TOPIC_TYPE";

const char* const BUILT_IN_TOPIC_TOPIC = "DCPSTopic";
const char* const BUILT_IN_TOPIC_TOPIC_TYPE = "TOPIC_BUILT_IN_TOPIC_TYPE";

const char* const BUILT_IN_SUBSCRIPTION_TOPIC = "DCPSSubscription";
const char* const BUILT_IN_SUBSCRIPTION_TOPIC_TYPE = "SUBSCRIPTION_BUILT_IN_TOPIC_TYPE";

const char* const BUILT_IN_PUBLICATION_TOPIC = "DCPSPublication";
const char* const BUILT_IN_PUBLICATION_TOPIC_TYPE = "PUBLICATION_BUILT_IN_TOPIC_TYPE";

const char* const BUILT_IN_PARTICIPANT_LOCATION_TOPIC = "OpenDDSParticipantLocation";
const char* const BUILT_IN_PARTICIPANT_LOCATION_TOPIC_TYPE = "PARTICIPANT_LOCATION_BUILT_IN_TOPIC_TYPE";

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
