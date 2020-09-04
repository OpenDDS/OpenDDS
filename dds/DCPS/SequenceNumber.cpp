/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <DCPS/DdsDcps_pch.h>

#include "SequenceNumber.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

const SequenceRange unknown_sequence_range(
  SequenceNumber::SEQUENCENUMBER_UNKNOWN(),
  SequenceNumber::SEQUENCENUMBER_UNKNOWN());

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
