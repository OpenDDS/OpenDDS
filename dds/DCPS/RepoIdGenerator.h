/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef REPOIDGENERATOR_H
#define REPOIDGENERATOR_H

#include "tao/Basic_Types.h"

#include "dds/DdsDcpsInfoUtilsC.h"
#include "dds/DdsDcpsGuidC.h"

#include "dds/DCPS/GuidUtils.h"

#include "dcps_export.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL
namespace OpenDDS {
namespace DCPS {

/**
 * @class RepoIdGenerator
 *
 * @brief Create RepoId values for use within DDS.
 *
 * Internal to the OpenDDS repository, the Repository Identifiers that
 * uniquely identify all DDS Entities within the service managed by this
 * (and other federated) repositories consist of GUID values.
 *
 * These GUID (Global Unique IDentifiers) values are based on the RTPS
 * specification (formal/08-04-09) GUID_t values.  They use the same
 * structure.  The VendorId value is applied in the first 2 bytes of the
 * prefix.  The remainder of the Participant Id value is composed of the
 * OpenDDS specific Federation Id value (the identifier of the repository
 * where the Entity was created) and a DomainParticipant identifier within
 * that repository.  In addition to the standard EntityKind values,
 * the EntityKind byte has an added value of ENTITYKIND_OPENDDS_TOPIC
 * allowed to permit the use of GUID values for Topic entities as well.
 * This is an OpenDDS specific extension.
 *
 * The EntityKey field is used to distinguish Topics, Publications and
 * Subscriptions within a single Participant.  Each of these is within
 * its own number (address) space.  The EntityKind byte will ensure that
 * identical EntityKey values of these different types will not conflict
 * in the final GUID value.
 *
 * Values are generated using the specified FederationId and
 * DomainParticipant identifiers.  The EntityKey for a type is
 * incremented each time a value is generated.
 *
 * The ability to reset the last used value is provided to allow the
 * reloading of Entities from persistent storage without inducing
 * conflicts with newly created Entities.  This is a simplistic mechanism
 * and requires that all information from the persistent storage be
 * processed *prior* to any new Entities being created within the
 * repository.  After this processing is complete, the last used value
 * should be reset to ensure that any new values will not conflict with
 * the restored values.
 *
 * NOTE: This mechanism does not work well for values that have wrapped
 *       around the 24 bit EntityKey space.  It is possible to extend the
 *       current mapping to overflow into the two '0' bytes (2 and 3)
 *       with the Key value number (address) space to increase the
 *       possible unique identifiers if this becomes an issue.  Doing
 *       that would alleviate the need to manage an arena of Key values.
 *
 * Even though each Domain could have a separate GUID (address) space,
 * since we do not currently include the domain value within the GUID
 * mapping, all domains within a repository will use the same generator
 * instance to ensure no conflicting GUID values.  This will restrict the
 * total number of DomainParticipants within *all* domains of a
 * repository to be within the 32 bit (2**32) DomainParticipant address
 * space.  It is likely that other limits will be exceeded before this
 * one is approached.
 *
 * The current mapping of meanings to the GUID component values is:
 *
 * Content:
 * | VendorId|  0 |  0 |    FederationId   |  DomainParticpant |   EntityKey  |Kind|
 * GUID_t bytes:
 * |  0 |  1 |  2 |  3 |  4 |  5 |  6 |  7 |  8 |  9 | 10 | 11 | 12 | 13 | 14 | 15 |
 * GuidPrefix_t GUID_t.guidPrefix bytes:
 * |  0 |  1 |  2 |  3 |  4 |  5 |  6 |  7 |  8 |  9 | 10 | 11 |
 * EntityId_t GUID_t.entityId bytes:                           |  0 |  1 |  2 |  3 |
 * EntityKey_t GUID_t.entityId.entityKey bytes:                |  0 |  1 |  2 |
 *
 * Where the VendorId value used for OpenDDS is defined in GuidUtils.h
 */
class OpenDDS_Dcps_Export RepoIdGenerator {
public:
  static const unsigned int KeyBits;

  static const unsigned int KeyMask;

  /**
   * @brief construct with at least a FederationId value.
   *
   * @param  federation  identifier for the repository.
   * @param  participant identifier for the participant.
   * @param  kind        type of Entities to generate Id values for.
   * @return GUID_t      generated unique identifier value.
   *
   * If the @c kind is KIND_PARTICIPANT then the generator will generate
   * Participant RepoId values.  Otherwise it will generate the specified
   * type of Entity values within the specified Participant.
   */
  RepoIdGenerator(
    long federation,
    long participant = 0,
    EntityKind kind = KIND_PARTICIPANT);

  virtual ~RepoIdGenerator();

  /// Obtain the next RepoId value.
  RepoId next();

  /**
   * Set the minimum of the last key (or participant) value used.
   *
   * @param key the smallest value that the last generated key can be
   *
   * If the supplied @c key value is larger than the actual last
   * generated key value, the new key value replaces the old one.
   */
  void last(long key);

private:
  /// Type of Entity to generate GUID values for.
  EntityKind kind_;

  /// Unique identifier for the repository.
  long federation_;

  /// Unique identifier for the DomainParticipant.
  long participant_;

  /// Unique value for the EntityKey.
  long lastKey_;
};

} // namespace DCPS
} // namespace OpenDDS
OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* REPOIDGENERATOR_H  */
