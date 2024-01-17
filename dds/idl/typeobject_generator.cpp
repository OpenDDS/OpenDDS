/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "typeobject_generator.h"

#include "topic_keys.h"
#include "dds_visitor.h"
#include "be_extern.h"
#include "be_util.h"

#include <dds/DCPS/Hash.h>
#include <dds/DCPS/SafetyProfileStreams.h>
#include <dds/DCPS/Definitions.h>

#include <utl_identifier.h>

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
#if OPENDDS_HAS_EXPLICIT_INTS
  case AST_Expression::EV_uint8:
    return ev.u.uint8val;
  case AST_Expression::EV_int8:
    return ev.u.int8val;
#endif
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
    // Spec issue DDSXTY14-36 (64-bit integers as case labels in TypeObject)
    be_util::misc_error_and_abort("Illegal conversion to long");
    return 0;
  }
}

struct Printer {
  virtual ~Printer() {}
  virtual std::ostream& print_on(std::ostream& out) const = 0;
};

std::ostream&
operator<<(std::ostream& out, const Printer& printer)
{
  return printer.print_on(out);
}

template <typename T>
struct ValuePrinter : Printer {
  explicit ValuePrinter(const T& a_value) : value(a_value) {}

  const T value;
};

template <typename T>
struct UintPrinter : ValuePrinter<T> {
  explicit UintPrinter(const T& a_value) : ValuePrinter<T>(a_value) {}

  std::ostream& print_on(std::ostream& out) const
  {
    return out << static_cast<unsigned int>(this->value);
  }
};

struct SBoundPrinter : UintPrinter<OpenDDS::XTypes::SBound> {
  explicit SBoundPrinter(const OpenDDS::XTypes::SBound a_value)
  : UintPrinter<OpenDDS::XTypes::SBound>(a_value) {}
};

struct LBoundPrinter : UintPrinter<OpenDDS::XTypes::LBound> {
  explicit LBoundPrinter(const OpenDDS::XTypes::LBound a_value)
  : UintPrinter<OpenDDS::XTypes::LBound>(a_value) {}
};

struct EquivalenceKindPrinter : ValuePrinter<OpenDDS::XTypes::EquivalenceKind> {
  explicit EquivalenceKindPrinter(const OpenDDS::XTypes::EquivalenceKind a_value)
  : ValuePrinter<OpenDDS::XTypes::EquivalenceKind>(a_value) {}

  std::ostream& print_on(std::ostream& out) const
  {
    switch (this->value) {
    case OpenDDS::XTypes::EK_MINIMAL:
      return out << "XTypes::EK_MINIMAL";
    case OpenDDS::XTypes::EK_COMPLETE:
      return out << "XTypes::EK_COMPLETE";
    case OpenDDS::XTypes::EK_BOTH:
      return out << "XTypes::EK_BOTH";
    }
    return out;
  }
};

struct BitmaskPrintHelper {
  explicit BitmaskPrintHelper(std::ostream& os) : os_(os), first_(true) {}

  std::ostream& os_;
  bool first_;

  BitmaskPrintHelper& operator<<(const char* str)
  {
    if (first_) {
      first_ = false;
    } else {
      os_ << " | ";
    }
    os_ << str;
    return *this;
  }

  ~BitmaskPrintHelper()
  {
    if (first_) {
      os_ << '0';
    }
  }
};

struct MemberFlagPrinter : ValuePrinter<OpenDDS::XTypes::MemberFlag> {
  explicit MemberFlagPrinter(const OpenDDS::XTypes::MemberFlag a_value)
  : ValuePrinter<OpenDDS::XTypes::MemberFlag>(a_value) {}

  std::ostream& print_on(std::ostream& out) const
  {
    BitmaskPrintHelper bph(out);
    if (this->value & OpenDDS::XTypes::TRY_CONSTRUCT1) {
      bph << "XTypes::TRY_CONSTRUCT1";
    }
    if (this->value & OpenDDS::XTypes::TRY_CONSTRUCT2) {
      bph << "XTypes::TRY_CONSTRUCT2";
    }
    if (this->value & OpenDDS::XTypes::IS_EXTERNAL) {
      bph << "XTypes::IS_EXTERNAL";
    }
    if (this->value & OpenDDS::XTypes::IS_OPTIONAL) {
      bph << "XTypes::IS_OPTIONAL";
    }
    if (this->value & OpenDDS::XTypes::IS_MUST_UNDERSTAND) {
      bph << "XTypes::IS_MUST_UNDERSTAND";
    }
    if (this->value & OpenDDS::XTypes::IS_KEY) {
      bph << "XTypes::IS_KEY";
    }
    if (this->value & OpenDDS::XTypes::IS_DEFAULT) {
      bph << "XTypes::IS_DEFAULT";
    }
    return out;
  }
};

struct CollectionElementFlagPrinter : MemberFlagPrinter {
  explicit CollectionElementFlagPrinter(const OpenDDS::XTypes::CollectionElementFlag a_value) : MemberFlagPrinter(a_value) {}
};

struct StructMemberFlagPrinter : MemberFlagPrinter {
  explicit StructMemberFlagPrinter(const OpenDDS::XTypes::StructMemberFlag a_value) : MemberFlagPrinter(a_value) {}
};

struct UnionMemberFlagPrinter : MemberFlagPrinter {
  explicit UnionMemberFlagPrinter(const OpenDDS::XTypes::UnionMemberFlag a_value) : MemberFlagPrinter(a_value) {}
};

struct UnionDiscriminatorFlagPrinter : MemberFlagPrinter {
  explicit UnionDiscriminatorFlagPrinter(const OpenDDS::XTypes::UnionDiscriminatorFlag a_value) : MemberFlagPrinter(a_value) {}
};

struct EnumeratedLiteralFlagPrinter : MemberFlagPrinter {
  explicit EnumeratedLiteralFlagPrinter(const OpenDDS::XTypes::EnumeratedLiteralFlag a_value) : MemberFlagPrinter(a_value) {}
};

struct AliasMemberFlagPrinter : MemberFlagPrinter {
  explicit AliasMemberFlagPrinter(const OpenDDS::XTypes::AliasMemberFlag a_value) : MemberFlagPrinter(a_value) {}
};

struct TypeFlagPrinter : ValuePrinter<OpenDDS::XTypes::TypeFlag> {
  explicit TypeFlagPrinter(const OpenDDS::XTypes::TypeFlag a_value)
  : ValuePrinter<OpenDDS::XTypes::TypeFlag>(a_value) {}

  std::ostream& print_on(std::ostream& out) const
  {
    BitmaskPrintHelper bph(out);
    if (this->value & OpenDDS::XTypes::IS_FINAL) {
      bph << "XTypes::IS_FINAL";
    }
    if (this->value & OpenDDS::XTypes::IS_APPENDABLE) {
      bph << "XTypes::IS_APPENDABLE";
    }
    if (this->value & OpenDDS::XTypes::IS_MUTABLE) {
      bph << "XTypes::IS_MUTABLE";
    }
    if (this->value & OpenDDS::XTypes::IS_NESTED) {
      bph << "XTypes::IS_NESTED";
    }
    if (this->value & OpenDDS::XTypes::IS_AUTOID_HASH) {
      bph << "XTypes::IS_AUTOID_HASH";
    }
    return out;
  }
};

struct StructTypeFlagPrinter : TypeFlagPrinter {
  explicit StructTypeFlagPrinter(const OpenDDS::XTypes::StructTypeFlag a_value) : TypeFlagPrinter(a_value) {}
};

struct UnionTypeFlagPrinter : TypeFlagPrinter {
  explicit UnionTypeFlagPrinter(const OpenDDS::XTypes::UnionTypeFlag a_value) : TypeFlagPrinter(a_value) {}
};

struct CollectionTypeFlagPrinter : TypeFlagPrinter {
  explicit CollectionTypeFlagPrinter(const OpenDDS::XTypes::CollectionTypeFlag a_value) : TypeFlagPrinter(a_value) {}
};

struct AliasTypeFlagPrinter : TypeFlagPrinter {
  explicit AliasTypeFlagPrinter(const OpenDDS::XTypes::AliasTypeFlag a_value) : TypeFlagPrinter(a_value) {}
};

struct EnumTypeFlagPrinter : TypeFlagPrinter {
  explicit EnumTypeFlagPrinter(const OpenDDS::XTypes::EnumTypeFlag a_value) : TypeFlagPrinter(a_value) {}
};

struct MemberIdPrinter : UintPrinter<OpenDDS::XTypes::MemberId> {
  explicit MemberIdPrinter(const OpenDDS::XTypes::MemberId a_value)
  : UintPrinter<OpenDDS::XTypes::MemberId>(a_value) {}
};

struct BitBoundPrinter : UintPrinter<OpenDDS::XTypes::BitBound> {
  explicit BitBoundPrinter(const OpenDDS::XTypes::BitBound a_value)
  : UintPrinter<OpenDDS::XTypes::BitBound>(a_value) {}
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
  return out << "XTypes::StringLTypeDefn(" << LBoundPrinter(string_ldefn.bound) << ")";
}


std::ostream&
operator<<(std::ostream& out, const OpenDDS::XTypes::PlainCollectionHeader& header)
{
  return out
    << "XTypes::PlainCollectionHeader("
    << EquivalenceKindPrinter(header.equiv_kind) << ", "
    << CollectionElementFlagPrinter(header.element_flags)
    << ")";
}

std::ostream&
operator<<(std::ostream& out, const OpenDDS::XTypes::PlainSequenceSElemDefn& seq_sdefn)
{
  return out
    << "XTypes::PlainSequenceSElemDefn("
    << seq_sdefn.header << ", "
    << SBoundPrinter(seq_sdefn.bound) << ", "
    << *seq_sdefn.element_identifier
    << ")";
}

std::ostream&
operator<<(std::ostream& out, const OpenDDS::XTypes::PlainSequenceLElemDefn& seq_ldefn)
{
  return out
    << "XTypes::PlainSequenceLElemDefn("
    << seq_ldefn.header << ", "
    << LBoundPrinter(seq_ldefn.bound) << ", "
    << *seq_ldefn.element_identifier
    << ")";
}

std::ostream&
operator<<(std::ostream& out, const OpenDDS::XTypes::PlainArraySElemDefn& array_sdefn)
{
  return out
    << "XTypes::PlainArraySElemDefn("
    << array_sdefn.header << ", "
    << array_sdefn.array_bound_seq << ", "
    << *array_sdefn.element_identifier
    << ")";
}

std::ostream&
operator<<(std::ostream& out, const OpenDDS::XTypes::PlainArrayLElemDefn& array_ldefn)
{
  return out
    << "XTypes::PlainArrayLElemDefn("
    << array_ldefn.header << ", "
    << array_ldefn.array_bound_seq << ", "
    << *array_ldefn.element_identifier
    << ")";
}

std::ostream&
operator<<(std::ostream& out, const OpenDDS::XTypes::PlainMapSTypeDefn& map_sdefn)
{
  return out
    << "XTypes::PlainMapSTypeDefn("
    << map_sdefn.header << ", "
    << SBoundPrinter(map_sdefn.bound) << ", "
    << *map_sdefn.element_identifier << ", "
    << CollectionElementFlagPrinter(map_sdefn.key_flags) << ", "
    << *map_sdefn.key_identifier
    << ")";
}

std::ostream&
operator<<(std::ostream& out, const OpenDDS::XTypes::PlainMapLTypeDefn& map_ldefn)
{
  return out
    << "XTypes::PlainMapLTypeDefn("
    << map_ldefn.header << ", "
    << LBoundPrinter(map_ldefn.bound) << ", "
    << *map_ldefn.element_identifier << ", "
    << CollectionElementFlagPrinter(map_ldefn.key_flags) << ", "
    << *map_ldefn.key_identifier
    << ")";
}

std::ostream&
operator<<(std::ostream& out, const OpenDDS::XTypes::EquivalenceHash& equivalence_hash)
{
  return out
    << "XTypes::EquivalenceHashWrapper("
    << OpenDDS::XTypes::equivalence_hash_to_string(equivalence_hash)
    << ")";
}

std::ostream&
operator<<(std::ostream& out, const OpenDDS::XTypes::TypeObjectHashId& id)
{
  return out
    << "XTypes::TypeObjectHashId("
    << EquivalenceKindPrinter(id.kind) << ", "
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
  case OpenDDS::XTypes::TK_INT8:
    out << "XTypes::TK_INT8";
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
  case OpenDDS::XTypes::TK_UINT8:
    out << "XTypes::TK_UINT8";
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
    out << "XTypes::TI_STRING8_SMALL, " << ti.string_sdefn();
    break;
  case OpenDDS::XTypes::TI_STRING16_SMALL:
    out << "XTypes::TI_STRING16_SMALL, " << ti.string_sdefn();
    break;
  case OpenDDS::XTypes::TI_STRING8_LARGE:
    out << "XTypes::TI_STRING8_LARGE, " << ti.string_ldefn();
    break;
  case OpenDDS::XTypes::TI_STRING16_LARGE:
    out << "XTypes::TI_STRING16_LARGE, " << ti.string_ldefn();
    break;
  case OpenDDS::XTypes::TI_PLAIN_SEQUENCE_SMALL:
    out << "XTypes::TI_PLAIN_SEQUENCE_SMALL, " << ti.seq_sdefn();
    break;
  case OpenDDS::XTypes::TI_PLAIN_SEQUENCE_LARGE:
    out << "XTypes::TI_PLAIN_SEQUENCE_LARGE, " << ti.seq_ldefn();
    break;
  case OpenDDS::XTypes::TI_PLAIN_ARRAY_SMALL:
    out << "XTypes::TI_PLAIN_ARRAY_SMALL, " << ti.array_sdefn();
    break;
  case OpenDDS::XTypes::TI_PLAIN_ARRAY_LARGE:
    out << "XTypes::TI_PLAIN_ARRAY_LARGE, " << ti.array_ldefn();
    break;
  case OpenDDS::XTypes::TI_PLAIN_MAP_SMALL:
    out << "XTypes::TI_PLAIN_MAP_SMALL, " << ti.map_sdefn();
    break;
  case OpenDDS::XTypes::TI_PLAIN_MAP_LARGE:
    out << "XTypes::TI_PLAIN_MAP_LARGE, " << ti.map_ldefn();
    break;
  case OpenDDS::XTypes::TI_STRONGLY_CONNECTED_COMPONENT:
    out << "XTypes::TI_STRONGLY_CONNECTED_COMPONENT, " << ti.sc_component_id();
    break;
  case OpenDDS::XTypes::EK_COMPLETE:
    out << "XTypes::EK_COMPLETE, " << ti.equivalence_hash();
    break;
  case OpenDDS::XTypes::EK_MINIMAL:
    out << "XTypes::EK_MINIMAL, " << ti.equivalence_hash();
    break;
  default:
    be_util::misc_error_and_abort("Extended type definitions output is not supported");
    break;
  }

  out << ')';
  return out;
}

void dump_bytes(const OpenDDS::XTypes::TypeObject& to)
{
  ACE_Message_Block buffer(OpenDDS::DCPS::serialized_size(OpenDDS::XTypes::get_typeobject_encoding(), to));
  OpenDDS::DCPS::Serializer ser(&buffer, OpenDDS::XTypes::get_typeobject_encoding());
  if (!(ser << to)) {
    be_util::misc_error_and_abort("Failed to serialize type object");
  }

  for (const char* ptr = buffer.rd_ptr(); ptr != buffer.wr_ptr(); ++ptr) {
    if (ptr != buffer.rd_ptr()) {
      be_global->impl_ << ", ";
    }
    be_global->impl_ << int(*reinterpret_cast<const unsigned char*>(ptr));
  }
}

}

void
typeobject_generator::declare_get_type_map()
{
  if (!produce_output_ || get_type_map_declared_) {
    return;
  }
  get_type_map_declared_ = true;

  be_global->add_include("dds/DCPS/XTypes/TypeObject.h", BE_GlobalData::STREAM_H);

  be_global->impl_ << "static const XTypes::TypeMap& OPENDDS_IDL_FILE_SPECIFIC(get_minimal_type_map, 0)();\n";

  if (produce_xtypes_complete_) {
    be_global->impl_ << "static const XTypes::TypeMap& OPENDDS_IDL_FILE_SPECIFIC(get_complete_type_map, 0)();\n";
  }
}

void
typeobject_generator::use_old_typeobject_encoding()
{
  static OpenDDS::DCPS::Encoding enc = OpenDDS::XTypes::get_typeobject_encoding();
  enc.skip_sequence_dheader(true);
  typeid_encoding_ = &enc;
}

void
typeobject_generator::gen_epilogue()
{
  be_global->add_include("dds/DCPS/Service_Participant.h");

  if (!produce_output_ || !get_type_map_declared_) {
    return;
  }

  NamespaceGuard ng;

  be_global->impl_ <<
    "namespace {\n";

  size_t idx = 0;
  for (OpenDDS::XTypes::TypeMap::const_iterator pos = minimal_type_map_.begin();
       pos != minimal_type_map_.end(); ++pos, ++idx) {
    be_global->impl_ <<
      "XTypes::TypeObject OPENDDS_IDL_FILE_SPECIFIC(minimal_to, " << idx << ")()\n"
      "{\n"
      "  const unsigned char to_bytes[] = { ";
    dump_bytes(pos->second);
    be_global->add_include("<stdexcept>", BE_GlobalData::STREAM_CPP);
    be_global->impl_ <<
      "  };\n"
      "  XTypes::TypeObject to;\n"
      "  if (!to_type_object(to_bytes, sizeof(to_bytes), to)) {\n"
      "    throw std::runtime_error(\"Could not deserialize minimal Type Object " << idx << "\");\n"
      "  }\n"
      "  return to;\n"
      "}\n\n";
  }

  be_global->impl_ <<
    "XTypes::TypeMap OPENDDS_IDL_FILE_SPECIFIC(get_minimal_type_map_private, 0)()\n"
    "{\n"
    "  XTypes::TypeMap tm;\n";

  idx = 0;
  for (OpenDDS::XTypes::TypeMap::const_iterator pos = minimal_type_map_.begin();
       pos != minimal_type_map_.end(); ++pos, ++idx) {
    be_global->impl_ << "  tm[" << pos->first << "] = OPENDDS_IDL_FILE_SPECIFIC(minimal_to, " << idx << ")();\n";
  }

  be_global->impl_ <<
    "  return tm;\n"
    "}\n\n";

  if (produce_xtypes_complete_) {
    idx = 0;
    for (OpenDDS::XTypes::TypeMap::const_iterator pos = complete_type_map_.begin();
         pos != complete_type_map_.end(); ++pos, ++idx) {
      be_global->impl_ <<
        "XTypes::TypeObject OPENDDS_IDL_FILE_SPECIFIC(complete_to, " << idx << ")()\n"
        "{\n"
        "  const unsigned char to_bytes[] = {\n";
      dump_bytes(pos->second);
      be_global->add_include("<stdexcept>", BE_GlobalData::STREAM_CPP);
      be_global->impl_ <<
        "  };\n"
        "  XTypes::TypeObject to;\n"
        "  if (!to_type_object(to_bytes, sizeof(to_bytes), to)) {\n"
        "    throw std::runtime_error(\"Could not deserialize complete Type Object " << idx << "\");\n"
        "  }\n"
        "  return to;\n"
        "}\n\n";
    }

    be_global->impl_ <<
      "XTypes::TypeMap OPENDDS_IDL_FILE_SPECIFIC(get_complete_type_map_private, 0)()\n"
      "{\n"
      "  XTypes::TypeMap tm;\n";

    idx = 0;
    for (OpenDDS::XTypes::TypeMap::const_iterator pos = complete_type_map_.begin();
         pos != complete_type_map_.end(); ++pos, ++idx) {
      be_global->impl_ << "  tm[" << pos->first << "] = OPENDDS_IDL_FILE_SPECIFIC(complete_to, " << idx << ")();\n";
    }

    be_global->impl_ <<
      "  return tm;\n"
      "}\n";
  }
  be_global->impl_ << "}\n\n";

  const std::string common = "{\n"
    "  static XTypes::TypeMap tm;\n"
    "  ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, TheServiceParticipant->get_static_xtypes_lock(), tm);\n"
    "  if (tm.empty()) {\n";

  be_global->impl_ <<
    "const XTypes::TypeMap& OPENDDS_IDL_FILE_SPECIFIC(get_minimal_type_map, 0)()\n" << common <<
    "    tm = OPENDDS_IDL_FILE_SPECIFIC(get_minimal_type_map_private, 0)();\n"
    "  }\n"
    "  return tm;\n"
    "}\n\n";

  if (produce_xtypes_complete_) {
    be_global->impl_ <<
      "const XTypes::TypeMap& OPENDDS_IDL_FILE_SPECIFIC(get_complete_type_map, 0)()\n" << common <<
      "    tm = OPENDDS_IDL_FILE_SPECIFIC(get_complete_type_map_private, 0)();\n"
      "  }\n"
      "  return tm;\n"
      "}\n";
  }
}

string
typeobject_generator::tag_type(UTL_ScopedName* name)
{
  return dds_generator::get_xtag_name(name);
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

void
typeobject_generator::consider(Element& v, AST_Type* type, const std::string& anonymous_name)
{
  switch (type->node_type()) {
  case AST_ConcreteType::NT_union_fwd:
    {
      AST_UnionFwd* const n = dynamic_cast<AST_UnionFwd*>(type);
      type = n->full_definition();
      break;
    }
  case AST_ConcreteType::NT_struct_fwd:
    {
      AST_StructureFwd* const n = dynamic_cast<AST_StructureFwd*>(type);
      type = n->full_definition();
      break;
    }
  default:
    break;
  }

  if (element_.find(type) == element_.end()) {
    strong_connect(type, anonymous_name);
    v.lowlink = std::min(v.lowlink, element_[type].lowlink);
  } else if (element_[type].on_stack) {
    v.lowlink = std::min(v.lowlink, element_[type].index);
  }
}

void
typeobject_generator::strong_connect(AST_Type* type, const std::string& anonymous_name)
{
  switch (type->node_type()) {
  case AST_ConcreteType::NT_union_fwd:
    {
      AST_UnionFwd* const n = dynamic_cast<AST_UnionFwd*>(type);
      type = n->full_definition();
      break;
    }
  case AST_ConcreteType::NT_struct_fwd:
    {
      AST_StructureFwd* const n = dynamic_cast<AST_StructureFwd*>(type);
      type = n->full_definition();
      break;
    }
  default:
    break;
  }

  Element& v = element_[type];
  v.type = type;
  v.index = index_;
  v.lowlink = index_;
  ++index_;
  stack_.push_back(type);
  v.on_stack = true;

  using OpenDDS::XTypes::MemberId;
  AST_Structure* const stru = dynamic_cast<AST_Structure*>(type);
  switch (type->node_type()) {

  case AST_ConcreteType::NT_union:
    {
      AST_Union* const n = dynamic_cast<AST_Union*>(type);
      v.name = canonical_name(n->name());

      AST_Type* discriminator = n->disc_type();
      const Fields fields(n);

      consider(v, discriminator, v.name + ".d");

      const AutoidKind auto_id = be_global->autoid(n);
      MemberId member_id = 0;

      for (Fields::Iterator i = fields.begin(); i != fields.end(); ++i) {
        AST_UnionBranch* ub = dynamic_cast<AST_UnionBranch*>(*i);
        const MemberId id = be_global->compute_id(stru, ub, auto_id, member_id);
        consider(v, ub->field_type(), v.name + "." + OpenDDS::DCPS::to_dds_string(id));
      }

      break;
    }

  case AST_ConcreteType::NT_struct:
    {
      AST_Structure* const n = dynamic_cast<AST_Structure*>(type);
      v.name = canonical_name(n->name());

      // TODO: Struct inheritance.

      const Fields fields(n);
      const AutoidKind auto_id = be_global->autoid(n);
      MemberId member_id = 0;

      for (Fields::Iterator i = fields.begin(); i != fields.end(); ++i) {
        AST_Field* field = *i;
        const MemberId id = be_global->compute_id(stru, field, auto_id, member_id);
        consider(v, field->field_type(), v.name + "." + OpenDDS::DCPS::to_dds_string(id));
      }

      break;
    }

  case AST_ConcreteType::NT_array:
    {
      AST_Array* const n = dynamic_cast<AST_Array*>(type);
      v.name = anonymous_name + ".a";
      consider(v, n->base_type(), v.name);
      break;
    }
#if OPENDDS_HAS_IDL_MAP
  case AST_ConcreteType::NT_map:
    {
      AST_Map* const n = dynamic_cast<AST_Map*>(type);
      v.name = anonymous_name + ".s";
      consider(v, n->key_type(), v.name);
      consider(v, n->value_type(), v.name);
      break;
    }
#endif
  case AST_ConcreteType::NT_sequence:
    {
      AST_Sequence* const n = dynamic_cast<AST_Sequence*>(type);
      v.name = anonymous_name + ".s";
      consider(v, n->base_type(), v.name);
      break;
    }

  case AST_ConcreteType::NT_typedef:
    {
      AST_Typedef* const n = dynamic_cast<AST_Typedef*>(type);
      v.name = canonical_name(n->name());
      // TODO: What is the member name for an anonymous type in a typedef?
      // 7.3.4.9.2
      consider(v, n->base_type(), v.name + ".0");
      break;
    }

  case AST_ConcreteType::NT_enum:
    {
      AST_Enum* const n = dynamic_cast<AST_Enum*>(type);
      v.name = canonical_name(n->name());
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

  default:
    // Different versions of TAO_IDL_FE can have additional NT_* enumerators.
    // Ignoring them with a default: case silences a compiler warning.
    break;
  }

  if (v.lowlink == v.index) {
    typedef std::vector<Element> List;
    List scc;
    AST_Type* wt;
    do {
      wt = stack_.back();
      stack_.pop_back();
      Element& w = element_[wt];
      w.on_stack = false;
      scc.push_back(w);
    } while (wt != v.type);

    // Sort types in SCC using lexicographic order of their fully qualified names.
    std::sort(scc.begin(), scc.end());

    if (scc.size() == 1) {
      generate_type_identifier(scc[0].type, false);
    } else {
      // Compute temporary type identifiers for the types in SCC with hash value of zero.
      OpenDDS::XTypes::TypeIdentifier minimal_ti(OpenDDS::XTypes::TI_STRONGLY_CONNECTED_COMPONENT);
      minimal_ti.sc_component_id().sc_component_id.kind = OpenDDS::XTypes::EK_MINIMAL;
      std::memset(&minimal_ti.sc_component_id().sc_component_id.hash, 0, sizeof(OpenDDS::XTypes::EquivalenceHash));
      minimal_ti.sc_component_id().scc_length = static_cast<int>(scc.size());

      OpenDDS::XTypes::TypeIdentifier complete_ti = minimal_ti;
      complete_ti.sc_component_id().sc_component_id.kind = OpenDDS::XTypes::EK_COMPLETE;

      {
        size_t idx = 0;
        for (List::const_iterator pos = scc.begin(); pos != scc.end(); ++pos) {
          minimal_ti.sc_component_id().scc_index = static_cast<int>(++idx); // Starts at 1.
          complete_ti.sc_component_id().scc_index = minimal_ti.sc_component_id().scc_index;
          const TypeIdentifierPair ti_pair = {minimal_ti, complete_ti};
          hash_type_identifier_map_[pos->type] = ti_pair;
        }
      }

      // Construct temporary type objects from the temporary type identifiers.
      OpenDDS::XTypes::TypeObjectSeq minimal_seq, complete_seq;
      for (List::const_iterator pos = scc.begin(); pos != scc.end(); ++pos) {
        generate_type_identifier(pos->type, true);
        OPENDDS_ASSERT(type_object_map_.count(pos->type) != 0);
        minimal_seq.append(type_object_map_[pos->type].minimal);
        complete_seq.append(type_object_map_[pos->type].complete);
      }

      // Compute the final type identifiers with the correct hash value.
      const OpenDDS::DCPS::Encoding& encoding = OpenDDS::XTypes::get_typeobject_encoding();
      size_t minimal_size = serialized_size(encoding, minimal_seq);
      ACE_Message_Block minimal_buff(minimal_size);
      OpenDDS::DCPS::Serializer minimal_ser(&minimal_buff, encoding);
      if (!(minimal_ser << minimal_seq)) {
        be_util::misc_error_and_abort("Failed to serialize minimal type object sequence in strongly-connected component", type);
      }

      unsigned char result[sizeof(OpenDDS::DCPS::MD5Result)];
      OpenDDS::DCPS::MD5Hash(result, minimal_buff.rd_ptr(), minimal_buff.length());

      // First 14 bytes of MD5 of the serialized TypeObject using XCDR
      // version 2 with Little Endian encoding
      std::memcpy(minimal_ti.sc_component_id().sc_component_id.hash, result, sizeof(OpenDDS::XTypes::EquivalenceHash));

      size_t complete_size = serialized_size(encoding, complete_seq);
      ACE_Message_Block complete_buff(complete_size);
      OpenDDS::DCPS::Serializer complete_ser(&complete_buff, encoding);
      if (!(complete_ser << complete_seq)) {
        be_util::misc_error_and_abort("Failed to serialize complete type object sequence in strongly-connected component", type);
      }

      OpenDDS::DCPS::MD5Hash(result, complete_buff.rd_ptr(), complete_buff.length());
      std::memcpy(complete_ti.sc_component_id().sc_component_id.hash, result, sizeof(OpenDDS::XTypes::EquivalenceHash));

      {
        size_t idx = 0;
        for (List::const_iterator pos = scc.begin(); pos != scc.end(); ++pos) {
          minimal_ti.sc_component_id().scc_index = static_cast<int>(++idx);
          complete_ti.sc_component_id().scc_index = minimal_ti.sc_component_id().scc_index;
          const TypeIdentifierPair ti_pair = {minimal_ti, complete_ti};
          hash_type_identifier_map_[pos->type] = ti_pair;
        }
      }

      // Compute the final type objects with the final type identifiers.
      for (List::const_iterator pos = scc.begin(); pos != scc.end(); ++pos) {
        generate_type_identifier(pos->type, true);
        const OpenDDS::XTypes::TypeIdentifier& minimal_ti = hash_type_identifier_map_[pos->type].minimal;
        const OpenDDS::XTypes::TypeIdentifier& complete_ti = hash_type_identifier_map_[pos->type].complete;
        minimal_type_map_[minimal_ti] = type_object_map_[pos->type].minimal;
        complete_type_map_[complete_ti] = type_object_map_[pos->type].complete;
      }
    }
  }
}

void
typeobject_generator::update_maps(AST_Type* type,
                                  const OpenDDS::XTypes::TypeObject& minimal_to,
                                  const OpenDDS::XTypes::TypeObject& complete_to)
{
  const TypeObjectPair to_pair = {minimal_to, complete_to};
  type_object_map_[type] = to_pair;

  // In case of SCC, type identifiers of the types in SCC are computed first and
  // type objects are constructed using them. In that case, we don't want to update
  // each individual type's type identifier by hashing the corresponding type object.
  // On the other hand, type identifier is computed if the type is not part of a SCC.
  if (hash_type_identifier_map_.count(type) == 0) {
    const OpenDDS::XTypes::TypeIdentifier minimal_ti = makeTypeIdentifier(minimal_to, typeid_encoding_);
    const OpenDDS::XTypes::TypeIdentifier complete_ti = makeTypeIdentifier(complete_to, typeid_encoding_);
    const TypeIdentifierPair ti_pair = {minimal_ti, complete_ti};
    hash_type_identifier_map_[type] = ti_pair;

    minimal_type_map_[minimal_ti] = minimal_to;
    complete_type_map_[complete_ti] = complete_to;
  }
}

void typeobject_generator::set_builtin_member_annotations(AST_Decl* member,
  OpenDDS::XTypes::Optional<OpenDDS::XTypes::AppliedBuiltinMemberAnnotations>& annotations)
{
  // Support only @hashid annotation for member at this time.
  const HashidAnnotation* hashid_ann = dynamic_cast<const HashidAnnotation*>(be_global->builtin_annotations_["::@hashid"]);
  std::string hash_name;
  if (hashid_ann->node_value_exists(member, hash_name)) {
    OpenDDS::XTypes::Optional<std::string> hash_id(hash_name);
    if (!annotations) {
      OpenDDS::XTypes::AppliedBuiltinMemberAnnotations value;
      annotations = OpenDDS::XTypes::Optional<OpenDDS::XTypes::AppliedBuiltinMemberAnnotations>(value);
    }
    annotations.value().hash_id = hash_id;
  }
}

void
typeobject_generator::generate_struct_type_identifier(AST_Type* type)
{
  AST_Structure* const n = dynamic_cast<AST_Structure*>(type);
  const Fields fields(n);

  const ExtensibilityKind exten = be_global->extensibility(n);
  const AutoidKind auto_id = be_global->autoid(n);

  OpenDDS::XTypes::TypeObject minimal_to, complete_to;
  minimal_to.kind = OpenDDS::XTypes::EK_MINIMAL;
  minimal_to.minimal.kind = OpenDDS::XTypes::TK_STRUCTURE;

  minimal_to.minimal.struct_type.struct_flags = extensibility_to_type_flag(exten);

  if (be_global->is_nested(n)) {
    minimal_to.minimal.struct_type.struct_flags |= OpenDDS::XTypes::IS_NESTED;
  }

  if (auto_id == autoidkind_hash) {
    minimal_to.minimal.struct_type.struct_flags |= OpenDDS::XTypes::IS_AUTOID_HASH;
  }

  // TODO: Support inheritance.
  minimal_to.minimal.struct_type.header.base_type = OpenDDS::XTypes::TypeIdentifier(OpenDDS::XTypes::TK_NONE);

  complete_to.kind = OpenDDS::XTypes::EK_COMPLETE;
  complete_to.complete.kind = OpenDDS::XTypes::TK_STRUCTURE;

  complete_to.complete.struct_type.struct_flags = minimal_to.minimal.struct_type.struct_flags;
  complete_to.complete.struct_type.header.base_type = OpenDDS::XTypes::TypeIdentifier(OpenDDS::XTypes::TK_NONE);
  complete_to.complete.struct_type.header.detail.type_name = canonical_name(type->name());
  // @verbatim and custom annotations are not supported.

  for (Fields::Iterator i = fields.begin(); i != fields.end(); ++i) {
    AST_Field* field = *i;
    const TryConstructFailAction trycon = be_global->try_construct(field);

    OpenDDS::XTypes::MinimalStructMember minimal_member;
    minimal_member.common.member_id = be_global->get_id(field);
    minimal_member.common.member_flags = try_construct_to_member_flag(trycon);

    if (be_global->is_optional(field)) {
      minimal_member.common.member_flags |= OpenDDS::XTypes::IS_OPTIONAL;
    }

    if (be_global->is_must_understand(field)) {
      minimal_member.common.member_flags |= OpenDDS::XTypes::IS_MUST_UNDERSTAND;
    }

    if (be_global->is_key(field)) {
      minimal_member.common.member_flags |= OpenDDS::XTypes::IS_KEY;
    }

    if (be_global->is_external(field)) {
      minimal_member.common.member_flags |= OpenDDS::XTypes::IS_EXTERNAL;
    }

    minimal_member.common.member_type_id = get_minimal_type_identifier(field->field_type());
    const std::string name = canonical_name(field->local_name());
    OpenDDS::XTypes::hash_member_name(minimal_member.detail.name_hash, name);
    minimal_to.minimal.struct_type.member_seq.append(minimal_member);

    OpenDDS::XTypes::CompleteStructMember complete_member;
    complete_member.common.member_id = minimal_member.common.member_id;
    complete_member.common.member_flags = minimal_member.common.member_flags;
    complete_member.common.member_type_id = get_complete_type_identifier(field->field_type());

    complete_member.detail.name = name;
    set_builtin_member_annotations(field, complete_member.detail.ann_builtin);

    complete_to.complete.struct_type.member_seq.append(complete_member);
  }

  if (be_global->old_typeobject_member_order()) {
    minimal_to.minimal.struct_type.member_seq.sort();
    complete_to.complete.struct_type.member_seq.sort();
  }

  update_maps(type, minimal_to, complete_to);
}

void
typeobject_generator::generate_union_type_identifier(AST_Type* type)
{
  AST_Union* const n = dynamic_cast<AST_Union*>(type);
  AST_Type* discriminator = n->disc_type();
  const Fields fields(n);

  const ExtensibilityKind exten = be_global->extensibility(n);
  const AutoidKind auto_id = be_global->autoid(n);

  OpenDDS::XTypes::TypeObject minimal_to, complete_to;
  minimal_to.kind = OpenDDS::XTypes::EK_MINIMAL;
  minimal_to.minimal.kind = OpenDDS::XTypes::TK_UNION;
  minimal_to.minimal.union_type.union_flags = extensibility_to_type_flag(exten);

  if (be_global->is_nested(n)) {
    minimal_to.minimal.union_type.union_flags |= OpenDDS::XTypes::IS_NESTED;
  }

  if (auto_id == autoidkind_hash) {
    minimal_to.minimal.union_type.union_flags |= OpenDDS::XTypes::IS_AUTOID_HASH;
  }

  const TryConstructFailAction trycon = be_global->union_discriminator_try_construct(n);
  minimal_to.minimal.union_type.discriminator.common.member_flags = try_construct_to_member_flag(trycon);
  if (be_global->union_discriminator_is_key(n)) {
    minimal_to.minimal.union_type.discriminator.common.member_flags |= OpenDDS::XTypes::IS_KEY;
  }
  minimal_to.minimal.union_type.discriminator.common.type_id = get_minimal_type_identifier(discriminator);

  complete_to.kind = OpenDDS::XTypes::EK_COMPLETE;
  complete_to.complete.kind = OpenDDS::XTypes::TK_UNION;
  complete_to.complete.union_type.union_flags = minimal_to.minimal.union_type.union_flags;

  complete_to.complete.union_type.header.detail.type_name = canonical_name(type->name());

  complete_to.complete.union_type.discriminator.common.member_flags =
    minimal_to.minimal.union_type.discriminator.common.member_flags;
  complete_to.complete.union_type.discriminator.common.type_id = get_complete_type_identifier(discriminator);

  for (Fields::Iterator i = fields.begin(); i != fields.end(); ++i) {
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

    OpenDDS::XTypes::MinimalUnionMember minimal_member;
    minimal_member.common.member_id = be_global->get_id(branch);
    minimal_member.common.member_flags = try_construct_to_member_flag(trycon);

    if (is_default) {
      minimal_member.common.member_flags |= OpenDDS::XTypes::IS_DEFAULT;
    }

    if (be_global->is_external(branch)) {
      minimal_member.common.member_flags |= OpenDDS::XTypes::IS_EXTERNAL;
    }

    minimal_member.common.type_id = get_minimal_type_identifier(branch->field_type());

    for (unsigned long j = 0; j < branch->label_list_length(); ++j) {
      AST_UnionLabel* label = branch->label(j);
      if (label->label_kind() != AST_UnionLabel::UL_default) {
        minimal_member.common.label_seq.append(to_long(*label->label_val()->ev()));
      }
    }
    minimal_member.common.label_seq.sort();

    const std::string name = canonical_name(branch->local_name());
    OpenDDS::XTypes::hash_member_name(minimal_member.detail.name_hash, name);
    minimal_to.minimal.union_type.member_seq.append(minimal_member);

    OpenDDS::XTypes::CompleteUnionMember complete_member;
    complete_member.common.member_id = minimal_member.common.member_id;
    complete_member.common.member_flags = minimal_member.common.member_flags;
    complete_member.common.type_id = get_complete_type_identifier(branch->field_type());
    complete_member.common.label_seq = minimal_member.common.label_seq;

    complete_member.detail.name = name;
    set_builtin_member_annotations(branch, complete_member.detail.ann_builtin);

    complete_to.complete.union_type.member_seq.append(complete_member);
  }

  if (be_global->old_typeobject_member_order()) {
    minimal_to.minimal.union_type.member_seq.sort();
    complete_to.complete.union_type.member_seq.sort();
  }

  update_maps(type, minimal_to, complete_to);
}

void
typeobject_generator::generate_enum_type_identifier(AST_Type* type)
{
  AST_Enum* const n = dynamic_cast<AST_Enum*>(type);
  std::vector<AST_EnumVal*> contents;
  scope2vector(contents, n, AST_Decl::NT_enum_val);
  bool has_extensibility_annotation = false;
  const ExtensibilityKind ek = be_global->extensibility(type, extensibilitykind_appendable, has_extensibility_annotation);

  if (ek == extensibilitykind_mutable) {
    be_util::misc_error_and_abort("MUTABLE extensibility for enum is not allowed", type);
  }

  size_t default_literal_idx = 0;
  for (size_t i = 0; i != contents.size(); ++i) {
    if (contents[i]->annotations().find("@default_literal")) {
      default_literal_idx = i;
      break;
    }
  }

  OpenDDS::XTypes::TypeObject minimal_to, complete_to;
  minimal_to.kind = OpenDDS::XTypes::EK_MINIMAL;
  minimal_to.minimal.kind = OpenDDS::XTypes::TK_ENUM;
  if (has_extensibility_annotation || !be_global->default_enum_extensibility_zero()) {
    minimal_to.minimal.enumerated_type.enum_flags = extensibility_to_type_flag(ek);
  }
  // TODO: Add support for @bit_bound.
  minimal_to.minimal.enumerated_type.header.common.bit_bound = 32;

  complete_to.kind = OpenDDS::XTypes::EK_COMPLETE;
  complete_to.complete.kind = OpenDDS::XTypes::TK_ENUM;
  if (has_extensibility_annotation || !be_global->default_enum_extensibility_zero()) {
    complete_to.complete.enumerated_type.enum_flags = extensibility_to_type_flag(ek);
  }
  complete_to.complete.enumerated_type.header.common.bit_bound = 32;
  complete_to.complete.enumerated_type.header.detail.type_name = canonical_name(type->name());

  for (size_t i = 0; i != contents.size(); ++i) {
    OpenDDS::XTypes::MinimalEnumeratedLiteral minimal_lit;
    minimal_lit.common.value = contents[i]->constant_value()->ev()->u.eval;
    minimal_lit.common.flags = (i == default_literal_idx ? OpenDDS::XTypes::IS_DEFAULT : 0);
    const std::string name = canonical_name(contents[i]->local_name());
    OpenDDS::XTypes::hash_member_name(minimal_lit.detail.name_hash, name);
    minimal_to.minimal.enumerated_type.literal_seq.append(minimal_lit);

    OpenDDS::XTypes::CompleteEnumeratedLiteral complete_lit;
    complete_lit.common = minimal_lit.common;
    complete_lit.detail.name = name;

    complete_to.complete.enumerated_type.literal_seq.append(complete_lit);
  }
  minimal_to.minimal.enumerated_type.literal_seq.sort();
  complete_to.complete.enumerated_type.literal_seq.sort();

  update_maps(type, minimal_to, complete_to);
}

void
typeobject_generator::generate_array_type_identifier(AST_Type* type, bool force_type_object)
{
  AST_Array* const n = dynamic_cast<AST_Array*>(type);

  const TryConstructFailAction trycon = be_global->try_construct(n->base_type());
  OpenDDS::XTypes::CollectionElementFlag cef = try_construct_to_member_flag(trycon);
  if (be_global->is_external(n->base_type())) {
    cef |= OpenDDS::XTypes::IS_EXTERNAL;
  }
  const OpenDDS::XTypes::TypeIdentifier minimal_elem_ti = get_minimal_type_identifier(n->base_type());
  const OpenDDS::XTypes::TypeIdentifier complete_elem_ti = get_complete_type_identifier(n->base_type());

  if (be_global->is_plain(type) && !force_type_object) {
    const OpenDDS::XTypes::EquivalenceKind minimal_ek =
      OpenDDS::XTypes::is_fully_descriptive(minimal_elem_ti) ? OpenDDS::XTypes::EK_BOTH : OpenDDS::XTypes::EK_MINIMAL;
    const OpenDDS::XTypes::EquivalenceKind complete_ek =
      minimal_ek == OpenDDS::XTypes::EK_BOTH ? minimal_ek : OpenDDS::XTypes::EK_COMPLETE;

    ACE_CDR::ULong max_bound = 0;
    for (ACE_CDR::ULong dim = 0; dim != n->n_dims(); ++dim) {
      max_bound = std::max(max_bound, n->dims()[dim]->ev()->u.ulval);
    }

    if (max_bound < 256) {
      OpenDDS::XTypes::TypeIdentifier minimal_ti(OpenDDS::XTypes::TI_PLAIN_ARRAY_SMALL);
      minimal_ti.array_sdefn().header.equiv_kind = minimal_ek;
      minimal_ti.array_sdefn().header.element_flags = cef;
      for (ACE_CDR::ULong dim = 0; dim != n->n_dims(); ++dim) {
        minimal_ti.array_sdefn().array_bound_seq.append(n->dims()[dim]->ev()->u.ulval);
      }
      minimal_ti.array_sdefn().element_identifier = minimal_elem_ti;

      if (minimal_ek == OpenDDS::XTypes::EK_BOTH) {
        fully_desc_type_identifier_map_[type] = minimal_ti;
      } else {
        OpenDDS::XTypes::TypeIdentifier complete_ti(OpenDDS::XTypes::TI_PLAIN_ARRAY_SMALL);
        complete_ti.array_sdefn().header.equiv_kind = complete_ek;
        complete_ti.array_sdefn().header.element_flags = cef;
        complete_ti.array_sdefn().array_bound_seq = minimal_ti.array_sdefn().array_bound_seq;
        complete_ti.array_sdefn().element_identifier = complete_elem_ti;

        const TypeIdentifierPair ti_pair = {minimal_ti, complete_ti};
        hash_type_identifier_map_[type] = ti_pair;
      }
    } else {
      OpenDDS::XTypes::TypeIdentifier minimal_ti(OpenDDS::XTypes::TI_PLAIN_ARRAY_LARGE);
      minimal_ti.array_ldefn().header.equiv_kind = minimal_ek;
      minimal_ti.array_ldefn().header.element_flags = cef;
      for (ACE_CDR::ULong dim = 0; dim != n->n_dims(); ++dim) {
        minimal_ti.array_ldefn().array_bound_seq.append(n->dims()[dim]->ev()->u.ulval);
      }
      minimal_ti.array_ldefn().element_identifier = minimal_elem_ti;

      if (minimal_ek == OpenDDS::XTypes::EK_BOTH) {
        fully_desc_type_identifier_map_[type] = minimal_ti;
      } else {
        OpenDDS::XTypes::TypeIdentifier complete_ti(OpenDDS::XTypes::TI_PLAIN_ARRAY_LARGE);
        complete_ti.array_ldefn().header.equiv_kind = complete_ek;
        complete_ti.array_ldefn().header.element_flags = cef;
        complete_ti.array_ldefn().array_bound_seq = minimal_ti.array_ldefn().array_bound_seq;
        complete_ti.array_ldefn().element_identifier = complete_elem_ti;

        const TypeIdentifierPair ti_pair = {minimal_ti, complete_ti};
        hash_type_identifier_map_[type] = ti_pair;
      }
    }
  } else {
    OpenDDS::XTypes::TypeObject minimal_to, complete_to;
    minimal_to.kind = OpenDDS::XTypes::EK_MINIMAL;
    minimal_to.minimal.kind = OpenDDS::XTypes::TK_ARRAY;
    for (ACE_CDR::ULong dim = 0; dim != n->n_dims(); ++dim) {
      minimal_to.minimal.array_type.header.common.bound_seq.append(n->dims()[dim]->ev()->u.ulval);
    }
    minimal_to.minimal.array_type.element.common.element_flags = cef;
    minimal_to.minimal.array_type.element.common.type = minimal_elem_ti;

    complete_to.kind = OpenDDS::XTypes::EK_COMPLETE;
    complete_to.complete.kind = OpenDDS::XTypes::TK_ARRAY;
    complete_to.complete.array_type.header.common.bound_seq = minimal_to.minimal.array_type.header.common.bound_seq;
    complete_to.complete.array_type.header.detail.type_name = canonical_name(type->name());

    complete_to.complete.array_type.element.common.element_flags = cef;
    complete_to.complete.array_type.element.common.type = complete_elem_ti;

    update_maps(type, minimal_to, complete_to);
  }
}

#if OPENDDS_HAS_IDL_MAP
void
typeobject_generator::generate_map_type_identifier(AST_Type* type, bool force_type_object)
{
  AST_Map* const n = dynamic_cast<AST_Map*>(type);

  // I'm not sure if this is setup for maps
  // ACE_CDR::ULong bound = 0;
  // if (!n->unbounded()) {
  //   bound = n->max_size()->ev()->u.ulval;
  // }

  const TryConstructFailAction trykey = be_global->try_construct(n->key_type());
  const TryConstructFailAction tryval = be_global->try_construct(n->value_type());
  OpenDDS::XTypes::CollectionElementFlag cef_key = try_construct_to_member_flag(trykey);

  // TODO combine these? Not sure how to deal with the key/val duplicate structs
  if (be_global->is_external(n->key_type())) {
    cef_key |= OpenDDS::XTypes::IS_EXTERNAL;
  }
  OpenDDS::XTypes::CollectionElementFlag cef_val = try_construct_to_member_flag(tryval);
  if (be_global->is_external(n->value_type())) {
    cef_val |= OpenDDS::XTypes::IS_EXTERNAL;
  }

  const OpenDDS::XTypes::TypeIdentifier minimal_key_ti = get_minimal_type_identifier(n->key_type());
  const OpenDDS::XTypes::TypeIdentifier minimal_val_ti = get_minimal_type_identifier(n->value_type());
  const OpenDDS::XTypes::TypeIdentifier complete_key_ti = get_complete_type_identifier(n->key_type());
  const OpenDDS::XTypes::TypeIdentifier complete_val_ti = get_complete_type_identifier(n->value_type());

  if (be_global->is_plain(type) && !force_type_object) {
    const OpenDDS::XTypes::EquivalenceKind key_minimal_ek =
      OpenDDS::XTypes::is_fully_descriptive(minimal_key_ti) ? OpenDDS::XTypes::EK_BOTH : OpenDDS::XTypes::EK_MINIMAL;
    const OpenDDS::XTypes::EquivalenceKind val_minimal_ek =
      OpenDDS::XTypes::is_fully_descriptive(minimal_val_ti) ? OpenDDS::XTypes::EK_BOTH : OpenDDS::XTypes::EK_MINIMAL;

    const OpenDDS::XTypes::EquivalenceKind key_complete_ek =
      key_minimal_ek == OpenDDS::XTypes::EK_BOTH ? key_minimal_ek : OpenDDS::XTypes::EK_COMPLETE;
    //const OpenDDS::XTypes::EquivalenceKind val_complete_ek =
      //val_minimal_ek == OpenDDS::XTypes::EK_BOTH ? val_minimal_ek : OpenDDS::XTypes::EK_COMPLETE;

    OpenDDS::XTypes::TypeIdentifier minimal_ti(OpenDDS::XTypes::TI_PLAIN_MAP_LARGE);
    minimal_ti.map_sdefn().header.element_flags = cef_key;
    minimal_ti.map_sdefn().header.equiv_kind = key_complete_ek;
    minimal_ti.map_sdefn().key_identifier = minimal_key_ti;
    minimal_ti.map_sdefn().element_identifier = minimal_val_ti;

    OpenDDS::XTypes::TypeIdentifier complete_ti(OpenDDS::XTypes::TI_PLAIN_MAP_LARGE);
    complete_ti.map_sdefn().header.element_flags = cef_key;
    complete_ti.map_sdefn().header.equiv_kind = key_complete_ek;
    complete_ti.map_sdefn().key_identifier = complete_key_ti;
    complete_ti.map_sdefn().element_identifier = complete_val_ti;

    const TypeIdentifierPair ti_pair = {minimal_ti, complete_ti};
    hash_type_identifier_map_[type] = ti_pair;

  } else {
    OpenDDS::XTypes::TypeObject minimal_to, complete_to;
    minimal_to.kind = OpenDDS::XTypes::EK_MINIMAL;
    minimal_to.minimal.kind = OpenDDS::XTypes::TK_MAP;
    minimal_to.minimal.map_type.key.common.type = minimal_key_ti;
    minimal_to.minimal.map_type.key.common.element_flags = cef_key;
    minimal_to.minimal.map_type.element.common.type = minimal_val_ti;
    minimal_to.minimal.map_type.element.common.element_flags = cef_val;

    complete_to.kind = OpenDDS::XTypes::EK_COMPLETE;
    complete_to.complete.kind = OpenDDS::XTypes::TK_MAP;
    complete_to.complete.map_type.key.common.element_flags = cef_key;
    complete_to.complete.map_type.key.common.type = complete_key_ti;
    complete_to.complete.map_type.element.common.element_flags = cef_val;
    complete_to.complete.map_type.element.common.type = complete_val_ti;

    update_maps(type, minimal_to, complete_to);
  }
}
#endif

void
typeobject_generator::generate_sequence_type_identifier(AST_Type* type, bool force_type_object)
{
  AST_Sequence* const n = dynamic_cast<AST_Sequence*>(type);

  ACE_CDR::ULong bound = 0;
  if (!n->unbounded()) {
    bound = n->max_size()->ev()->u.ulval;
  }

  const TryConstructFailAction trycon = be_global->try_construct(n->base_type());
  OpenDDS::XTypes::CollectionElementFlag cef = try_construct_to_member_flag(trycon);
  if (be_global->is_external(n->base_type())) {
    cef |= OpenDDS::XTypes::IS_EXTERNAL;
  }
  const OpenDDS::XTypes::TypeIdentifier minimal_elem_ti = get_minimal_type_identifier(n->base_type());
  const OpenDDS::XTypes::TypeIdentifier complete_elem_ti = get_complete_type_identifier(n->base_type());

  if (be_global->is_plain(type) && !force_type_object) {
    const OpenDDS::XTypes::EquivalenceKind minimal_ek =
      OpenDDS::XTypes::is_fully_descriptive(minimal_elem_ti) ? OpenDDS::XTypes::EK_BOTH : OpenDDS::XTypes::EK_MINIMAL;
    const OpenDDS::XTypes::EquivalenceKind complete_ek =
      minimal_ek == OpenDDS::XTypes::EK_BOTH ? minimal_ek : OpenDDS::XTypes::EK_COMPLETE;
    if (bound < 256) {
      OpenDDS::XTypes::TypeIdentifier minimal_ti(OpenDDS::XTypes::TI_PLAIN_SEQUENCE_SMALL);
      minimal_ti.seq_sdefn().header.equiv_kind = minimal_ek;
      minimal_ti.seq_sdefn().header.element_flags = cef;
      minimal_ti.seq_sdefn().bound = bound;
      minimal_ti.seq_sdefn().element_identifier = minimal_elem_ti;

      if (minimal_ek == OpenDDS::XTypes::EK_BOTH) {
        fully_desc_type_identifier_map_[type] = minimal_ti;
      } else {
        OpenDDS::XTypes::TypeIdentifier complete_ti(OpenDDS::XTypes::TI_PLAIN_SEQUENCE_SMALL);
        complete_ti.seq_sdefn().header.equiv_kind = complete_ek;
        complete_ti.seq_sdefn().header.element_flags = cef;
        complete_ti.seq_sdefn().bound = bound;
        complete_ti.seq_sdefn().element_identifier = complete_elem_ti;

        const TypeIdentifierPair ti_pair = {minimal_ti, complete_ti};
        hash_type_identifier_map_[type] = ti_pair;
      }
    } else {
      OpenDDS::XTypes::TypeIdentifier minimal_ti(OpenDDS::XTypes::TI_PLAIN_SEQUENCE_LARGE);
      minimal_ti.seq_ldefn().header.equiv_kind = minimal_ek;
      minimal_ti.seq_ldefn().header.element_flags = cef;
      minimal_ti.seq_ldefn().bound = bound;
      minimal_ti.seq_ldefn().element_identifier = minimal_elem_ti;

      if (minimal_ek == OpenDDS::XTypes::EK_BOTH) {
        fully_desc_type_identifier_map_[type] = minimal_ti;
      } else {
        OpenDDS::XTypes::TypeIdentifier complete_ti(OpenDDS::XTypes::TI_PLAIN_SEQUENCE_LARGE);
        complete_ti.seq_ldefn().header.equiv_kind = complete_ek;
        complete_ti.seq_ldefn().header.element_flags = cef;
        complete_ti.seq_ldefn().bound = bound;
        complete_ti.seq_ldefn().element_identifier = complete_elem_ti;

        const TypeIdentifierPair ti_pair = {minimal_ti, complete_ti};
        hash_type_identifier_map_[type] = ti_pair;
      }
    }
  } else {
    OpenDDS::XTypes::TypeObject minimal_to, complete_to;
    minimal_to.kind = OpenDDS::XTypes::EK_MINIMAL;
    minimal_to.minimal.kind = OpenDDS::XTypes::TK_SEQUENCE;
    minimal_to.minimal.sequence_type.header.common.bound = bound;
    minimal_to.minimal.sequence_type.element.common.element_flags = cef;
    minimal_to.minimal.sequence_type.element.common.type = minimal_elem_ti;

    complete_to.kind = OpenDDS::XTypes::EK_COMPLETE;
    complete_to.complete.kind = OpenDDS::XTypes::TK_SEQUENCE;
    complete_to.complete.sequence_type.header.common.bound = bound;
    complete_to.complete.sequence_type.element.common.element_flags = cef;
    complete_to.complete.sequence_type.element.common.type = complete_elem_ti;

    update_maps(type, minimal_to, complete_to);
  }
}

void
typeobject_generator::generate_alias_type_identifier(AST_Type* type)
{
  AST_Typedef* const n = dynamic_cast<AST_Typedef*>(type);

  OpenDDS::XTypes::TypeObject minimal_to, complete_to;
  minimal_to.kind = OpenDDS::XTypes::EK_MINIMAL;
  minimal_to.minimal.kind = OpenDDS::XTypes::TK_ALIAS;
  minimal_to.minimal.alias_type.body.common.related_type = get_minimal_type_identifier(n->base_type());

  complete_to.kind = OpenDDS::XTypes::EK_COMPLETE;
  complete_to.complete.kind = OpenDDS::XTypes::TK_ALIAS;
  complete_to.complete.alias_type.header.detail.type_name = canonical_name(type->name());
  complete_to.complete.alias_type.body.common.related_type = get_complete_type_identifier(n->base_type());

  update_maps(type, minimal_to, complete_to);
}

void
typeobject_generator::generate_primitive_type_identifier(AST_Type* type)
{
  AST_PredefinedType* const n = dynamic_cast<AST_PredefinedType*>(type);
  switch (n->pt()) {
  case AST_PredefinedType::PT_long:
    fully_desc_type_identifier_map_[type] = OpenDDS::XTypes::TypeIdentifier(OpenDDS::XTypes::TK_INT32);
    break;
  case AST_PredefinedType::PT_ulong:
    fully_desc_type_identifier_map_[type] = OpenDDS::XTypes::TypeIdentifier(OpenDDS::XTypes::TK_UINT32);
    break;
  case AST_PredefinedType::PT_longlong:
    fully_desc_type_identifier_map_[type] = OpenDDS::XTypes::TypeIdentifier(OpenDDS::XTypes::TK_INT64);
    break;
  case AST_PredefinedType::PT_ulonglong:
    fully_desc_type_identifier_map_[type] = OpenDDS::XTypes::TypeIdentifier(OpenDDS::XTypes::TK_UINT64);
    break;
  case AST_PredefinedType::PT_short:
    fully_desc_type_identifier_map_[type] = OpenDDS::XTypes::TypeIdentifier(OpenDDS::XTypes::TK_INT16);
    break;
  case AST_PredefinedType::PT_ushort:
    fully_desc_type_identifier_map_[type] = OpenDDS::XTypes::TypeIdentifier(OpenDDS::XTypes::TK_UINT16);
    break;
#if OPENDDS_HAS_EXPLICIT_INTS
  case AST_PredefinedType::PT_int8:
    fully_desc_type_identifier_map_[type] = OpenDDS::XTypes::TypeIdentifier(OpenDDS::XTypes::TK_INT8);
    break;
  case AST_PredefinedType::PT_uint8:
    fully_desc_type_identifier_map_[type] = OpenDDS::XTypes::TypeIdentifier(OpenDDS::XTypes::TK_UINT8);
    break;
#endif
  case AST_PredefinedType::PT_float:
    fully_desc_type_identifier_map_[type] = OpenDDS::XTypes::TypeIdentifier(OpenDDS::XTypes::TK_FLOAT32);
    break;
  case AST_PredefinedType::PT_double:
    fully_desc_type_identifier_map_[type] = OpenDDS::XTypes::TypeIdentifier(OpenDDS::XTypes::TK_FLOAT64);
    break;
  case AST_PredefinedType::PT_longdouble:
    fully_desc_type_identifier_map_[type] = OpenDDS::XTypes::TypeIdentifier(OpenDDS::XTypes::TK_FLOAT128);
    break;
  case AST_PredefinedType::PT_char:
    fully_desc_type_identifier_map_[type] = OpenDDS::XTypes::TypeIdentifier(OpenDDS::XTypes::TK_CHAR8);
    break;
  case AST_PredefinedType::PT_wchar:
    fully_desc_type_identifier_map_[type] = OpenDDS::XTypes::TypeIdentifier(OpenDDS::XTypes::TK_CHAR16);
    break;
  case AST_PredefinedType::PT_boolean:
    fully_desc_type_identifier_map_[type] = OpenDDS::XTypes::TypeIdentifier(OpenDDS::XTypes::TK_BOOLEAN);
    break;
  case AST_PredefinedType::PT_octet:
    fully_desc_type_identifier_map_[type] = OpenDDS::XTypes::TypeIdentifier(OpenDDS::XTypes::TK_BYTE);
    break;
  case AST_PredefinedType::PT_any:
  case AST_PredefinedType::PT_object:
  case AST_PredefinedType::PT_value:
  case AST_PredefinedType::PT_abstract:
  case AST_PredefinedType::PT_void:
  case AST_PredefinedType::PT_pseudo:
    be_util::misc_error_and_abort("Unexpected primitive type");
  }
}

void
typeobject_generator::generate_type_identifier(AST_Type* type, bool force_type_object)
{
  // Generate both minimal and complete identifiers (and type objects) and cache.
  switch (type->node_type()) {

  case AST_ConcreteType::NT_union:
    {
      generate_union_type_identifier(type);
      break;
    }

  case AST_ConcreteType::NT_struct:
    {
      generate_struct_type_identifier(type);
      break;
    }

  case AST_ConcreteType::NT_enum:
    {
      generate_enum_type_identifier(type);
      break;
    }

  case AST_ConcreteType::NT_string:
    {
      AST_String* const n = dynamic_cast<AST_String*>(type);
      ACE_CDR::ULong bound = n->max_size()->ev()->u.ulval;
      if (bound < 256) {
        OpenDDS::XTypes::TypeIdentifier ti(OpenDDS::XTypes::TI_STRING8_SMALL);
        ti.string_sdefn().bound = bound;
        fully_desc_type_identifier_map_[type] = ti;
      } else {
        OpenDDS::XTypes::TypeIdentifier ti(OpenDDS::XTypes::TI_STRING8_LARGE);
        ti.string_ldefn().bound = bound;
        fully_desc_type_identifier_map_[type] = ti;
      }
      break;
    }

  case AST_ConcreteType::NT_wstring:
    {
      AST_String* const n = dynamic_cast<AST_String*>(type);
      ACE_CDR::ULong bound = n->max_size()->ev()->u.ulval;
      if (bound < 256) {
        OpenDDS::XTypes::TypeIdentifier ti(OpenDDS::XTypes::TI_STRING16_SMALL);
        ti.string_sdefn().bound = bound;
        fully_desc_type_identifier_map_[type] = ti;
      } else {
        OpenDDS::XTypes::TypeIdentifier ti(OpenDDS::XTypes::TI_STRING16_LARGE);
        ti.string_ldefn().bound = bound;
        fully_desc_type_identifier_map_[type] = ti;
      }
      break;
    }

  case AST_ConcreteType::NT_array:
    {
      generate_array_type_identifier(type, force_type_object);
      break;
    }

#if OPENDDS_HAS_IDL_MAP
  case AST_ConcreteType::NT_map:
    {
      generate_map_type_identifier(type, force_type_object);
      break;
    }
#endif

  case AST_ConcreteType::NT_sequence:
    {
      generate_sequence_type_identifier(type, force_type_object);
      break;
    }

  case AST_ConcreteType::NT_typedef:
    {
      generate_alias_type_identifier(type);
      break;
    }

  case AST_ConcreteType::NT_pre_defined:
    {
      generate_primitive_type_identifier(type);
      break;
    }

  case AST_ConcreteType::NT_fixed:
  case AST_ConcreteType::NT_interface:
  case AST_ConcreteType::NT_interface_fwd:
    {
      fully_desc_type_identifier_map_[type] = OpenDDS::XTypes::TypeIdentifier(OpenDDS::XTypes::TK_NONE);
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

  default:
    // Different versions of TAO_IDL_FE can have additional NT_* enumerators.
    // Ignoring them with a default: case silences a compiler warning.
    break;
  }
}

// Get minimal or fully descriptive type identifier
OpenDDS::XTypes::TypeIdentifier
typeobject_generator::get_minimal_type_identifier(AST_Type* type)
{
  switch(type->node_type()) {
  case AST_Decl::NT_union_fwd:
    {
      AST_UnionFwd* const td = dynamic_cast<AST_UnionFwd*>(type);
      return get_minimal_type_identifier(td->full_definition());
    }
  case AST_Decl::NT_struct_fwd:
    {
      AST_StructureFwd* const td = dynamic_cast<AST_StructureFwd*>(type);
      return get_minimal_type_identifier(td->full_definition());
    }
  default:
    break;
  }

  if (fully_desc_type_identifier_map_.count(type) != 0) {
    return fully_desc_type_identifier_map_[type];
  }

  HashTypeIdentifierMap::const_iterator pos = hash_type_identifier_map_.find(type);
  OPENDDS_ASSERT(pos != hash_type_identifier_map_.end());
  return pos->second.minimal;
}

// Get complete or fully descriptive type identifier
OpenDDS::XTypes::TypeIdentifier
typeobject_generator::get_complete_type_identifier(AST_Type* type)
{
  switch(type->node_type()) {
  case AST_Decl::NT_union_fwd:
    {
      AST_UnionFwd* const td = dynamic_cast<AST_UnionFwd*>(type);
      return get_complete_type_identifier(td->full_definition());
    }
  case AST_Decl::NT_struct_fwd:
    {
      AST_StructureFwd* const td = dynamic_cast<AST_StructureFwd*>(type);
      return get_complete_type_identifier(td->full_definition());
    }
  default:
    break;
  }

  if (fully_desc_type_identifier_map_.count(type) != 0) {
    return fully_desc_type_identifier_map_[type];
  }

  HashTypeIdentifierMap::const_iterator pos = hash_type_identifier_map_.find(type);
  OPENDDS_ASSERT(pos != hash_type_identifier_map_.end());
  return pos->second.complete;
}

bool
typeobject_generator::generate(AST_Type* node, UTL_ScopedName* name)
{
  strong_connect(node, "");

  if (!produce_output_) {
    return true;
  }

  NamespaceGuard ng;
  const string clazz = tag_type(name);

  be_global->header_ << "struct " << clazz << " {};\n";

  const std::string common = "  static XTypes::TypeIdentifier ti;\n"
    "  ACE_GUARD_RETURN(ACE_Thread_Mutex, guard, TheServiceParticipant->get_static_xtypes_lock(), ti);\n"
    "  if (ti.kind() == XTypes::TK_NONE) {\n";

  {
    const string decl = "getMinimalTypeIdentifier<" + clazz + ">";
    Function gti(decl.c_str(), "const XTypes::TypeIdentifier&", "");
    gti.endArgs();
    const OpenDDS::XTypes::TypeIdentifier ti = get_minimal_type_identifier(node);
    be_global->impl_ << common <<
      "    ti = " << ti << ";\n"
      "  }\n"
      "  return ti;\n";
  }

  declare_get_type_map();
  {
    const string decl = "getMinimalTypeMap<" + clazz + ">";
    Function gti(decl.c_str(), "const XTypes::TypeMap&", "");
    gti.endArgs();
    be_global->impl_ <<
      "  return OPENDDS_IDL_FILE_SPECIFIC(get_minimal_type_map, 0)();\n";
  }

  if (produce_xtypes_complete_) {
    {
      const string decl = "getCompleteTypeIdentifier<" + clazz + ">";
      Function gti(decl.c_str(), "const XTypes::TypeIdentifier&", "");
      gti.endArgs();

      if (produce_xtypes_complete_) {
        const OpenDDS::XTypes::TypeIdentifier ti = get_complete_type_identifier(node);
        be_global->impl_ << common <<
          "    ti = " << ti << ";\n"
          "  }\n"
          "  return ti;\n";
      } else {
        be_global->impl_ <<
          "  static XTypes::TypeIdentifier ti;\n"
          "  return ti;\n";
      }
    }

    {
      const string decl = "getCompleteTypeMap<" + clazz + ">";
      Function gti(decl.c_str(), "const XTypes::TypeMap&", "");
      gti.endArgs();
      be_global->impl_ <<
        "  return OPENDDS_IDL_FILE_SPECIFIC(get_complete_type_map, 0)();\n";
    }
  }

  return true;
}
