/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "typeobject_generator.h"

#include "be_extern.h"
#include "be_util.h"
#include "dds_visitor.h"
#include "topic_keys.h"
#include "utl_identifier.h"

#include <dds/DCPS/Hash.h>

using std::string;
using namespace AstTypeClassification;

namespace {

OpenDDS::XTypes::TypeFlag
extensibility_to_type_flag(ExtensibilityKind exten)
{
  switch (exten) {
  case extensibilitykind_final:
    return OpenDDS::XTypes::IS_FINAL;
  case extensibilitykind_appendable:
    return OpenDDS::XTypes::IS_APPENDABLE;
  case extensibilitykind_mutable:
    return OpenDDS::XTypes::IS_MUTABLE;
  }

  return 0;
}

OpenDDS::XTypes::MemberFlag
try_construct_to_member_flag(TryConstructFailAction trycon)
{
  switch (trycon) {
  case tryconstructfailaction_discard:
    return OpenDDS::XTypes::TryConstructDiscardValue;
  case tryconstructfailaction_use_default:
    return OpenDDS::XTypes::TryConstructUseDefaultValue;
  case tryconstructfailaction_trim:
    return OpenDDS::XTypes::TryConstructTrimValue;
  }

  return 0;
}

ACE_CDR::Long
to_long(const AST_Expression::AST_ExprValue& ev)
{
  switch (ev.et) {
  case AST_Expression::EV_octet:
    return ev.u.oval;
  case AST_Expression::EV_short:
    return ev.u.sval;
  case AST_Expression::EV_ushort:
    return ev.u.usval;
  case AST_Expression::EV_long:
    return ev.u.lval;
  case AST_Expression::EV_ulong:
    return ev.u.ulval;
  case AST_Expression::EV_wchar:
    return ev.u.wcval;
  case AST_Expression::EV_char:
    return ev.u.cval;
  case AST_Expression::EV_bool:
    return ev.u.bval;
  case AST_Expression::EV_enum:
    return ev.u.eval;
  default:
    be_util::misc_error_and_abort("Illegal conversion to long");
    return 0;
  }
}

template <typename T, typename P>
struct Printer {
  Printer(const T& a_value)
    : value(a_value)
  {}

  void print_on(std::ostream& out) const
  {
    out << static_cast<P>(value);
  }

  const T value;
};

template <typename T, typename P>
std::ostream&
operator<<(std::ostream& out, const Printer<T, P>& printer)
{
  printer.print_on(out);
  return out;
}

struct SBoundPrinter : public Printer<OpenDDS::XTypes::SBound, unsigned int> {
  SBoundPrinter(const OpenDDS::XTypes::SBound a_value) : Printer(a_value) {}
};

struct LBoundPrinter : public Printer<OpenDDS::XTypes::LBound, unsigned int> {
  LBoundPrinter(const OpenDDS::XTypes::LBound a_value) : Printer(a_value) {}
};

struct EquivalenceKindPrinter : public Printer<OpenDDS::XTypes::EquivalenceKind, unsigned int> {
  EquivalenceKindPrinter(const OpenDDS::XTypes::EquivalenceKind a_value) : Printer(a_value) {}
};

struct CollectionElementFlagsPrinter : public Printer<OpenDDS::XTypes::CollectionElementFlag, unsigned int> {
  CollectionElementFlagsPrinter(const OpenDDS::XTypes::CollectionElementFlag a_value) : Printer(a_value) {}
};

struct AliasTypeFlagsPrinter : public Printer<OpenDDS::XTypes::AliasTypeFlag, unsigned int> {
  AliasTypeFlagsPrinter(const OpenDDS::XTypes::AliasTypeFlag a_value) : Printer(a_value) {}
};

struct StructTypeFlagsPrinter : public Printer<OpenDDS::XTypes::StructTypeFlag, unsigned int> {
  StructTypeFlagsPrinter(const OpenDDS::XTypes::StructTypeFlag a_value) : Printer(a_value) {}
};

struct EnumTypeFlagsPrinter : public Printer<OpenDDS::XTypes::EnumTypeFlag, unsigned int> {
  EnumTypeFlagsPrinter(const OpenDDS::XTypes::EnumTypeFlag a_value) : Printer(a_value) {}
};

struct UnionTypeFlagsPrinter : public Printer<OpenDDS::XTypes::UnionTypeFlag, unsigned int> {
  UnionTypeFlagsPrinter(const OpenDDS::XTypes::UnionTypeFlag a_value) : Printer(a_value) {}
};

struct AliasMemberFlagsPrinter : public Printer<OpenDDS::XTypes::AliasMemberFlag, unsigned int> {
  AliasMemberFlagsPrinter(const OpenDDS::XTypes::AliasMemberFlag a_value) : Printer(a_value) {}
};

struct StructMemberFlagsPrinter : public Printer<OpenDDS::XTypes::StructMemberFlag, unsigned int> {
  StructMemberFlagsPrinter(const OpenDDS::XTypes::StructMemberFlag a_value) : Printer(a_value) {}
};

struct UnionMemberFlagsPrinter : public Printer<OpenDDS::XTypes::UnionMemberFlag, unsigned int> {
  UnionMemberFlagsPrinter(const OpenDDS::XTypes::StructMemberFlag a_value) : Printer(a_value) {}
};

struct EnumeratedLiteralFlagsPrinter : public Printer<OpenDDS::XTypes::EnumeratedLiteralFlag, unsigned int> {
  EnumeratedLiteralFlagsPrinter(const OpenDDS::XTypes::StructMemberFlag a_value) : Printer(a_value) {}
};

struct UnionDiscriminatorFlagsPrinter : public Printer<OpenDDS::XTypes::UnionDiscriminatorFlag, unsigned int> {
  UnionDiscriminatorFlagsPrinter(const OpenDDS::XTypes::StructMemberFlag a_value) : Printer(a_value) {}
};

struct MemberIdPrinter : public Printer<OpenDDS::XTypes::UnionDiscriminatorFlag, unsigned int> {
  MemberIdPrinter(const OpenDDS::XTypes::StructMemberFlag a_value) : Printer(a_value) {}
};

struct BitBoundPrinter : public Printer<OpenDDS::XTypes::UnionDiscriminatorFlag, unsigned int> {
  BitBoundPrinter(const OpenDDS::XTypes::StructMemberFlag a_value) : Printer(a_value) {}
};

std::ostream&
operator<<(std::ostream& out, const OpenDDS::XTypes::TypeIdentifier& ti);

std::ostream&
operator<<(std::ostream& out, const OpenDDS::XTypes::SBoundSeq& seq)
{
  out << "XTypes::SBoundSeq()";
  for (OpenDDS::XTypes::SBoundSeq::const_iterator pos = seq.begin(), limit = seq.end(); pos != limit; ++pos) {
    out << ".append(" << SBoundPrinter(*pos) << ")";
  }
  return out;
}

std::ostream&
operator<<(std::ostream& out, const OpenDDS::XTypes::LBoundSeq& seq)
{
  out << "XTypes::LBoundSeq()";
  for (OpenDDS::XTypes::LBoundSeq::const_iterator pos = seq.begin(), limit = seq.end(); pos != limit; ++pos) {
    out << ".append(" << LBoundPrinter(*pos) << ")";
  }
  return out;
}

std::ostream&
operator<<(std::ostream& out, const OpenDDS::XTypes::StringSTypeDefn& string_sdefn)
{
  return out << "XTypes::StringSTypeDefn(" << SBoundPrinter(string_sdefn.bound) << ")";
}

std::ostream&
operator<<(std::ostream& out, const OpenDDS::XTypes::StringLTypeDefn& string_ldefn)
{
  return out << "XTypes::StringSTypeDefn(" << LBoundPrinter(string_ldefn.bound) << ")";
}


std::ostream&
operator<<(std::ostream& out, const OpenDDS::XTypes::PlainCollectionHeader& header)
{
  return out
    << "XTypes::PlainCollectionHeader("
    << EquivalenceKindPrinter(header.equiv_kind) << ","
    << CollectionElementFlagsPrinter(header.element_flags)
    << ")";
}

std::ostream&
operator<<(std::ostream& out, const OpenDDS::XTypes::PlainSequenceSElemDefn& seq_sdefn)
{
  return out
    << "XTypes::PlainSequenceSElemDefn("
    << seq_sdefn.header << ","
    << SBoundPrinter(seq_sdefn.bound) << ","
    << *seq_sdefn.element_identifier
    << ")";
}

std::ostream&
operator<<(std::ostream& out, const OpenDDS::XTypes::PlainSequenceLElemDefn& seq_ldefn)
{
  return out
    << "XTypes::PlainSequenceLElemDefn("
    << seq_ldefn.header << ","
    << SBoundPrinter(seq_ldefn.bound) << ","
    << *seq_ldefn.element_identifier
    << ")";
}

std::ostream&
operator<<(std::ostream& out, const OpenDDS::XTypes::PlainArraySElemDefn& array_sdefn)
{
  return out
    << "XTypes::PlainArraySElemDefn("
    << array_sdefn.header << ","
    << array_sdefn.array_bound_seq << ","
    << *array_sdefn.element_identifier
    << ")";
}

std::ostream&
operator<<(std::ostream& out, const OpenDDS::XTypes::PlainArrayLElemDefn& array_ldefn)
{
  return out
    << "XTypes::PlainArrayLElemDefn("
    << array_ldefn.header << ","
    << array_ldefn.array_bound_seq << ","
    << *array_ldefn.element_identifier
    << ")";
}

std::ostream&
operator<<(std::ostream& out, const OpenDDS::XTypes::PlainMapSTypeDefn& map_sdefn)
{
  return out
    << "XTypes::PlainMapSTypeDefn("
    << map_sdefn.header << ","
    << SBoundPrinter(map_sdefn.bound) << ","
    << *map_sdefn.element_identifier << ","
    << CollectionElementFlagsPrinter(map_sdefn.key_flags) << ","
    << *map_sdefn.key_identifier
    << ")";
}

std::ostream&
operator<<(std::ostream& out, const OpenDDS::XTypes::PlainMapLTypeDefn& map_ldefn)
{
  return out
    << "XTypes::PlainMapLTypeDefn("
    << map_ldefn.header << ","
    << LBoundPrinter(map_ldefn.bound) << ","
    << *map_ldefn.element_identifier << ","
    << CollectionElementFlagsPrinter(map_ldefn.key_flags) << ","
    << *map_ldefn.key_identifier
    << ")";
}

std::ostream&
operator<<(std::ostream& out, const OpenDDS::XTypes::EquivalenceHash& equivalence_hash)
{
  return out
    << '{'
    << static_cast<int>(equivalence_hash[0])
    << ','
    << static_cast<int>(equivalence_hash[1])
    << ','
    << static_cast<int>(equivalence_hash[2])
    << ','
    << static_cast<int>(equivalence_hash[3])
    << ','
    << static_cast<int>(equivalence_hash[4])
    << ','
    << static_cast<int>(equivalence_hash[5])
    << ','
    << static_cast<int>(equivalence_hash[6])
    << ','
    << static_cast<int>(equivalence_hash[7])
    << ','
    << static_cast<int>(equivalence_hash[8])
    << ','
    << static_cast<int>(equivalence_hash[9])
    << ','
    << static_cast<int>(equivalence_hash[10])
    << ','
    << static_cast<int>(equivalence_hash[11])
    << ','
    << static_cast<int>(equivalence_hash[12])
    << ','
    << static_cast<int>(equivalence_hash[13])
    << '}';
}

std::ostream&
operator<<(std::ostream& out, const OpenDDS::XTypes::TypeObjectHashId& id)
{
  return out
    << "XTypes::TypeObjectHashId("
    << EquivalenceKindPrinter(id.kind) << ","
    << id.hash
    << ")";
}

std::ostream&
operator<<(std::ostream& out, const OpenDDS::XTypes::StronglyConnectedComponentId& scc)
{
  return out
    << "XTypes::StronglyConnectedComponentId("
    << scc.sc_component_id << ", "
    << scc.scc_length << ", "
    << scc.scc_index
    << ")";
}

std::ostream&
operator<<(std::ostream& out, const OpenDDS::XTypes::TypeIdentifier& ti)
{
  out << "XTypes::TypeIdentifier(";
  switch (ti.kind()) {
  case OpenDDS::XTypes::TK_NONE:
    out << "XTypes::TK_NONE";
    break;
  case OpenDDS::XTypes::TK_BOOLEAN:
    out << "XTypes::TK_BOOLEAN";
    break;
  case OpenDDS::XTypes::TK_BYTE:
    out << "XTypes::TK_BYTE";
    break;
  case OpenDDS::XTypes::TK_INT16:
    out << "XTypes::TK_INT16";
    break;
  case OpenDDS::XTypes::TK_INT32:
    out << "XTypes::TK_INT32";
    break;
  case OpenDDS::XTypes::TK_INT64:
    out << "XTypes::TK_INT64";
    break;
  case OpenDDS::XTypes::TK_UINT16:
    out << "XTypes::TK_UINT16";
    break;
  case OpenDDS::XTypes::TK_UINT32:
    out << "XTypes::TK_UINT32";
    break;
  case OpenDDS::XTypes::TK_UINT64:
    out << "XTypes::TK_UINT64";
    break;
  case OpenDDS::XTypes::TK_FLOAT32:
    out << "XTypes::TK_FLOAT32";
    break;
  case OpenDDS::XTypes::TK_FLOAT64:
    out << "XTypes::TK_FLOAT64";
    break;
  case OpenDDS::XTypes::TK_FLOAT128:
    out << "XTypes::TK_FLOAT128";
    break;
  case OpenDDS::XTypes::TK_CHAR8:
    out << "XTypes::TK_CHAR8";
    break;
  case OpenDDS::XTypes::TK_CHAR16:
    out << "XTypes::TK_CHAR16";
    break;
  case OpenDDS::XTypes::TI_STRING8_SMALL:
    out << "XTypes::TI_STRING8_SMALL," << ti.string_sdefn();
    break;
  case OpenDDS::XTypes::TI_STRING16_SMALL:
    out << "XTypes::TI_STRING16_SMALL," << ti.string_sdefn();
    break;
  case OpenDDS::XTypes::TI_STRING8_LARGE:
    out << "XTypes::TI_STRING8_LARGE," << ti.string_ldefn();
    break;
  case OpenDDS::XTypes::TI_STRING16_LARGE:
    out << "XTypes::TI_STRING16_LARGE," << ti.string_ldefn();
    break;
  case OpenDDS::XTypes::TI_PLAIN_SEQUENCE_SMALL:
    out << "XTypes::TI_PLAIN_SEQUENCE_SMALL," << ti.seq_sdefn();
    break;
  case OpenDDS::XTypes::TI_PLAIN_SEQUENCE_LARGE:
    out << "XTypes::TI_PLAIN_SEQUENCE_LARGE," << ti.seq_ldefn();
    break;
  case OpenDDS::XTypes::TI_PLAIN_ARRAY_SMALL:
    out << "XTypes::TI_PLAIN_ARRAY_SMALL," << ti.array_sdefn();
    break;
  case OpenDDS::XTypes::TI_PLAIN_ARRAY_LARGE:
    out << "XTypes::TI_PLAIN_ARRAY_LARGE," << ti.array_ldefn();
    break;
  case OpenDDS::XTypes::TI_PLAIN_MAP_SMALL:
    out << "XTypes::TI_PLAIN_MAP_SMALL," << ti.map_sdefn();
    break;
  case OpenDDS::XTypes::TI_PLAIN_MAP_LARGE:
    out << "XTypes::TI_PLAIN_MAP_LARGE," << ti.map_ldefn();
    break;
  case OpenDDS::XTypes::TI_STRONGLY_CONNECTED_COMPONENT:
    out << "XTypes::TI_STRONGLY_CONNECTED_COMPONENT," << ti.sc_component_id();
    break;
  case OpenDDS::XTypes::EK_COMPLETE:
    out << "XTypes::EK_COMPLETE," << ti.equivalence_hash();
    break;
  case OpenDDS::XTypes::EK_MINIMAL:
    out << "XTypes::EK_MINIMAL," << ti.equivalence_hash();
    break;
  default:
    be_util::misc_error_and_abort("Extended type definitions output is not supported");
    break;
  }

  out << ')';
  return out;
}

std::ostream&
operator<<(std::ostream& out, const OpenDDS::XTypes::MinimalAliasHeader&)
{
  return out << "XTypes::MinimalAliasHeader()";
}

std::ostream&
operator<<(std::ostream& out, const OpenDDS::XTypes::CommonAliasBody& common)
{
  return out
    << "XTypes::CommonAliasBody("
    << AliasMemberFlagsPrinter(common.related_flags) << ","
    << common.related_type
    << ")";
}

std::ostream&
operator<<(std::ostream& out, const OpenDDS::XTypes::MinimalAliasBody& body)
{
  return out << "XTypes::MinimalAliasBody(" << body.common << ")";
}

std::ostream&
operator<<(std::ostream& out, const OpenDDS::XTypes::MinimalAliasType& alias_type)
{
  return out
    << "XTypes::MinimalAliasType("
    << AliasTypeFlagsPrinter(alias_type.alias_flags) << ","
    << alias_type.header << ","
    << alias_type.body
    << ")";
}

std::ostream&
operator<<(std::ostream& out, const OpenDDS::XTypes::MinimalTypeDetail&)
{
  return out << "XTypes::MinimalTypeDetail()";
}

std::ostream&
operator<<(std::ostream& out, const OpenDDS::XTypes::MinimalStructHeader& header)
{
  return out
    << "XTypes::MinimalStructHeader("
    << header.base_type << ", "
    << header.detail
    << ")";
}


std::ostream&
operator<<(std::ostream& out, const OpenDDS::XTypes::CommonStructMember& common)
{
  return out
    << "XTypes::CommonStructMember("
    << MemberIdPrinter(common.member_id) << ","
    << StructMemberFlagsPrinter(common.member_flags) << ","
    << common.member_type_id
    << ")";
}

std::ostream&
operator<<(std::ostream& out, const OpenDDS::XTypes::NameHash& name_hash)
{
  return out
    << '{'
    << static_cast<int>(name_hash[0])
    << ','
    << static_cast<int>(name_hash[1])
    << ','
    << static_cast<int>(name_hash[2])
    << ','
    << static_cast<int>(name_hash[3])
    << '}';
}

std::ostream&
operator<<(std::ostream& out, const OpenDDS::XTypes::MinimalMemberDetail& detail)
{
  return out << "XTypes::MinimalMemberDetail(" << detail.name_hash << ")";
}

std::ostream&
operator<<(std::ostream& out, const OpenDDS::XTypes::MinimalStructMember& member)
{
  return out
    << "XTypes::MinimalStructMember("
    << member.common << ","
    << member.detail
    << ")";
}

std::ostream&
operator<<(std::ostream& out, const OpenDDS::XTypes::MinimalStructMemberSeq& member_seq)
{
  out << "XTypes::MinimalStructMemberSeq()";
  for (OpenDDS::XTypes::MinimalStructMemberSeq::const_iterator pos = member_seq.begin(), limit = member_seq.end(); pos != limit; ++pos) {
    out << ".append(" << *pos << ")";
  }
  return out;
}

std::ostream&
operator<<(std::ostream& out, const OpenDDS::XTypes::MinimalStructType& struct_type)
{
  return out
    << "XTypes::MinimalStructType("
    << StructTypeFlagsPrinter(struct_type.struct_flags) << ","
    << struct_type.header << ","
    << struct_type.member_seq
    << ")";
}


std::ostream&
operator<<(std::ostream& out, const OpenDDS::XTypes::CommonEnumeratedHeader& common)
{
  return out << "XTypes::CommonEnumeratedHeader(" << BitBoundPrinter(common.bit_bound) << ")";
}

std::ostream&
operator<<(std::ostream& out, const OpenDDS::XTypes::MinimalEnumeratedHeader& header)
{
  return out << "XTypes::MinimalEnumeratedHeader(" << header.common << ")";
}

std::ostream&
operator<<(std::ostream& out, const OpenDDS::XTypes::CommonEnumeratedLiteral& common)
{
  return out
    << "XTypes::CommonEnumeratedLiteral("
    << common.value << ","
    << EnumeratedLiteralFlagsPrinter(common.flags)
    << ")";
}

std::ostream&
operator<<(std::ostream& out, const OpenDDS::XTypes::MinimalEnumeratedLiteral& literal)
{
  return out
    << "XTypes::MinimalEnumeratedLiteral("
    << literal.common << ","
    << literal.detail
    << ")";
}

std::ostream&
operator<<(std::ostream& out, const OpenDDS::XTypes::MinimalEnumeratedLiteralSeq& literal_seq)
{
  out << "XTypes::MinimalEnumeratedLiteralSeq()";
  for (OpenDDS::XTypes::MinimalEnumeratedLiteralSeq::const_iterator pos = literal_seq.begin(), limit = literal_seq.end(); pos != limit; ++pos) {
    out << ".append(" << *pos << ")";
  }
  return out;
}

std::ostream&
operator<<(std::ostream& out, const OpenDDS::XTypes::MinimalEnumeratedType& enumerated_type)
{
  return out
    << "XTypes::MinimalEnumeratedType("
    << EnumTypeFlagsPrinter(enumerated_type.enum_flags) << ","
    << enumerated_type.header << ","
    << enumerated_type.literal_seq
    << ")";
}

std::ostream&
operator<<(std::ostream& out, const OpenDDS::XTypes::MinimalUnionHeader& header)
{
  return out << "XTypes::MinimalUnionHeader(" << header.detail << ")";
}

std::ostream&
operator<<(std::ostream& out, const OpenDDS::XTypes::CommonDiscriminatorMember& member)
{
  return out
    << "XTypes::CommonDiscriminatorMember("
    << UnionDiscriminatorFlagsPrinter(member.member_flags) << ","
    << member.type_id
   << ")";
}

std::ostream&
operator<<(std::ostream& out, const OpenDDS::XTypes::MinimalDiscriminatorMember& member)
{
  return out << "XTypes::MinimalDiscriminatorMember(" << member.common << ")";
}

std::ostream&
operator<<(std::ostream& out, const OpenDDS::XTypes::UnionCaseLabelSeq& seq)
{
  out << "XTypes::UnionCaseLabelSeq()";
  for (OpenDDS::XTypes::UnionCaseLabelSeq::const_iterator pos = seq.begin(), limit = seq.end(); pos != limit; ++pos) {
    out << ".append(" << *pos << ")";
  }
  return out;
}

std::ostream&
operator<<(std::ostream& out, const OpenDDS::XTypes::CommonUnionMember& member)
{
  return out
    << "XTypes::CommonUnionMember("
    << MemberIdPrinter(member.member_id) << ","
    << UnionMemberFlagsPrinter(member.member_flags) << ","
    << member.type_id << ","
    << member.label_seq
    << ")";
}

std::ostream&
operator<<(std::ostream& out, const OpenDDS::XTypes::MinimalUnionMember& member)
{
  return out
    << "XTypes::MinimalUnionMember("
    << member.common << ","
    << member.detail
    << ")";
}

std::ostream&
operator<<(std::ostream& out, const OpenDDS::XTypes::MinimalUnionMemberSeq& seq)
{
  out << "XTypes::MinimalUnionMemberSeq()";
  for (OpenDDS::XTypes::MinimalUnionMemberSeq::const_iterator pos = seq.begin(), limit = seq.end(); pos != limit; ++pos) {
    out << ".append(" << *pos << ")";
  }
  return out;
}

std::ostream&
operator<<(std::ostream& out, const OpenDDS::XTypes::MinimalUnionType& union_type)
{
  return out
    << "XTypes::MinimalUnionType("
    << UnionTypeFlagsPrinter(union_type.union_flags) << ","
    << union_type.header << ","
    << union_type.discriminator << ","
    << union_type.member_seq
    << ")";
}

std::ostream&
operator<<(std::ostream& out, const OpenDDS::XTypes::MinimalTypeObject& minimal)
{
  out << "XTypes::MinimalTypeObject(";
  switch (minimal.kind) {
  case OpenDDS::XTypes::TK_ALIAS:
    out << minimal.alias_type;
    break;
  case OpenDDS::XTypes::TK_ANNOTATION:
    be_util::misc_error_and_abort("Annotation output is not supported");
    break;
  case OpenDDS::XTypes::TK_STRUCTURE:
    out << minimal.struct_type;
    break;
  case OpenDDS::XTypes::TK_UNION:
    out << minimal.union_type;
    break;
  case OpenDDS::XTypes::TK_BITSET:
    be_util::misc_error_and_abort("Bitset output is not supported");
    break;
  case OpenDDS::XTypes::TK_SEQUENCE:
    be_util::misc_error_and_abort("Sequence output is not supported");
    break;
  case OpenDDS::XTypes::TK_ARRAY:
    be_util::misc_error_and_abort("Array output is not supported");
    break;
  case OpenDDS::XTypes::TK_MAP:
    be_util::misc_error_and_abort("Map output is not supported");
    break;
  case OpenDDS::XTypes::TK_ENUM:
    out << minimal.enumerated_type;
    break;
  case OpenDDS::XTypes::TK_BITMASK:
    be_util::misc_error_and_abort("Bitmask output is not supported");
    break;
  }
  out << ")";
  return out;
}

std::ostream&
operator<<(std::ostream& out, const OpenDDS::XTypes::TypeObject& to)
{
  out << "XTypes::TypeObject(";
  if (to.kind == OpenDDS::XTypes::EK_MINIMAL) {
    out << to.minimal;
  } else {
    be_util::misc_error_and_abort("Complete output is not supported");
  }
  out << ")";
  return out;
}

}

string typeobject_generator::tag_type(UTL_ScopedName* name)
{
  return dds_generator::scoped_helper(name, "_") + "_xtag";
}

bool
typeobject_generator::gen_enum(AST_Enum* node, UTL_ScopedName* name,
  const std::vector<AST_EnumVal*>&, const char*)
{
  return generate(node, name);
}

bool
typeobject_generator::gen_struct(AST_Structure* node, UTL_ScopedName* name,
  const std::vector<AST_Field*>&, AST_Type::SIZE_TYPE, const char*)
{
  return generate(node, name);
}

bool
typeobject_generator::gen_typedef(AST_Typedef* node, UTL_ScopedName* name,
                                  AST_Type*, const char*)
{
  return generate(node, name);
}

bool
typeobject_generator::gen_union(AST_Union* node, UTL_ScopedName* name,
  const std::vector<AST_UnionBranch*>&, AST_Type*,
  const char*)
{
  return generate(node, name);
}

size_t
typeobject_generator::compute_dependencies(AST_Type* type, const std::string& anonymous_name)
{
  switch (type->node_type()) {
  case AST_ConcreteType::NT_union_fwd:
    {
      AST_UnionFwd* n = AST_UnionFwd::narrow_from_decl(type);
      type = n->full_definition();
      break;
    }
  case AST_ConcreteType::NT_struct_fwd:
    {
      AST_StructureFwd* n = AST_StructureFwd::narrow_from_decl(type);
      type = n->full_definition();
      break;
    }
  default:
    break;
  }

  if (minimal_type_identifier_map_.find(type) != minimal_type_identifier_map_.end()) {
    // Done.
    return -1;
  }

  if (in_sorted_dependencies_.count(type) != 0) {
    // Done.
    return -1;
  }

  if (in_progress_.count(type) != 0) {
    // Recursion.
    return in_progress_[type];
  }

  const size_t group = group_counter_++;
  in_progress_[type] = group;
  size_t dependencies_group = -1;
  std::string name;

  switch (type->node_type()) {

  case AST_ConcreteType::NT_union:
    {
      AST_Union* n = AST_Union::narrow_from_decl(type);
      name = dds_generator::scoped_helper(n->name(), "::");

      AST_Type* discriminator = n->disc_type();
      const Fields fields(n);
      const Fields::Iterator fields_end = fields.end();

      dependencies_group = std::min(compute_dependencies(discriminator, name), dependencies_group);
      for (Fields::Iterator i = fields.begin(); i != fields_end; ++i) {
        AST_UnionBranch* ub = dynamic_cast<AST_UnionBranch*>(*i);
        dependencies_group = std::min(compute_dependencies(ub->field_type(), name + '.' + ub->local_name()->get_string()), dependencies_group);
      }

      break;
    }

  case AST_ConcreteType::NT_struct:
    {
      AST_Structure* n = AST_Structure::narrow_from_decl(type);
      name = dds_generator::scoped_helper(n->name(), "::");

      const Fields fields(n);
      const Fields::Iterator fields_end = fields.end();

      for (Fields::Iterator i = fields.begin(); i != fields_end; ++i) {
        AST_Field* field = *i;
        dependencies_group = std::min(compute_dependencies(field->field_type(), name + '.' + field->local_name()->get_string()), dependencies_group);
      }

      break;
    }

  case AST_ConcreteType::NT_array:
    {
      AST_Array* n = AST_Array::narrow_from_decl(type);
      name = anonymous_name;
      dependencies_group = std::min(compute_dependencies(n->base_type(), name), dependencies_group);
      break;
    }

  case AST_ConcreteType::NT_sequence:
    {
      AST_Sequence* n = AST_Sequence::narrow_from_decl(type);
      name = anonymous_name;
      dependencies_group = std::min(compute_dependencies(n->base_type(), name), dependencies_group);
      break;
    }

  case AST_ConcreteType::NT_typedef:
    {
      AST_Typedef* n = AST_Typedef::narrow_from_decl(type);
      name = dds_generator::scoped_helper(n->name(), "::");
      // TODO: What is the member name for an anonymous type in a  typedef?
      // 7.3.4.9.2
      dependencies_group = std::min(compute_dependencies(n->base_type(), name + '.'), dependencies_group);
      break;
    }

  case AST_ConcreteType::NT_enum:
    {
      AST_Enum* n = AST_Enum::narrow_from_decl(type);
      name = dds_generator::scoped_helper(n->name(), "::");
      break;
    }

  case AST_ConcreteType::NT_string:
  case AST_ConcreteType::NT_wstring:
  case AST_ConcreteType::NT_pre_defined:
  case AST_ConcreteType::NT_fixed:
      break;

  case AST_ConcreteType::NT_struct_fwd:
  case AST_ConcreteType::NT_union_fwd:
  case AST_ConcreteType::NT_native:
  case AST_ConcreteType::NT_factory:
  case AST_ConcreteType::NT_finder:
  case AST_ConcreteType::NT_component:
  case AST_ConcreteType::NT_component_fwd:
  case AST_ConcreteType::NT_home:
  case AST_ConcreteType::NT_eventtype:
  case AST_ConcreteType::NT_eventtype_fwd:
  case AST_ConcreteType::NT_valuebox:
  case AST_ConcreteType::NT_type:
  case AST_ConcreteType::NT_porttype:
  case AST_ConcreteType::NT_provides:
  case AST_ConcreteType::NT_uses:
  case AST_ConcreteType::NT_publishes:
  case AST_ConcreteType::NT_emits:
  case AST_ConcreteType::NT_consumes:
  case AST_ConcreteType::NT_ext_port:
  case AST_ConcreteType::NT_mirror_port:
  case AST_ConcreteType::NT_connector:
  case AST_ConcreteType::NT_param_holder:
  case AST_ConcreteType::NT_annotation_decl:
  case AST_ConcreteType::NT_annotation_appl:
  case AST_ConcreteType::NT_annotation_member:
  case AST_ConcreteType::NT_module:
  case AST_ConcreteType::NT_root:
  case AST_ConcreteType::NT_interface:
  case AST_ConcreteType::NT_interface_fwd:
  case AST_ConcreteType::NT_valuetype:
  case AST_ConcreteType::NT_valuetype_fwd:
  case AST_ConcreteType::NT_const:
  case AST_ConcreteType::NT_except:
  case AST_ConcreteType::NT_attr:
  case AST_ConcreteType::NT_op:
  case AST_ConcreteType::NT_argument:
  case AST_ConcreteType::NT_union_branch:
  case AST_ConcreteType::NT_field:
  case AST_ConcreteType::NT_enum_val:
    be_util::misc_error_and_abort("Unexpected AST type", type);
    break;
  }

  in_progress_.erase(type);
  sorted_dependencies_.push_back(Element(type, dependencies_group, name));
  in_sorted_dependencies_.insert(type);

  return dependencies_group;
}

void
typeobject_generator::generate_minimal_type_identifier(AST_Type* type)
{
  // Generate the minimal identifier (and type object) and cache.
  switch (type->node_type()) {

  case AST_ConcreteType::NT_union:
    {
      AST_Union* n = AST_Union::narrow_from_decl(type);
      AST_Type* discriminator = n->disc_type();
      const Fields fields(n);
      const Fields::Iterator fields_end = fields.end();

      const ExtensibilityKind exten = be_global->extensibility(n);
      const AutoidKind auto_id = be_global->autoid(n);

      OpenDDS::XTypes::TypeObject to;
      to.kind = OpenDDS::XTypes::EK_MINIMAL;
      to.minimal.kind = OpenDDS::XTypes::TK_UNION;

      to.minimal.union_type.union_flags = extensibility_to_type_flag(exten);

      if (be_global->is_nested(n)) {
        to.minimal.union_type.union_flags |= OpenDDS::XTypes::IS_NESTED;
      }

      if (auto_id == autoidkind_hash) {
        to.minimal.union_type.union_flags |= OpenDDS::XTypes::IS_AUTOID_HASH;
      }

      // to.minimal.union_type.header.detail is not used.

      const TryConstructFailAction trycon = be_global->union_discriminator_try_construct(n);
      to.minimal.union_type.discriminator.common.member_flags = try_construct_to_member_flag(trycon);
      if (be_global->union_discriminator_is_key(n)) {
        to.minimal.union_type.discriminator.common.member_flags |= OpenDDS::XTypes::IS_KEY;
      }
      to.minimal.union_type.discriminator.common.type_id = get_minimal_type_identifier(discriminator);

      ACE_CDR::ULong member_id = 0;

      for (Fields::Iterator i = fields.begin(); i != fields_end; ++i) {
        AST_UnionBranch* branch = dynamic_cast<AST_UnionBranch*>(*i);
        const TryConstructFailAction trycon = be_global->try_construct(branch);

        bool is_default = false;
        for (unsigned long j = 0; j < branch->label_list_length(); ++j) {
          AST_UnionLabel* label = branch->label(j);
          if (label->label_kind() == AST_UnionLabel::UL_default) {
            is_default = true;
            break;
          }
        }

        OpenDDS::XTypes::MinimalUnionMember member;

        member.common.member_id = be_global->get_id(branch, auto_id, member_id);

        member.common.member_flags = try_construct_to_member_flag(trycon);

        if (is_default) {
          member.common.member_flags |= OpenDDS::XTypes::IS_DEFAULT;
        }

        if (be_global->is_external(branch)) {
          member.common.member_flags |= OpenDDS::XTypes::IS_EXTERNAL;
        }

        member.common.type_id = get_minimal_type_identifier(branch->field_type());

        for (unsigned long j = 0; j < branch->label_list_length(); ++j) {
          AST_UnionLabel* label = branch->label(j);
          if (label->label_kind() != AST_UnionLabel::UL_default) {
            member.common.label_seq.append(to_long(*label->label_val()->ev()));
          }
        }
        member.common.label_seq.sort();

        OpenDDS::XTypes::hash_member_name(member.detail.name_hash, branch->local_name()->get_string());

        to.minimal.union_type.member_seq.append(member);
      }
      to.minimal.union_type.member_seq.sort();

      OpenDDS::XTypes::TypeIdentifier ti = OpenDDS::XTypes::makeTypeIdentifier(to);

      minimal_type_object_map_[type] = to;
      if (minimal_type_identifier_map_.count(type) == 0) {
        minimal_type_identifier_map_[type] = makeTypeIdentifier(to);
      }
      break;
    }

  case AST_ConcreteType::NT_struct:
    {
      AST_Structure* n = AST_Structure::narrow_from_decl(type);

      const Fields fields(n);
      const Fields::Iterator fields_end = fields.end();

      const ExtensibilityKind exten = be_global->extensibility(n);
      const AutoidKind auto_id = be_global->autoid(n);

      OpenDDS::XTypes::TypeObject to;
      to.kind = OpenDDS::XTypes::EK_MINIMAL;
      to.minimal.kind = OpenDDS::XTypes::TK_STRUCTURE;

      to.minimal.struct_type.struct_flags = extensibility_to_type_flag(exten);

      if (be_global->is_nested(n)) {
        to.minimal.struct_type.struct_flags |= OpenDDS::XTypes::IS_NESTED;
      }

      if (auto_id == autoidkind_hash) {
        to.minimal.struct_type.struct_flags |= OpenDDS::XTypes::IS_AUTOID_HASH;
      }

      // TODO: Support inheritance.
      to.minimal.struct_type.header.base_type = OpenDDS::XTypes::TypeIdentifier(OpenDDS::XTypes::TK_NONE);
      // to.minimal.struct_type.header.detail is not used.

      ACE_CDR::ULong member_id = 0;

      for (Fields::Iterator i = fields.begin(); i != fields_end; ++i) {
        AST_Field* field = *i;
        const TryConstructFailAction trycon = be_global->try_construct(field);

        OpenDDS::XTypes::MinimalStructMember member;

        member.common.member_id = be_global->get_id(field, auto_id, member_id);

        member.common.member_flags = try_construct_to_member_flag(trycon);

        if (be_global->is_optional(field)) {
          member.common.member_flags |= OpenDDS::XTypes::IS_OPTIONAL;
        }

        if (be_global->is_must_understand(field)) {
          member.common.member_flags |= OpenDDS::XTypes::IS_MUST_UNDERSTAND;
        }

        if (be_global->is_key(field)) {
          member.common.member_flags |= OpenDDS::XTypes::IS_KEY;
        }

        if (be_global->is_external(field)) {
          member.common.member_flags |= OpenDDS::XTypes::IS_EXTERNAL;
        }

        member.common.member_type_id = get_minimal_type_identifier((field)->field_type());

        OpenDDS::XTypes::hash_member_name(member.detail.name_hash, (field)->local_name()->get_string());

        to.minimal.struct_type.member_seq.append(member);
      }
      to.minimal.struct_type.member_seq.sort();

      OpenDDS::XTypes::TypeIdentifier ti = OpenDDS::XTypes::makeTypeIdentifier(to);

      minimal_type_object_map_[type] = to;
      if (minimal_type_identifier_map_.count(type) == 0) {
        minimal_type_identifier_map_[type] = makeTypeIdentifier(to);
      }
      break;
    }

  case AST_ConcreteType::NT_enum:
    {
      AST_Enum* n = AST_Enum::narrow_from_decl(type);
      std::vector<AST_EnumVal*> contents;
      scope2vector(contents, n, AST_Decl::NT_enum_val);

      size_t default_literal_idx = 0;
      for (size_t i = 0; i != contents.size(); ++i) {
        if (contents[i]->annotations().find("@default_literal")) {
          default_literal_idx = i;
        }
      }

      OpenDDS::XTypes::TypeObject to;
      to.kind = OpenDDS::XTypes::EK_MINIMAL;
      to.minimal.kind = OpenDDS::XTypes::TK_ENUM;
      // to.minimal.enumerated_type.enum_flags not used.
      // TODO: Add support for @bit_bound.
      to.minimal.enumerated_type.header.common.bit_bound = 32;

      for (size_t i = 0; i != contents.size(); ++i) {
        OpenDDS::XTypes::MinimalEnumeratedLiteral lit;
        lit.common.value = contents[i]->constant_value()->ev()->u.eval;
        lit.common.flags = (i == default_literal_idx ? OpenDDS::XTypes::IS_DEFAULT : 0);
        OpenDDS::XTypes::hash_member_name(lit.detail.name_hash, contents[i]->local_name()->get_string());
        to.minimal.enumerated_type.literal_seq.append(lit);
      }
      to.minimal.enumerated_type.literal_seq.sort();

      minimal_type_object_map_[type] = to;
      minimal_type_identifier_map_[type] = makeTypeIdentifier(to);
      break;
    }

  case AST_ConcreteType::NT_string:
    {
      AST_String* n = AST_String::narrow_from_decl(type);
      ACE_CDR::ULong bound = n->max_size()->ev()->u.ulval;
      if (bound < 256) {
        OpenDDS::XTypes::TypeIdentifier ti(OpenDDS::XTypes::TI_STRING8_SMALL);
        ti.seq_sdefn().bound = bound;
        minimal_type_identifier_map_[type] = ti;
      } else {
        OpenDDS::XTypes::TypeIdentifier ti(OpenDDS::XTypes::TI_STRING8_LARGE);
        ti.seq_ldefn().bound = bound;
        minimal_type_identifier_map_[type] = ti;
      }
      break;
    }

  case AST_ConcreteType::NT_wstring:
    {
      AST_String* n = AST_String::narrow_from_decl(type);
      ACE_CDR::ULong bound = n->max_size()->ev()->u.ulval;
      if (bound < 256) {
        OpenDDS::XTypes::TypeIdentifier ti(OpenDDS::XTypes::TI_STRING16_SMALL);
        ti.seq_sdefn().bound = bound;
        minimal_type_identifier_map_[type] = ti;
      } else {
        OpenDDS::XTypes::TypeIdentifier ti(OpenDDS::XTypes::TI_STRING16_LARGE);
        ti.seq_ldefn().bound = bound;
        minimal_type_identifier_map_[type] = ti;
      }
      break;
    }

  case AST_ConcreteType::NT_array:
    {
      AST_Array* n = AST_Array::narrow_from_decl(type);

      const TryConstructFailAction trycon = be_global->try_construct(n->base_type());
      OpenDDS::XTypes::CollectionElementFlag cef = try_construct_to_member_flag(trycon);
      if (be_global->is_external(n->base_type())) {
        cef |= OpenDDS::XTypes::IS_EXTERNAL;
      }
      const OpenDDS::XTypes::TypeIdentifier elem_ti = get_minimal_type_identifier(n->base_type());
      if (be_global->is_plain(type)) {
        const OpenDDS::XTypes::EquivalenceKind ek = OpenDDS::XTypes::is_fully_descriptive(elem_ti) ? OpenDDS::XTypes::EK_BOTH : OpenDDS::XTypes::EK_MINIMAL;
        ACE_CDR::ULong max_bound = 0;
        for (ACE_CDR::ULong dim = 0; dim != n->n_dims(); ++dim) {
          max_bound = std::max(max_bound, n->dims()[dim]->ev()->u.ulval);
        }

        if (max_bound < 256) {
          OpenDDS::XTypes::TypeIdentifier ti(OpenDDS::XTypes::TI_PLAIN_ARRAY_SMALL);
          ti.array_sdefn().header.equiv_kind = ek;
          ti.array_sdefn().header.element_flags = cef;
          for (ACE_CDR::ULong dim = 0; dim != n->n_dims(); ++dim) {
            ti.array_sdefn().array_bound_seq.append(n->dims()[dim]->ev()->u.ulval);
          }
          ti.array_sdefn().element_identifier = elem_ti;
          minimal_type_identifier_map_[type] = ti;
        } else {
          OpenDDS::XTypes::TypeIdentifier ti(OpenDDS::XTypes::TI_PLAIN_ARRAY_LARGE);
          ti.array_ldefn().header.equiv_kind = ek;
          ti.array_ldefn().header.element_flags = cef;
          for (ACE_CDR::ULong dim = 0; dim != n->n_dims(); ++dim) {
            ti.array_ldefn().array_bound_seq.append(n->dims()[dim]->ev()->u.ulval);
          }
          ti.array_ldefn().element_identifier = elem_ti;
          minimal_type_identifier_map_[type] = ti;
        }
      } else {
        OpenDDS::XTypes::TypeObject to;
        to.kind = OpenDDS::XTypes::EK_MINIMAL;
        to.minimal.kind = OpenDDS::XTypes::TK_ARRAY;
        // to.minimal.array_type.collection_flag is not used.
        for (ACE_CDR::ULong dim = 0; dim != n->n_dims(); ++dim) {
          to.minimal.array_type.header.common.bound_seq.append(n->dims()[dim]->ev()->u.ulval);
        }
        to.minimal.array_type.element.common.element_flags = cef;
        to.minimal.array_type.element.common.type = elem_ti;
        minimal_type_object_map_[type] = to;
        if (minimal_type_identifier_map_.count(type) == 0) {
          minimal_type_identifier_map_[type] = makeTypeIdentifier(to);
        }
      }
      break;
    }

  case AST_ConcreteType::NT_sequence:
    {
      AST_Sequence* n = AST_Sequence::narrow_from_decl(type);

      ACE_CDR::ULong bound = 0;
      if (!n->unbounded()) {
        bound = n->max_size()->ev()->u.ulval;
      }
      const TryConstructFailAction trycon = be_global->try_construct(n->base_type());
      OpenDDS::XTypes::CollectionElementFlag cef = try_construct_to_member_flag(trycon);
      if (be_global->is_external(n->base_type())) {
        cef |= OpenDDS::XTypes::IS_EXTERNAL;
      }
      const OpenDDS::XTypes::TypeIdentifier elem_ti = get_minimal_type_identifier(n->base_type());
      if (be_global->is_plain(type)) {
        const OpenDDS::XTypes::EquivalenceKind ek = OpenDDS::XTypes::is_fully_descriptive(elem_ti) ? OpenDDS::XTypes::EK_BOTH : OpenDDS::XTypes::EK_MINIMAL;
        if (bound < 256) {
          OpenDDS::XTypes::TypeIdentifier ti(OpenDDS::XTypes::TI_PLAIN_SEQUENCE_SMALL);
          ti.seq_sdefn().header.equiv_kind = ek;
          ti.seq_sdefn().header.element_flags = cef;
          ti.seq_sdefn().bound = bound;
          ti.seq_sdefn().element_identifier = elem_ti;
          minimal_type_identifier_map_[type] = ti;
        } else {
          OpenDDS::XTypes::TypeIdentifier ti(OpenDDS::XTypes::TI_PLAIN_SEQUENCE_LARGE);
          ti.seq_ldefn().header.equiv_kind = ek;
          ti.seq_ldefn().header.element_flags = cef;
          ti.seq_ldefn().bound = bound;
          ti.seq_ldefn().element_identifier = elem_ti;
          minimal_type_identifier_map_[type] = ti;
        }
      } else {
        OpenDDS::XTypes::TypeObject to;
        to.kind = OpenDDS::XTypes::EK_MINIMAL;
        to.minimal.kind = OpenDDS::XTypes::TK_SEQUENCE;
        // to.minimal.seq_type.collection_flag is not used.
        to.minimal.sequence_type.header.common.bound = bound;
        to.minimal.sequence_type.element.common.element_flags = cef;
        to.minimal.sequence_type.element.common.type = elem_ti;
        minimal_type_object_map_[type] = to;
        if (minimal_type_identifier_map_.count(type) == 0) {
          minimal_type_identifier_map_[type] = makeTypeIdentifier(to);
        }
      }
      break;
    }

  case AST_ConcreteType::NT_typedef:
    {
      AST_Typedef* n = AST_Typedef::narrow_from_decl(type);

      OpenDDS::XTypes::TypeObject to;
      to.kind = OpenDDS::XTypes::EK_MINIMAL;
      to.minimal.kind = OpenDDS::XTypes::TK_ALIAS;
      // to.minimal.alias_type.alias_flags not used.
      // to.minimal.alias_type.header not used.
      // to.minimal.alias_type.body.common.related_flags not used;
      to.minimal.alias_type.body.common.related_type = get_minimal_type_identifier(n->base_type());
      minimal_type_object_map_[type] = to;
      if (minimal_type_identifier_map_.count(type) == 0) {
        minimal_type_identifier_map_[type] = makeTypeIdentifier(to);
      }
      break;
    }

  case AST_ConcreteType::NT_pre_defined:
    {
      AST_PredefinedType* n = AST_PredefinedType::narrow_from_decl(type);
      switch (n->pt()) {
      case AST_PredefinedType::PT_long:
        minimal_type_identifier_map_[type] = OpenDDS::XTypes::TypeIdentifier(OpenDDS::XTypes::TK_INT32);
        break;
      case AST_PredefinedType::PT_ulong:
        minimal_type_identifier_map_[type] = OpenDDS::XTypes::TypeIdentifier(OpenDDS::XTypes::TK_UINT32);
        break;
      case AST_PredefinedType::PT_longlong:
        minimal_type_identifier_map_[type] = OpenDDS::XTypes::TypeIdentifier(OpenDDS::XTypes::TK_INT64);
        break;
      case AST_PredefinedType::PT_ulonglong:
        minimal_type_identifier_map_[type] = OpenDDS::XTypes::TypeIdentifier(OpenDDS::XTypes::TK_UINT64);
        break;
      case AST_PredefinedType::PT_short:
        minimal_type_identifier_map_[type] = OpenDDS::XTypes::TypeIdentifier(OpenDDS::XTypes::TK_INT16);
        break;
      case AST_PredefinedType::PT_ushort:
        minimal_type_identifier_map_[type] = OpenDDS::XTypes::TypeIdentifier(OpenDDS::XTypes::TK_UINT16);
        break;
      case AST_PredefinedType::PT_float:
        minimal_type_identifier_map_[type] = OpenDDS::XTypes::TypeIdentifier(OpenDDS::XTypes::TK_FLOAT32);
        break;
      case AST_PredefinedType::PT_double:
        minimal_type_identifier_map_[type] = OpenDDS::XTypes::TypeIdentifier(OpenDDS::XTypes::TK_FLOAT64);
        break;
      case AST_PredefinedType::PT_longdouble:
        minimal_type_identifier_map_[type] = OpenDDS::XTypes::TypeIdentifier(OpenDDS::XTypes::TK_FLOAT128);
        break;
      case AST_PredefinedType::PT_char:
        minimal_type_identifier_map_[type] = OpenDDS::XTypes::TypeIdentifier(OpenDDS::XTypes::TK_CHAR8);
        break;
      case AST_PredefinedType::PT_wchar:
        minimal_type_identifier_map_[type] = OpenDDS::XTypes::TypeIdentifier(OpenDDS::XTypes::TK_CHAR16);
        break;
      case AST_PredefinedType::PT_boolean:
        minimal_type_identifier_map_[type] = OpenDDS::XTypes::TypeIdentifier(OpenDDS::XTypes::TK_BOOLEAN);
        break;
      case AST_PredefinedType::PT_octet:
        minimal_type_identifier_map_[type] = OpenDDS::XTypes::TypeIdentifier(OpenDDS::XTypes::TK_BYTE);
        break;
      case AST_PredefinedType::PT_any:
      case AST_PredefinedType::PT_object:
      case AST_PredefinedType::PT_value:
      case AST_PredefinedType::PT_abstract:
      case AST_PredefinedType::PT_void:
      case AST_PredefinedType::PT_pseudo:
        be_util::misc_error_and_abort("Unexpected primitive type");
      }
      break;
    }

  case AST_ConcreteType::NT_fixed: {
    minimal_type_identifier_map_[type] = OpenDDS::XTypes::TypeIdentifier(OpenDDS::XTypes::TK_NONE);
    break;
  }

  case AST_ConcreteType::NT_struct_fwd:
  case AST_ConcreteType::NT_union_fwd:
  case AST_ConcreteType::NT_native:
  case AST_ConcreteType::NT_factory:
  case AST_ConcreteType::NT_finder:
  case AST_ConcreteType::NT_component:
  case AST_ConcreteType::NT_component_fwd:
  case AST_ConcreteType::NT_home:
  case AST_ConcreteType::NT_eventtype:
  case AST_ConcreteType::NT_eventtype_fwd:
  case AST_ConcreteType::NT_valuebox:
  case AST_ConcreteType::NT_type:
  case AST_ConcreteType::NT_porttype:
  case AST_ConcreteType::NT_provides:
  case AST_ConcreteType::NT_uses:
  case AST_ConcreteType::NT_publishes:
  case AST_ConcreteType::NT_emits:
  case AST_ConcreteType::NT_consumes:
  case AST_ConcreteType::NT_ext_port:
  case AST_ConcreteType::NT_mirror_port:
  case AST_ConcreteType::NT_connector:
  case AST_ConcreteType::NT_param_holder:
  case AST_ConcreteType::NT_annotation_decl:
  case AST_ConcreteType::NT_annotation_appl:
  case AST_ConcreteType::NT_annotation_member:
  case AST_ConcreteType::NT_module:
  case AST_ConcreteType::NT_root:
  case AST_ConcreteType::NT_interface:
  case AST_ConcreteType::NT_interface_fwd:
  case AST_ConcreteType::NT_valuetype:
  case AST_ConcreteType::NT_valuetype_fwd:
  case AST_ConcreteType::NT_const:
  case AST_ConcreteType::NT_except:
  case AST_ConcreteType::NT_attr:
  case AST_ConcreteType::NT_op:
  case AST_ConcreteType::NT_argument:
  case AST_ConcreteType::NT_union_branch:
  case AST_ConcreteType::NT_field:
  case AST_ConcreteType::NT_enum_val:
    be_util::misc_error_and_abort("Unexpected AST type", type);
  }
}

bool
typeobject_generator::name_sorter(const Element& x, const Element& y)
{
  return x.name < y.name;
}

void
typeobject_generator::generate_minimal(AST_Type* type)
{
  switch (type->node_type()) {
  case AST_ConcreteType::NT_union_fwd:
    {
      AST_UnionFwd* n = AST_UnionFwd::narrow_from_decl(type);
      type = n->full_definition();
      break;
    }
  case AST_ConcreteType::NT_struct_fwd:
    {
      AST_StructureFwd* n = AST_StructureFwd::narrow_from_decl(type);
      type = n->full_definition();
      break;
    }
  default:
    break;
  }

  group_counter_ = 0;
  compute_dependencies(type, "");

  for (SortedDependencies::iterator pos = sorted_dependencies_.begin(), limit = sorted_dependencies_.end(); pos != limit;) {
    if (pos->group == static_cast<size_t>(-1)) {
      generate_minimal_type_identifier(pos->type);
      ++pos;
      continue;
    }

    // Process strongly connected components.
    const size_t group = pos->group;
    const SortedDependencies::iterator group_begin = pos;
    while (pos != limit && pos->group == group) {
      ++pos;
    }
    const SortedDependencies::iterator group_end = pos;

    std::sort(group_begin, group_end, name_sorter);

    OpenDDS::XTypes::TypeIdentifier ti(OpenDDS::XTypes::TI_STRONGLY_CONNECTED_COMPONENT);
    ti.sc_component_id().sc_component_id.kind = OpenDDS::XTypes::EK_MINIMAL;
    std::memset(&ti.sc_component_id().sc_component_id.hash, 0, sizeof(OpenDDS::XTypes::EquivalenceHash));
    ti.sc_component_id().scc_length = std::distance(group_begin, group_end);

    {
      size_t idx = 0;
      for (SortedDependencies::const_iterator group_pos = group_begin; group_pos != group_end; ++group_pos) {
        ti.sc_component_id().scc_index = ++idx;
        minimal_type_identifier_map_[group_pos->type] = ti;
      }
    }

    OpenDDS::XTypes::TypeObjectSeq seq;
    for (SortedDependencies::const_iterator group_pos = group_begin; group_pos != group_end; ++group_pos) {
      generate_minimal_type_identifier(group_pos->type);
      seq.append(minimal_type_object_map_[group_pos->type]);
    }

    const OpenDDS::DCPS::Encoding& encoding = OpenDDS::XTypes::get_typeobject_encoding();
    size_t size = serialized_size(encoding, seq);
    ACE_Message_Block buff(size);
    OpenDDS::DCPS::Serializer ser(&buff, encoding);
    if (!(ser << seq)) {
      be_util::misc_error_and_abort("Failed to serialize type object sequence in strongly-connected component", type);
    }

    unsigned char result[16];
    OpenDDS::DCPS::MD5Hash(result, buff.rd_ptr(), buff.length());

    // First 14 bytes of MD5 of the serialized TypeObject using XCDR
    // version 2 with Little Endian encoding
    std::memcpy(ti.sc_component_id().sc_component_id.hash, result, sizeof(OpenDDS::XTypes::EquivalenceHash));

    {
      size_t idx = 0;
      for (SortedDependencies::const_iterator group_pos = group_begin; group_pos != group_end; ++group_pos) {
        ti.sc_component_id().scc_index = ++idx;
        minimal_type_identifier_map_[group_pos->type] = ti;
      }
    }
  }

  sorted_dependencies_.clear();
  in_sorted_dependencies_.clear();
  OPENDDS_ASSERT(in_progress_.empty());
}

OpenDDS::XTypes::TypeObject
typeobject_generator::get_minimal_type_object(AST_Type* type)
{
  switch(type->node_type()) {
  case AST_Decl::NT_union_fwd:
    {
      AST_UnionFwd* td = AST_UnionFwd::narrow_from_decl(type);
      return get_minimal_type_object(td->full_definition());
    }
  case AST_Decl::NT_struct_fwd:
    {
      AST_StructureFwd* td = AST_StructureFwd::narrow_from_decl(type);
      return get_minimal_type_object(td->full_definition());
    }
  default:
    break;
  }

  MinimalTypeObjectMap::const_iterator pos = minimal_type_object_map_.find(type);
  OPENDDS_ASSERT(pos != minimal_type_object_map_.end());
  return pos->second;
}

OpenDDS::XTypes::TypeIdentifier
typeobject_generator::get_minimal_type_identifier(AST_Type* type)
{
  switch(type->node_type()) {
  case AST_Decl::NT_union_fwd:
    {
      AST_UnionFwd* td = AST_UnionFwd::narrow_from_decl(type);
      return get_minimal_type_identifier(td->full_definition());
    }
  case AST_Decl::NT_struct_fwd:
    {
      AST_StructureFwd* td = AST_StructureFwd::narrow_from_decl(type);
      return get_minimal_type_identifier(td->full_definition());
    }
  default:
    break;
  }

  MinimalTypeIdentifierMap::const_iterator pos = minimal_type_identifier_map_.find(type);
  OPENDDS_ASSERT(pos != minimal_type_identifier_map_.end());
  return pos->second;
}

bool
typeobject_generator::generate(AST_Type* node, UTL_ScopedName* name)
{
  generate_minimal(node);

  be_global->add_include("dds/DCPS/XTypes/TypeObject.h", BE_GlobalData::STREAM_H);
  NamespaceGuard ng;
  const string clazz = tag_type(name);

  be_global->header_ << "struct " << clazz << " {};\n";

  {
    const string decl_gto = "getMinimalTypeObject<" + clazz + ">";
    Function gto(decl_gto.c_str(), "const XTypes::TypeObject&", "");
    gto.endArgs();
    const OpenDDS::XTypes::TypeObject to = get_minimal_type_object(node);
    be_global->impl_ <<
      "  static const XTypes::TypeObject to = " << to << ";\n"
      "  return to;\n";
  }
  {
    const string decl_gti = "getMinimalTypeIdentifier<" + clazz + ">";
    Function gti(decl_gti.c_str(), "XTypes::TypeIdentifier", "");
    gti.endArgs();
    const OpenDDS::XTypes::TypeIdentifier ti = get_minimal_type_identifier(node);
    be_global->impl_ <<
      "  static const XTypes::TypeIdentifier ti = " << ti << ";\n"
      "  return ti;\n";
  }
  return true;
}
