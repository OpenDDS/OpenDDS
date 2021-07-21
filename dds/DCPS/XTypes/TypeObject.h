/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_XTYPES_TYPE_OBJECT_H
#define OPENDDS_DCPS_XTYPES_TYPE_OBJECT_H

#include "External.h"

#include <dds/DCPS/PoolAllocationBase.h>
#include <dds/DCPS/PoolAllocator.h>
#include <dds/DCPS/Serializer.h>

#include <ace/CDR_Base.h>

#include <algorithm>
#include <cstring>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL
namespace OpenDDS {

namespace XTypes {
  struct TypeInformation;
}

namespace DCPS {
  OpenDDS_Dcps_Export
  bool operator<<(Serializer& ser, const XTypes::TypeInformation& type_info);

  OpenDDS_Dcps_Export
  bool operator>>(Serializer& ser, XTypes::TypeInformation& type_info);
}

namespace XTypes {

  template<typename T, typename T_slice, typename TAG>
  class Fake_TAO_Array_Forany_T {
  public:
    typedef T_slice _slice_type;

    Fake_TAO_Array_Forany_T (_slice_type *p)
      : ptr_(p)
    {}

    typedef const _slice_type *   _in_type;
    typedef       _slice_type * & _out_type;

    _in_type      in (void) const
    {
      return (const T_slice *) this->ptr_;
    }

    _out_type     out (void)
    {
      return this->ptr_;
    }

  private:
    _slice_type * ptr_;
  };

  OpenDDS_Dcps_Export
  const DCPS::Encoding& get_typeobject_encoding();

  template <typename T>
  struct Optional {
    bool present;
    T value;

    Optional()
      : present(false)
    {}

    Optional(const T& v)
      : present(true)
      , value(v)
    {}
  };

  template <typename T>
  struct Sequence {
    typedef OPENDDS_VECTOR(T) Members;
    Members members;

    Sequence& append(const T& member)
    {
      members.push_back(member);
      return *this;
    }

    Sequence& sort()
    {
      std::sort(members.begin(), members.end());
      return *this;
    }

    ACE_CDR::ULong length() const
    {
      return static_cast<ACE_CDR::ULong>(members.size());
    }

    void length(ACE_CDR::ULong len)
    {
      return members.resize(len);
    }

    const T& operator[](ACE_CDR::ULong i) const
    {
      return members[i];
    }

    T& operator[](ACE_CDR::ULong i)
    {
      return members[i];
    }

    bool operator<(const Sequence& other) const { return members < other.members; }

    T* get_buffer() { return &members[0]; }
    const T* get_buffer() const { return &members[0]; }

    typedef typename Members::const_iterator const_iterator;
    const_iterator begin() const { return members.begin(); }
    const_iterator end() const { return members.end(); }
  };

  // Based on dds-xtypes_typeobject.idl

  // The types in this file shall be serialized with XCDR encoding version 2

  // ---------- Equivalence Kinds -------------------
  typedef ACE_CDR::Octet EquivalenceKind;
  const EquivalenceKind EK_MINIMAL   = 0xF1; // 0x1111 0001
  const EquivalenceKind EK_COMPLETE  = 0xF2; // 0x1111 0010
  const EquivalenceKind EK_BOTH      = 0xF3; // 0x1111 0011

  // ---------- TypeKinds (begin) -------------------
  typedef ACE_CDR::Octet TypeKind;

  // Primitive TKs
  const TypeKind TK_NONE       = 0x00;
  const TypeKind TK_BOOLEAN    = 0x01;
  const TypeKind TK_BYTE       = 0x02;
  const TypeKind TK_INT16      = 0x03;
  const TypeKind TK_INT32      = 0x04;
  const TypeKind TK_INT64      = 0x05;
  const TypeKind TK_UINT16     = 0x06;
  const TypeKind TK_UINT32     = 0x07;
  const TypeKind TK_UINT64     = 0x08;
  const TypeKind TK_FLOAT32    = 0x09;
  const TypeKind TK_FLOAT64    = 0x0A;
  const TypeKind TK_FLOAT128   = 0x0B;
  const TypeKind TK_INT8       = 0x0C; // XTypes 1.3 Annex B
  const TypeKind TK_UINT8      = 0x0D; // XTypes 1.3 Annex B
  const TypeKind TK_CHAR8      = 0x10;
  const TypeKind TK_CHAR16     = 0x11;

  // String TKs
  const TypeKind TK_STRING8    = 0x20;
  const TypeKind TK_STRING16   = 0x21;

  // Constructed/Named types
  const TypeKind TK_ALIAS      = 0x30;

  // Enumerated TKs
  const TypeKind TK_ENUM       = 0x40;
  const TypeKind TK_BITMASK    = 0x41;

  // Structured TKs
  const TypeKind TK_ANNOTATION = 0x50;
  const TypeKind TK_STRUCTURE  = 0x51;
  const TypeKind TK_UNION      = 0x52;
  const TypeKind TK_BITSET     = 0x53;

  // Collection TKs
  const TypeKind TK_SEQUENCE   = 0x60;
  const TypeKind TK_ARRAY      = 0x61;
  const TypeKind TK_MAP        = 0x62;
  // ---------- TypeKinds (end) -------------------

  // ---------- Extra TypeIdentifiers (begin) ------------
  typedef ACE_CDR::Octet TypeIdentifierKind;
  const TypeIdentifierKind TI_STRING8_SMALL        = 0x70;
  const TypeIdentifierKind TI_STRING8_LARGE        = 0x71;
  const TypeIdentifierKind TI_STRING16_SMALL       = 0x72;
  const TypeIdentifierKind TI_STRING16_LARGE       = 0x73;

  const TypeIdentifierKind TI_PLAIN_SEQUENCE_SMALL = 0x80;
  const TypeIdentifierKind TI_PLAIN_SEQUENCE_LARGE = 0x81;

  const TypeIdentifierKind TI_PLAIN_ARRAY_SMALL    = 0x90;
  const TypeIdentifierKind TI_PLAIN_ARRAY_LARGE    = 0x91;

  const TypeIdentifierKind TI_PLAIN_MAP_SMALL      = 0xA0;
  const TypeIdentifierKind TI_PLAIN_MAP_LARGE      = 0xA1;

  const TypeIdentifierKind TI_STRONGLY_CONNECTED_COMPONENT = 0xB0;
  // ---------- Extra TypeIdentifiers (end) --------------

  // The name of some element (e.g. type, type member, module)
  // Valid characters are alphanumeric plus the "_" cannot start with digit
  const ACE_CDR::Long MEMBER_NAME_MAX_LENGTH = 256;
  typedef OPENDDS_STRING MemberName;

  // Qualified type name includes the name of containing modules
  // using "::" as separator. No leading "::". E.g. "MyModule::MyType"
  const ACE_CDR::Long TYPE_NAME_MAX_LENGTH = 256;
  typedef OPENDDS_STRING QualifiedTypeName;

  // Every type has an ID. Those of the primitive types are pre-defined.
  typedef ACE_CDR::Octet PrimitiveTypeId;

  // First 14 bytes of MD5 of the serialized TypeObject using XCDR
  // version 2 with Little Endian encoding
  typedef ACE_CDR::Octet EquivalenceHash[14];
  struct EquivalenceHash_tag {};
  typedef ACE_CDR::Octet EquivalenceHash_slice;
  typedef Fake_TAO_Array_Forany_T<EquivalenceHash, EquivalenceHash_slice, EquivalenceHash_tag> EquivalenceHash_forany;

  // First 4 bytes of MD5 of of a member name converted to bytes
  // using UTF-8 encoding and without a 'nul' terminator.
  // Example: the member name "color" has NameHash {0x70, 0xDD, 0xA5, 0xDF}
  typedef ACE_CDR::Octet NameHash[4];
  struct NameHash_tag {};
  typedef ACE_CDR::Octet NameHash_slice;
  typedef Fake_TAO_Array_Forany_T<NameHash, NameHash_slice, NameHash_tag> NameHash_forany;

  // Long Bound of a collection type
  typedef ACE_CDR::ULong LBound;
  typedef Sequence<LBound> LBoundSeq;
  const LBound INVALID_LBOUND = 0;

  // Short Bound of a collection type
  typedef ACE_CDR::Octet SBound;
  typedef Sequence<SBound> SBoundSeq;
  const SBound INVALID_SBOUND = 0;

  struct EquivalenceHashWrapper { // not in spec
    EquivalenceHashWrapper(ACE_CDR::Octet a, ACE_CDR::Octet b, ACE_CDR::Octet c, ACE_CDR::Octet d,
                           ACE_CDR::Octet e, ACE_CDR::Octet f, ACE_CDR::Octet g, ACE_CDR::Octet h,
                           ACE_CDR::Octet i, ACE_CDR::Octet j, ACE_CDR::Octet k, ACE_CDR::Octet l,
                           ACE_CDR::Octet m, ACE_CDR::Octet n)
    {
      eh_[0] = a; eh_[1] = b; eh_[2] = c; eh_[3] = d; eh_[4] = e; eh_[5] = f; eh_[6] = g;
      eh_[7] = h; eh_[8] = i; eh_[9] = j; eh_[10] = k; eh_[11] = l; eh_[12] = m; eh_[13] = n;
    }
    EquivalenceHash eh_;
  };

  // union TypeObjectHashId switch (octet) {
  //     case EK_COMPLETE:
  //     case EK_MINIMAL:
  //         EquivalenceHash  hash;
  // };
  struct TypeObjectHashId {
    EquivalenceKind kind;
    EquivalenceHash hash;

    TypeObjectHashId()
      : kind(0)
    {}

    TypeObjectHashId(const EquivalenceKind& a_kind,
                     const EquivalenceHashWrapper& a_hash)
      : kind(a_kind)
    {
      std::memcpy(hash, a_hash.eh_, sizeof hash);
    }

    bool operator<(const TypeObjectHashId& other) const
    {
      if (kind < other.kind) return true;
      if (other.kind < kind) return false;
      int ret = std::memcmp(hash, other.hash, sizeof hash);
      if (ret < 0) return true;
      if (ret > 0) return false;
      return false;
    }
  };

  // Flags that apply to struct/union/collection/enum/bitmask/bitset
  // members/elements and DO affect type assignability
  // Depending on the flag it may not apply to members of all types
  // When not all, the applicable member types are listed
  typedef ACE_CDR::UShort MemberFlag;
  const MemberFlag TRY_CONSTRUCT1 = 1 << 0;     // T1 | 00 = INVALID, 01 = DISCARD
  const MemberFlag TRY_CONSTRUCT2 = 1 << 1;     // T2 | 10 = USE_DEFAULT, 11 = TRIM
  const MemberFlag IS_EXTERNAL = 1 << 2;        // X  StructMember, UnionMember,
  //    CollectionElement
  const MemberFlag IS_OPTIONAL = 1 << 3;        // O  StructMember
  const MemberFlag IS_MUST_UNDERSTAND = 1 << 4; // M  StructMember
  const MemberFlag IS_KEY = 1 << 5;             // K  StructMember, UnionDiscriminator
  const MemberFlag IS_DEFAULT = 1 << 6;         // D  UnionMember, EnumerationLiteral

  typedef MemberFlag CollectionElementFlag;   // T1, T2, X
  typedef MemberFlag StructMemberFlag;        // T1, T2, O, M, K, X
  typedef MemberFlag UnionMemberFlag;         // T1, T2, D, X
  typedef MemberFlag UnionDiscriminatorFlag;  // T1, T2, K
  typedef MemberFlag EnumeratedLiteralFlag;   // D
  typedef MemberFlag AnnotationParameterFlag; // Unused. No flags apply
  typedef MemberFlag AliasMemberFlag;         // Unused. No flags apply
  typedef MemberFlag BitflagFlag;             // Unused. No flags apply
  typedef MemberFlag BitsetMemberFlag;        // Unused. No flags apply

  const MemberFlag TryConstructDiscardValue    = TRY_CONSTRUCT1;
  const MemberFlag TryConstructUseDefaultValue = TRY_CONSTRUCT2;
  const MemberFlag TryConstructTrimValue       = TRY_CONSTRUCT1 | TRY_CONSTRUCT2;

  // Mask used to remove the flags that do not affect assignability
  // Selects  T1, T2, O, M, K, D
  const MemberFlag MemberFlagMinimalMask = 0x003f;

  // Flags that apply to type declarations and DO affect assignability
  // Depending on the flag it may not apply to all types
  // When not all, the applicable types are listed
  typedef ACE_CDR::UShort TypeFlag;
  const TypeFlag IS_FINAL = 1 << 0;        // F |
  const TypeFlag IS_APPENDABLE = 1 << 1;   // A |-  Struct, Union
  const TypeFlag IS_MUTABLE = 1 << 2;      // M |   (exactly one flag)

  const TypeFlag IS_NESTED = 1 << 3;       // N     Struct, Union
  const TypeFlag IS_AUTOID_HASH = 1 << 4;  // H     Struct

  typedef TypeFlag StructTypeFlag;      // All flags apply
  typedef TypeFlag UnionTypeFlag;       // All flags apply
  typedef TypeFlag CollectionTypeFlag;  // Unused. No flags apply
  typedef TypeFlag AnnotationTypeFlag;  // Unused. No flags apply
  typedef TypeFlag AliasTypeFlag;       // Unused. No flags apply
  typedef TypeFlag EnumTypeFlag;        // Unused. No flags apply
  typedef TypeFlag BitmaskTypeFlag;     // Unused. No flags apply
  typedef TypeFlag BitsetTypeFlag;      // Unused. No flags apply

  // Mask used to remove the flags that do no affect assignability
  const TypeFlag TypeFlagMinimalMask = 0x0007; // Selects  M, A, F

  // Forward declaration
  class TypeIdentifier;

  // 1 Byte
  struct StringSTypeDefn {
    SBound bound;

    StringSTypeDefn()
      : bound(0)
    {}

    explicit StringSTypeDefn(const SBound a_bound)
      : bound(a_bound)
    {}

    bool operator<(const StringSTypeDefn& other) const
    {
      return bound < other.bound;
    }
  };

  // 4 Bytes
  struct StringLTypeDefn {
    LBound bound;

    StringLTypeDefn()
      : bound(0)
    {}

    explicit StringLTypeDefn(const LBound a_bound)
      : bound(a_bound)
    {}

    bool operator<(const StringLTypeDefn& other) const
    {
      return bound < other.bound;
    }
  };

  struct PlainCollectionHeader {
    EquivalenceKind equiv_kind;
    CollectionElementFlag element_flags;

    PlainCollectionHeader()
      : equiv_kind(0)
      , element_flags(0)
    {}

    PlainCollectionHeader(const EquivalenceKind& a_equiv_kind,
                          const CollectionElementFlag& a_element_flags)
      : equiv_kind(a_equiv_kind)
      , element_flags(a_element_flags)
    {}

    bool operator<(const PlainCollectionHeader& other) const
    {
      if (equiv_kind < other.equiv_kind) return true;
      if (other.equiv_kind < equiv_kind) return false;
      if (element_flags < other.element_flags) return true;
      if (other.element_flags < element_flags) return false;
      return false;
    }
  };

  struct PlainSequenceSElemDefn {
    PlainCollectionHeader header;
    SBound bound;
    External<TypeIdentifier> element_identifier;

    PlainSequenceSElemDefn()
      : bound(INVALID_SBOUND)
    {}

    PlainSequenceSElemDefn(const PlainCollectionHeader& a_header,
                           const SBound& a_bound,
                           const TypeIdentifier& a_element_identifier)
      : header(a_header)
      , bound(a_bound)
      , element_identifier(a_element_identifier)
    {}

    bool operator<(const PlainSequenceSElemDefn& other) const;
  };

  struct PlainSequenceLElemDefn {
    PlainCollectionHeader header;
    LBound bound;
    External<TypeIdentifier> element_identifier;

    PlainSequenceLElemDefn()
      : bound(INVALID_LBOUND)
    {}

    PlainSequenceLElemDefn(const PlainCollectionHeader& a_header,
                           const LBound& a_bound,
                           const TypeIdentifier& a_element_identifier)
      : header(a_header)
      , bound(a_bound)
      , element_identifier(a_element_identifier)
    {}

    bool operator<(const PlainSequenceLElemDefn& other) const;
  };

  struct PlainArraySElemDefn {
    PlainCollectionHeader header;
    SBoundSeq array_bound_seq;
    External<TypeIdentifier> element_identifier;

    PlainArraySElemDefn() {}

    PlainArraySElemDefn(const PlainCollectionHeader& a_header,
                        const SBoundSeq& a_array_bound_seq,
                        const TypeIdentifier& a_element_identifier)
      : header(a_header)
      , array_bound_seq(a_array_bound_seq)
      , element_identifier(a_element_identifier)
    {}

    bool operator<(const PlainArraySElemDefn& other) const;
  };

  struct PlainArrayLElemDefn {
    PlainCollectionHeader header;
    LBoundSeq array_bound_seq;
    External<TypeIdentifier> element_identifier;

    PlainArrayLElemDefn() {}

    PlainArrayLElemDefn(const PlainCollectionHeader& a_header,
                        const LBoundSeq& a_array_bound_seq,
                        const TypeIdentifier& a_element_identifier)
      : header(a_header)
      , array_bound_seq(a_array_bound_seq)
      , element_identifier(a_element_identifier)
    {}

    bool operator<(const PlainArrayLElemDefn& other) const;
  };

  struct PlainMapSTypeDefn {
    PlainCollectionHeader header;
    SBound bound;
    External<TypeIdentifier> element_identifier;
    CollectionElementFlag key_flags;
    External<TypeIdentifier> key_identifier;

    PlainMapSTypeDefn()
      : bound(INVALID_SBOUND)
      , key_flags(0)
    {}

    PlainMapSTypeDefn(const PlainCollectionHeader& a_header,
                      const SBound& a_bound,
                      const TypeIdentifier& a_element_identifier,
                      const CollectionElementFlag& a_key_flags,
                      const TypeIdentifier& a_key_identifier)
      : header(a_header)
      , bound(a_bound)
      , element_identifier(a_element_identifier)
      , key_flags(a_key_flags)
      , key_identifier(a_key_identifier)
    {}

    bool operator<(const PlainMapSTypeDefn& other) const;
  };

  struct PlainMapLTypeDefn {
    PlainCollectionHeader header;
    LBound bound;
    External<TypeIdentifier> element_identifier;
    CollectionElementFlag key_flags;
    External<TypeIdentifier> key_identifier;

    PlainMapLTypeDefn()
      : bound(INVALID_LBOUND)
      , key_flags(0)
    {}

    PlainMapLTypeDefn(const PlainCollectionHeader& a_header,
                      const LBound& a_bound,
                      const TypeIdentifier& a_element_identifier,
                      const CollectionElementFlag& a_key_flags,
                      const TypeIdentifier& a_key_identifier)
      : header(a_header)
      , bound(a_bound)
      , element_identifier(a_element_identifier)
      , key_flags(a_key_flags)
      , key_identifier(a_key_identifier)
    {}

    bool operator<(const PlainMapLTypeDefn& other) const;
  };

  // Used for Types that have cyclic dependencies with other types
  struct StronglyConnectedComponentId {
    TypeObjectHashId sc_component_id; // Hash StronglyConnectedComponent
    ACE_CDR::Long scc_length; // StronglyConnectedComponent.length
    ACE_CDR::Long scc_index; // identify type in Strongly Connected Comp.

    StronglyConnectedComponentId()
      : scc_length(0)
      , scc_index(0)
    {}

    StronglyConnectedComponentId(const TypeObjectHashId& a_sc_component_id,
                                 const ACE_CDR::Long& a_scc_length,
                                 const ACE_CDR::Long& a_scc_index)
      : sc_component_id(a_sc_component_id)
      , scc_length(a_scc_length)
      , scc_index(a_scc_index)
    {}

    bool operator<(const StronglyConnectedComponentId& other) const
    {
      if (sc_component_id < other.sc_component_id) return true;
      if (other.sc_component_id < sc_component_id) return false;
      if (scc_length < other.scc_length) return true;
      if (other.scc_length < scc_length) return false;
      if (scc_index < other.scc_index) return true;
      if (other.scc_index < scc_index) return false;
      return false;
    }
  };

  // Future extensibility
  struct ExtendedTypeDefn {
    // Empty. Available for future extension
  };

  // The TypeIdentifier uniquely identifies a type (a set of equivalent
  // types according to an equivalence relationship:  COMPLETE, MNIMAL).
  //
  // In some cases (primitive types, strings, plain types) the identifier
  // is a explicit description of the type.
  // In other cases the Identifier is a Hash of the type description
  //
  // In the case of primitive types and strings the implied equivalence
  // relation is the identity.
  //
  // For Plain Types and Hash-defined TypeIdentifiers there are three
  //  possibilities: MINIMAL, COMPLETE, and COMMON:
  //   - MINIMAL indicates the TypeIdentifier identifies equivalent types
  //     according to the MINIMAL equivalence relation
  //   - COMPLETE indicates the TypeIdentifier identifies equivalent types
  //     according to the COMPLETE equivalence relation
  //   - COMMON indicates the TypeIdentifier identifies equivalent types
  //     according to both the MINIMAL and the COMMON equivalence relation.
  //     This means the TypeIdentifier is the same for both relationships
  //
  // @extensibility(FINAL) @nested
  // union TypeIdentifier switch (octet) {
  //   // ============  Primitive types - use TypeKind ====================
  //   // All primitive types fall here.
  //   // Commented-out because Unions cannot have cases with no member.
  //   /*
  //     case TK_NONE:
  //     case TK_BOOLEAN:
  //     case TK_BYTE:
  //     case TK_INT16:
  //     case TK_INT32:
  //     case TK_INT64:
  //     case TK_UINT16:
  //     case TK_UINT32:
  //     case TK_UINT64:
  //     case TK_FLOAT32:
  //     case TK_FLOAT64:
  //     case TK_FLOAT128:
  //     case TK_CHAR8:
  //     case TK_CHAR16:
  //     // No Value
  //     */

  //   // ============ Strings - use TypeIdentifierKind ===================
  // case TI_STRING8_SMALL:
  // case TI_STRING16_SMALL:
  //   StringSTypeDefn         string_sdefn;

  // case TI_STRING8_LARGE:
  // case TI_STRING16_LARGE:
  //   StringLTypeDefn         string_ldefn;

  //   // ============  Plain collections - use TypeIdentifierKind =========
  // case TI_PLAIN_SEQUENCE_SMALL:
  //   PlainSequenceSElemDefn  seq_sdefn;
  // case TI_PLAIN_SEQUENCE_LARGE:
  //   PlainSequenceLElemDefn  seq_ldefn;

  // case TI_PLAIN_ARRAY_SMALL:
  //   PlainArraySElemDefn     array_sdefn;
  // case TI_PLAIN_ARRAY_LARGE:
  //   PlainArrayLElemDefn     array_ldefn;

  // case TI_PLAIN_MAP_SMALL:
  //   PlainMapSTypeDefn       map_sdefn;
  // case TI_PLAIN_MAP_LARGE:
  //   PlainMapLTypeDefn       map_ldefn;

  //   // ============  Types that are mutually dependent on each other ===
  // case TI_STRONGLY_CONNECTED_COMPONENT:
  //   StronglyConnectedComponentId  sc_component_id;

  //   // ============  The remaining cases - use EquivalenceKind =========
  // case EK_COMPLETE:
  // case EK_MINIMAL:
  //   EquivalenceHash         equivalence_hash;

  //   // ===================  Future extensibility  ============
  //   // Future extensions
  // default:
  //   ExtendedTypeDefn        extended_defn;
  // };

  class OpenDDS_Dcps_Export TypeIdentifier : public DCPS::PoolAllocationBase {
  public:
    explicit TypeIdentifier(ACE_CDR::Octet kind = TK_NONE);
    TypeIdentifier(const TypeIdentifier& other);
    TypeIdentifier& operator=(const TypeIdentifier& other);
    ~TypeIdentifier() { reset(); }

    TypeIdentifier(ACE_CDR::Octet kind, const StringSTypeDefn& sdefn);
    TypeIdentifier(ACE_CDR::Octet kind, const StringLTypeDefn& ldefn);
    TypeIdentifier(ACE_CDR::Octet kind, const PlainSequenceSElemDefn& sdefn);
    TypeIdentifier(ACE_CDR::Octet kind, const PlainSequenceLElemDefn& ldefn);
    TypeIdentifier(ACE_CDR::Octet kind, const PlainArraySElemDefn& sdefn);
    TypeIdentifier(ACE_CDR::Octet kind, const PlainArrayLElemDefn& ldefn);
    TypeIdentifier(ACE_CDR::Octet kind, const EquivalenceHashWrapper& equivalence_hash);
    TypeIdentifier(ACE_CDR::Octet kind, const StronglyConnectedComponentId& sc_component_id);

    ACE_CDR::Octet kind() const { return kind_; }

#define OPENDDS_UNION_ACCESSORS(T, N)                         \
    const T& N() const { return *static_cast<T*>(active_); }  \
    T& N() { return *static_cast<T*>(active_); }
    OPENDDS_UNION_ACCESSORS(StringSTypeDefn, string_sdefn);
    OPENDDS_UNION_ACCESSORS(StringLTypeDefn, string_ldefn);
    OPENDDS_UNION_ACCESSORS(PlainSequenceSElemDefn, seq_sdefn);
    OPENDDS_UNION_ACCESSORS(PlainSequenceLElemDefn, seq_ldefn);
    OPENDDS_UNION_ACCESSORS(PlainArraySElemDefn, array_sdefn);
    OPENDDS_UNION_ACCESSORS(PlainArrayLElemDefn, array_ldefn);
    OPENDDS_UNION_ACCESSORS(PlainMapSTypeDefn, map_sdefn);
    OPENDDS_UNION_ACCESSORS(PlainMapLTypeDefn, map_ldefn);
    OPENDDS_UNION_ACCESSORS(StronglyConnectedComponentId, sc_component_id);
    OPENDDS_UNION_ACCESSORS(EquivalenceHash, equivalence_hash);
    OPENDDS_UNION_ACCESSORS(ExtendedTypeDefn, extended_defn);
#undef OPENDDS_UNION_ACCESSORS

    bool operator<(const TypeIdentifier& other) const
    {
      if (kind_ != other.kind_) {
        return kind_ < other.kind_;
      }

      switch (kind_) {
      case TI_STRONGLY_CONNECTED_COMPONENT:
        return sc_component_id() < other.sc_component_id();
      case EK_COMPLETE:
      case EK_MINIMAL:
        return std::memcmp(equivalence_hash(), other.equivalence_hash(), sizeof equivalence_hash()) < 0;
      case TI_STRING8_SMALL:
      case TI_STRING16_SMALL:
        return string_sdefn() < other.string_sdefn();
      case TI_STRING8_LARGE:
      case TI_STRING16_LARGE:
        return string_ldefn() < other.string_ldefn();
      case TI_PLAIN_SEQUENCE_SMALL:
        return seq_sdefn() < other.seq_sdefn();
      case TI_PLAIN_SEQUENCE_LARGE:
        return seq_ldefn() < other.seq_ldefn();
      case TI_PLAIN_ARRAY_SMALL:
        return array_sdefn() < other.array_sdefn();
      case TI_PLAIN_ARRAY_LARGE:
        return array_ldefn() < other.array_ldefn();
      case TI_PLAIN_MAP_SMALL:
        return map_sdefn() < other.map_sdefn();
      case TI_PLAIN_MAP_LARGE:
        return map_ldefn() < other.map_ldefn();

      default:
        return false;
      }
    }

    bool operator==(const TypeIdentifier& other) const
    {
      return !(*this < other) && !(other < *this);
    }

  private:
    ACE_CDR::Octet kind_;
    void* active_;
    union {
      ACE_CDR::ULongLong max_alignment;
#define OPENDDS_UNION_MEMBER(T, N) char N ## _[sizeof(T)]
      OPENDDS_UNION_MEMBER(StringSTypeDefn, string_sdefn);
      OPENDDS_UNION_MEMBER(StringLTypeDefn, string_ldefn);
      OPENDDS_UNION_MEMBER(PlainSequenceSElemDefn, seq_sdefn);
      OPENDDS_UNION_MEMBER(PlainSequenceLElemDefn, seq_ldefn);
      OPENDDS_UNION_MEMBER(PlainArraySElemDefn, array_sdefn);
      OPENDDS_UNION_MEMBER(PlainArrayLElemDefn, array_ldefn);
      OPENDDS_UNION_MEMBER(PlainMapSTypeDefn, map_sdefn);
      OPENDDS_UNION_MEMBER(PlainMapLTypeDefn, map_ldefn);
      OPENDDS_UNION_MEMBER(StronglyConnectedComponentId, sc_component_id);
      OPENDDS_UNION_MEMBER(EquivalenceHash, equivalence_hash);
      OPENDDS_UNION_MEMBER(ExtendedTypeDefn, extended_defn);
#undef OPENDDS_UNION_MEMBER
    };
    void activate(const TypeIdentifier* other = 0);
    void reset();
  };

  typedef Sequence<TypeIdentifier> TypeIdentifierSeq;

  // Operators less-than of member types of TypeIdentifier
  inline bool PlainSequenceSElemDefn::operator<(const PlainSequenceSElemDefn& other) const
  {
    if (header < other.header) return true;
    if (other.header < header) return false;
    if (bound < other.bound) return true;
    if (other.bound < bound) return false;
    if (*element_identifier < *other.element_identifier) return true;
    if (*other.element_identifier < *element_identifier) return false;
    return false;
  }

  inline bool PlainSequenceLElemDefn::operator<(const PlainSequenceLElemDefn& other) const
  {
    if (header < other.header) return true;
    if (other.header < header) return false;
    if (bound < other.bound) return true;
    if (other.bound < bound) return false;
    if (*element_identifier < *other.element_identifier) return true;
    if (*other.element_identifier < *element_identifier) return false;
    return false;
  }

  inline bool PlainArraySElemDefn::operator<(const PlainArraySElemDefn& other) const
  {
    if (header < other.header) return true;
    if (other.header < header) return false;
    if (array_bound_seq < other.array_bound_seq) return true;
    if (other.array_bound_seq < array_bound_seq) return false;
    if (*element_identifier < *other.element_identifier) return true;
    if (*other.element_identifier < *element_identifier) return false;
    return false;
  }

  inline bool PlainArrayLElemDefn::operator<(const PlainArrayLElemDefn& other) const
  {
    if (header < other.header) return true;
    if (other.header < header) return false;
    if (array_bound_seq < other.array_bound_seq) return true;
    if (other.array_bound_seq < array_bound_seq) return false;
    if (*element_identifier < *other.element_identifier) return true;
    if (*other.element_identifier < *element_identifier) return false;
    return false;
  }

  inline bool PlainMapSTypeDefn::operator<(const PlainMapSTypeDefn& other) const
  {
    if (header < other.header) return true;
    if (other.header < header) return false;
    if (bound < other.bound) return true;
    if (other.bound < bound) return false;
    if (*element_identifier < *other.element_identifier) return true;
    if (*other.element_identifier < *element_identifier) return false;
    if (key_flags < other.key_flags) return true;
    if (other.key_flags < key_flags) return false;
    if (*key_identifier < *other.key_identifier) return true;
    if (*other.key_identifier < *key_identifier) return false;
    return false;
  }

  inline bool PlainMapLTypeDefn::operator<(const PlainMapLTypeDefn& other) const
  {
    if (header < other.header) return true;
    if (other.header < header) return false;
    if (bound < other.bound) return true;
    if (other.bound < bound) return false;
    if (*element_identifier < *other.element_identifier) return true;
    if (*other.element_identifier < *element_identifier) return false;
    if (key_flags < other.key_flags) return true;
    if (other.key_flags < key_flags) return false;
    if (*key_identifier < *other.key_identifier) return true;
    if (*other.key_identifier < *key_identifier) return false;
    return false;
  }

  // --- Annotation usage: -----------------------------------------------

  // ID of a type member
  typedef ACE_CDR::ULong MemberId;
  const ACE_CDR::ULong ANNOTATION_STR_VALUE_MAX_LEN = 128;
  const ACE_CDR::ULong ANNOTATION_OCTETSEC_VALUE_MAX_LEN = 128;

  struct ExtendedAnnotationParameterValue {
    // Empty. Available for future extension
  };

  /* Literal value of an annotation member: either the default value in its
   * definition or the value applied in its usage.
   */
  // @extensibility(FINAL) @nested
  // union AnnotationParameterValue switch (octet) {
  // case TK_BOOLEAN:
  //   boolean             boolean_value;
  // case TK_BYTE:
  //   octet               byte_value;
  // case TK_INT16:
  //   short               int16_value;
  // case TK_UINT16:
  //   unsigned short      uint_16_value;
  // case TK_INT32:
  //   long                int32_value;
  // case TK_UINT32:
  //   unsigned long       uint32_value;
  // case TK_INT64:
  //   long long           int64_value;
  // case TK_UINT64:
  //   unsigned long long  uint64_value;
  // case TK_FLOAT32:
  //   float               float32_value;
  // case TK_FLOAT64:
  //   double              float64_value;
  // case TK_FLOAT128:
  //   long double         float128_value;
  // case TK_CHAR8:
  //   char                char_value;
  // case TK_CHAR16:
  //   wchar               wchar_value;
  // case TK_ENUM:
  //   long                enumerated_value;
  // case TK_STRING8:
  //   string<ANNOTATION_STR_VALUE_MAX_LEN>  string8_value;
  // case TK_STRING16:
  //   wstring<ANNOTATION_STR_VALUE_MAX_LEN> string16_value;
  // default:
  //   ExtendedAnnotationParameterValue      extended_value;
  // };

  struct AnnotationParameterValue {
    ACE_CDR::Octet kind;
    ACE_CDR::Boolean boolean_value;
    ACE_CDR::Octet byte_value;
    ACE_CDR::Short int16_value;
    ACE_CDR::UShort uint16_value; // OMG Issue DDSXTY14-46.
    ACE_CDR::Long int32_value;
    ACE_CDR::ULong uint32_value;
    ACE_CDR::LongLong int64_value;
    ACE_CDR::ULongLong uint64_value;
    ACE_CDR::Float float32_value;
    ACE_CDR::Double float64_value;
    ACE_CDR::LongDouble float128_value;
    ACE_CDR::Char char_value;
    ACE_CDR::WChar wchar_value;
    ACE_CDR::Long enumerated_value;
    OPENDDS_STRING string8_value;
    OPENDDS_WSTRING string16_value;
    ExtendedAnnotationParameterValue extended_value;

    struct WCharValue {
      ACE_CDR::WChar value;

      WCharValue() {}

      WCharValue(ACE_CDR::WChar a_value)
        : value(a_value) {}
    };

    struct EnumValue {
      ACE_CDR::Long value;

      EnumValue() {}

      EnumValue(ACE_CDR::Long a_value)
        : value(a_value) {}
    };

    AnnotationParameterValue() {}

    explicit AnnotationParameterValue(ACE_CDR::Boolean value)
      : kind(TK_BOOLEAN)
      , boolean_value(value)
    {}

    explicit AnnotationParameterValue(ACE_CDR::Octet value)
      : kind(TK_BYTE)
      , byte_value(value)
    {}

    explicit AnnotationParameterValue(ACE_CDR::Short value)
      : kind(TK_INT16)
      , int16_value(value)
    {}

    explicit AnnotationParameterValue(ACE_CDR::UShort value)
      : kind(TK_UINT16)
      , uint16_value(value)
    {}

    explicit AnnotationParameterValue(ACE_CDR::Long value)
      : kind(TK_INT32)
      , int32_value(value)
    {}

    explicit AnnotationParameterValue(ACE_CDR::ULong value)
      : kind(TK_UINT32)
      , uint32_value(value)
    {}

    explicit AnnotationParameterValue(ACE_CDR::LongLong value)
      : kind(TK_INT64)
      , int64_value(value)
    {}

    explicit AnnotationParameterValue(ACE_CDR::ULongLong value)
      : kind(TK_UINT64)
      , uint64_value(value)
    {}

    explicit AnnotationParameterValue(ACE_CDR::Float value)
      : kind(TK_FLOAT32)
      , float32_value(value)
    {}

    explicit AnnotationParameterValue(ACE_CDR::Double value)
      : kind(TK_FLOAT64)
      , float64_value(value)
    {}

    explicit AnnotationParameterValue(ACE_CDR::LongDouble value)
      : kind(TK_FLOAT128)
      , float128_value(value)
    {}

    explicit AnnotationParameterValue(ACE_CDR::Char value)
      : kind(TK_CHAR8)
      , char_value(value)
    {}

    explicit AnnotationParameterValue(WCharValue value)
      : kind(TK_CHAR16)
      , wchar_value(value.value)
    {}

    explicit AnnotationParameterValue(EnumValue value)
      : kind(TK_ENUM)
      , enumerated_value(value.value)
    {}

    explicit AnnotationParameterValue(const OPENDDS_STRING& value)
      : kind(TK_STRING8)
      , string8_value(value)
    {}

    explicit AnnotationParameterValue(const OPENDDS_WSTRING& value)
      : kind(TK_STRING16)
      , string16_value(value)
    {}
  };

  // The application of an annotation to some type or type member
  struct AppliedAnnotationParameter {
    NameHash paramname_hash;
    AnnotationParameterValue value;

    AppliedAnnotationParameter() {}

    AppliedAnnotationParameter(ACE_CDR::Octet a, ACE_CDR::Octet b, ACE_CDR::Octet c, ACE_CDR::Octet d, const AnnotationParameterValue& a_value)
      : value(a_value)
    {
      paramname_hash[0] = a;
      paramname_hash[1] = b;
      paramname_hash[2] = c;
      paramname_hash[3] = d;
    }

    AppliedAnnotationParameter(const NameHash& a_name_hash,
                               const AnnotationParameterValue& a_value)
      : value(a_value)
    {
      std::memcpy(&paramname_hash, a_name_hash, sizeof paramname_hash);
    }

    bool operator<(const AppliedAnnotationParameter& other) const
    {
      return std::memcmp(paramname_hash, other.paramname_hash, sizeof paramname_hash) < 0;
    }
  };
  // Sorted by AppliedAnnotationParameter.paramname_hash
  typedef Sequence<AppliedAnnotationParameter> AppliedAnnotationParameterSeq;

  struct AppliedAnnotation {
    TypeIdentifier annotation_typeid;
    Optional<AppliedAnnotationParameterSeq> param_seq;

    AppliedAnnotation() {}

    AppliedAnnotation(const TypeIdentifier& ann_typeid,
                      const Optional<AppliedAnnotationParameterSeq>& a_param_seq)
      : annotation_typeid(ann_typeid)
      , param_seq(a_param_seq)
    {}

    bool operator<(const AppliedAnnotation& other) const
    {
      return annotation_typeid < other.annotation_typeid;
    }
  };
  // Sorted by AppliedAnnotation.annotation_typeid
  typedef Sequence<AppliedAnnotation> AppliedAnnotationSeq;

  // @verbatim(placement="<placement>", language="<lang>", text="<text>")
  struct AppliedVerbatimAnnotation {
    OPENDDS_STRING placement;
    OPENDDS_STRING language;
    OPENDDS_STRING text;

    AppliedVerbatimAnnotation() {}

    AppliedVerbatimAnnotation(const OPENDDS_STRING& a_placement,
                              const OPENDDS_STRING& a_language,
                              const OPENDDS_STRING& a_text)
      : placement(a_placement)
      , language(a_language)
      , text(a_text)
    {}
  };


  // --- Aggregate types: ------------------------------------------------
  struct AppliedBuiltinMemberAnnotations {
    Optional<OPENDDS_STRING> unit; // @unit("<unit>")
    Optional<AnnotationParameterValue> min; // @min , @range
    Optional<AnnotationParameterValue> max; // @max , @range
    Optional<OPENDDS_STRING> hash_id; // @hash_id("<membername>")

    AppliedBuiltinMemberAnnotations() {}

    AppliedBuiltinMemberAnnotations(const Optional<OPENDDS_STRING>& a_unit,
                                    const Optional<AnnotationParameterValue>& a_min,
                                    const Optional<AnnotationParameterValue>& a_max,
                                    const Optional<OPENDDS_STRING>& a_hash_id)
      : unit(a_unit)
      , min(a_min)
      , max(a_max)
      , hash_id(a_hash_id)
    {}
  };

  struct CommonStructMember {
    MemberId member_id;
    StructMemberFlag member_flags;
    TypeIdentifier member_type_id;

    CommonStructMember()
      : member_id(0)
      , member_flags(0)
    {}

    CommonStructMember (const MemberId& a_member_id,
                        const StructMemberFlag& a_member_flags,
                        const TypeIdentifier& a_member_type_id)
      : member_id(a_member_id)
      , member_flags(a_member_flags)
      , member_type_id(a_member_type_id)
    {}
  };

  // COMPLETE Details for a member of an aggregate type
  struct CompleteMemberDetail {
    MemberName name;
    Optional<AppliedBuiltinMemberAnnotations> ann_builtin;
    Optional<AppliedAnnotationSeq> ann_custom;

    CompleteMemberDetail() {}

    CompleteMemberDetail(const MemberName& a_name,
                         const Optional<AppliedBuiltinMemberAnnotations>& an_ann_builtin,
                         const Optional<AppliedAnnotationSeq>& an_ann_custom)
      : name(a_name)
      , ann_builtin(an_ann_builtin)
      , ann_custom(an_ann_custom)
    {}
  };

  // MINIMAL Details for a member of an aggregate type
  struct OpenDDS_Dcps_Export MinimalMemberDetail {
    NameHash name_hash;

    MinimalMemberDetail() {}

    MinimalMemberDetail(ACE_CDR::Octet a, ACE_CDR::Octet b, ACE_CDR::Octet c, ACE_CDR::Octet d)
    {
      name_hash[0] = a; name_hash[1] = b; name_hash[2] = c; name_hash[3] = d;
    }

    explicit MinimalMemberDetail(const NameHash& a_name_hash)
    {
      std::memcpy(&name_hash, &a_name_hash, sizeof name_hash);
    }

    explicit MinimalMemberDetail(const OPENDDS_STRING& name);
  };

  // Member of an aggregate type
  struct CompleteStructMember {
    CommonStructMember common;
    CompleteMemberDetail detail;

    CompleteStructMember() {}

    CompleteStructMember(const CommonStructMember& a_common,
                         const CompleteMemberDetail& a_detail)
      : common(a_common)
      , detail(a_detail)
    {}

    bool operator<(const CompleteStructMember& other) const
    {
      return common.member_id < other.common.member_id;
    }
  };
  // Ordered by the member_index
  typedef Sequence<CompleteStructMember> CompleteStructMemberSeq;

  // Member of an aggregate type
  struct MinimalStructMember {
    CommonStructMember common;
    MinimalMemberDetail detail;

    MinimalStructMember () {}

    MinimalStructMember(const CommonStructMember& a_common,
                        const MinimalMemberDetail& a_detail)
      : common(a_common)
      , detail(a_detail)
    {}

    bool operator<(const MinimalStructMember& other) const
    {
      return common.member_id < other.common.member_id;
    }
  };
  // Ordered by common.member_id
  typedef Sequence<MinimalStructMember> MinimalStructMemberSeq;

  struct AppliedBuiltinTypeAnnotations {
    Optional<AppliedVerbatimAnnotation> verbatim;  // @verbatim(...)

    AppliedBuiltinTypeAnnotations() {}

    explicit AppliedBuiltinTypeAnnotations(const Optional<AppliedVerbatimAnnotation>& a_verbatim)
      : verbatim(a_verbatim)
    {}
  };

  struct MinimalTypeDetail {
    // Empty. Available for future extension
  };

  struct CompleteTypeDetail {
    Optional<AppliedBuiltinTypeAnnotations> ann_builtin;
    Optional<AppliedAnnotationSeq> ann_custom;
    QualifiedTypeName type_name;

    CompleteTypeDetail() {}

    CompleteTypeDetail(const Optional<AppliedBuiltinTypeAnnotations>& an_ann_builtin,
                       const Optional<AppliedAnnotationSeq>& an_ann_custom,
                       const QualifiedTypeName& a_type_name)
      : ann_builtin(an_ann_builtin)
      , ann_custom(an_ann_custom)
      , type_name(a_type_name)
    {}
  };

  struct CompleteStructHeader {
    TypeIdentifier base_type;
    CompleteTypeDetail detail;

    CompleteStructHeader() {}

    CompleteStructHeader(const TypeIdentifier& a_base_type,
                         const CompleteTypeDetail& a_detail)
      : base_type(a_base_type)
      , detail(a_detail)
    {}
  };

  struct MinimalStructHeader {
    TypeIdentifier base_type;
    MinimalTypeDetail detail;

    MinimalStructHeader() {}

    MinimalStructHeader(const TypeIdentifier& a_base_type,
                        const MinimalTypeDetail& a_detail)
      : base_type(a_base_type)
      , detail(a_detail)
    {}
  };

  struct CompleteStructType {
    StructTypeFlag struct_flags;
    CompleteStructHeader header;
    CompleteStructMemberSeq member_seq;

    CompleteStructType() {}

    CompleteStructType(const StructTypeFlag& a_struct_flags,
                       const CompleteStructHeader& a_header,
                       const CompleteStructMemberSeq& a_member_seq)
      : struct_flags(a_struct_flags)
      , header(a_header)
      , member_seq(a_member_seq)
    {}
  };

  struct MinimalStructType {
    StructTypeFlag struct_flags;
    MinimalStructHeader header;
    MinimalStructMemberSeq member_seq;

    MinimalStructType()
      : struct_flags(0)
    {}

    MinimalStructType(const StructTypeFlag& a_struct_flags,
                      const MinimalStructHeader& a_header,
                      const MinimalStructMemberSeq& a_member_seq)
      : struct_flags(a_struct_flags)
      , header(a_header)
      , member_seq(a_member_seq)
    {}
  };

  // --- Union: ----------------------------------------------------------

  // Case labels that apply to a member of a union type
  // Ordered by their values
  typedef Sequence<ACE_CDR::Long> UnionCaseLabelSeq;

  struct CommonUnionMember {
    MemberId member_id;
    UnionMemberFlag member_flags;
    TypeIdentifier type_id;
    UnionCaseLabelSeq label_seq;

    CommonUnionMember()
      : member_id(0)
      , member_flags(0)
    {}

    CommonUnionMember(const MemberId& a_member_id,
                      const UnionMemberFlag& a_member_flags,
                      const TypeIdentifier& a_type_id,
                      const UnionCaseLabelSeq& a_label_seq)
      : member_id(a_member_id)
      , member_flags(a_member_flags)
      , type_id(a_type_id)
      , label_seq(a_label_seq)
    {}
  };

  // Member of a union type
  struct CompleteUnionMember {
    CommonUnionMember common;
    CompleteMemberDetail detail;

    CompleteUnionMember() {}

    CompleteUnionMember(const CommonUnionMember& a_common,
                        const CompleteMemberDetail& a_detail)
      : common(a_common)
      , detail(a_detail)
    {}

    bool operator<(const CompleteUnionMember& other) const
    {
      return common.member_id < other.common.member_id;
    }
  };
  // Ordered by member_index
  typedef Sequence<CompleteUnionMember> CompleteUnionMemberSeq;

  // Member of a union type
  struct MinimalUnionMember {
    CommonUnionMember common;
    MinimalMemberDetail detail;

    MinimalUnionMember() {}

    MinimalUnionMember(const CommonUnionMember& a_common,
                       const MinimalMemberDetail& a_detail)
      : common(a_common)
      , detail(a_detail)
    {}

    bool operator<(const MinimalUnionMember& other) const
    {
      return common.member_id < other.common.member_id;
    }
  };
  // Ordered by MinimalUnionMember.common.member_id
  typedef Sequence<MinimalUnionMember> MinimalUnionMemberSeq;

  struct CommonDiscriminatorMember {
    UnionDiscriminatorFlag member_flags;
    TypeIdentifier type_id;

    CommonDiscriminatorMember()
      : member_flags(0)
    {}

    CommonDiscriminatorMember(const UnionDiscriminatorFlag& a_member_flags,
                              const TypeIdentifier& a_type_id)
      : member_flags(a_member_flags)
      , type_id(a_type_id)
    {}
  };

  // Member of a union type
  struct CompleteDiscriminatorMember {
    CommonDiscriminatorMember common;
    Optional<AppliedBuiltinTypeAnnotations> ann_builtin;
    Optional<AppliedAnnotationSeq> ann_custom;

    CompleteDiscriminatorMember() {}

    CompleteDiscriminatorMember(const CommonDiscriminatorMember& a_common,
                                const Optional<AppliedBuiltinTypeAnnotations>& an_ann_builtin,
                                const Optional<AppliedAnnotationSeq>& an_ann_custom)
      : common(a_common)
      , ann_builtin(an_ann_builtin)
      , ann_custom(an_ann_custom)
    {}
  };

  // Member of a union type
  struct MinimalDiscriminatorMember {
    CommonDiscriminatorMember common;

    MinimalDiscriminatorMember() {}

    explicit MinimalDiscriminatorMember(const CommonDiscriminatorMember& a_common)
      : common(a_common)
    {}
  };

  struct CompleteUnionHeader {
    CompleteTypeDetail detail;

    CompleteUnionHeader() {}

    explicit CompleteUnionHeader(const CompleteTypeDetail& a_detail)
      : detail(a_detail)
    {}
  };

  struct MinimalUnionHeader {
    MinimalTypeDetail detail;

    MinimalUnionHeader() {}

    explicit MinimalUnionHeader(const MinimalTypeDetail& a_detail)
      : detail(a_detail)
    {}
  };

  struct CompleteUnionType {
    UnionTypeFlag union_flags;
    CompleteUnionHeader header;
    CompleteDiscriminatorMember discriminator;
    CompleteUnionMemberSeq member_seq;

    CompleteUnionType()
      : union_flags(0)
    {}

    CompleteUnionType(const UnionTypeFlag& a_union_flags,
                      const CompleteUnionHeader& a_header,
                      const CompleteDiscriminatorMember& a_discriminator,
                      const CompleteUnionMemberSeq& a_member_seq)
      : union_flags(a_union_flags)
      , header(a_header)
      , discriminator(a_discriminator)
      , member_seq(a_member_seq)
    {}
  };

  struct MinimalUnionType {
    UnionTypeFlag union_flags;
    MinimalUnionHeader header;
    MinimalDiscriminatorMember discriminator;
    MinimalUnionMemberSeq member_seq;

    MinimalUnionType()
      : union_flags(0)
    {}

    MinimalUnionType(const UnionTypeFlag& a_union_flags,
                     const MinimalUnionHeader& a_header,
                     const MinimalDiscriminatorMember& a_discriminator,
                     const MinimalUnionMemberSeq& a_member_seq)
      : union_flags(a_union_flags)
      , header(a_header)
      , discriminator(a_discriminator)
      , member_seq(a_member_seq)
    {}
  };

  // --- Annotation: ----------------------------------------------------
  struct CommonAnnotationParameter {
    AnnotationParameterFlag member_flags;
    TypeIdentifier member_type_id;
  };

  // Member of an annotation type
  struct CompleteAnnotationParameter {
    CommonAnnotationParameter common;
    MemberName name;
    AnnotationParameterValue default_value;
  };
  // Ordered by CompleteAnnotationParameter.name
  typedef Sequence<CompleteAnnotationParameter> CompleteAnnotationParameterSeq;

  struct MinimalAnnotationParameter {
    CommonAnnotationParameter common;
    NameHash name_hash;
    AnnotationParameterValue default_value;
  };
  // Ordered by MinimalAnnotationParameter.name_hash
  typedef Sequence<MinimalAnnotationParameter> MinimalAnnotationParameterSeq;

  struct CompleteAnnotationHeader {
    QualifiedTypeName annotation_name;
  };

  struct MinimalAnnotationHeader {
    // Empty. Available for future extension
  };

  struct CompleteAnnotationType {
    AnnotationTypeFlag annotation_flag;
    CompleteAnnotationHeader header;
    CompleteAnnotationParameterSeq member_seq;
  };

  struct MinimalAnnotationType {
    AnnotationTypeFlag annotation_flag;
    MinimalAnnotationHeader header;
    MinimalAnnotationParameterSeq member_seq;
  };

  // --- Alias: ----------------------------------------------------------
  struct CommonAliasBody {
    AliasMemberFlag related_flags;
    TypeIdentifier related_type;

    CommonAliasBody()
      : related_flags(0)
    {}

    CommonAliasBody(const AliasMemberFlag& a_related_flags,
                    const TypeIdentifier& a_related_type)
      : related_flags(a_related_flags)
      , related_type(a_related_type)
    {}
  };

  struct CompleteAliasBody {
    CommonAliasBody common;
    Optional<AppliedBuiltinMemberAnnotations> ann_builtin;
    Optional<AppliedAnnotationSeq> ann_custom;

    CompleteAliasBody() {}

    CompleteAliasBody(const CommonAliasBody& a_common,
                      const Optional<AppliedBuiltinMemberAnnotations>& an_ann_builtin,
                      const Optional<AppliedAnnotationSeq>& an_ann_custom)
      : common(a_common)
      , ann_builtin(an_ann_builtin)
      , ann_custom(an_ann_custom)
    {}
  };

  struct MinimalAliasBody {
    CommonAliasBody common;

    MinimalAliasBody() {}

    explicit MinimalAliasBody(const CommonAliasBody& a_common)
      : common(a_common)
    {}
  };

  struct CompleteAliasHeader {
    CompleteTypeDetail detail;

    CompleteAliasHeader() {}

    explicit CompleteAliasHeader(const CompleteTypeDetail& a_detail)
      : detail(a_detail)
    {}
  };

  struct MinimalAliasHeader {
    // Empty. Available for future extension
  };

  struct CompleteAliasType {
    AliasTypeFlag alias_flags;
    CompleteAliasHeader header;
    CompleteAliasBody body;

    CompleteAliasType()
      : alias_flags(0)
    {}

    CompleteAliasType(const AliasTypeFlag& a_alias_flags,
                      const CompleteAliasHeader& a_header,
                      const CompleteAliasBody& a_body)
      : alias_flags(a_alias_flags)
      , header(a_header)
      , body(a_body)
    {}
  };

  struct MinimalAliasType {
    AliasTypeFlag alias_flags;
    MinimalAliasHeader header;
    MinimalAliasBody body;

    MinimalAliasType()
      : alias_flags(0)
    {}

    MinimalAliasType(const AliasTypeFlag& a_alias_flags,
                     const MinimalAliasHeader& a_header,
                     const MinimalAliasBody& a_body)
      : alias_flags(a_alias_flags)
      , header(a_header)
      , body(a_body)
    {}
  };

  // --- Collections: ----------------------------------------------------
  struct CompleteElementDetail {
    Optional<AppliedBuiltinMemberAnnotations> ann_builtin;
    Optional<AppliedAnnotationSeq> ann_custom;

    CompleteElementDetail() {}

    CompleteElementDetail(const Optional<AppliedBuiltinMemberAnnotations>& an_ann_builtin,
                          const Optional<AppliedAnnotationSeq>& an_ann_custom)
      : ann_builtin(an_ann_builtin)
      , ann_custom(an_ann_custom)
    {}
  };

  struct CommonCollectionElement {
    CollectionElementFlag element_flags;
    TypeIdentifier type;

    CommonCollectionElement()
      : element_flags(0)
    {}

    CommonCollectionElement(CollectionElementFlag a_element_flags,
                            const TypeIdentifier& a_type)
      : element_flags(a_element_flags)
      , type(a_type)
    {}
  };

  struct CompleteCollectionElement {
    CommonCollectionElement common;
    CompleteElementDetail detail;

    CompleteCollectionElement() {}

    CompleteCollectionElement(const CommonCollectionElement& a_common,
                              const CompleteElementDetail& a_detail)
      : common(a_common)
      , detail(a_detail)
    {}
  };

  struct MinimalCollectionElement {
    CommonCollectionElement common;

    MinimalCollectionElement() {}

    explicit MinimalCollectionElement(const CommonCollectionElement& a_common)
      : common(a_common) {}
  };

  struct CommonCollectionHeader {
    LBound bound;

    CommonCollectionHeader()
      : bound(0)
    {}

    explicit CommonCollectionHeader(LBound a_bound) : bound(a_bound) {}
  };

  struct CompleteCollectionHeader {
    CommonCollectionHeader common;
    Optional<CompleteTypeDetail> detail; // not present for anonymous

    CompleteCollectionHeader() {}

    CompleteCollectionHeader(const CommonCollectionHeader& a_common,
                             const Optional<CompleteTypeDetail>& a_detail)
      : common(a_common)
      , detail(a_detail)
    {}
  };

  struct MinimalCollectionHeader {
    CommonCollectionHeader common;

    MinimalCollectionHeader() {}

    explicit MinimalCollectionHeader(const CommonCollectionHeader& a_common)
      : common(a_common) {}
  };

  // --- Sequence: ------------------------------------------------------
  struct CompleteSequenceType {
    CollectionTypeFlag collection_flag;
    CompleteCollectionHeader header;
    CompleteCollectionElement element;

    CompleteSequenceType()
      : collection_flag(0)
      , header()
      , element()
    {}

    CompleteSequenceType(CollectionTypeFlag a_collection_flag,
                         const CompleteCollectionHeader& a_header,
                         const CompleteCollectionElement& an_element)
      : collection_flag(a_collection_flag)
      , header(a_header)
      , element(an_element)
  };

  struct MinimalSequenceType {
    CollectionTypeFlag collection_flag;
    MinimalCollectionHeader header;
    MinimalCollectionElement element;

    MinimalSequenceType()
      : collection_flag(0)
      , header()
      , element()
    {}

    MinimalSequenceType(CollectionTypeFlag a_collection_flag,
                        const MinimalCollectionHeader& a_header,
                        const MinimalCollectionElement& a_element)
      : collection_flag(a_collection_flag)
      , header(a_header)
      , element(a_element)
    {}
  };

  // --- Array: ------------------------------------------------------
  struct CommonArrayHeader {
    LBoundSeq bound_seq;

    CommonArrayHeader() {}

    explicit CommonArrayHeader(const LBoundSeq& a_bound_seq)
      : bound_seq(a_bound_seq) {}
  };

  struct CompleteArrayHeader {
    CommonArrayHeader common;
    CompleteTypeDetail detail;

    CompleteArrayHeader() {}

    CompleteArrayHeader(const CommonArrayHeader& a_common,
                        const CompleteTypeDetail& a_detail)
      : common(a_common)
      , detail(a_detail)
    {}
  };

  struct MinimalArrayHeader {
    CommonArrayHeader common;

    MinimalArrayHeader() {}

    explicit MinimalArrayHeader(const CommonArrayHeader& a_common)
      : common(a_common)
    {}
  };

  struct CompleteArrayType  {
    CollectionTypeFlag collection_flag;
    CompleteArrayHeader header;
    CompleteCollectionElement element;

    CompleteArrayType()
      : collection_flag(0)
      , header()
      , element()
    {}

    CompleteArrayType(CollectionTypeFlag a_collection_flag,
                      const CompleteArrayHeader& a_header,
                      const CompleteCollectionElement& an_element)
      : collection_flag(a_collection_flag)
      , header(a_header)
      , element(an_element)
    {}
  };

  struct MinimalArrayType  {
    CollectionTypeFlag collection_flag;
    MinimalArrayHeader header;
    MinimalCollectionElement element;

    MinimalArrayType()
      : collection_flag(0)
      , header()
      , element()
    {}

    MinimalArrayType(CollectionTypeFlag a_collection_flag,
                     const MinimalArrayHeader& a_header,
                     const MinimalCollectionElement& a_element)
      : collection_flag(a_collection_flag)
      , header(a_header)
      , element(a_element)
    {}
  };

  // --- Map: ------------------------------------------------------
  struct CompleteMapType {
    CollectionTypeFlag collection_flag;
    CompleteCollectionHeader header;
    CompleteCollectionElement key;
    CompleteCollectionElement element;
  };

  struct MinimalMapType {
    CollectionTypeFlag collection_flag;
    MinimalCollectionHeader header;
    MinimalCollectionElement key;
    MinimalCollectionElement element;
  };

  // --- Enumeration: ----------------------------------------------------
  typedef ACE_CDR::UShort BitBound;

  // Constant in an enumerated type
  struct CommonEnumeratedLiteral {
    ACE_CDR::Long value;
    EnumeratedLiteralFlag flags;

    CommonEnumeratedLiteral()
      : value(0)
      , flags(0)
    {}

    CommonEnumeratedLiteral(ACE_CDR::Long a_value,
                            EnumeratedLiteralFlag a_flags)
      : value(a_value)
      , flags(a_flags)
    {}
  };

  // Constant in an enumerated type
  struct CompleteEnumeratedLiteral {
    CommonEnumeratedLiteral common;
    CompleteMemberDetail detail;

    CompleteEnumeratedLiteral() {}

    CompleteEnumeratedLiteral(const CommonEnumeratedLiteral& a_common,
                              const CompleteMemberDetail& a_detail)
      : common(a_common)
      , detail(a_detail)
    {}

    bool operator<(const CompleteEnumeratedLiteral& other) const
    {
      return common.value < other.common.value;
    }
  };
  // Ordered by EnumeratedLiteral.common.value
  typedef Sequence<CompleteEnumeratedLiteral> CompleteEnumeratedLiteralSeq;

  // Constant in an enumerated type
  struct MinimalEnumeratedLiteral {
    CommonEnumeratedLiteral common;
    MinimalMemberDetail detail;

    MinimalEnumeratedLiteral() {}

    MinimalEnumeratedLiteral(const CommonEnumeratedLiteral& a_common,
                             const MinimalMemberDetail& a_detail)
      : common(a_common)
      , detail(a_detail)
    {}

    bool operator<(const MinimalEnumeratedLiteral& other) const
    {
      return common.value < other.common.value;
    }
  };
  // Ordered by EnumeratedLiteral.common.value
  typedef Sequence<MinimalEnumeratedLiteral> MinimalEnumeratedLiteralSeq;

  struct CommonEnumeratedHeader {
    BitBound bit_bound;

    CommonEnumeratedHeader()
      : bit_bound(0)
    {}

    explicit CommonEnumeratedHeader(BitBound a_bit_bound)
      : bit_bound(a_bit_bound)
    {}
  };

  struct CompleteEnumeratedHeader {
    CommonEnumeratedHeader common;
    CompleteTypeDetail detail;

    CompleteEnumeratedHeader() {}

    CompleteEnumeratedHeader(const CommonEnumeratedHeader& a_common,
                             const CompleteTypeDetail& a_detail)
      : common(a_common)
      , detail(a_detail)
    {}
  };

  struct MinimalEnumeratedHeader {
    CommonEnumeratedHeader common;

    MinimalEnumeratedHeader() {}

    explicit MinimalEnumeratedHeader(const CommonEnumeratedHeader& a_common)
      : common(a_common)
    {}
  };

  // Enumerated type
  struct CompleteEnumeratedType  {
    EnumTypeFlag enum_flags; // unused
    CompleteEnumeratedHeader header;
    CompleteEnumeratedLiteralSeq literal_seq;

    CompleteEnumeratedType()
      : enum_flags(0)
    {}

    CompleteEnumeratedType(const EnumTypeFlag& a_enum_flags,
                           const CompleteEnumeratedHeader& a_header,
                           const CompleteEnumeratedLiteralSeq& a_literal_seq)
      : enum_flags(a_enum_flags)
      , header(a_header)
      , literal_seq(a_literal_seq)
    {}
  };

  // Enumerated type
  struct MinimalEnumeratedType  {
    EnumTypeFlag enum_flags; // unused
    MinimalEnumeratedHeader header;
    MinimalEnumeratedLiteralSeq literal_seq;

    MinimalEnumeratedType()
      : enum_flags(0)
    {}

    MinimalEnumeratedType(const EnumTypeFlag& a_enum_flags,
                          const MinimalEnumeratedHeader& a_header,
                          const MinimalEnumeratedLiteralSeq& a_literal_seq)
      : enum_flags(a_enum_flags)
      , header(a_header)
      , literal_seq(a_literal_seq)
    {}
  };

  // --- Bitmask: --------------------------------------------------------
  // Bit in a bit mask
  struct CommonBitflag {
    ACE_CDR::UShort position;
    BitflagFlag flags;
  };

  struct CompleteBitflag {
    CommonBitflag common;
    CompleteMemberDetail detail;
  };
  // Ordered by Bitflag.position
  typedef Sequence<CompleteBitflag> CompleteBitflagSeq;

  struct MinimalBitflag {
    CommonBitflag common;
    MinimalMemberDetail detail;
  };
  // Ordered by Bitflag.position
  typedef Sequence<MinimalBitflag> MinimalBitflagSeq;

  struct CommonBitmaskHeader {
    BitBound bit_bound;
  };

  typedef CompleteEnumeratedHeader CompleteBitmaskHeader;

  typedef MinimalEnumeratedHeader MinimalBitmaskHeader;

  struct CompleteBitmaskType {
    BitmaskTypeFlag bitmask_flags; // unused
    CompleteBitmaskHeader header;
    CompleteBitflagSeq flag_seq;
  };

  struct MinimalBitmaskType {
    BitmaskTypeFlag bitmask_flags; // unused
    MinimalBitmaskHeader header;
    MinimalBitflagSeq flag_seq;
  };

  // --- Bitset: ----------------------------------------------------------
  struct CommonBitfield {
    ACE_CDR::UShort position;
    BitsetMemberFlag flags;
    ACE_CDR::Octet bitcount;
    TypeKind holder_type; // Must be primitive integer type
  };

  struct CompleteBitfield {
    CommonBitfield common;
    CompleteMemberDetail detail;
  };
  // Ordered by Bitfield.position
  typedef Sequence<CompleteBitfield> CompleteBitfieldSeq;

  struct MinimalBitfield {
    CommonBitfield common;
    NameHash name_hash;
  };
  // Ordered by Bitfield.position
  typedef Sequence<MinimalBitfield> MinimalBitfieldSeq;

  struct CompleteBitsetHeader {
    CompleteTypeDetail detail;
  };

  struct MinimalBitsetHeader {
    // Empty. Available for future extension
  };

  struct CompleteBitsetType  {
    BitsetTypeFlag bitset_flags; // unused
    CompleteBitsetHeader header;
    CompleteBitfieldSeq field_seq;
  };

  struct MinimalBitsetType  {
    BitsetTypeFlag bitset_flags; // unused
    MinimalBitsetHeader header;
    MinimalBitfieldSeq field_seq;
  };

  // --- Type Object: ---------------------------------------------------
  // The types associated with each case selection must have extensibility
  // kind APPENDABLE or MUTABLE so that they can be extended in the future

  struct CompleteExtendedType {
    // Empty. Available for future extension
  };

  // @extensibility(FINAL)     @nested
  // union CompleteTypeObject switch (octet) {
  // case TK_ALIAS:
  //   CompleteAliasType      alias_type;
  // case TK_ANNOTATION:
  //   CompleteAnnotationType annotation_type;
  // case TK_STRUCTURE:
  //   CompleteStructType     struct_type;
  // case TK_UNION:
  //   CompleteUnionType      union_type;
  // case TK_BITSET:
  //   CompleteBitsetType     bitset_type;
  // case TK_SEQUENCE:
  //   CompleteSequenceType   sequence_type;
  // case TK_ARRAY:
  //   CompleteArrayType      array_type;
  // case TK_MAP:
  //   CompleteMapType        map_type;
  // case TK_ENUM:
  //   CompleteEnumeratedType enumerated_type;
  // case TK_BITMASK:
  //   CompleteBitmaskType    bitmask_type;

  //   // ===================  Future extensibility  ============
  // default:
  //   CompleteExtendedType   extended_type;
  // };

  struct CompleteTypeObject {
    ACE_CDR::Octet kind;
    CompleteAliasType alias_type;
    CompleteAnnotationType annotation_type;
    CompleteStructType struct_type;
    CompleteUnionType union_type;
    CompleteBitsetType bitset_type;
    CompleteSequenceType sequence_type;
    CompleteArrayType array_type;
    CompleteMapType map_type;
    CompleteEnumeratedType enumerated_type;
    CompleteBitmaskType bitmask_type;

    // ===================  Future extensibility  ============
    CompleteExtendedType extended_type;

    CompleteTypeObject() {}

    explicit CompleteTypeObject(const CompleteAliasType& alias)
      : kind(TK_ALIAS)
      , alias_type(alias)
    {}

    explicit CompleteTypeObject(const CompleteAnnotationType& annotation)
      : kind(TK_ANNOTATION)
      , annotation_type(annotation)
    {}

    explicit CompleteTypeObject(const CompleteStructType& struct_)
      : kind(TK_STRUCTURE)
      , struct_type(struct_)
    {}

    explicit CompleteTypeObject(const CompleteUnionType& union_)
      : kind(TK_UNION)
      , union_type(union_)
    {}

    explicit CompleteTypeObject(const CompleteBitsetType& bitset)
      : kind(TK_BITSET)
      , bitset_type(bitset)
    {}

    explicit CompleteTypeObject(const CompleteSequenceType& sequence)
      : kind(TK_SEQUENCE)
      , sequence_type(sequence)
    {}

    explicit CompleteTypeObject(const CompleteArrayType& array)
      : kind(TK_ARRAY)
      , array_type(array)
    {}

    explicit CompleteTypeObject(const CompleteMapType& map)
      : kind(TK_MAP)
      , map_type(map)
    {}

    explicit CompleteTypeObject(const CompleteEnumeratedType& enum_)
      : kind(TK_ENUM)
      , enumerated_type(enum_)
    {}

    explicit CompleteTypeObject(const CompleteBitmaskType& bitmask)
      : kind(TK_BITMASK)
      , bitmask_type(bitmask)
    {}
  };

  struct MinimalExtendedType {
    // Empty. Available for future extension
  };

  // @extensibility(FINAL)     @nested
  // union MinimalTypeObject switch (octet) {
  // case TK_ALIAS:
  //   MinimalAliasType       alias_type;
  // case TK_ANNOTATION:
  //   MinimalAnnotationType  annotation_type;
  // case TK_STRUCTURE:
  //   MinimalStructType      struct_type;
  // case TK_UNION:
  //   MinimalUnionType       union_type;
  // case TK_BITSET:
  //   MinimalBitsetType      bitset_type;
  // case TK_SEQUENCE:
  //   MinimalSequenceType    sequence_type;
  // case TK_ARRAY:
  //   MinimalArrayType       array_type;
  // case TK_MAP:
  //   MinimalMapType         map_type;
  // case TK_ENUM:
  //   MinimalEnumeratedType  enumerated_type;
  // case TK_BITMASK:
  //   MinimalBitmaskType     bitmask_type;

  //   // ===================  Future extensibility  ============
  // default:
  //   MinimalExtendedType    extended_type;
  // };

  struct MinimalTypeObject {
    ACE_CDR::Octet kind;
    MinimalAliasType alias_type;
    MinimalAnnotationType annotation_type;
    MinimalStructType struct_type;
    MinimalUnionType union_type;
    MinimalBitsetType bitset_type;
    MinimalSequenceType sequence_type;
    MinimalArrayType array_type;
    MinimalMapType map_type;
    MinimalEnumeratedType enumerated_type;
    MinimalBitmaskType bitmask_type;

    // ===================  Future extensibility  ============
    MinimalExtendedType extended_type;

    MinimalTypeObject()
      : kind(TK_NONE)
    {}

    explicit MinimalTypeObject(const MinimalAliasType& alias)
      : kind(TK_ALIAS)
      , alias_type(alias)
    {}

    explicit MinimalTypeObject(const MinimalAnnotationType& annotation)
      : kind(TK_ANNOTATION)
      , annotation_type(annotation)
    {}

    explicit MinimalTypeObject(const MinimalStructType& struct_)
      : kind(TK_STRUCTURE)
      , struct_type(struct_)
    {}

    explicit MinimalTypeObject(const MinimalUnionType& union_)
      : kind(TK_UNION)
      , union_type(union_)
    {}

    explicit MinimalTypeObject(const MinimalBitsetType& bitset)
      : kind(TK_BITSET)
      , bitset_type(bitset)
    {}

    explicit MinimalTypeObject(const MinimalSequenceType& sequence)
      : kind(TK_SEQUENCE)
      , sequence_type(sequence)
    {}

    explicit MinimalTypeObject(const MinimalArrayType& array)
      : kind(TK_ARRAY)
      , array_type(array)
    {}

    explicit MinimalTypeObject(const MinimalMapType& map)
      : kind(TK_MAP)
      , map_type(map)
    {}

    explicit MinimalTypeObject(const MinimalEnumeratedType& enum_)
      : kind(TK_ENUM)
      , enumerated_type(enum_)
    {}

    explicit MinimalTypeObject(const MinimalBitmaskType& bitmask)
      : kind(TK_BITMASK)
      , bitmask_type(bitmask)
    {}
  };

  // @extensibility(APPENDABLE)  @nested
  // union TypeObject switch (octet) { // EquivalenceKind
  // case EK_COMPLETE:
  //   CompleteTypeObject   complete;
  // case EK_MINIMAL:
  //   MinimalTypeObject    minimal;
  // };

  struct TypeObject {
    ACE_CDR::Octet kind;
    CompleteTypeObject complete;
    MinimalTypeObject minimal;

    TypeObject()
      : kind(0)
    {}

    explicit TypeObject(const CompleteTypeObject& a_complete)
      : kind(EK_COMPLETE)
      , complete(a_complete)
    {}

    explicit TypeObject(const MinimalTypeObject& a_minimal)
      : kind(EK_MINIMAL)
      , minimal(a_minimal)
    {}
  };

  typedef Sequence<TypeObject> TypeObjectSeq;

  // Set of TypeObjects representing a strong component: Equivalence class
  // for the Strong Connectivity relationship (mutual reachability between
  // types).
  // Ordered by fully qualified typename lexicographic order
  typedef TypeObjectSeq StronglyConnectedComponent;

  struct TypeIdentifierTypeObjectPair {
    TypeIdentifier type_identifier;
    TypeObject type_object;

    TypeIdentifierTypeObjectPair() {}

    TypeIdentifierTypeObjectPair(const TypeIdentifier& ti, const TypeObject& to)
      : type_identifier(ti)
      , type_object(to)
    {}
  };
  typedef Sequence<TypeIdentifierTypeObjectPair> TypeIdentifierTypeObjectPairSeq;

  struct TypeIdentifierPair {
    TypeIdentifier type_identifier1;
    TypeIdentifier type_identifier2;

    TypeIdentifierPair() {}

    TypeIdentifierPair(const TypeIdentifier& t1, const TypeIdentifier& t2)
      : type_identifier1(t1)
      , type_identifier2(t2)
    {}
  };
  typedef Sequence<TypeIdentifierPair> TypeIdentifierPairSeq;

  struct TypeIdentifierWithSize {
    TypeIdentifier type_id;
    ACE_CDR::ULong typeobject_serialized_size;
  };
  typedef Sequence<TypeIdentifierWithSize> TypeIdentifierWithSizeSeq;

  struct TypeIdentifierWithDependencies {
    TypeIdentifierWithSize typeid_with_size;
    // The total additional types related to minimal_type
    ACE_CDR::Long dependent_typeid_count;
    TypeIdentifierWithSizeSeq dependent_typeids;
  };

  typedef Sequence<TypeIdentifierWithDependencies> TypeIdentifierWithDependenciesSeq;

  // This appears in the builtin DDS topics PublicationBuiltinTopicData
  // and SubscriptionBuiltinTopicData

  struct TypeInformation {
    TypeIdentifierWithDependencies minimal;
    TypeIdentifierWithDependencies complete;
  };

  OpenDDS_Dcps_Export
  TypeIdentifier makeTypeIdentifier(const TypeObject& type_object,
                                    const DCPS::Encoding* encoding_option = 0);

  template <typename T>
  void serialize_type_info(const TypeInformation& type_info, T& seq,
                           const DCPS::Encoding* encoding_option = 0)
  {
    const DCPS::Encoding& encoding = encoding_option ? *encoding_option : get_typeobject_encoding();
    const size_t sz = DCPS::serialized_size(encoding, type_info);
    seq.length(static_cast<unsigned>(sz));
    DCPS::MessageBlockHelper<T> helper(seq);
    DCPS::Serializer serializer(helper, encoding);
    if (!(serializer << type_info)) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: serialize_type_info ")
                 ACE_TEXT("serialization of type information failed.\n")));
    }
  }

  template <typename T>
  bool deserialize_type_info(TypeInformation& type_info, const T& seq)
  {
    DCPS::MessageBlockHelper<T> helper(seq);
    DCPS::Serializer serializer(helper, XTypes::get_typeobject_encoding());
    if (!(serializer >> type_info)) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("(%P|%t) ERROR: deserialize_type_info ")
                 ACE_TEXT("deserialization of type information failed.\n")));
      return false;
    }
    return true;
  }

  OpenDDS_Dcps_Export
  ACE_CDR::ULong hash_member_name_to_id(const OPENDDS_STRING& name);

  OpenDDS_Dcps_Export
  void hash_member_name(NameHash& name_hash, const OPENDDS_STRING& name);

  OpenDDS_Dcps_Export
  bool is_fully_descriptive(const TypeIdentifier& ti);

  OpenDDS_Dcps_Export
  bool is_plain_collection(const TypeIdentifier& ti);

  OpenDDS_Dcps_Export
  bool has_type_object(const TypeIdentifier& ti);

  typedef OPENDDS_MAP(TypeIdentifier, TypeObject) TypeMap;

  struct TypeMapBuilder {
    TypeMap type_map_;

    TypeMapBuilder& insert(const TypeIdentifier& ti, const TypeObject& to)
    {
      type_map_[ti] = to;
      return *this;
    }

    operator TypeMap&() { return type_map_; }
  };

  void compute_dependencies(const TypeMap& type_map,
                            const TypeIdentifier& type_identifier,
                            OPENDDS_SET(TypeIdentifier)& dependencies);

} // namespace XTypes

namespace DCPS {

template<typename T>
const XTypes::TypeIdentifier& getMinimalTypeIdentifier();

template<typename T>
const XTypes::TypeMap& getMinimalTypeMap();

template<typename T>
const XTypes::TypeIdentifier& getCompleteTypeIdentifier();

template<typename T>
const XTypes::TypeMap& getCompleteTypeMap();

template<typename T>
void serialized_size(const Encoding& encoding, size_t& size,
                     const XTypes::Optional<T>& opt)
{
  size += DCPS::boolean_cdr_size;
  if (opt.present) {
    serialized_size(encoding, size, opt.value);
  }
}

template<typename T>
bool operator<<(Serializer& strm, const XTypes::Optional<T>& opt)
{
  if (!(strm << ACE_OutputCDR::from_boolean(opt.present))) {
    return false;
  }
  return !opt.present || strm << opt.value;
}

template<typename T>
bool operator>>(Serializer& strm, XTypes::Optional<T>& opt)
{
  if (!(strm >> ACE_InputCDR::to_boolean(opt.present))) {
    return false;
  }
  return !opt.present || strm >> opt.value;
}


// XCDR2 encoding rule 12 - Sequences not "of primitive element type"

template<typename T>
void serialized_size(const Encoding& encoding, size_t& size,
                     const XTypes::Sequence<T>& seq)
{
  if (!encoding.skip_sequence_dheader()) {
    serialized_size_delimiter(encoding, size);
  }
  primitive_serialized_size_ulong(encoding, size);
  for (ACE_CDR::ULong i = 0; i < seq.length(); ++i) {
    serialized_size(encoding, size, seq[i]);
  }
}

template<typename T>
bool operator<<(Serializer& strm, const XTypes::Sequence<T>& seq)
{
  if (!strm.encoding().skip_sequence_dheader()) {
    size_t total_size = 0;
    serialized_size(strm.encoding(), total_size, seq);
    if (!strm.write_delimiter(total_size)) {
      return false;
    }
  }
  const ACE_CDR::ULong length = seq.length();
  if (!(strm << length)) {
    return false;
  }
  for (ACE_CDR::ULong i = 0; i < length; ++i) {
    if (!(strm << seq[i])) {
      return false;
    }
  }
  return true;
}

template<typename T>
bool operator>>(Serializer& strm, XTypes::Sequence<T>& seq)
{
  size_t total_size = 0;
  if (!strm.read_delimiter(total_size)) {
    return false;
  }

  // special cases for compatibility with older versions that encoded this
  // sequence incorrectly - if the DHeader was read as a 0, it's an empty
  // sequence although it should have been encoded as DHeader (4) + Length (0)
  if (total_size == 0) {
    seq.length(0);
    return true;
  }

  if (total_size < 4) {
    return false;
  }

  const size_t end_of_seq = strm.rpos() + total_size;
  ACE_CDR::ULong length;
  if (!(strm >> length)) {
    return false;
  }

  if (length > strm.length()) {
    // if encoded incorrectly, the first 4 bytes of the elements were read
    // as if they were the length - this may end up being larger than the
    // number of bytes remaining in the Serializer
    return false;
  }

  seq.length(length);
  for (ACE_CDR::ULong i = 0; i < length; ++i) {
    if (!(strm >> seq[i])) {
      return false;
    }
  }
  return strm.skip(end_of_seq - strm.rpos());
}


// non-template overloads for sequences of basic types:
// XCDR2 encoding rule 11 - Sequences of primitive element type

OpenDDS_Dcps_Export
void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::LBoundSeq& seq);
OpenDDS_Dcps_Export
bool operator<<(Serializer& strm, const XTypes::LBoundSeq& seq);
OpenDDS_Dcps_Export
bool operator>>(Serializer& strm, XTypes::LBoundSeq& seq);

OpenDDS_Dcps_Export
void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::SBoundSeq& seq);
OpenDDS_Dcps_Export
bool operator<<(Serializer& strm, const XTypes::SBoundSeq& seq);
OpenDDS_Dcps_Export
bool operator>>(Serializer& strm, XTypes::SBoundSeq& seq);

OpenDDS_Dcps_Export
void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::UnionCaseLabelSeq& seq);
OpenDDS_Dcps_Export
bool operator<<(Serializer& strm, const XTypes::UnionCaseLabelSeq& seq);
OpenDDS_Dcps_Export
bool operator>>(Serializer& strm, XTypes::UnionCaseLabelSeq& seq);


inline void serialized_size(const Encoding&, size_t&, const XTypes::MinimalTypeDetail&)
{}
inline bool operator<<(Serializer&, const XTypes::MinimalTypeDetail&) { return true; }
inline bool operator>>(Serializer&, XTypes::MinimalTypeDetail&) { return true; }

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::ExtendedAnnotationParameterValue& stru);
bool operator<<(Serializer& strm,
  const XTypes::ExtendedAnnotationParameterValue& stru);
bool operator>>(Serializer& strm,
  XTypes::ExtendedAnnotationParameterValue& stru);

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::NameHash_forany& arr);
bool operator<<(Serializer& ser, const XTypes::NameHash_forany& arr);
bool operator>>(Serializer& ser, XTypes::NameHash_forany& arr);

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::EquivalenceHash_forany& arr);
bool operator<<(Serializer& ser, const XTypes::EquivalenceHash_forany& arr);
bool operator>>(Serializer& ser, XTypes::EquivalenceHash_forany& arr);

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CompleteTypeDetail& stru);
bool operator<<(Serializer& ser, const XTypes::CompleteTypeDetail& stru);
bool operator>>(Serializer& ser, XTypes::CompleteTypeDetail& stru);

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CompleteStructHeader& stru);
bool operator<<(Serializer& ser, const XTypes::CompleteStructHeader& stru);
bool operator>>(Serializer& ser, XTypes::CompleteStructHeader& stru);

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::MinimalStructHeader& stru);
bool operator<<(Serializer& ser, const XTypes::MinimalStructHeader& stru);
bool operator>>(Serializer& ser, XTypes::MinimalStructHeader& stru);

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CompleteStructType& stru);
bool operator<<(Serializer& ser, const XTypes::CompleteStructType& stru);
bool operator>>(Serializer& ser, XTypes::CompleteStructType& stru);

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::MinimalStructType& stru);
bool operator<<(Serializer& ser, const XTypes::MinimalStructType& stru);
bool operator>>(Serializer& ser, XTypes::MinimalStructType& stru);

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CompleteUnionType& stru);
bool operator<<(Serializer& ser, const XTypes::CompleteUnionType& stru);
bool operator>>(Serializer& ser, XTypes::CompleteUnionType& stru);

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::MinimalUnionType& stru);
bool operator<<(Serializer& ser, const XTypes::MinimalUnionType& stru);
bool operator>>(Serializer& ser, XTypes::MinimalUnionType& stru);

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CompleteAnnotationType& stru);
bool operator<<(Serializer& ser, const XTypes::CompleteAnnotationType& stru);
bool operator>>(Serializer& ser, XTypes::CompleteAnnotationType& stru);

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::MinimalAnnotationType& stru);
bool operator>>(Serializer& ser, const XTypes::MinimalAnnotationType& stru);
bool operator<<(Serializer& ser, XTypes::MinimalAnnotationType& stru);

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CompleteAliasType& stru);
bool operator>>(Serializer& ser, const XTypes::CompleteAliasType& stru);
bool operator<<(Serializer& ser, XTypes::CompleteAliasType& stru);

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::MinimalAliasType& stru);
bool operator>>(Serializer& ser, const XTypes::MinimalAliasType& stru);
bool operator<<(Serializer& ser, XTypes::MinimalAliasType& stru);

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CompleteSequenceType& stru);
bool operator<<(Serializer& ser, const XTypes::CompleteSequenceType& stru);
bool operator>>(Serializer& ser, XTypes::CompleteSequenceType& stru);

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::MinimalSequenceType& stru);
bool operator<<(Serializer& ser, const XTypes::MinimalSequenceType& stru);
bool operator>>(Serializer& ser, XTypes::MinimalSequenceType& stru);

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CompleteArrayType& stru);
bool operator<<(Serializer& ser, const XTypes::CompleteArrayType& stru);
bool operator>>(Serializer& ser, XTypes::CompleteArrayType& stru);

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::MinimalArrayType& stru);
bool operator<<(Serializer& ser, const XTypes::MinimalArrayType& stru);
bool operator>>(Serializer& ser, XTypes::MinimalArrayType& stru);

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CompleteMapType& stru);
bool operator<<(Serializer& ser, const XTypes::CompleteMapType& stru);
bool operator>>(Serializer& ser, XTypes::CompleteMapType& stru);

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::MinimalMapType& stru);
bool operator<<(Serializer& ser, const XTypes::MinimalMapType& stru);
bool operator>>(Serializer& ser, XTypes::MinimalMapType& stru);

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CompleteEnumeratedHeader& stru);
bool operator<<(Serializer& ser, const XTypes::CompleteEnumeratedHeader& stru);
bool operator>>(Serializer& ser, XTypes::CompleteEnumeratedHeader& stru);

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::MinimalEnumeratedHeader& stru);
bool operator<<(Serializer& ser, const XTypes::MinimalEnumeratedHeader& stru);
bool operator>>(Serializer& ser, XTypes::MinimalEnumeratedHeader& stru);

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CompleteEnumeratedType& stru);
bool operator<<(Serializer& ser, const XTypes::CompleteEnumeratedType& stru);
bool operator>>(Serializer& ser, XTypes::CompleteEnumeratedType& stru);

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::MinimalEnumeratedType& stru);
bool operator<<(Serializer& ser, const XTypes::MinimalEnumeratedType& stru);
bool operator>>(Serializer& ser, XTypes::MinimalEnumeratedType& stru);

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::MinimalBitmaskType& stru);
bool operator<<(Serializer& ser, const XTypes::MinimalBitmaskType& stru);
bool operator>>(Serializer& ser, XTypes::MinimalBitmaskType& stru);

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CompleteBitmaskType& stru);
bool operator<<(Serializer& ser, const XTypes::CompleteBitmaskType& stru);
bool operator>>(Serializer& ser, XTypes::CompleteBitmaskType& stru);

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CompleteBitsetType& stru);
bool operator<<(Serializer& ser, const XTypes::CompleteBitsetType& stru);
bool operator>>(Serializer& ser, XTypes::CompleteBitsetType& stru);

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::MinimalBitsetType& stru);
bool operator<<(Serializer& ser, const XTypes::MinimalBitsetType& stru);
bool operator>>(Serializer& ser, XTypes::MinimalBitsetType& stru);

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CompleteExtendedType& stru);
bool operator<<(Serializer& strm, const XTypes::CompleteExtendedType& stru);
bool operator>>(Serializer& strm, XTypes::CompleteExtendedType& stru);

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CompleteTypeObject& type_object);
bool operator<<(Serializer& ser, const XTypes::CompleteTypeObject& type_object);
bool operator>>(Serializer& ser, XTypes::CompleteTypeObject& type_object);

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::MinimalExtendedType& stru);
bool operator<<(Serializer& strm, const XTypes::MinimalExtendedType& stru);
bool operator>>(Serializer& strm, XTypes::MinimalExtendedType& stru);

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::MinimalTypeObject& type_object);
bool operator<<(Serializer& ser, const XTypes::MinimalTypeObject& type_object);
bool operator>>(Serializer& ser, XTypes::MinimalTypeObject& type_object);

OpenDDS_Dcps_Export
void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::TypeObject& type_object);

OpenDDS_Dcps_Export
bool operator<<(Serializer& ser, const XTypes::TypeObject& type_object);

OpenDDS_Dcps_Export
bool operator>>(Serializer& ser, XTypes::TypeObject& type_object);

OpenDDS_Dcps_Export
void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::TypeInformation& type_info);

OpenDDS_Dcps_Export
void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::TypeIdentifier& stru);
OpenDDS_Dcps_Export
bool operator<<(Serializer& ser, const XTypes::TypeIdentifier& stru);
OpenDDS_Dcps_Export
bool operator>>(Serializer& ser, XTypes::TypeIdentifier& stru);

OpenDDS_Dcps_Export
void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::TypeIdentifierWithSize& stru);
OpenDDS_Dcps_Export
bool operator<<(Serializer& ser, const XTypes::TypeIdentifierWithSize& stru);
OpenDDS_Dcps_Export
bool operator>>(Serializer& ser, XTypes::TypeIdentifierWithSize& stru);

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::TypeIdentifierWithDependencies& stru);
bool operator<<(Serializer& ser, const XTypes::TypeIdentifierWithDependencies& stru);
bool operator>>(Serializer& ser, XTypes::TypeIdentifierWithDependencies& stru);

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::AppliedAnnotation& stru);
bool operator<<(Serializer& ser, const XTypes::AppliedAnnotation& stru);
bool operator>>(Serializer& ser, XTypes::AppliedAnnotation& stru);

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::AppliedBuiltinTypeAnnotations& stru);
bool operator<<(Serializer& ser, const XTypes::AppliedBuiltinTypeAnnotations& stru);
bool operator>>(Serializer& ser, XTypes::AppliedBuiltinTypeAnnotations& stru);

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CompleteAliasBody& stru);
bool operator<<(Serializer& ser, const XTypes::CompleteAliasBody& stru);
bool operator>>(Serializer& ser, XTypes::CompleteAliasBody& stru);

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CompleteAliasHeader& stru);
bool operator<<(Serializer& ser, const XTypes::CompleteAliasHeader& stru);
bool operator>>(Serializer& ser, XTypes::CompleteAliasHeader& stru);

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CompleteAnnotationHeader& stru);
bool operator<<(Serializer& ser, const XTypes::CompleteAnnotationHeader& stru);
bool operator>>(Serializer& ser, XTypes::CompleteAnnotationHeader& stru);

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CompleteAnnotationParameter& stru);
bool operator<<(Serializer& ser, const XTypes::CompleteAnnotationParameter& stru);
bool operator>>(Serializer& ser, XTypes::CompleteAnnotationParameter& stru);

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CompleteArrayHeader& stru);
bool operator<<(Serializer& ser, const XTypes::CompleteArrayHeader& stru);
bool operator>>(Serializer& ser, XTypes::CompleteArrayHeader& stru);

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CompleteBitfield& stru);
bool operator<<(Serializer& ser, const XTypes::CompleteBitfield& stru);
bool operator>>(Serializer& ser, XTypes::CompleteBitfield& stru);

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CompleteBitflag& stru);
bool operator<<(Serializer& ser, const XTypes::CompleteBitflag& stru);
bool operator>>(Serializer& ser, XTypes::CompleteBitflag& stru);

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CompleteBitsetHeader& stru);
bool operator<<(Serializer& ser, const XTypes::CompleteBitsetHeader& stru);
bool operator>>(Serializer& ser, XTypes::CompleteBitsetHeader& stru);

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CompleteCollectionElement& stru);
bool operator<<(Serializer& ser, const XTypes::CompleteCollectionElement& stru);
bool operator>>(Serializer& ser, XTypes::CompleteCollectionElement& stru);

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CompleteCollectionHeader& stru);
bool operator<<(Serializer& ser, const XTypes::CompleteCollectionHeader& stru);
bool operator>>(Serializer& ser, XTypes::CompleteCollectionHeader& stru);

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CompleteDiscriminatorMember& stru);
bool operator<<(Serializer& ser, const XTypes::CompleteDiscriminatorMember& stru);
bool operator>>(Serializer& ser, XTypes::CompleteDiscriminatorMember& stru);

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CompleteEnumeratedLiteral& stru);
bool operator<<(Serializer& ser, const XTypes::CompleteEnumeratedLiteral& stru);
bool operator>>(Serializer& ser, XTypes::CompleteEnumeratedLiteral& stru);

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CompleteStructMember& stru);
bool operator<<(Serializer& ser, const XTypes::CompleteStructMember& stru);
bool operator>>(Serializer& ser, XTypes::CompleteStructMember& stru);

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CompleteUnionHeader& stru);
bool operator<<(Serializer& ser, const XTypes::CompleteUnionHeader& stru);
bool operator>>(Serializer& ser, XTypes::CompleteUnionHeader& stru);

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CompleteUnionMember& stru);
bool operator<<(Serializer& ser, const XTypes::CompleteUnionMember& stru);
bool operator>>(Serializer& ser, XTypes::CompleteUnionMember& stru);

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::MinimalAliasBody& stru);
bool operator<<(Serializer& ser, const XTypes::MinimalAliasBody& stru);
bool operator>>(Serializer& ser, XTypes::MinimalAliasBody& stru);

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::MinimalAliasHeader& stru);
bool operator<<(Serializer& ser, const XTypes::MinimalAliasHeader& stru);
bool operator>>(Serializer& ser, XTypes::MinimalAliasHeader& stru);

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::MinimalAnnotationHeader& stru);
bool operator<<(Serializer& ser, const XTypes::MinimalAnnotationHeader& stru);
bool operator>>(Serializer& ser, XTypes::MinimalAnnotationHeader& stru);

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::MinimalAnnotationParameter& stru);
bool operator<<(Serializer& ser, const XTypes::MinimalAnnotationParameter& stru);
bool operator>>(Serializer& ser, XTypes::MinimalAnnotationParameter& stru);

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::MinimalArrayHeader& stru);
bool operator<<(Serializer& ser, const XTypes::MinimalArrayHeader& stru);
bool operator>>(Serializer& ser, XTypes::MinimalArrayHeader& stru);

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::MinimalBitfield& stru);
bool operator<<(Serializer& ser, const XTypes::MinimalBitfield& stru);
bool operator>>(Serializer& ser, XTypes::MinimalBitfield& stru);

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::MinimalBitflag& stru);
bool operator<<(Serializer& ser, const XTypes::MinimalBitflag& stru);
bool operator>>(Serializer& ser, XTypes::MinimalBitflag& stru);

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::MinimalBitsetHeader& stru);
bool operator<<(Serializer& ser, const XTypes::MinimalBitsetHeader& stru);
bool operator>>(Serializer& ser, XTypes::MinimalBitsetHeader& stru);

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::MinimalCollectionElement& stru);
bool operator<<(Serializer& ser, const XTypes::MinimalCollectionElement& stru);
bool operator>>(Serializer& ser, XTypes::MinimalCollectionElement& stru);

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::MinimalCollectionHeader& stru);
bool operator<<(Serializer& ser, const XTypes::MinimalCollectionHeader& stru);
bool operator>>(Serializer& ser, XTypes::MinimalCollectionHeader& stru);

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::MinimalDiscriminatorMember& stru);
bool operator<<(Serializer& ser, const XTypes::MinimalDiscriminatorMember& stru);
bool operator>>(Serializer& ser, XTypes::MinimalDiscriminatorMember& stru);

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::MinimalEnumeratedLiteral& stru);
bool operator<<(Serializer& ser, const XTypes::MinimalEnumeratedLiteral& stru);
bool operator>>(Serializer& ser, XTypes::MinimalEnumeratedLiteral& stru);

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::MinimalStructMember& stru);
bool operator<<(Serializer& ser, const XTypes::MinimalStructMember& stru);
bool operator>>(Serializer& ser, XTypes::MinimalStructMember& stru);

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::MinimalUnionHeader& stru);
bool operator<<(Serializer& ser, const XTypes::MinimalUnionHeader& stru);
bool operator>>(Serializer& ser, XTypes::MinimalUnionHeader& stru);

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::MinimalUnionMember& stru);
bool operator<<(Serializer& ser, const XTypes::MinimalUnionMember& stru);
bool operator>>(Serializer& ser, XTypes::MinimalUnionMember& stru);

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::AnnotationParameterValue& stru);
bool operator<<(Serializer& strm, const XTypes::AnnotationParameterValue& stru);
bool operator>>(Serializer& strm, XTypes::AnnotationParameterValue& stru);

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::AppliedAnnotationParameter& stru);
bool operator<<(Serializer& strm, const XTypes::AppliedAnnotationParameter& stru);
bool operator>>(Serializer& strm, XTypes::AppliedAnnotationParameter& stru);

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::AppliedBuiltinMemberAnnotations& stru);
bool operator<<(Serializer& strm, const XTypes::AppliedBuiltinMemberAnnotations& stru);
bool operator>>(Serializer& strm, XTypes::AppliedBuiltinMemberAnnotations& stru);

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::AppliedVerbatimAnnotation& stru);
bool operator<<(Serializer& strm, const XTypes::AppliedVerbatimAnnotation& stru);
bool operator>>(Serializer& strm, XTypes::AppliedVerbatimAnnotation& stru);

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CommonAliasBody& stru);
bool operator<<(Serializer& strm, const XTypes::CommonAliasBody& stru);
bool operator>>(Serializer& strm, XTypes::CommonAliasBody& stru);

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CommonAnnotationParameter& stru);
bool operator<<(Serializer& strm, const XTypes::CommonAnnotationParameter& stru);
bool operator>>(Serializer& strm, XTypes::CommonAnnotationParameter& stru);

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CommonArrayHeader& stru);
bool operator<<(Serializer& strm, const XTypes::CommonArrayHeader& stru);
bool operator>>(Serializer& strm, XTypes::CommonArrayHeader& stru);

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CommonBitfield& stru);
bool operator<<(Serializer& strm, const XTypes::CommonBitfield& stru);
bool operator>>(Serializer& strm, XTypes::CommonBitfield& stru);

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CommonBitflag& stru);
bool operator<<(Serializer& strm, const XTypes::CommonBitflag& stru);
bool operator>>(Serializer& strm, XTypes::CommonBitflag& stru);

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CommonCollectionElement& stru);
bool operator<<(Serializer& strm, const XTypes::CommonCollectionElement& stru);
bool operator>>(Serializer& strm, XTypes::CommonCollectionElement& stru);

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CommonCollectionHeader& stru);
bool operator<<(Serializer& strm, const XTypes::CommonCollectionHeader& stru);
bool operator>>(Serializer& strm, XTypes::CommonCollectionHeader& stru);

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CommonDiscriminatorMember& stru);
bool operator<<(Serializer& strm, const XTypes::CommonDiscriminatorMember& stru);
bool operator>>(Serializer& strm, XTypes::CommonDiscriminatorMember& stru);

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CommonEnumeratedHeader& stru);
bool operator<<(Serializer& strm, const XTypes::CommonEnumeratedHeader& stru);
bool operator>>(Serializer& strm, XTypes::CommonEnumeratedHeader& stru);

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CommonEnumeratedLiteral& stru);
bool operator<<(Serializer& strm, const XTypes::CommonEnumeratedLiteral& stru);
bool operator>>(Serializer& strm, XTypes::CommonEnumeratedLiteral& stru);

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CommonStructMember& stru);
bool operator<<(Serializer& strm, const XTypes::CommonStructMember& stru);
bool operator>>(Serializer& strm, XTypes::CommonStructMember& stru);

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CommonUnionMember& stru);
bool operator<<(Serializer& strm, const XTypes::CommonUnionMember& stru);
bool operator>>(Serializer& strm, XTypes::CommonUnionMember& stru);

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CompleteElementDetail& stru);
bool operator<<(Serializer& strm, const XTypes::CompleteElementDetail& stru);
bool operator>>(Serializer& strm, XTypes::CompleteElementDetail& stru);

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::CompleteMemberDetail& stru);
bool operator<<(Serializer& strm, const XTypes::CompleteMemberDetail& stru);
bool operator>>(Serializer& strm, XTypes::CompleteMemberDetail& stru);

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::MinimalMemberDetail& stru);
bool operator<<(Serializer& strm, const XTypes::MinimalMemberDetail& stru);
bool operator>>(Serializer& strm, XTypes::MinimalMemberDetail& stru);

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::ExtendedTypeDefn& stru);
bool operator<<(Serializer& strm, const XTypes::ExtendedTypeDefn& stru);
bool operator>>(Serializer& strm, XTypes::ExtendedTypeDefn& stru);

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::PlainArrayLElemDefn& stru);
bool operator<<(Serializer& strm, const XTypes::PlainArrayLElemDefn& stru);
bool operator>>(Serializer& strm, XTypes::PlainArrayLElemDefn& stru);

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::PlainArraySElemDefn& stru);
bool operator<<(Serializer& strm, const XTypes::PlainArraySElemDefn& stru);
bool operator>>(Serializer& strm, XTypes::PlainArraySElemDefn& stru);

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::PlainMapLTypeDefn& stru);
bool operator<<(Serializer& strm, const XTypes::PlainMapLTypeDefn& stru);
bool operator>>(Serializer& strm, XTypes::PlainMapLTypeDefn& stru);

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::PlainMapSTypeDefn& stru);
bool operator<<(Serializer& strm, const XTypes::PlainMapSTypeDefn& stru);
bool operator>>(Serializer& strm, XTypes::PlainMapSTypeDefn& stru);

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::PlainSequenceLElemDefn& stru);
bool operator<<(Serializer& strm, const XTypes::PlainSequenceLElemDefn& stru);
bool operator>>(Serializer& strm, XTypes::PlainSequenceLElemDefn& stru);

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::PlainSequenceSElemDefn& stru);
bool operator<<(Serializer& strm, const XTypes::PlainSequenceSElemDefn& stru);
bool operator>>(Serializer& strm, XTypes::PlainSequenceSElemDefn& stru);

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::StringLTypeDefn& stru);
bool operator<<(Serializer& strm, const XTypes::StringLTypeDefn& stru);
bool operator>>(Serializer& strm, XTypes::StringLTypeDefn& stru);

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::StringSTypeDefn& stru);
bool operator<<(Serializer& strm, const XTypes::StringSTypeDefn& stru);
bool operator>>(Serializer& strm, XTypes::StringSTypeDefn& stru);

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::StronglyConnectedComponentId& stru);
bool operator<<(Serializer& strm, const XTypes::StronglyConnectedComponentId& stru);
bool operator>>(Serializer& strm, XTypes::StronglyConnectedComponentId& stru);

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::PlainCollectionHeader& stru);
bool operator<<(Serializer& strm, const XTypes::PlainCollectionHeader& stru);
bool operator>>(Serializer& strm, XTypes::PlainCollectionHeader& stru);

void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::TypeObjectHashId& stru);
bool operator<<(Serializer& strm, const XTypes::TypeObjectHashId& stru);
bool operator>>(Serializer& strm, XTypes::TypeObjectHashId& stru);

OpenDDS_Dcps_Export
void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::TypeIdentifierTypeObjectPair& stru);
OpenDDS_Dcps_Export
bool operator<<(Serializer& strm, const XTypes::TypeIdentifierTypeObjectPair& stru);
OpenDDS_Dcps_Export
bool operator>>(Serializer& strm, XTypes::TypeIdentifierTypeObjectPair& stru);

OpenDDS_Dcps_Export
void serialized_size(const Encoding& encoding, size_t& size,
  const XTypes::TypeIdentifierPair& stru);
OpenDDS_Dcps_Export
bool operator<<(Serializer& strm, const XTypes::TypeIdentifierPair& stru);
OpenDDS_Dcps_Export
bool operator>>(Serializer& strm, XTypes::TypeIdentifierPair& stru);

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif  /* OPENDDS_DCPS_XTYPES_TYPE_OBJECT_H */
