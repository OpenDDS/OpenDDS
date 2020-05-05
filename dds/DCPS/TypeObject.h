/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_TYPE_OBJECT_H
#define OPENDDS_DCPS_TYPE_OBJECT_H

#include "dds/DCPS/RcHandle_T.h"
#include "dds/DCPS/RcObject.h"
#include "dds/DCPS/Serializer.h"

#include "ace/CDR_Base.h"

#include <cstring>
#include <string>
#include <vector>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace XTypes {

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
    std::vector<T> members;

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
  typedef std::string MemberName;

  // Qualified type name includes the name of containing modules
  // using "::" as separator. No leading "::". E.g. "MyModule::MyType"
  const ACE_CDR::Long TYPE_NAME_MAX_LENGTH = 256;
  typedef std::string QualifiedTypeName;

  // Every type has an ID. Those of the primitive types are pre-defined.
  typedef ACE_CDR::Octet PrimitiveTypeId;

  // First 14 bytes of MD5 of the serialized TypeObject using XCDR
  // version 2 with Little Endian encoding
  typedef ACE_CDR::Octet EquivalenceHash[14];

  // First 4 bytes of MD5 of of a member name converted to bytes
  // using UTF-8 encoding and without a 'nul' terminator.
  // Example: the member name "color" has NameHash {0x70, 0xDD, 0xA5, 0xDF}
  typedef ACE_CDR::Octet NameHash[4];

  // Long Bound of a collection type
  typedef ACE_CDR::ULong LBound;
  typedef Sequence<LBound> LBoundSeq;
  const LBound INVALID_LBOUND = 0;

  // Short Bound of a collection type
  typedef ACE_CDR::Octet SBound;
  typedef Sequence<SBound> SBoundSeq;
  const SBound INVALID_SBOUND = 0;

  // union TypeObjectHashId switch (octet) {
  //     case EK_COMPLETE:
  //     case EK_MINIMAL:
  //         EquivalenceHash  hash;
  // };
  struct TypeObjectHashId {
    EquivalenceKind kind;
    EquivalenceHash hash;

    TypeObjectHashId() {}

    TypeObjectHashId(const EquivalenceKind& a_kind,
                     const EquivalenceHash& a_hash)
      : kind(a_kind)
    {
      std::memcpy(hash, a_hash, sizeof hash);
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

  typedef MemberFlag   CollectionElementFlag;   // T1, T2, X
  typedef MemberFlag   StructMemberFlag;        // T1, T2, O, M, K, X
  typedef MemberFlag   UnionMemberFlag;         // T1, T2, D, X
  typedef MemberFlag   UnionDiscriminatorFlag;  // T1, T2, K
  typedef MemberFlag   EnumeratedLiteralFlag;   // D
  typedef MemberFlag   AnnotationParameterFlag; // Unused. No flags apply
  typedef MemberFlag   AliasMemberFlag;         // Unused. No flags apply
  typedef MemberFlag   BitflagFlag;             // Unused. No flags apply
  typedef MemberFlag   BitsetMemberFlag;        // Unused. No flags apply

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

  typedef TypeFlag   StructTypeFlag;      // All flags apply
  typedef TypeFlag   UnionTypeFlag;       // All flags apply
  typedef TypeFlag   CollectionTypeFlag;  // Unused. No flags apply
  typedef TypeFlag   AnnotationTypeFlag;  // Unused. No flags apply
  typedef TypeFlag   AliasTypeFlag;       // Unused. No flags apply
  typedef TypeFlag   EnumTypeFlag;        // Unused. No flags apply
  typedef TypeFlag   BitmaskTypeFlag;     // Unused. No flags apply
  typedef TypeFlag   BitsetTypeFlag;      // Unused. No flags apply

  // Mask used to remove the flags that do no affect assignability
  const TypeFlag TypeFlagMinimalMask = 0x0007; // Selects  M, A, F

  // Forward declaration
  struct TypeIdentifier;

  using DCPS::RcHandle;
  using DCPS::make_rch;
  typedef RcHandle<TypeIdentifier> TypeIdentifierPtr;

  // 1 Byte
  struct StringSTypeDefn {
    SBound                  bound;

    StringSTypeDefn() {}

    StringSTypeDefn(const SBound a_bound)
      : bound(a_bound)
    {}
  };

  // 4 Bytes
  struct StringLTypeDefn {
    LBound                  bound;

    StringLTypeDefn() {}

    StringLTypeDefn(const LBound a_bound)
      : bound(a_bound)
    {}
  };

  struct PlainCollectionHeader {
    EquivalenceKind        equiv_kind;
    CollectionElementFlag  element_flags;

    PlainCollectionHeader() {}

    PlainCollectionHeader(const EquivalenceKind& a_equiv_kind,
                          const CollectionElementFlag& a_element_flags)
      : equiv_kind(a_equiv_kind)
      , element_flags(a_element_flags)
    {}
  };

  struct PlainSequenceSElemDefn {
    PlainCollectionHeader  header;
    SBound                 bound;
    TypeIdentifierPtr element_identifier;

    PlainSequenceSElemDefn() {}

    PlainSequenceSElemDefn(const PlainCollectionHeader& a_header,
                           const SBound& a_bound,
                           const TypeIdentifierPtr& a_element_identifier)
      : header(a_header)
      , bound(a_bound)
      , element_identifier(a_element_identifier)
    {}
  };

  struct PlainSequenceLElemDefn {
    PlainCollectionHeader  header;
    LBound                 bound;
    TypeIdentifierPtr element_identifier;

    PlainSequenceLElemDefn() {}

    PlainSequenceLElemDefn(const PlainCollectionHeader& a_header,
                           const LBound& a_bound,
                           const TypeIdentifierPtr& a_element_identifier)
      : header(a_header)
      , bound(a_bound)
      , element_identifier(a_element_identifier)
    {}
  };

  struct PlainArraySElemDefn {
    PlainCollectionHeader  header;
    SBoundSeq              array_bound_seq;
    TypeIdentifierPtr element_identifier;

    PlainArraySElemDefn() {}

    PlainArraySElemDefn(const PlainCollectionHeader& a_header,
                        const SBoundSeq& a_array_bound_seq,
                        const TypeIdentifierPtr& a_element_identifier)
      : header(a_header)
      , array_bound_seq(a_array_bound_seq)
      , element_identifier(a_element_identifier)
    {}
  };

  struct PlainArrayLElemDefn {
    PlainCollectionHeader  header;
    LBoundSeq              array_bound_seq;
    TypeIdentifierPtr element_identifier;

    PlainArrayLElemDefn() {}

    PlainArrayLElemDefn(const PlainCollectionHeader& a_header,
                        const LBoundSeq& a_array_bound_seq,
                        const TypeIdentifierPtr& a_element_identifier)
      : header(a_header)
      , array_bound_seq(a_array_bound_seq)
      , element_identifier(a_element_identifier)
    {}
  };

  struct PlainMapSTypeDefn {
    PlainCollectionHeader  header;
    SBound                 bound;
    TypeIdentifierPtr element_identifier;
    CollectionElementFlag  key_flags;
    TypeIdentifierPtr key_identifier;

    PlainMapSTypeDefn() {}

    PlainMapSTypeDefn(const PlainCollectionHeader&  a_header,
                      const SBound&                 a_bound,
                      const TypeIdentifierPtr& a_element_identifier,
                      const CollectionElementFlag&  a_key_flags,
                      const TypeIdentifierPtr& a_key_identifier)
      : header(a_header)
      , bound(a_bound)
      , element_identifier(a_element_identifier)
      , key_flags(a_key_flags)
      , key_identifier(a_key_identifier)
    {}
  };

  struct PlainMapLTypeDefn {
    PlainCollectionHeader  header;
    LBound                 bound;
    TypeIdentifierPtr element_identifier;
    CollectionElementFlag  key_flags;
    TypeIdentifierPtr key_identifier;

    PlainMapLTypeDefn() {}

    PlainMapLTypeDefn(const PlainCollectionHeader&  a_header,
                      const LBound&                 a_bound,
                      const TypeIdentifierPtr& a_element_identifier,
                      const CollectionElementFlag&  a_key_flags,
                      const TypeIdentifierPtr& a_key_identifier)
      : header(a_header)
      , bound(a_bound)
      , element_identifier(a_element_identifier)
      , key_flags(a_key_flags)
      , key_identifier(a_key_identifier)
    {}
  };

  // Used for Types that have cyclic depencencies with other types
  struct StronglyConnectedComponentId {
    TypeObjectHashId sc_component_id; // Hash StronglyConnectedComponent
    ACE_CDR::Long   scc_length; // StronglyConnectedComponent.length
    ACE_CDR::Long   scc_index ; // identify type in Strongly Connected Comp.

    StronglyConnectedComponentId() {}

    StronglyConnectedComponentId(const TypeObjectHashId& a_sc_component_id,
                                 const ACE_CDR::Long& a_scc_length,
                                 const ACE_CDR::Long& a_scc_index)
      : sc_component_id(a_sc_component_id)
      , scc_length(a_scc_length)
      , scc_index(a_scc_index)
    {}
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
  //     case TK_BYTE_TYPE:
  //     case TK_INT16_TYPE:
  //     case TK_INT32_TYPE:
  //     case TK_INT64_TYPE:
  //     case TK_UINT16_TYPE:
  //     case TK_UINT32_TYPE:
  //     case TK_UINT64_TYPE:
  //     case TK_FLOAT32_TYPE:
  //     case TK_FLOAT64_TYPE:
  //     case TK_FLOAT128_TYPE:
  //     case TK_CHAR8_TYPE:
  //     case TK_CHAR16_TYPE:
  //     // No Value
  //     */

  //   // ============ Strings - use TypeIdentifierKind ===================
  // case TI_STRING8_SMALL:
  // case TI_STRING16_SMALL:
  //   StringSTypeDefn         string_sdefn;

  // case TI_STRING8_LARGE:
  // case TI_STRING16_LARGE:
  //   StringLTypeDefn         string_ldefn;

  //   // ============  Plain collectios - use TypeIdentifierKind =========
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

  struct TypeIdentifier : public DCPS::RcObject {
    ACE_CDR::Octet kind;
    // ============ Strings - use TypeIdentifierKind ===================
    StringSTypeDefn         string_sdefn;
    StringLTypeDefn         string_ldefn;

    // ============  Plain collectios - use TypeIdentifierKind =========
    PlainSequenceSElemDefn  seq_sdefn;
    PlainSequenceLElemDefn  seq_ldefn;

    PlainArraySElemDefn     array_sdefn;
    PlainArrayLElemDefn     array_ldefn;

    PlainMapSTypeDefn       map_sdefn;
    PlainMapLTypeDefn       map_ldefn;

    // ============  Types that are mutually dependent on each other ===
    StronglyConnectedComponentId  sc_component_id;

    // ============  The remaining cases - use EquivalenceKind =========
    EquivalenceHash         equivalence_hash;

    // ===================  Future extensibility  ============
    // Future extensions
    ExtendedTypeDefn        extended_defn;

    static TypeIdentifierPtr make(ACE_CDR::Octet k)
    {
      TypeIdentifierPtr ti = make_rch<TypeIdentifier>();
      ti->kind = k;
      return ti;
    }

    static TypeIdentifierPtr makeString(bool wide, const StringSTypeDefn& string_sdefn)
    {
      TypeIdentifierPtr ti = make(wide ? TI_STRING16_SMALL : TI_STRING8_SMALL);
      ti->string_sdefn = string_sdefn;
      return ti;
    }

    static TypeIdentifierPtr makeString(bool wide, const StringLTypeDefn& string_ldefn)
    {
      TypeIdentifierPtr ti = make(wide ? TI_STRING16_LARGE : TI_STRING8_LARGE);
      ti->string_ldefn = string_ldefn;
      return ti;
    }

    static TypeIdentifierPtr makePlainSequence(const TypeIdentifierPtr& base_type,
                                               const SBound& bound)
    {
      TypeIdentifierPtr ti = make(TI_PLAIN_SEQUENCE_SMALL);
      ti->seq_sdefn = PlainSequenceSElemDefn
        (
         PlainCollectionHeader
         (EquivalenceKind(EK_MINIMAL), // TODO: Pick the correct kind.
          CollectionElementFlag()), // TODO: Set this
         bound,
         base_type);
      return ti;
    }

    static TypeIdentifierPtr makePlainSequence(const TypeIdentifierPtr& base_type,
                                               const LBound& bound)
    {
      TypeIdentifierPtr ti = make(TI_PLAIN_SEQUENCE_LARGE);
      ti->seq_ldefn = PlainSequenceLElemDefn
        (
         PlainCollectionHeader
         (EquivalenceKind(EK_MINIMAL), // TODO:  Pick the correct kind.
          CollectionElementFlag()), // TODO Set this.
         bound,
         base_type);
      return ti;
    }

    static TypeIdentifierPtr makePlainArray(const TypeIdentifierPtr& base_type,
                                            const SBoundSeq& bound_seq)
    {
      TypeIdentifierPtr ti = make(TI_PLAIN_ARRAY_SMALL);
      ti->array_sdefn = PlainArraySElemDefn
        (
         PlainCollectionHeader
         (EquivalenceKind(EK_MINIMAL), // TODO: Pick the correct kind.
          CollectionElementFlag()), // TODO: Set this
         bound_seq,
         base_type);
      return ti;
    }

    static TypeIdentifierPtr makePlainArray(const TypeIdentifierPtr& base_type,
                                            const LBoundSeq& bound_seq)
    {
      TypeIdentifierPtr ti = make(TI_PLAIN_ARRAY_LARGE);
      ti->array_ldefn = PlainArrayLElemDefn
        (
         PlainCollectionHeader
         (EquivalenceKind(EK_MINIMAL), // TODO:  Pick the correct kind.
          CollectionElementFlag()), // TODO Set this.
         bound_seq,
         base_type);
      return ti;
    }

    static TypeIdentifierPtr make(ACE_CDR::Octet k,
                                  const PlainMapSTypeDefn& map_sdefn)
    {
      TypeIdentifierPtr ti = make_rch<TypeIdentifier>();
      ti->kind = k;
      ti->map_sdefn = map_sdefn;
      return ti;
    }

    static TypeIdentifierPtr make(ACE_CDR::Octet k,
                                  const PlainMapLTypeDefn& map_ldefn)
    {
      TypeIdentifierPtr ti = make_rch<TypeIdentifier>();
      ti->kind = k;
      ti->map_ldefn = map_ldefn;
      return ti;
    }

    static TypeIdentifierPtr make(ACE_CDR::Octet k,
                                  const StronglyConnectedComponentId& sc_component_id)
    {
      TypeIdentifierPtr ti = make_rch<TypeIdentifier>();
      ti->kind = k;
      ti->sc_component_id = sc_component_id;
      return ti;
    }

    static TypeIdentifierPtr make(ACE_CDR::Octet k,
                                  const EquivalenceHash& equivalence_hash)
    {
      TypeIdentifierPtr ti = make_rch<TypeIdentifier>();
      ti->kind = k;
      std::memcpy(ti->equivalence_hash, equivalence_hash, sizeof equivalence_hash);
      return ti;
    }

    static TypeIdentifierPtr make(ACE_CDR::Octet k,
                                  const ExtendedTypeDefn& extended_defn)
    {
      TypeIdentifierPtr ti = make_rch<TypeIdentifier>();
      ti->kind = k;
      ti->extended_defn = extended_defn;
      return ti;
    }
  };

  size_t find_size(const TypeIdentifier& stru, size_t& size);
  bool operator<<(DCPS::Serializer& ser, const TypeIdentifier& stru);

  typedef Sequence<TypeIdentifierPtr> TypeIdentifierSeq;

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
    ACE_CDR::Boolean             boolean_value;
    ACE_CDR::Octet               byte_value;
    ACE_CDR::Short               int16_value;
    ACE_CDR::UShort              uint_16_value;
    ACE_CDR::Long                int32_value;
    ACE_CDR::ULong               uint32_value;
    ACE_CDR::LongLong            int64_value;
    ACE_CDR::ULongLong           uint64_value;
    ACE_CDR::Float               float32_value;
    ACE_CDR::Double              float64_value;
    ACE_CDR::LongDouble          float128_value;
    ACE_CDR::Char                char_value;
    ACE_CDR::WChar               wchar_value;
    ACE_CDR::Long                enumerated_value;
    std::string                  string8_value;
    std::wstring                 string16_value;
    ExtendedAnnotationParameterValue      extended_value;
  };

  // The application of an annotation to some type or type member
  struct AppliedAnnotationParameter {
    NameHash                  paramname_hash;
    AnnotationParameterValue  value;
  };
  // Sorted by AppliedAnnotationParameter.paramname_hash
  typedef Sequence<AppliedAnnotationParameter> AppliedAnnotationParameterSeq;

  struct AppliedAnnotation {
    TypeIdentifierPtr                   annotation_typeid;
    Optional<AppliedAnnotationParameterSeq>    param_seq;
  };
  // Sorted by AppliedAnnotation.annotation_typeid
  typedef Sequence<AppliedAnnotation> AppliedAnnotationSeq;

  // @verbatim(placement="<placement>", language="<lang>", text="<text>")
  struct AppliedVerbatimAnnotation {
    std::string placement;
    std::string language;
    std::string text;
  };


  // --- Aggregate types: ------------------------------------------------
  struct AppliedBuiltinMemberAnnotations {
    Optional<std::string>                  unit; // @unit("<unit>")
    Optional<AnnotationParameterValue>      min; // @min , @range
    Optional<AnnotationParameterValue>      max; // @max , @range
    Optional<std::string>               hash_id; // @hash_id("<membername>")
  };

  struct CommonStructMember {
    MemberId                                   member_id;
    StructMemberFlag                           member_flags;
    TypeIdentifierPtr                   member_type_id;

    CommonStructMember (const MemberId& a_member_id,
                        const StructMemberFlag& a_member_flags,
                        const TypeIdentifierPtr& a_member_type_id)
      : member_id(a_member_id)
      , member_flags(a_member_flags)
      , member_type_id(a_member_type_id)
    {}
  };

  // COMPLETE Details for a member of an aggregate type
  struct CompleteMemberDetail {
    MemberName                                 name;
    Optional<AppliedBuiltinMemberAnnotations>  ann_builtin;
    Optional<AppliedAnnotationSeq>             ann_custom;
  };

  // MINIMAL Details for a member of an aggregate type
  struct OpenDDS_Dcps_Export MinimalMemberDetail {
    NameHash                                  name_hash;

    MinimalMemberDetail() {}
    MinimalMemberDetail(const std::string& name);
  };

  // Member of an aggregate type
  struct CompleteStructMember {
    CommonStructMember                         common;
    CompleteMemberDetail                       detail;
  };
  // Ordered by the member_index
  typedef Sequence<CompleteStructMember> CompleteStructMemberSeq;

  // Member of an aggregate type
  struct MinimalStructMember {
    CommonStructMember                         common;
    MinimalMemberDetail                        detail;

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
  };

  struct MinimalTypeDetail {
    // Empty. Available for future extension
  };

  struct CompleteTypeDetail {
    Optional<AppliedBuiltinTypeAnnotations>  ann_builtin;
    Optional<AppliedAnnotationSeq>           ann_custom;
    QualifiedTypeName                        type_name;
  };

  struct CompleteStructHeader {
    TypeIdentifierPtr                 base_type;
    CompleteTypeDetail                       detail;
  };

  struct MinimalStructHeader {
    TypeIdentifierPtr                 base_type;
    MinimalTypeDetail                        detail;

    MinimalStructHeader() {}

    MinimalStructHeader(const TypeIdentifierPtr& a_base_type,
                        const MinimalTypeDetail& a_detail)
      : base_type(a_base_type)
      , detail(a_detail)
    {}
  };

  struct CompleteStructType {
    StructTypeFlag             struct_flags;
    CompleteStructHeader       header;
    CompleteStructMemberSeq    member_seq;
  };

  size_t find_size(const CompleteStructType& stru, size_t& size);
  bool operator<<(DCPS::Serializer& ser, const CompleteStructType& stru);

  struct MinimalStructType {
    StructTypeFlag             struct_flags;
    MinimalStructHeader        header;
    MinimalStructMemberSeq     member_seq;

    MinimalStructType() {}

    MinimalStructType(const StructTypeFlag& a_struct_flags,
                      const MinimalStructHeader& a_header,
                      const MinimalStructMemberSeq& a_member_seq)
      : struct_flags(a_struct_flags)
      , header(a_header)
      , member_seq(a_member_seq)
    {}
  };

  size_t find_size(const MinimalStructType& stru, size_t& size);
  bool operator<<(DCPS::Serializer& ser, const MinimalStructType& stru);

  // --- Union: ----------------------------------------------------------

  // Case labels that apply to a member of a union type
  // Ordered by their values
  typedef Sequence<ACE_CDR::Long> UnionCaseLabelSeq;

  struct CommonUnionMember {
    MemberId                    member_id;
    UnionMemberFlag             member_flags;
    TypeIdentifierPtr    type_id;
    UnionCaseLabelSeq           label_seq;

    CommonUnionMember() {}

    CommonUnionMember(const MemberId& a_member_id,
                      const UnionMemberFlag& a_member_flags,
                      const TypeIdentifierPtr& a_type_id,
                      const UnionCaseLabelSeq& a_label_seq)
      : member_id(a_member_id)
      , member_flags(a_member_flags)
      , type_id(a_type_id)
      , label_seq(a_label_seq)
    {}
  };

  // Member of a union type
  struct CompleteUnionMember {
    CommonUnionMember      common;
    CompleteMemberDetail   detail;
  };
  // Ordered by member_index
  typedef Sequence<CompleteUnionMember> CompleteUnionMemberSeq;

  // Member of a union type
  struct MinimalUnionMember {
    CommonUnionMember   common;
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
    UnionDiscriminatorFlag       member_flags;
    TypeIdentifierPtr     type_id;

    CommonDiscriminatorMember() {}

    CommonDiscriminatorMember(const UnionDiscriminatorFlag& a_member_flags,
                              const TypeIdentifierPtr& a_type_id)
      : member_flags(a_member_flags)
      , type_id(a_type_id)
    {}
  };

  // Member of a union type
  struct CompleteDiscriminatorMember {
    CommonDiscriminatorMember                common;
    Optional<AppliedBuiltinTypeAnnotations>  ann_builtin;
    Optional<AppliedAnnotationSeq>           ann_custom;
  };

  // Member of a union type
  struct MinimalDiscriminatorMember {
    CommonDiscriminatorMember   common;

    MinimalDiscriminatorMember() {}

    MinimalDiscriminatorMember(const CommonDiscriminatorMember& a_common)
      : common(a_common)
    {}
  };

  struct CompleteUnionHeader {
    CompleteTypeDetail          detail;
  };

  struct MinimalUnionHeader {
    MinimalTypeDetail           detail;

    MinimalUnionHeader() {}

    MinimalUnionHeader(const MinimalTypeDetail& a_detail)
      : detail(a_detail)
    {}
  };

  struct CompleteUnionType {
    UnionTypeFlag                union_flags;
    CompleteUnionHeader          header;
    CompleteDiscriminatorMember  discriminator;
    CompleteUnionMemberSeq       member_seq;
  };

  size_t find_size(const CompleteUnionType& stru, size_t& size);
  bool operator<<(DCPS::Serializer& ser, const CompleteUnionType& stru);

  struct MinimalUnionType {
    UnionTypeFlag                union_flags;
    MinimalUnionHeader           header;
    MinimalDiscriminatorMember   discriminator;
    MinimalUnionMemberSeq        member_seq;

    MinimalUnionType() {}

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

  size_t find_size(const MinimalUnionType& stru, size_t& size);
  bool operator<<(DCPS::Serializer& ser, const MinimalUnionType& stru);

  // --- Annotation: ----------------------------------------------------
  struct CommonAnnotationParameter {
    AnnotationParameterFlag      member_flags;
    TypeIdentifierPtr     member_type_id;
  };

  // Member of an annotation type
  struct CompleteAnnotationParameter {
    CommonAnnotationParameter  common;
    MemberName                 name;
    AnnotationParameterValue   default_value;
  };
  // Ordered by CompleteAnnotationParameter.name
  typedef Sequence<CompleteAnnotationParameter> CompleteAnnotationParameterSeq;

  struct MinimalAnnotationParameter {
    CommonAnnotationParameter  common;
    NameHash                   name_hash;
    AnnotationParameterValue   default_value;
  };
  // Ordered by MinimalAnnotationParameter.name_hash
  typedef Sequence<MinimalAnnotationParameter> MinimalAnnotationParameterSeq;

  struct CompleteAnnotationHeader {
    QualifiedTypeName         annotation_name;
  };

  struct MinimalAnnotationHeader {
    // Empty. Available for future extension
  };

  struct CompleteAnnotationType {
    AnnotationTypeFlag             annotation_flag;
    CompleteAnnotationHeader       header;
    CompleteAnnotationParameterSeq member_seq;
  };

  size_t find_size(const CompleteAnnotationType& stru, size_t& size);
  bool operator<<(DCPS::Serializer& ser, const CompleteAnnotationType& stru);

  struct MinimalAnnotationType {
    AnnotationTypeFlag             annotation_flag;
    MinimalAnnotationHeader        header;
    MinimalAnnotationParameterSeq  member_seq;
  };

  size_t find_size(const MinimalAnnotationType& stru, size_t& size);
  bool operator<<(DCPS::Serializer& ser, const MinimalAnnotationType& stru);


  // --- Alias: ----------------------------------------------------------
  struct CommonAliasBody {
    AliasMemberFlag          related_flags;
    TypeIdentifierPtr related_type;

    CommonAliasBody() {}

    CommonAliasBody(const AliasMemberFlag& a_related_flags,
                    const TypeIdentifierPtr& a_related_type)
      : related_flags(a_related_flags)
      , related_type(a_related_type)
    {}
  };

  struct CompleteAliasBody {
    CommonAliasBody       common;
    Optional<AppliedBuiltinMemberAnnotations>  ann_builtin;
    Optional<AppliedAnnotationSeq>             ann_custom;
  };

  struct MinimalAliasBody {
    CommonAliasBody       common;

    MinimalAliasBody() {}

    MinimalAliasBody(const CommonAliasBody a_common)
      : common(a_common)
    {}
  };

  struct CompleteAliasHeader {
    CompleteTypeDetail    detail;
  };

  struct MinimalAliasHeader {
    // Empty. Available for future extension
  };

  struct CompleteAliasType {
    AliasTypeFlag         alias_flags;
    CompleteAliasHeader   header;
    CompleteAliasBody     body;
  };

  size_t find_size(const CompleteAliasType& stru, size_t& size);
  bool operator<<(DCPS::Serializer& ser, const CompleteAliasType& stru);

  struct MinimalAliasType {
    AliasTypeFlag         alias_flags;
    MinimalAliasHeader    header;
    MinimalAliasBody      body;

    MinimalAliasType() {}

    MinimalAliasType(const AliasTypeFlag& a_alias_flags,
                     const MinimalAliasHeader& a_header,
                     const MinimalAliasBody& a_body)
      : alias_flags(a_alias_flags)
      , header(a_header)
      , body(a_body)
    {}
  };

  size_t find_size(const MinimalAliasType& stru, size_t& size);
  bool operator<<(DCPS::Serializer& ser, const MinimalAliasType& stru);

  // --- Collections: ----------------------------------------------------
  struct CompleteElementDetail {
    Optional<AppliedBuiltinMemberAnnotations>  ann_builtin;
    Optional<AppliedAnnotationSeq>             ann_custom;
  };

  struct CommonCollectionElement {
    CollectionElementFlag     element_flags;
    TypeIdentifierPtr  type;
  };

  struct CompleteCollectionElement {
    CommonCollectionElement   common;
    CompleteElementDetail     detail;
  };

  struct MinimalCollectionElement {
    CommonCollectionElement   common;
  };

  struct CommonCollectionHeader {
    LBound                    bound;
  };

  struct CompleteCollectionHeader {
    CommonCollectionHeader        common;
    Optional<CompleteTypeDetail>  detail; // not present for anonymous
  };

  struct MinimalCollectionHeader {
    CommonCollectionHeader        common;
  };

  // --- Sequence: ------------------------------------------------------
  struct CompleteSequenceType {
    CollectionTypeFlag         collection_flag;
    CompleteCollectionHeader   header;
    CompleteCollectionElement  element;
  };

  size_t find_size(const CompleteSequenceType& stru, size_t& size);
  bool operator<<(DCPS::Serializer& ser, const CompleteSequenceType& stru);

  struct MinimalSequenceType {
    CollectionTypeFlag         collection_flag;
    MinimalCollectionHeader    header;
    MinimalCollectionElement   element;
  };

  size_t find_size(const MinimalSequenceType& stru, size_t& size);
  bool operator<<(DCPS::Serializer& ser, const MinimalSequenceType& stru);

  // --- Array: ------------------------------------------------------
  struct CommonArrayHeader {
    LBoundSeq           bound_seq;
  };

  struct CompleteArrayHeader {
    CommonArrayHeader   common;
    CompleteTypeDetail  detail;
  };

  struct MinimalArrayHeader {
    CommonArrayHeader   common;
  };

  struct CompleteArrayType  {
    CollectionTypeFlag          collection_flag;
    CompleteArrayHeader         header;
    CompleteCollectionElement   element;
  };

  size_t find_size(const CompleteArrayType& stru, size_t& size);
  bool operator<<(DCPS::Serializer& ser, const CompleteArrayType& stru);

  struct MinimalArrayType  {
    CollectionTypeFlag         collection_flag;
    MinimalArrayHeader         header;
    MinimalCollectionElement   element;
  };

  size_t find_size(const MinimalArrayType& stru, size_t& size);
  bool operator<<(DCPS::Serializer& ser, const MinimalArrayType& stru);

  // --- Map: ------------------------------------------------------
  struct CompleteMapType {
    CollectionTypeFlag            collection_flag;
    CompleteCollectionHeader      header;
    CompleteCollectionElement     key;
    CompleteCollectionElement     element;
  };

  size_t find_size(const CompleteMapType& stru, size_t& size);
  bool operator<<(DCPS::Serializer& ser, const CompleteMapType& stru);

  struct MinimalMapType {
    CollectionTypeFlag          collection_flag;
    MinimalCollectionHeader     header;
    MinimalCollectionElement    key;
    MinimalCollectionElement    element;
  };

  size_t find_size(const MinimalMapType& stru, size_t& size);
  bool operator<<(DCPS::Serializer& ser, const MinimalMapType& stru);

  // --- Enumeration: ----------------------------------------------------
  typedef ACE_CDR::UShort BitBound;

  // Constant in an enumerated type
  struct CommonEnumeratedLiteral {
    ACE_CDR::Long                     value;
    EnumeratedLiteralFlag    flags;

    CommonEnumeratedLiteral() {}

    CommonEnumeratedLiteral(const ACE_CDR::Long& a_value,
                            const EnumeratedLiteralFlag a_flags)
      : value(a_value)
      , flags(a_flags)
    {}
  };

  // Constant in an enumerated type
  struct CompleteEnumeratedLiteral {
    CommonEnumeratedLiteral  common;
    CompleteMemberDetail     detail;
  };
  // Ordered by EnumeratedLiteral.common.value
  typedef Sequence<CompleteEnumeratedLiteral> CompleteEnumeratedLiteralSeq;

  // Constant in an enumerated type
  struct MinimalEnumeratedLiteral {
    CommonEnumeratedLiteral  common;
    MinimalMemberDetail      detail;

    MinimalEnumeratedLiteral() {}

    MinimalEnumeratedLiteral(const CommonEnumeratedLiteral& a_common,
                             const MinimalMemberDetail& a_detail)
      : common(a_common)
      , detail(a_detail)
    {}

    bool operator<(const MinimalEnumeratedLiteral& other) const {
      return common.value < other.common.value;
    }
  };
  // Ordered by EnumeratedLiteral.common.value
  typedef Sequence<MinimalEnumeratedLiteral> MinimalEnumeratedLiteralSeq;

  struct CommonEnumeratedHeader {
    BitBound                bit_bound;

    CommonEnumeratedHeader() {}

    CommonEnumeratedHeader(const BitBound& a_bit_bound)
      : bit_bound(a_bit_bound)
    {}
  };

  struct CompleteEnumeratedHeader {
    CommonEnumeratedHeader  common;
    CompleteTypeDetail      detail;
  };

  struct MinimalEnumeratedHeader {
    CommonEnumeratedHeader  common;

    MinimalEnumeratedHeader() {}

    MinimalEnumeratedHeader(const CommonEnumeratedHeader& a_common)
      : common(a_common)
    {}
  };

  // Enumerated type
  struct CompleteEnumeratedType  {
    EnumTypeFlag                    enum_flags; // unused
    CompleteEnumeratedHeader        header;
    CompleteEnumeratedLiteralSeq    literal_seq;
  };

  size_t find_size(const CompleteEnumeratedType& stru, size_t& size);
  bool operator<<(DCPS::Serializer& ser, const CompleteEnumeratedType& stru);

  // Enumerated type
  struct MinimalEnumeratedType  {
    EnumTypeFlag                  enum_flags; // unused
    MinimalEnumeratedHeader       header;
    MinimalEnumeratedLiteralSeq   literal_seq;

    MinimalEnumeratedType() {}

    MinimalEnumeratedType(const EnumTypeFlag& a_enum_flags,
                          const MinimalEnumeratedHeader& a_header,
                          const MinimalEnumeratedLiteralSeq& a_literal_seq)
      : enum_flags(a_enum_flags)
      , header(a_header)
      , literal_seq(a_literal_seq)
    {}
  };

  size_t find_size(const MinimalEnumeratedType& stru, size_t& size);
  bool operator<<(DCPS::Serializer& ser, const MinimalEnumeratedType& stru);

  // --- Bitmask: --------------------------------------------------------
  // Bit in a bit mask
  struct CommonBitflag {
    ACE_CDR::UShort        position;
    BitflagFlag            flags;
  };

  struct CompleteBitflag {
    CommonBitflag          common;
    CompleteMemberDetail   detail;
  };
  // Ordered by Bitflag.position
  typedef Sequence<CompleteBitflag> CompleteBitflagSeq;

  struct MinimalBitflag {
    CommonBitflag        common;
    MinimalMemberDetail  detail;
  };
  // Ordered by Bitflag.position
  typedef Sequence<MinimalBitflag> MinimalBitflagSeq;

  struct CommonBitmaskHeader {
    BitBound             bit_bound;
  };

  typedef CompleteEnumeratedHeader CompleteBitmaskHeader;

  typedef MinimalEnumeratedHeader  MinimalBitmaskHeader;

  struct CompleteBitmaskType {
    BitmaskTypeFlag          bitmask_flags; // unused
    CompleteBitmaskHeader    header;
    CompleteBitflagSeq       flag_seq;
  };

  size_t find_size(const CompleteBitmaskType& stru, size_t& size);
  bool operator<<(DCPS::Serializer& ser, const CompleteBitmaskType& stru);

  struct MinimalBitmaskType {
    BitmaskTypeFlag          bitmask_flags; // unused
    MinimalBitmaskHeader     header;
    MinimalBitflagSeq        flag_seq;
  };

  size_t find_size(const MinimalBitmaskType& stru, size_t& size);
  bool operator<<(DCPS::Serializer& ser, const MinimalBitmaskType& stru);

  // --- Bitset: ----------------------------------------------------------
  struct CommonBitfield {
    ACE_CDR::UShort       position;
    BitsetMemberFlag      flags;
    ACE_CDR::Octet        bitcount;
    TypeKind              holder_type; // Must be primitive integer type
  };

  struct CompleteBitfield {
    CommonBitfield           common;
    CompleteMemberDetail     detail;
  };
  // Ordered by Bitfield.position
  typedef Sequence<CompleteBitfield> CompleteBitfieldSeq;

  struct MinimalBitfield {
    CommonBitfield       common;
    NameHash             name_hash;
  };
  // Ordered by Bitfield.position
  typedef Sequence<MinimalBitfield> MinimalBitfieldSeq;

  struct CompleteBitsetHeader {
    CompleteTypeDetail   detail;
  };

  struct MinimalBitsetHeader {
    // Empty. Available for future extension
  };

  struct CompleteBitsetType  {
    BitsetTypeFlag         bitset_flags; // unused
    CompleteBitsetHeader   header;
    CompleteBitfieldSeq    field_seq;
  };

  size_t find_size(const CompleteBitsetType& stru, size_t& size);
  bool operator<<(DCPS::Serializer& ser, const CompleteBitsetType& stru);

  struct MinimalBitsetType  {
    BitsetTypeFlag       bitset_flags; // unused
    MinimalBitsetHeader  header;
    MinimalBitfieldSeq   field_seq;
  };

  size_t find_size(const MinimalBitsetType& stru, size_t& size);
  bool operator<<(DCPS::Serializer& ser, const MinimalBitsetType& stru);

  // --- Type Object: ---------------------------------------------------
  // The types associated with each case selection must have extensibility
  // kind APPENDABLE or MUTABLE so that they can be extended in the future

  struct CompleteExtendedType {
    // Empty. Available for future extension
  };

  inline size_t find_size(const CompleteExtendedType&, size_t& size) { return size; }
  inline bool operator<<(DCPS::Serializer&, const CompleteExtendedType&) { return true; }

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
    CompleteAliasType      alias_type;
    CompleteAnnotationType annotation_type;
    CompleteStructType     struct_type;
    CompleteUnionType      union_type;
    CompleteBitsetType     bitset_type;
    CompleteSequenceType   sequence_type;
    CompleteArrayType      array_type;
    CompleteMapType        map_type;
    CompleteEnumeratedType enumerated_type;
    CompleteBitmaskType    bitmask_type;

    // ===================  Future extensibility  ============
    CompleteExtendedType   extended_type;
  };

  size_t find_size(const CompleteTypeObject& type_object, size_t& size);
  bool operator<<(DCPS::Serializer& ser, const CompleteTypeObject& type_object);

  struct MinimalExtendedType {
    // Empty. Available for future extension
  };

  inline size_t find_size(const MinimalExtendedType&, size_t& size) { return size; }
  inline bool operator<<(DCPS::Serializer&, const MinimalExtendedType&) { return true; }

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
    MinimalAliasType       alias_type;
    MinimalAnnotationType  annotation_type;
    MinimalStructType      struct_type;
    MinimalUnionType       union_type;
    MinimalBitsetType      bitset_type;
    MinimalSequenceType    sequence_type;
    MinimalArrayType       array_type;
    MinimalMapType         map_type;
    MinimalEnumeratedType  enumerated_type;
    MinimalBitmaskType     bitmask_type;

    // ===================  Future extensibility  ============
    MinimalExtendedType    extended_type;

    MinimalTypeObject() {}

    MinimalTypeObject(const MinimalAliasType& alias)
      : kind(TK_ALIAS)
      , alias_type(alias)
    {}

    MinimalTypeObject(const MinimalAnnotationType& annotation)
      : kind(TK_ANNOTATION)
      , annotation_type(annotation)
    {}

    MinimalTypeObject(const MinimalStructType& struct_)
      : kind(TK_STRUCTURE)
      , struct_type(struct_)
    {}

    MinimalTypeObject(const MinimalUnionType& union_)
      : kind(TK_UNION)
      , union_type(union_)
    {}

    MinimalTypeObject(const MinimalBitsetType& bitset)
      : kind(TK_BITSET)
      , bitset_type(bitset)
    {}

    MinimalTypeObject(const MinimalSequenceType& sequence)
      : kind(TK_SEQUENCE)
      , sequence_type(sequence)
    {}

    MinimalTypeObject(const MinimalArrayType& array)
      : kind(TK_ARRAY)
      , array_type(array)
    {}

    MinimalTypeObject(const MinimalMapType& map)
      : kind(TK_MAP)
      , map_type(map)
    {}

    MinimalTypeObject(const MinimalEnumeratedType& enum_)
      : kind(TK_ENUM)
      , enumerated_type(enum_)
    {}

    MinimalTypeObject(const MinimalBitmaskType& bitmask)
      : kind(TK_BITMASK)
      , bitmask_type(bitmask)
    {}
  };

  size_t find_size(const MinimalTypeObject& type_object, size_t& size);
  bool operator<<(DCPS::Serializer& ser, const MinimalTypeObject& type_object);

  // @extensibility(APPENDABLE)  @nested
  // union TypeObject switch (octet) { // EquivalenceKind
  // case EK_COMPLETE:
  //   CompleteTypeObject   complete;
  // case EK_MINIMAL:
  //   MinimalTypeObject    minimal;
  // };

  struct TypeObject {
    ACE_CDR::Octet kind;
    CompleteTypeObject   complete;
    MinimalTypeObject    minimal;

    TypeObject(const CompleteTypeObject& a_complete)
      : kind(EK_COMPLETE)
      , complete(a_complete)
    {}

    TypeObject(const MinimalTypeObject& a_minimal)
      : kind(EK_MINIMAL)
      , minimal(a_minimal)
    {}
  };

  OpenDDS_Dcps_Export
  size_t find_size(const TypeObject& type_object);

  OpenDDS_Dcps_Export
  bool operator<<(DCPS::Serializer& ser, const TypeObject& type_object);

  typedef Sequence<TypeObject> TypeObjectSeq;

  // Set of TypeObjects representing a strong component: Equivalence class
  // for the Strong Connectivity relationship (mutual reachability between
  // types).
  // Ordered by fully qualified typename lexicographic order
  typedef TypeObjectSeq        StronglyConnectedComponent;

  struct TypeIdentifierTypeObjectPair {
    TypeIdentifier  type_identifier;
    TypeObject      type_object;
  };
  typedef Sequence<TypeIdentifierTypeObjectPair> TypeIdentifierTypeObjectPairSeq;

  struct TypeIdentifierPair {
    TypeIdentifier  type_identifier1;
    TypeIdentifier  type_identifier2;
  };
  typedef Sequence<TypeIdentifierPair> TypeIdentifierPairSeq;

  struct TypeIdentifierWithSize {
    TypeIdentifierPtr  type_id;
    ACE_CDR::ULong  typeobject_serialized_size;
  };
  size_t find_size(const TypeIdentifierWithSize& stru, size_t& size);
  bool operator<<(DCPS::Serializer& ser, const TypeIdentifierWithSize& stru);

  typedef Sequence<TypeIdentifierWithSize> TypeIdentifierWithSizeSeq;
  size_t find_size(const TypeIdentifierWithSizeSeq& stru, size_t& size);
  bool operator<<(DCPS::Serializer& ser, const TypeIdentifierWithSizeSeq& stru);

  struct TypeIdentifierWithDependencies {
    TypeIdentifierWithSize            typeid_with_size;
    // The total additional types related to minimal_type
    ACE_CDR::Long                             dependent_typeid_count;
    std::vector<TypeIdentifierWithSize>  dependent_typeids;
  };
  size_t find_size(const TypeIdentifierWithDependencies& stru, size_t& size);
  bool operator<<(DCPS::Serializer& ser, const TypeIdentifierWithDependencies& stru);

  typedef Sequence<TypeIdentifierWithDependencies> TypeIdentifierWithDependenciesSeq;

  // // This appears in the builtin DDS topics PublicationBuiltinTopicData
  // // and SubscriptionBuiltinTopicData

  struct TypeInformation {
    TypeIdentifierWithDependencies minimal;
    TypeIdentifierWithDependencies complete;
  };
  typedef Sequence<TypeInformation> TypeInformationSeq;


  OpenDDS_Dcps_Export
  size_t find_size(const TypeInformation& type_info);

  OpenDDS_Dcps_Export
  bool operator<<(DCPS::Serializer& ser, const TypeInformation& type_info);

  OpenDDS_Dcps_Export
  bool operator>>(DCPS::Serializer& ser, TypeInformation& type_info);

  OpenDDS_Dcps_Export
  TypeIdentifierPtr makeTypeIdentifier(const TypeObject& type_object);
} // namespace XTypes

namespace DCPS {

template<typename T>
const XTypes::TypeObject& getMinimalTypeObject();

template<typename T>
RcHandle<XTypes::TypeIdentifier> getMinimalTypeIdentifier();

template<> OpenDDS_Dcps_Export
RcHandle<XTypes::TypeIdentifier> getMinimalTypeIdentifier<void>();

template<> OpenDDS_Dcps_Export
RcHandle<XTypes::TypeIdentifier> getMinimalTypeIdentifier<ACE_CDR::Boolean>();

template<> OpenDDS_Dcps_Export
RcHandle<XTypes::TypeIdentifier> getMinimalTypeIdentifier<ACE_CDR::Octet>();

template<> OpenDDS_Dcps_Export
RcHandle<XTypes::TypeIdentifier> getMinimalTypeIdentifier<ACE_CDR::Short>();

template<> OpenDDS_Dcps_Export
RcHandle<XTypes::TypeIdentifier> getMinimalTypeIdentifier<ACE_CDR::Long>();

template<> OpenDDS_Dcps_Export
RcHandle<XTypes::TypeIdentifier> getMinimalTypeIdentifier<ACE_CDR::LongLong>();

template<> OpenDDS_Dcps_Export
RcHandle<XTypes::TypeIdentifier> getMinimalTypeIdentifier<ACE_CDR::UShort>();

template<> OpenDDS_Dcps_Export
RcHandle<XTypes::TypeIdentifier> getMinimalTypeIdentifier<ACE_CDR::ULong>();

template<> OpenDDS_Dcps_Export
RcHandle<XTypes::TypeIdentifier> getMinimalTypeIdentifier<ACE_CDR::ULongLong>();

template<> OpenDDS_Dcps_Export
RcHandle<XTypes::TypeIdentifier> getMinimalTypeIdentifier<ACE_CDR::Float>();

template<> OpenDDS_Dcps_Export
RcHandle<XTypes::TypeIdentifier> getMinimalTypeIdentifier<ACE_CDR::Double>();

template<> OpenDDS_Dcps_Export
RcHandle<XTypes::TypeIdentifier> getMinimalTypeIdentifier<ACE_CDR::LongDouble>();

template<> OpenDDS_Dcps_Export
RcHandle<XTypes::TypeIdentifier> getMinimalTypeIdentifier<ACE_CDR::Char>();

template<> OpenDDS_Dcps_Export
RcHandle<XTypes::TypeIdentifier> getMinimalTypeIdentifier<ACE_OutputCDR::from_wchar>();

template<> OpenDDS_Dcps_Export
RcHandle<XTypes::TypeIdentifier> getMinimalTypeIdentifier<ACE_CDR::Char*>();

template<> OpenDDS_Dcps_Export
RcHandle<XTypes::TypeIdentifier> getMinimalTypeIdentifier<ACE_CDR::WChar*>();

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif  /* OPENDDS_DCPS_TYPE_OBJECT_H */
