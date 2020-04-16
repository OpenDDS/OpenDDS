/*
 *
 *
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

    static TypeObjectHashId make(const EquivalenceKind& kind,
                                 const EquivalenceHash& hash)
    {
      TypeObjectHashId tohi;
      tohi.kind = kind;
      std::memcpy(tohi.hash, hash, sizeof hash);
      return tohi;
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

  // Mask used to remove the flags that do no affect assignability
  // Selects  T1, T2, O, M, K, D
  const MemberFlag MemberFlagMinimalMask = 0x003f;

  // Flags that apply to type declarationa and DO affect assignability
  // Depending on the flag it may not apply to all types
  // When not all, the applicable  types are listed
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

    static StringSTypeDefn make(const SBound bound)
    {
      StringSTypeDefn s;
      s.bound = bound;
      return s;
    }
  };

  // 4 Bytes
  struct StringLTypeDefn {
    LBound                  bound;

    static StringLTypeDefn make(const LBound bound)
    {
      StringLTypeDefn s;
      s.bound = bound;
      return s;
    }
  };

  struct PlainCollectionHeader {
    EquivalenceKind        equiv_kind;
    CollectionElementFlag  element_flags;

    static PlainCollectionHeader make(const EquivalenceKind& equiv_kind,
                                      const CollectionElementFlag& element_flags)
    {
      PlainCollectionHeader pch;
      pch.equiv_kind = equiv_kind;
      pch.element_flags = element_flags;
      return pch;
    }
  };

  struct PlainSequenceSElemDefn {
    PlainCollectionHeader  header;
    SBound                 bound;
    TypeIdentifierPtr element_identifier;

    static PlainSequenceSElemDefn make(const PlainCollectionHeader& header,
                                       const SBound& bound,
                                       const TypeIdentifierPtr& element_identifier)
    {
      PlainSequenceSElemDefn p;
      p.header = header;
      p.bound = bound;
      p.element_identifier = element_identifier;
      return p;
    }
  };

  struct PlainSequenceLElemDefn {
    PlainCollectionHeader  header;
    LBound                 bound;
    TypeIdentifierPtr element_identifier;

    static PlainSequenceLElemDefn make(const PlainCollectionHeader& header,
                                       const LBound& bound,
                                       const TypeIdentifierPtr& element_identifier)
    {
      PlainSequenceLElemDefn p;
      p.header = header;
      p.bound = bound;
      p.element_identifier = element_identifier;
      return p;
    }
  };

  struct PlainArraySElemDefn {
    PlainCollectionHeader  header;
    SBoundSeq              array_bound_seq;
    TypeIdentifierPtr element_identifier;

    static PlainArraySElemDefn make(const PlainCollectionHeader& header,
                                    const SBoundSeq& array_bound_seq,
                                    const TypeIdentifierPtr& element_identifier)
    {
      PlainArraySElemDefn p;
      p.header = header;
      p.array_bound_seq = array_bound_seq;
      p.element_identifier = element_identifier;
      return p;
    }
  };

  struct PlainArrayLElemDefn {
    PlainCollectionHeader  header;
    LBoundSeq              array_bound_seq;
    TypeIdentifierPtr element_identifier;

    static PlainArrayLElemDefn make(const PlainCollectionHeader& header,
                                    const LBoundSeq& array_bound_seq,
                                    const TypeIdentifierPtr& element_identifier)
    {
      PlainArrayLElemDefn p;
      p.header = header;
      p.array_bound_seq = array_bound_seq;
      p.element_identifier = element_identifier;
      return p;
    }
  };

  struct PlainMapSTypeDefn {
    PlainCollectionHeader  header;
    SBound                 bound;
    TypeIdentifierPtr element_identifier;
    CollectionElementFlag  key_flags;
    TypeIdentifierPtr key_identifier;

    static PlainMapSTypeDefn make(const PlainCollectionHeader&  header,
                                  const SBound&                 bound,
                                  const TypeIdentifierPtr& element_identifier,
                                  const CollectionElementFlag&  key_flags,
                                  const TypeIdentifierPtr& key_identifier)
    {
      PlainMapSTypeDefn p;
      p.header = header;
      p.bound = bound;
      p.element_identifier = element_identifier;
      p.key_flags = key_flags;
      p.key_identifier = key_identifier;
      return p;
    }
  };

  struct PlainMapLTypeDefn {
    PlainCollectionHeader  header;
    LBound                 bound;
    TypeIdentifierPtr element_identifier;
    CollectionElementFlag  key_flags;
    TypeIdentifierPtr key_identifier;

    static PlainMapLTypeDefn make(const PlainCollectionHeader&  header,
                                  const LBound&                 bound,
                                  const TypeIdentifierPtr& element_identifier,
                                  const CollectionElementFlag&  key_flags,
                                  const TypeIdentifierPtr& key_identifier)
    {
      PlainMapLTypeDefn p;
      p.header = header;
      p.bound = bound;
      p.element_identifier = element_identifier;
      p.key_flags = key_flags;
      p.key_identifier = key_identifier;
      return p;
    }
  };

  // Used for Types that have cyclic depencencies with other types
  struct StronglyConnectedComponentId {
    TypeObjectHashId sc_component_id; // Hash StronglyConnectedComponent
    ACE_CDR::Long   scc_length; // StronglyConnectedComponent.length
    ACE_CDR::Long   scc_index ; // identify type in Strongly Connected Comp.

    static StronglyConnectedComponentId make(const TypeObjectHashId& sc_component_id,
                                             const ACE_CDR::Long& scc_length,
                                             const ACE_CDR::Long& scc_index)
    {
      StronglyConnectedComponentId s;
      s.sc_component_id = sc_component_id;
      s.scc_length = scc_length;
      s.scc_index = scc_index;
      return s;
    }
  };

  // Future extensibility
  struct ExtendedTypeDefn {
    // Empty. Available for future extension

    static ExtendedTypeDefn make()
    {
      return ExtendedTypeDefn();
    }
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

    static TypeIdentifierPtr make(ACE_CDR::Octet k,
                                  const StringSTypeDefn& string_sdefn)
    {
      TypeIdentifierPtr ti = make_rch<TypeIdentifier>();
      ti->kind = k;
      ti->string_sdefn = string_sdefn;
      return ti;
    }

    static TypeIdentifierPtr make(ACE_CDR::Octet k,
                                  const StringLTypeDefn& string_ldefn)
    {
      TypeIdentifierPtr ti = make_rch<TypeIdentifier>();
      ti->kind = k;
      ti->string_ldefn = string_ldefn;
      return ti;
    }

    static TypeIdentifierPtr make(ACE_CDR::Octet k,
                                  const PlainSequenceSElemDefn& seq_sdefn)
    {
      TypeIdentifierPtr ti = make_rch<TypeIdentifier>();
      ti->kind = k;
      ti->seq_sdefn = seq_sdefn;
      return ti;
    }

    static TypeIdentifierPtr make(ACE_CDR::Octet k,
                                  const PlainSequenceLElemDefn& seq_ldefn)
    {
      TypeIdentifierPtr ti = make_rch<TypeIdentifier>();
      ti->kind = k;
      ti->seq_ldefn = seq_ldefn;
      return ti;
    }

    static TypeIdentifierPtr make(ACE_CDR::Octet k,
                                  const PlainArraySElemDefn& array_sdefn)
    {
      TypeIdentifierPtr ti = make_rch<TypeIdentifier>();
      ti->kind = k;
      ti->array_sdefn = array_sdefn;
      return ti;
    }

    static TypeIdentifierPtr make(ACE_CDR::Octet k,
                                  const PlainArrayLElemDefn& array_ldefn)
    {
      TypeIdentifierPtr ti = make_rch<TypeIdentifier>();
      ti->kind = k;
      ti->array_ldefn = array_ldefn;
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

    static CommonStructMember make(const MemberId& member_id,
                                   const StructMemberFlag& member_flags,
                                   const TypeIdentifierPtr& member_type_id)
    {
      CommonStructMember c;
      c.member_id = member_id;
      c.member_flags = member_flags;
      c.member_type_id = member_type_id;
      return c;
    }
  };

  // COMPLETE Details for a member of an aggregate type
  struct CompleteMemberDetail {
    MemberName                                 name;
    Optional<AppliedBuiltinMemberAnnotations>  ann_builtin;
    Optional<AppliedAnnotationSeq>             ann_custom;
  };

  // MINIMAL Details for a member of an aggregate type
  struct MinimalMemberDetail {
    NameHash                                  name_hash;

    static MinimalMemberDetail make(const std::string& name);
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
    static MinimalStructMember make(const CommonStructMember& common,
                                    const MinimalMemberDetail& detail)
    {
      MinimalStructMember msm;
      msm.common = common;
      msm.detail = detail;
      return msm;
    }
  };
  // Ordered by common.member_id
  // TODO: Implement sorting.
  typedef Sequence<MinimalStructMember> MinimalStructMemberSeq;

  struct AppliedBuiltinTypeAnnotations {
    Optional<AppliedVerbatimAnnotation> verbatim;  // @verbatim(...)
  };

  struct MinimalTypeDetail {
    // Empty. Available for future extension

    static MinimalTypeDetail make()
    {
      return MinimalTypeDetail();
    }
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

    static MinimalStructHeader make(const TypeIdentifierPtr& base_type,
                                    const MinimalTypeDetail& detail)
    {
      MinimalStructHeader msh;
      msh.base_type = base_type;
      msh.detail = detail;
      return msh;
    }
  };

  struct CompleteStructType {
    StructTypeFlag             struct_flags;
    CompleteStructHeader       header;
    CompleteStructMemberSeq    member_seq;
  };

  struct MinimalStructType {
    StructTypeFlag             struct_flags;
    MinimalStructHeader        header;
    MinimalStructMemberSeq     member_seq;

    static MinimalStructType make(const StructTypeFlag& struct_flags,
                                  const MinimalStructHeader& header,
                                  const MinimalStructMemberSeq& member_seq)
    {
      MinimalStructType mst;
      mst.struct_flags = struct_flags;
      mst.header = header;
      mst.member_seq = member_seq;
      return mst;
    }
  };

  // --- Union: ----------------------------------------------------------

  // Case labels that apply to a member of a union type
  // Ordered by their values
  typedef Sequence<ACE_CDR::Long> UnionCaseLabelSeq;

  struct CommonUnionMember {
    MemberId                    member_id;
    UnionMemberFlag             member_flags;
    TypeIdentifierPtr    type_id;
    UnionCaseLabelSeq           label_seq;
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
  };
  // Ordered by MinimalUnionMember.common.member_id
  typedef Sequence<MinimalUnionMember> MinimalUnionMemberSeq;

  struct CommonDiscriminatorMember {
    UnionDiscriminatorFlag       member_flags;
    TypeIdentifierPtr     type_id;
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
  };

  struct CompleteUnionHeader {
    CompleteTypeDetail          detail;
  };

  struct MinimalUnionHeader {
    MinimalTypeDetail           detail;
  };

  struct CompleteUnionType {
    UnionTypeFlag                union_flags;
    CompleteUnionHeader          header;
    CompleteDiscriminatorMember  discriminator;
    CompleteUnionMemberSeq       member_seq;
  };

  struct MinimalUnionType {
    UnionTypeFlag                union_flags;
    MinimalUnionHeader           header;
    MinimalDiscriminatorMember   discriminator;
    MinimalUnionMemberSeq        member_seq;

    static MinimalUnionType make(const UnionTypeFlag& union_flags,
                                 const MinimalUnionHeader& header,
                                 const MinimalDiscriminatorMember& discriminator,
                                 const MinimalUnionMemberSeq& member_seq)
    {
      MinimalUnionType mut;
      mut.union_flags = union_flags;
      mut.header = header;
      mut.discriminator = discriminator;
      mut.member_seq = member_seq;
      return mut;
    }
  };

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

  struct MinimalAnnotationType {
    AnnotationTypeFlag             annotation_flag;
    MinimalAnnotationHeader        header;
    MinimalAnnotationParameterSeq  member_seq;
  };


  // --- Alias: ----------------------------------------------------------
  struct CommonAliasBody {
    AliasMemberFlag          related_flags;
    TypeIdentifierPtr related_type;
  };

  struct CompleteAliasBody {
    CommonAliasBody       common;
    Optional<AppliedBuiltinMemberAnnotations>  ann_builtin;
    Optional<AppliedAnnotationSeq>             ann_custom;
  };

  struct MinimalAliasBody {
    CommonAliasBody       common;
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

  struct MinimalAliasType {
    AliasTypeFlag         alias_flags;
    MinimalAliasHeader    header;
    MinimalAliasBody      body;
  };

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

  struct MinimalSequenceType {
    CollectionTypeFlag         collection_flag;
    MinimalCollectionHeader    header;
    MinimalCollectionElement   element;
  };

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

  struct MinimalArrayType  {
    CollectionTypeFlag         collection_flag;
    MinimalArrayHeader         header;
    MinimalCollectionElement   element;
  };

  // --- Map: ------------------------------------------------------
  struct CompleteMapType {
    CollectionTypeFlag            collection_flag;
    CompleteCollectionHeader      header;
    CompleteCollectionElement     key;
    CompleteCollectionElement     element;
  };

  struct MinimalMapType {
    CollectionTypeFlag          collection_flag;
    MinimalCollectionHeader     header;
    MinimalCollectionElement    key;
    MinimalCollectionElement    element;
  };

  // --- Enumeration: ----------------------------------------------------
  typedef ACE_CDR::UShort BitBound;

  // Constant in an enumerated type
  struct CommonEnumeratedLiteral {
    ACE_CDR::Long                     value;
    EnumeratedLiteralFlag    flags;
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
  };
  // Ordered by EnumeratedLiteral.common.value
  typedef Sequence<MinimalEnumeratedLiteral> MinimalEnumeratedLiteralSeq;

  struct CommonEnumeratedHeader {
    BitBound                bit_bound;
  };

  struct CompleteEnumeratedHeader {
    CommonEnumeratedHeader  common;
    CompleteTypeDetail      detail;
  };

  struct MinimalEnumeratedHeader {
    CommonEnumeratedHeader  common;
  };

  // Enumerated type
  struct CompleteEnumeratedType  {
    EnumTypeFlag                    enum_flags; // unused
    CompleteEnumeratedHeader        header;
    CompleteEnumeratedLiteralSeq    literal_seq;
  };

  // Enumerated type
  struct MinimalEnumeratedType  {
    EnumTypeFlag                  enum_flags; // unused
    MinimalEnumeratedHeader       header;
    MinimalEnumeratedLiteralSeq   literal_seq;
  };

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

  struct MinimalBitmaskType {
    BitmaskTypeFlag          bitmask_flags; // unused
    MinimalBitmaskHeader     header;
    MinimalBitflagSeq        flag_seq;
  };

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

  struct MinimalBitsetType  {
    BitsetTypeFlag       bitset_flags; // unused
    MinimalBitsetHeader  header;
    MinimalBitfieldSeq   field_seq;
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

    static MinimalTypeObject make(const MinimalAliasType& alias)
    {
      MinimalTypeObject mto;
      mto.kind = TK_ALIAS;
      mto.alias_type = alias;
      return mto;
    }

    static MinimalTypeObject make(const MinimalAnnotationType& annotation)
    {
      MinimalTypeObject mto;
      mto.kind = TK_ANNOTATION;
      mto.annotation_type = annotation;
      return mto;
    }

    static MinimalTypeObject make(const MinimalStructType& struct_)
    {
      MinimalTypeObject mto;
      mto.kind = TK_STRUCTURE;
      mto.struct_type = struct_;
      return mto;
    }

    static MinimalTypeObject make(const MinimalUnionType& union_)
    {
      MinimalTypeObject mto;
      mto.kind = TK_UNION;
      mto.union_type = union_;
      return mto;
    }

    static MinimalTypeObject make(const MinimalBitsetType& bitset)
    {
      MinimalTypeObject mto;
      mto.kind = TK_BITSET;
      mto.bitset_type = bitset;
      return mto;
    }

    static MinimalTypeObject make(const MinimalSequenceType& sequence)
    {
      MinimalTypeObject mto;
      mto.kind = TK_SEQUENCE;
      mto.sequence_type = sequence;
      return mto;
    }

    static MinimalTypeObject make(const MinimalArrayType& array)
    {
      MinimalTypeObject mto;
      mto.kind = TK_ARRAY;
      mto.array_type = array;
      return mto;
    }

    static MinimalTypeObject make(const MinimalMapType& map)
    {
      MinimalTypeObject mto;
      mto.kind = TK_MAP;
      mto.map_type = map;
      return mto;
    }

    static MinimalTypeObject make(const MinimalEnumeratedType& enum_)
    {
      MinimalTypeObject mto;
      mto.kind = TK_ENUM;
      mto.enumerated_type = enum_;
      return mto;
    }

    static MinimalTypeObject make(const MinimalBitmaskType& bitmask)
    {
      MinimalTypeObject mto;
      mto.kind = TK_BITMASK;
      mto.bitmask_type = bitmask;
      return mto;
    }

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
    CompleteTypeObject   complete;
    MinimalTypeObject    minimal;

    static TypeObject make(const CompleteTypeObject& complete)
    {
      TypeObject to;
      to.kind = EK_COMPLETE;
      to.complete = complete;
      return to;
    }

    static TypeObject make(const MinimalTypeObject& minimal)
    {
      TypeObject to;
      to.kind = EK_MINIMAL;
      to.minimal = minimal;
      return to;
    }
  };

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

  struct TypeIdentfierWithSize {
    TypeIdentifier  type_id;
    ACE_CDR::ULong  typeobject_serialized_size;
  };
  typedef Sequence<TypeIdentfierWithSize> TypeIdentfierWithSizeSeq;

  struct TypeIdentifierWithDependencies {
    TypeIdentfierWithSize            typeid_with_size;
    // The total additional types related to minimal_type
    ACE_CDR::Long                             dependent_typeid_count;
    std::vector<TypeIdentfierWithSize>  dependent_typeids;
  };
  typedef Sequence<TypeIdentifierWithDependencies> TypeIdentifierWithDependenciesSeq;

  // // This appears in the builtin DDS topics PublicationBuiltinTopicData
  // // and SubscriptionBuiltinTopicData
  // @extensibility(MUTABLE) @nested
  // struct TypeInformation {
  //   @id(0x1001) TypeIdentifierWithDependencies minimal;
  //   @id(0x1002) TypeIdentifierWithDependencies complete;
  // };
  // typedef sequence<TypeInformation> TypeInformationSeq;

  TypeIdentifierPtr makeTypeIdentifier(const TypeObject& type_object);

} // namespace XTypes

namespace DCPS {

template<typename T>
const XTypes::TypeObject& getTypeObject();

template<typename T>
RcHandle<XTypes::TypeIdentifier> getTypeIdentifier();

template<>
RcHandle<XTypes::TypeIdentifier> getTypeIdentifier<void>();

template<>
RcHandle<XTypes::TypeIdentifier> getTypeIdentifier<ACE_CDR::Boolean>();

template<>
RcHandle<XTypes::TypeIdentifier> getTypeIdentifier<ACE_CDR::Octet>();

template<>
RcHandle<XTypes::TypeIdentifier> getTypeIdentifier<ACE_CDR::Short>();

template<>
RcHandle<XTypes::TypeIdentifier> getTypeIdentifier<ACE_CDR::Long>();

template<>
RcHandle<XTypes::TypeIdentifier> getTypeIdentifier<ACE_CDR::LongLong>();

template<>
RcHandle<XTypes::TypeIdentifier> getTypeIdentifier<ACE_CDR::UShort>();

template<>
RcHandle<XTypes::TypeIdentifier> getTypeIdentifier<ACE_CDR::ULong>();

template<>
RcHandle<XTypes::TypeIdentifier> getTypeIdentifier<ACE_CDR::ULongLong>();

template<>
RcHandle<XTypes::TypeIdentifier> getTypeIdentifier<ACE_CDR::Float>();

template<>
RcHandle<XTypes::TypeIdentifier> getTypeIdentifier<ACE_CDR::Double>();

template<>
RcHandle<XTypes::TypeIdentifier> getTypeIdentifier<ACE_CDR::LongDouble>();

template<>
RcHandle<XTypes::TypeIdentifier> getTypeIdentifier<ACE_CDR::Char>();

template<>
RcHandle<XTypes::TypeIdentifier> getTypeIdentifier<ACE_CDR::WChar>();

template<>
RcHandle<XTypes::TypeIdentifier> getTypeIdentifier<ACE_CDR::Char*>();

template<>
RcHandle<XTypes::TypeIdentifier> getTypeIdentifier<ACE_CDR::WChar*>();

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif  /* OPENDDS_DCPS_TYPE_OBJECT_H */
