/*
 * Distributed under the OpenDDS License.
 * See: http://www.OpenDDS.org/license.html
 */

#ifndef OPENDDS_DCPS_SECURITY_ACCESS_CONTROL_DOMAIN_ID_SET_H
#define OPENDDS_DCPS_SECURITY_ACCESS_CONTROL_DOMAIN_ID_SET_H

#include <dds/DCPS/DisjointSequence.h>
#include <dds/DCPS/security/OpenDDS_Security_Export.h>
#include <dds/Versioned_Namespace.h>

#include <dds/DdsSecurityCoreC.h>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace Security {

typedef DCPS::DisjointSequence::OrderedRanges<DDS::Security::DomainId_t> DomainIdSet;
const DDS::Security::DomainId_t domain_id_min = 0;
const DDS::Security::DomainId_t domain_id_max = ACE_INT32_MAX;

} // namespace Security
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif
