// -*- C++ -*-
// ============================================================================
/**
 *  @file   GuidUtils.h
 *
 *  $Id$
 *
 *
 */
// ============================================================================

#ifndef GUIDUTILS_H
#define GUIDUTILS_H

#include "dcps_export.h"
// #include "dds/DdsDcpsInfrastructureC.h"
#include "dds/DdsDcpsGuidC.h"

#include <iosfwd>

namespace OpenDDS { namespace DCPS {
  /// Vendor Id value specified for OCI is used for OpenDDS.
  const GuidVendorId_t VENDORID_OCI = { 0x00, 0x03 };

  /// Nil value for the GUID prefix (participant identifier).
  const GuidPrefix_t GUIDPREFIX_UNKNOWN = { 0 };

  /// Entity Id values specified in Version 2.0 of RTPS specification.
  const EntityId_t ENTITYID_UNKNOWN                                = { {0x00,0x00,0x00}, 0x00};
  const EntityId_t ENTITYID_PARTICIPANT                            = { {0x00,0x00,0x01}, 0xc1};
  const EntityId_t ENTITYID_SEDP_BUILTIN_TOPIC_WRITER              = { {0x00,0x00,0x02}, 0xc2};
  const EntityId_t ENTITYID_SEDP_BUILTIN_TOPIC_READER              = { {0x00,0x00,0x02}, 0xc7};
  const EntityId_t ENTITYID_SEDP_BUILTIN_PUBLICATIONS_WRITER       = { {0x00,0x00,0x03}, 0xc2};
  const EntityId_t ENTITYID_SEDP_BUILTIN_PUBLICATIONS_READER       = { {0x00,0x00,0x03}, 0xc7};
  const EntityId_t ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_WRITER      = { {0x00,0x00,0x04}, 0xc2};
  const EntityId_t ENTITYID_SEDP_BUILTIN_SUBSCRIPTIONS_READER      = { {0x00,0x00,0x04}, 0xc7};
  const EntityId_t ENTITYID_SPDP_BUILTIN_PARTICIPANT_WRITER        = { {0x00,0x01,0x00}, 0xc2};
  const EntityId_t ENTITYID_SPDP_BUILTIN_PARTICIPANT_READER        = { {0x00,0x01,0x00}, 0xc7};
  const EntityId_t ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_WRITER = { {0x00,0x02,0x00}, 0xC2};
  const EntityId_t ENTITYID_P2P_BUILTIN_PARTICIPANT_MESSAGE_READER = { {0x00,0x02,0x00}, 0xC7};

  /// Nil value for GUID.
  const GUID_t GUID_UNKNOWN = { {0,0,0,0,0,0,0,0,0,0,0,0}, { {0,0,0}, 0} };

  /// Summary kinds of entities within the service.
  enum EntityKind {     // EntityId_t.entityKind value(s)
    KIND_UNKNOWN,       // 0x3f & 0x00 (and all other unspecified values)
    KIND_PARTICIPANT,   // 0x3f & 0x01 (invalid for isUser()
    KIND_WRITER,        // 0x3f & 0x02 | 0x3f & 0x03
    KIND_READER,        // 0x3f & 0x04 | 0x3f & 0x07
    KIND_TOPIC          // 0x3f & 0x05
  };

  /**
   * @class GuidConverter
   *
   * @brief Conversion processing and GUID value testing utilities.
   *
   * This class encapsulates the conversion of a GUID_t value to and from
   * other types used within OpenDDS.  This includes the ability to
   * create new GUID_t values with a specific federation Id and Participant
   * instance handle as well as extracting the Instance handle for an
   * Entity from the GUID_t value.  Tests for Entity Kind as well as the
   * type (User v. Builtin) of Entity are also included.
   *
   * Since the GUID_t type is formed of octets in network order, we do all
   * processing byte by byte to avoid any endian issues.
   *
   * Currently the GUID_t is mapped from various internal values.
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
  class OpenDDS_Dcps_Export GuidConverter {
    public:
      /// Construct from a GUID in order to modify it.
      GuidConverter( GUID_t& guid);

      /// Construct from a GUID pointer in order to modify it.
      GuidConverter( GUID_t* guid);

      /// Construct with federation and participant id values to form a GUID with.
      GuidConverter( long federation, long participant);

      /// Copy out a GUID value.
      operator GUID_t() const;

      /// Convert to long value.
      operator long() const;

      /// Convert to diagnostic string.
      operator const char*() const;

      /// Extract the federation Id value.
      long federationId() const;

      /// Extract the participant unique identifier.
      long participantId() const;

      /// Extract the entity key value.
      long value() const;

      /// Extract the VendorId value.
      long vendor() const;

      /// Extract the summary EntityKind value.
      EntityKind type() const;

      /// Access the actual kind octet.
      CORBA::Octet  kind() const;
      CORBA::Octet& kind();

      /// Access the key value.
      CORBA::Octet* key();

      // Entity kind flags.

      /// The GUID identifies an Entity of a Builtin type.
      bool isBuiltin() const;

      /// Thie GUID identifies an Entity defined by the User.
      bool isUser() const;

      /// The GUID identifies an Entity defined by a specific Vendor.
      bool isVendor() const;

      /// The GUID identifies a Reader or Writer that is for a Keyed topic.
      bool isKeyed() const;

      /// The GUID_t.entityId.entityKey is 3 bytes.
      enum { KeyBits = 24 };

      /// And is maskable.
      enum { KeyMask = ((0x1<<KeyBits)-1) };

    private:
      /// Reference of a GUID to modify.
      GUID_t& guid_;

      /// GUID formed with handles.
      GUID_t newGuid_;

      /// String conversion buffer.
      mutable char output_[64];
  };

}} // End namespace OpenDDS::DCPS

// Check for equality using the generated logical functor.
inline OpenDDS_Dcps_Export bool
operator==( const OpenDDS::DCPS::GUID_t& lhs, const OpenDDS::DCPS::GUID_t& rhs)
{
  GUID_tKeyLessThan lessThan;
  return !lessThan(lhs, rhs) && !lessThan(rhs, lhs);
}

// Serialize to ASCII Hex string: "xxxx.xxxx.xxxx.xxxx"
OpenDDS_Dcps_Export std::ostream&
operator<<( std::ostream& str, const OpenDDS::DCPS::GUID_t& value);

// Deserialize from ASCII Hex string: "xxxx.xxxx.xxxx.xxxx"
OpenDDS_Dcps_Export std::istream&
operator>>( std::istream& str, OpenDDS::DCPS::GUID_t& value);

#endif /* GUIDUTILS_H */

