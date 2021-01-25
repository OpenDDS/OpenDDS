/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_REPOIDCONVERTER_H
#define OPENDDS_DCPS_REPOIDCONVERTER_H

#include "dds/DdsDcpsInfrastructureC.h"
#include "dds/DdsDcpsInfoUtilsC.h"

#include "GuidConverter.h"

#include "dcps_export.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {

namespace Federator {
  // reproducing type created by dds/InfoRepo/Federator.idl
  typedef ::CORBA::Long RepoKey;
}

namespace DCPS {
  typedef ::CORBA::Long ParticipantId;

/**
 * @class RepoIdConverter
 *
 * @brief Conversion processing and value testing utilities for
          DCPS Information Repository identifiers.
 *
 * This class encapsulates the conversion of a GUID_t value to and from
 * other types used within OpenDDS.
 *
 * Currently the RepoId is a typedef for GUID_t.  This class assumes
 * responsibility for insulating users from future change.
 *
 * Currently the RepoId is mapped from various internal values.
 * These mappings are:
 *
 * byte  structure reference     content
 * ---- ---------------------    --------------------------
 *   0  GUID_t.guidPrefix[ 0] == VendorId_t == 0x00 for OCI (used for OpenDDS)
 *   1  GUID_t.guidPrefix[ 1] == VendorId_t == 0x03 for OCI (used for OpenDDS)
 *   2  GUID_t.guidPrefix[ 2] == 0x00
 *   3  GUID_t.guidPrefix[ 3] == 0x00
 *
 *   4  GUID_t.guidPrefix[ 4] == federation id (MS byte)
 *   5  GUID_t.guidPrefix[ 5] == federation id
 *   6  GUID_t.guidPrefix[ 6] == federation id
 *   7  GUID_t.guidPrefix[ 7] == federation id (LS byte)
 *
 *   8  GUID_t.guidPrefix[ 8] == particpant id (MS byte)
 *   9  GUID_t.guidPrefix[ 9] == particpant id
 *  10  GUID_t.guidPrefix[10] == particpant id
 *  11  GUID_t.guidPrefix[11] == particpant id (LS byte)
 *
 *  12  GUID_t.entityId.entityKey[ 0] == entity id[0] (MS byte)
 *  13  GUID_t.entityId.entityKey[ 1] == entity id[1]
 *  14  GUID_t.entityId.entityKey[ 2] == entity id[2] (LS byte)
 *  15  GUID_t.entityId.entityKind    == entity kind
 */
class OpenDDS_Dcps_Export RepoIdConverter : public GuidConverter {
public:
  RepoIdConverter(const RepoId& repoId);

  ~RepoIdConverter();

  /// Get the federeation id from the GUID
  OpenDDS::Federator::RepoKey federationId() const;

  /// Get the participant id from the GUID
  ParticipantId participantId() const;
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* DCPS_REPOIDCONVERTER_H */
