/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "marshal_generator.h"

#include "dds_generator.h"
#include "field_info.h"
#include "topic_keys.h"
#include "be_util.h"

#include <dds/DCPS/SafetyProfileStreams.h>
#include <dds/DCPS/Definitions.h>
#include <dds/DCPS/Util.h>
#include <dds/DCPS/Serializer.h>

#include <utl_identifier.h>

#include <string>
#include <sstream>
#include <iostream>
#include <cctype>
#include <map>

#define OPENDDS_IDL_STR(X) #X

using std::string;
using namespace AstTypeClassification;
using OpenDDS::DCPS::array_count;
using namespace OpenDDS::DCPS;

namespace {
  const string RtpsNamespace = " ::OpenDDS::RTPS::", DdsNamespace = " ::DDS::";

  typedef bool (*gen_special_case)(const string& cxx);
  typedef std::map<std::string, gen_special_case> SpecialCases;
  SpecialCases special_struct_cases;
  SpecialCases special_seq_cases;

  typedef bool (*gen_special_union)(const string& cxx,
                                    AST_Union* u,
                                    AST_Type* discriminator,
                                    const std::vector<AST_UnionBranch*>& branches);
  typedef std::map<std::string, gen_special_union> SpecialUnionCases;
  SpecialUnionCases special_union_cases;

  bool genRtpsSpecialSequence(const string& cxx);
  bool genPropertySpecialSequence(const string& cxx);
  bool genRtpsSpecialStruct(const string& cxx);
  bool genRtpsParameter(const string& cxx,
                        AST_Union* u,
                        AST_Type* discriminator,
                        const std::vector<AST_UnionBranch*>& branches);
  bool genRtpsSubmessage(const string& cxx,
                         AST_Union* u,
                         AST_Type* discriminator,
                         const std::vector<AST_UnionBranch*>& branches);
  bool genProperty_t(const string& cxx);
  bool genBinaryProperty_t(const string& cxx);
  bool genPropertyQosPolicy(const string& cxx);
  bool genSecuritySubmessage(const string& cxx);

  void init_special_cases()
  {
    if (special_seq_cases.empty()) {
      special_seq_cases["ParameterList"] = genRtpsSpecialSequence;
      special_seq_cases["prop_seq"] = genPropertySpecialSequence;
    }
    if (special_struct_cases.empty()) {
      special_struct_cases["rtps_set"] = genRtpsSpecialStruct;
      special_struct_cases["Property_t"] = genProperty_t;
      special_struct_cases["BinaryProperty_t"] = genBinaryProperty_t;
      special_struct_cases["PropertyQosPolicy"] = genPropertyQosPolicy;
      special_struct_cases["SecuritySubmessage"] = genSecuritySubmessage;
    }
    if (special_union_cases.empty()) {
      special_union_cases["Parameter"] = genRtpsParameter;
      special_union_cases["Submessage"] = genRtpsSubmessage;
    }
  }

  bool generate_special_sequence(
    AST_Typedef* typedef_node, const std::string& cpp_name, bool& result)
  {
    std::string template_name;
    if (!be_global->special_serialization(typedef_node, template_name)) {
      return false;
    }

    init_special_cases();
    SpecialCases::iterator it = special_seq_cases.find(template_name);
    if (it == special_seq_cases.end()) {
      be_util::misc_error_and_abort(
        std::string("Invalid special case sequence template name \"") +
        template_name + "\"", typedef_node);
      result = false;
    } else {
      result = (it->second)(cpp_name);
    }
    return true;
  }

  bool generate_special_struct(AST_Structure* node, const std::string& cpp_name, bool& result)
  {
    std::string template_name;
    if (!be_global->special_serialization(node, template_name)) {
      return false;
    }

    init_special_cases();
    SpecialCases::iterator it = special_struct_cases.find(template_name);
    if (it == special_struct_cases.end()) {
      be_util::misc_error_and_abort(
        std::string("Invalid special case struct template name \"") +
        template_name + "\"", node);
      result = false;
    } else {
      result = (it->second)(cpp_name);
    }
    return true;
  }

  bool generate_special_union(const std::string& cpp_name, AST_Union* node,
    AST_Type* disc_type, const std::vector<AST_UnionBranch*>& branches, bool& result)
  {
    std::string template_name;
    if (!be_global->special_serialization(node, template_name)) {
      return false;
    }

    init_special_cases();
    SpecialUnionCases::iterator it = special_union_cases.find(template_name);
    if (it == special_union_cases.end()) {
      be_util::misc_error_and_abort(
        std::string("Invalid special case struct template name \"") +
        template_name + "\"", node);
      result = false;
    } else {
      result = (it->second)(cpp_name, node, disc_type, branches);
    }
    return true;
  }

  string streamCommon(const std::string& indent, AST_Decl* node, const string& name,
                      AST_Type* type, const string& prefix, bool wrap_nested_key_only,
                      Intro& intro, const string& stru = "");

  const std::string construct_bound_fail =
    "strm.get_construction_status() == Serializer::BoundConstructionFailure";
  const std::string construct_elem_fail =
    "strm.get_construction_status() == Serializer::ElementConstructionFailure";
} /* namespace */

bool marshal_generator::gen_enum(AST_Enum*, UTL_ScopedName* name,
  const std::vector<AST_EnumVal*>&, const char*)
{
  NamespaceGuard ng;
  be_global->add_include("dds/DCPS/Serializer.h");
  const string cxx = scoped(name), // name as a C++ class
    underscores = scoped_helper(name, "_");
  {
    Function insertion("operator<<", "bool");
    insertion.addArg("strm", "Serializer&");
    insertion.addArg("enumval", "const " + cxx + "&");
    insertion.endArgs();
    be_global->impl_ <<
      "  return strm << static_cast<ACE_CDR::Long>(enumval);\n";
  }
  {
    Function extraction("operator>>", "bool");
    extraction.addArg("strm", "Serializer&");
    extraction.addArg("enumval", cxx + "&");
    extraction.endArgs();
    be_global->impl_ <<
      "  ACE_CDR::Long temp = 0;\n"
      "  if (strm >> temp) {\n"
      "    if (! ::OpenDDS::DCPS::gen_" << underscores << "_helper->valid(temp)) {\n"
      "      strm.set_construction_status(Serializer::ElementConstructionFailure);\n"
      "      return false;\n"
      "    }\n"
      "    enumval = static_cast<" << cxx << ">(temp);\n"
      "    return true;\n"
      "  }\n"
      "  return false;\n";
  }
  return true;
}

namespace {
  string getSizeExprPrimitive(AST_Type* type,
    const string& count_expr = "count", const string& size_expr = "size",
    const string& encoding_expr = "encoding")
  {
    if (type->node_type() != AST_Decl::NT_pre_defined) {
      return "";
    }
    AST_PredefinedType* pt = dynamic_cast<AST_PredefinedType*>(type);
    const string first_args = encoding_expr + ", " + size_expr;
    switch (pt->pt()) {
    case AST_PredefinedType::PT_octet:
      return "primitive_serialized_size_octet(" + first_args + ", " + count_expr + ")";
    case AST_PredefinedType::PT_char:
      return "primitive_serialized_size_char(" + first_args + ", " + count_expr + ")";
    case AST_PredefinedType::PT_wchar:
      return "primitive_serialized_size_wchar(" + first_args + ", " + count_expr + ")";
    case AST_PredefinedType::PT_boolean:
      return "primitive_serialized_size_boolean(" + first_args + ", " + count_expr + ")";
#if OPENDDS_HAS_EXPLICIT_INTS
    case AST_PredefinedType::PT_uint8:
      return "primitive_serialized_size_uint8(" + first_args + ", " + count_expr + ")";
    case AST_PredefinedType::PT_int8:
      return "primitive_serialized_size_int8(" + first_args + ", " + count_expr + ")";
#endif
    default:
      return "primitive_serialized_size(" + first_args + ", " +
        scoped(type->name()) + "(), " + count_expr + ")";
    }
  }

  string getSerializerName(AST_Type* type)
  {
    switch (dynamic_cast<AST_PredefinedType*>(type)->pt()) {
    case AST_PredefinedType::PT_long:
      return "long";
    case AST_PredefinedType::PT_ulong:
      return "ulong";
    case AST_PredefinedType::PT_short:
      return "short";
    case AST_PredefinedType::PT_ushort:
      return "ushort";
#if OPENDDS_HAS_EXPLICIT_INTS
    case AST_PredefinedType::PT_int8:
      return "int8";
    case AST_PredefinedType::PT_uint8:
      return "uint8";
#endif
    case AST_PredefinedType::PT_octet:
      return "octet";
    case AST_PredefinedType::PT_char:
      return "char";
    case AST_PredefinedType::PT_wchar:
      return "wchar";
    case AST_PredefinedType::PT_float:
      return "float";
    case AST_PredefinedType::PT_double:
      return "double";
    case AST_PredefinedType::PT_longlong:
      return "longlong";
    case AST_PredefinedType::PT_ulonglong:
      return "ulonglong";
    case AST_PredefinedType::PT_longdouble:
      return "longdouble";
    case AST_PredefinedType::PT_boolean:
      return "boolean";
    default:
      return "";
    }
  }

  string nameOfSeqHeader(AST_Type* elem)
  {
    string ser = getSerializerName(elem);
    if (ser.size()) {
      ser[0] = static_cast<char>(std::toupper(ser[0]));
    }
    if (ser[0] == 'U' || ser[0] == 'W') {
      ser[1] = static_cast<char>(std::toupper(ser[1]));
    }
    const size_t fourthLast = ser.size() - 4;
    if (ser.size() > 7 && ser.substr(fourthLast) == "long") {
      ser[fourthLast] = static_cast<char>(std::toupper(ser[fourthLast]));
    }
    if (ser == "Longdouble") return "LongDouble";
    return ser;
  }

  string streamAndCheck(const string& expr, size_t indent = 2)
  {
    string idt(indent, ' ');
    return idt + "if (!(strm " + expr + ")) {\n" +
      idt + "  return false;\n" +
      idt + "}\n";
  }

  string checkAlignment(AST_Type* elem)
  {
    // At this point the stream must be 4-byte aligned (from the sequence
    // length), but it might need to be 8-byte aligned for primitives > 4.
    // (If XCDR version is < 2)
    switch (dynamic_cast<AST_PredefinedType*>(elem)->pt()) {
    case AST_PredefinedType::PT_longlong:
    case AST_PredefinedType::PT_ulonglong:
    case AST_PredefinedType::PT_double:
    case AST_PredefinedType::PT_longdouble:
      return "  encoding.align(size, 8);\n";
    default:
      return "";
    }
  }

  bool genRtpsSpecialSequence(const string& cxx)
  {
    {
      Function serialized_size("serialized_size", "void");
      serialized_size.addArg("encoding", "const Encoding&");
      serialized_size.addArg("size", "size_t&");
      serialized_size.addArg("seq", "const " + cxx + "&");
      serialized_size.endArgs();
      be_global->impl_ <<
        "  for (CORBA::ULong i = 0; i < seq.length(); ++i) {\n"
        "    if (seq[i]._d() == OpenDDS::RTPS::PID_SENTINEL) continue;\n"
        "    serialized_size(encoding, size, seq[i]);\n"
        "    align(size, 4);\n"
        "  }\n"
        "  size += 4; /* PID_SENTINEL */\n";
    }
    {
      Function insertion("operator<<", "bool");
      insertion.addArg("strm", "Serializer&");
      insertion.addArg("seq", "const " + cxx + "&");
      insertion.endArgs();
      be_global->impl_ <<
        "  for (CORBA::ULong i = 0; i < seq.length(); ++i) {\n"
        "    if (seq[i]._d() == OpenDDS::RTPS::PID_SENTINEL) continue;\n"
        "    if (!(strm << seq[i])) {\n"
        "      return false;\n"
        "    }\n"
        "  }\n"
        "  return (strm << OpenDDS::RTPS::PID_SENTINEL)\n"
        "    && (strm << OpenDDS::RTPS::PID_PAD);\n";
    }
    {
      Function extraction("operator>>", "bool");
      extraction.addArg("strm", "Serializer&");
      extraction.addArg("seq", cxx + "&");
      extraction.endArgs();
      be_global->impl_ <<
        "  while (true) {\n"
        "    const CORBA::ULong idx = OpenDDS::DCPS::grow(seq) - 1;\n"
        "    if (!(strm >> seq[idx])) {\n"
        "      return false;\n"
        "    }\n"
        "    if (seq[idx]._d() == OpenDDS::RTPS::PID_SENTINEL) {\n"
        "      seq.length(idx);\n"
        "      return true;\n"
        "    }\n"
        "  }\n";
    }
    return true;
  }

  bool genPropertySpecialSequence(const string& cxx)
  {
    {
      Function serialized_size("serialized_size", "void");
      serialized_size.addArg("encoding", "const Encoding&");
      serialized_size.addArg("size", "size_t&");
      serialized_size.addArg("seq", "const " + cxx + "&");
      serialized_size.endArgs();
      be_global->impl_ <<
        "  primitive_serialized_size_ulong(encoding, size);\n"
        "  for (CORBA::ULong i = 0; i < seq.length(); ++i) {\n"
        "    serialized_size(encoding, size, seq[i]);\n"
        "  }\n";
    }
    {
      Function insertion("operator<<", "bool");
      insertion.addArg("strm", "Serializer&");
      insertion.addArg("seq", "const " + cxx + "&");
      insertion.endArgs();
      be_global->impl_ <<
        "  CORBA::ULong serlen = 0;\n"
        "  for (CORBA::ULong i = 0; i < seq.length(); ++i) {\n"
        "    if (seq[i].propagate) {\n"
        "      ++serlen;\n"
        "    }\n"
        "  }\n"
        "  if (!(strm << serlen)) {\n"
        "    return false;\n"
        "  }\n"
        "  for (CORBA::ULong i = 0; i < seq.length(); ++i) {\n"
        "    if (!(strm << seq[i])) {\n"
        "      return false;\n"
        "    }\n"
        "  }\n"
        "  return true;\n";
    }
    {
      Function extraction("operator>>", "bool");
      extraction.addArg("strm", "Serializer&");
      extraction.addArg("seq", cxx + "&");
      extraction.endArgs();
      be_global->impl_ <<
        "  CORBA::ULong length;\n"
        "  if (!(strm >> length)) {\n"
        "    return false;\n"
        "  }\n"
        "  if (length > strm.length()) {\n"
        "    return false;\n"
        "  }\n"
        "  seq.length(length);\n"
        "  for (CORBA::ULong i = 0; i < length; ++i) {\n"
        "    if (!(strm >> seq[i])) {\n"
        "      return false;\n"
        "    }\n"
        "  }\n"
        "  return true;\n";
    }
    return true;
  }

  void skip_to_end_sequence(const std::string indent,
    std::string start, std::string end, std::string seq_type_name,
    bool use_cxx11, Classification cls, AST_Sequence* seq)
  {
    std::string elem_type_name = seq_type_name + "::value_type";

    if (cls & CL_STRING) {
      if (cls & CL_WIDE) {
        elem_type_name = use_cxx11 ? "std::wstring" : "CORBA::WString_var";
      } else {
        elem_type_name = use_cxx11 ? "std::string" : "CORBA::String_var";
      }
    }

    std::string tempvar = "tempvar";
    be_global->impl_ <<
      indent << "if (encoding.xcdr_version() == Encoding::XCDR_VERSION_2) {\n" <<
      indent << "  strm.skip(end_of_seq - strm.rpos());\n" <<
      indent << "} else {\n";

    const bool classic_array_copy = !use_cxx11 && (cls & CL_ARRAY);

    if (!classic_array_copy) {
      be_global->impl_ <<
        indent << "  " << elem_type_name << " " << tempvar << ";\n";
    }

    std::string stream_to = tempvar;
    if (cls & CL_STRING) {
      if (cls & CL_BOUNDED) {
        AST_Type* elem = resolveActualType(seq->base_type());
        const string args = stream_to + (use_cxx11 ? ", " : ".out(), ") + bounded_arg(elem);
        stream_to = getWrapper(args, elem, WD_INPUT);
      }
    } else {
      Intro intro;
      RefWrapper wrapper(seq->base_type(), scoped(deepest_named_type(seq->base_type())->name()),
        classic_array_copy ? tempvar : stream_to, false);
      wrapper.classic_array_copy_ = classic_array_copy;
      wrapper.done(&intro);
      stream_to = wrapper.ref();
      intro.join(be_global->impl_, indent + "    ");
    }

    be_global->impl_ <<
      indent << "  for (CORBA::ULong j = " << start << " + 1; j < " << end << "; ++j) {\n" <<
      indent << "    strm >> " << stream_to << ";\n" <<
      indent << "  }\n" <<
      indent << "}\n";
  }

  void skip_to_end_array(const std::string& indent)
  {
    be_global->impl_ <<
      indent << "if (encoding.xcdr_version() == Encoding::XCDR_VERSION_2) {\n" <<
      indent << "  strm.set_construction_status(Serializer::ElementConstructionFailure);\n" <<
      indent << "  strm.skip(end_of_arr - strm.rpos());\n" <<
      indent << "  return false;\n" <<
      indent << "} else {\n" <<
      indent << "  strm.set_construction_status(Serializer::ConstructionSuccessful);\n" <<
      indent << "  discard_flag = true;\n" <<
      indent << "}\n";
  }

  void gen_sequence_i(
    UTL_ScopedName* tdname, AST_Sequence* seq, bool nested_key_only, AST_Typedef* typedef_node = 0,
    const FieldInfo* anonymous = 0)
  {
    be_global->add_include("dds/DCPS/Util.h");
    be_global->add_include("dds/DCPS/Serializer.h");
    if (anonymous) {
      seq = dynamic_cast<AST_Sequence*>(anonymous->type_);
    }
    const std::string named_as = anonymous ? anonymous->scoped_type_ : scoped(tdname);
    RefWrapper base_wrapper(seq, named_as, "seq");
    base_wrapper.typedef_node_ = typedef_node;
    base_wrapper.nested_key_only_ = nested_key_only;

    NamespaceGuard ng(!anonymous);

    if (!anonymous) {
      bool special_result;
      if (generate_special_sequence(typedef_node, base_wrapper.type_name_, special_result)) {
        return;
      }
    }

    AST_Type* elem = resolveActualType(seq->base_type());
    TryConstructFailAction try_construct = be_global->sequence_element_try_construct(seq);

    const Classification elem_cls = classify(elem);
    const bool primitive = elem_cls & CL_PRIMITIVE;
    if (!elem->in_main_file()) {
      if (elem->node_type() == AST_Decl::NT_pre_defined) {
        if (be_global->language_mapping() != BE_GlobalData::LANGMAP_FACE_CXX &&
            be_global->language_mapping() != BE_GlobalData::LANGMAP_SP_CXX) {
          const std::string hdr = "dds/CorbaSeq/" + nameOfSeqHeader(elem) + "SeqTypeSupportImpl.h";
          be_global->conditional_include(hdr.c_str(), BE_GlobalData::STREAM_CPP,
                                         "#ifndef OPENDDS_SAFETY_PROFILE");
        }
      } else {
        be_global->add_referenced(elem->file_name().c_str());
      }
    }

    const std::string cxx_elem =
      anonymous ? anonymous->scoped_elem_ : scoped(deepest_named_type(seq->base_type())->name());
    const bool use_cxx11 = be_global->language_mapping() == BE_GlobalData::LANGMAP_CXX11;

    RefWrapper(base_wrapper).done().generate_tag();

    {
      Intro intro;
      RefWrapper wrapper(base_wrapper);
      wrapper.done(&intro);
      const std::string value_access = wrapper.value_access();
      const std::string get_length = wrapper.seq_get_length();
      const std::string check_empty = wrapper.seq_check_empty();
      Function serialized_size("serialized_size", "void");
      serialized_size.addArg("encoding", "const Encoding&");
      serialized_size.addArg("size", "size_t&");
      serialized_size.addArg("seq", wrapper.wrapped_type_name());
      serialized_size.endArgs();

      if ((elem_cls & CL_INTERFACE) == 0) {
        marshal_generator::generate_dheader_code("    serialized_size_delimiter(encoding, size);\n", !primitive, false);

        intro.join(be_global->impl_, "  ");

        be_global->impl_ <<
          "  primitive_serialized_size_ulong(encoding, size);\n"
          "  if (" << check_empty << ") {\n"
          "    return;\n"
          "  }\n";
      }

      if (elem_cls & CL_ENUM) {
        be_global->impl_ <<
          "  primitive_serialized_size_ulong(encoding, size, " + get_length + ");\n";
      } else if (elem_cls & CL_PRIMITIVE) {
        be_global->impl_ << checkAlignment(elem) <<
          "  " + getSizeExprPrimitive(elem, get_length) << ";\n";
      } else if (elem_cls & CL_INTERFACE) {
        be_global->impl_ <<
          "  // sequence of objrefs is not marshaled\n";
      } else if (elem_cls == CL_UNKNOWN) {
        be_global->impl_ <<
          "  // sequence of unknown/unsupported type\n";
      } else { // String, Struct, Array, Sequence, Union
        be_global->impl_ <<
          "  for (CORBA::ULong i = 0; i < " << get_length << "; ++i) {\n";
        if (elem_cls & CL_STRING) {
          be_global->impl_ <<
            "    primitive_serialized_size_ulong(encoding, size);\n";
          const string strlen_suffix = (elem_cls & CL_WIDE)
            ? " * char16_cdr_size;\n" : " + 1;\n";
          if (use_cxx11) {
            be_global->impl_ <<
              "    size += " + value_access + "[i].size()" << strlen_suffix;
          } else {
            be_global->impl_ <<
              "    if (" + value_access + "[i]) {\n"
              "      size += ACE_OS::strlen(" + value_access + "[i])" << strlen_suffix <<
              "    }\n";
          }
        } else {
          RefWrapper elem_wrapper(elem, cxx_elem, value_access + "[i]");
          elem_wrapper.nested_key_only_ = nested_key_only;
          Intro intro;
          elem_wrapper.done(&intro);
          const std::string indent = "    ";
          intro.join(be_global->impl_, indent);
          be_global->impl_ <<
            indent << "serialized_size(encoding, size, " << elem_wrapper.ref() << ");\n";
        }
        be_global->impl_ <<
          "  }\n";
      }
    }

    {
      Intro intro;
      RefWrapper wrapper(base_wrapper);
      wrapper.done(&intro);
      const std::string value_access = wrapper.value_access();
      const std::string get_length = wrapper.seq_get_length();
      const std::string check_empty = wrapper.seq_check_empty();
      const std::string get_buffer = wrapper.seq_get_buffer();
      Function insertion("operator<<", "bool");
      insertion.addArg("strm", "Serializer&");
      insertion.addArg("seq", wrapper.wrapped_type_name());
      insertion.endArgs();

      if ((elem_cls & CL_INTERFACE) == 0) {
        be_global->impl_ <<
          "  const Encoding& encoding = strm.encoding();\n"
          "  ACE_UNUSED_ARG(encoding);\n";
        marshal_generator::generate_dheader_code(
          "    serialized_size(encoding, total_size, seq);\n"
          "    if (!strm.write_delimiter(total_size)) {\n"
          "      return false;\n"
          "    }\n", !primitive);

        intro.join(be_global->impl_, "  ");

        be_global->impl_ <<
          "  const CORBA::ULong length = " << get_length << ";\n";
        if (!seq->unbounded()) {
          be_global->impl_ <<
            "  if (length > " << bounded_arg(seq) << ") {\n"
            "    return false;\n"
            "  }\n";
        }
        be_global->impl_ <<
          streamAndCheck("<< length") <<
          "  if (length == 0) {\n"
          "    return true;\n"
          "  }\n";
      }

      if (elem_cls & CL_PRIMITIVE) {
        AST_PredefinedType* predef = dynamic_cast<AST_PredefinedType*>(elem);
        if (use_cxx11 && predef->pt() == AST_PredefinedType::PT_boolean) {
          be_global->impl_ <<
            "  for (CORBA::ULong i = 0; i < length; ++i) {\n" <<
            streamAndCheck("<< ACE_OutputCDR::from_boolean(" + value_access + "[i])", 4) <<
            "  }\n"
            "  return true;\n";
        } else {
          be_global->impl_ <<
            "  return strm.write_" << getSerializerName(elem)
            << "_array(" << get_buffer << ", length);\n";
        }
      } else if (elem_cls & CL_INTERFACE) {
        be_global->impl_ <<
          "  return false; // sequence of objrefs is not marshaled\n";
      } else if (elem_cls == CL_UNKNOWN) {
        be_global->impl_ <<
          "  return false; // sequence of unknown/unsupported type\n";
      } else { // Enum, String, Struct, Array, Sequence, Union
        be_global->impl_ <<
          "  for (CORBA::ULong i = 0; i < length; ++i) {\n";
        if ((elem_cls & (CL_STRING | CL_BOUNDED)) == (CL_STRING | CL_BOUNDED)) {
          const string args = value_access + "[i], " + bounded_arg(elem);
          be_global->impl_ <<
            streamAndCheck("<< " + getWrapper(args, elem, WD_OUTPUT), 4);
        } else {
          RefWrapper elem_wrapper(elem, cxx_elem, value_access + "[i]");
          elem_wrapper.nested_key_only_ = nested_key_only;
          Intro intro;
          elem_wrapper.done(&intro);
          intro.join(be_global->impl_, "    ");
          be_global->impl_ << streamAndCheck("<< " + elem_wrapper.ref(), 4);
        }
        be_global->impl_ <<
          "  }\n"
          "  return true;\n";
      }
    }

    {
      Intro intro;
      RefWrapper wrapper(base_wrapper);
      wrapper.is_const_ = false;
      wrapper.done(&intro);
      const std::string value_access = wrapper.value_access();
      const std::string get_length = wrapper.seq_get_length();
      const std::string check_empty = wrapper.seq_check_empty();
      const std::string get_buffer = wrapper.seq_get_buffer();
      Function extraction("operator>>", "bool");
      extraction.addArg("strm", "Serializer&");
      extraction.addArg("seq", wrapper.wrapped_type_name());
      extraction.endArgs();

      if ((elem_cls & CL_INTERFACE) == 0) {
        be_global->impl_ <<
          "  const Encoding& encoding = strm.encoding();\n"
          "  ACE_UNUSED_ARG(encoding);\n";
        marshal_generator::generate_dheader_code(
          "    if (!strm.read_delimiter(total_size)) {\n"
          "      return false;\n"
          "    }\n", !primitive);

        if (!primitive) {
          be_global->impl_ << "  const size_t end_of_seq = strm.rpos() + total_size;\n";
        }
        be_global->impl_ <<
          "  CORBA::ULong length;\n"
          << streamAndCheck(">> length");
        // The check here is to prevent very large sequences from being allocated.
        be_global->impl_ <<
          "  if (length > strm.length()) {\n"
          "    if (DCPS_debug_level >= 8) {\n"
          "      ACE_DEBUG((LM_DEBUG, ACE_TEXT(\"(%P|%t) Invalid sequence length (%u)\\n\"), length));\n"
          "    }\n"
          "    return false;\n"
          "  }\n";
      }

      AST_PredefinedType* predef = dynamic_cast<AST_PredefinedType*>(elem);
      string bound;
      if (!seq->unbounded()) {
        bound = use_cxx11 ? bounded_arg(seq) : value_access + ".maximum()";
      }
      //create a variable called newlength which tells us how long we need to copy to
      //for an unbounded sequence this is just our length
      //for a bounded sequence this is our maximum
      //we save the old length so we know how far we need to read until
      if ((elem_cls & CL_INTERFACE) == 0) {
        be_global->impl_ << "  CORBA::ULong new_length = length;\n";
      }

      if (elem_cls & CL_PRIMITIVE) {
        // if we are a bounded primitive, we read to our max then return false
        if (!seq->unbounded()) {
          be_global->impl_ <<
            "  if (length > " << bound << ") {\n"
            "    new_length = " << bound << ";\n"
            "  }\n"
            "  " << wrapper.seq_resize("new_length");
          if (use_cxx11 && predef->pt() == AST_PredefinedType::PT_boolean) {
            be_global->impl_ <<
            "  for (CORBA::ULong i = 0; i < new_length; ++i) {\n"
            "    bool b;\n" <<
            streamAndCheck(">> ACE_InputCDR::to_boolean(b)", 4) <<
            "    " + value_access + "[i] = b;\n"
            "  }\n";
          } else {
            be_global->impl_ <<
              "  strm.read_" << getSerializerName(elem) << "_array(" << get_buffer << ", new_length);\n";
          }
          be_global->impl_ <<
            "  if (new_length != length) {\n"
            "    size_t skip_length = 0;\n"
            "    " << getSizeExprPrimitive(elem, "(length - new_length)", "skip_length", "encoding") << ";\n"
            "    strm.set_construction_status(Serializer::BoundConstructionFailure);\n"
            "    strm.skip(skip_length);\n"
            "    return false;\n"
            "  }\n";
        } else {
          be_global->impl_ <<
            "  " << wrapper.seq_resize("new_length");
          if (use_cxx11 && predef->pt() == AST_PredefinedType::PT_boolean) {
            be_global->impl_ <<
              "  for (CORBA::ULong i = 0; i < length; ++i) {\n"
              "    bool b;\n" <<
              streamAndCheck(">> ACE_InputCDR::to_boolean(b)", 4) <<
              "    " << value_access << "[i] = b;\n"
              "  }\n"
              "  return true;\n";
          } else {
            be_global->impl_ <<
              "  if (length == 0) {\n"
              "    return true;\n"
              "  }\n"
              "  return strm.read_" << getSerializerName(elem)
              << "_array(" << get_buffer << ", length);\n";
          }
        }
      } else if (elem_cls & CL_INTERFACE) {
        be_global->impl_ <<
          "  return false; // sequence of objrefs is not marshaled\n";
        return;
      } else if (elem_cls == CL_UNKNOWN) {
        be_global->impl_ <<
          "  return false; // sequence of unknown/unsupported type\n";
        return;
      } else { // Enum, String, Struct, Array, Sequence, Union
        const std::string elem_access = value_access + "[i]";
        if (!seq->unbounded()) {
          be_global->impl_ <<
            "  if (length > " << (use_cxx11 ? bounded_arg(seq) : value_access + ".maximum()") << ") {\n"
            "    new_length = " << bound << ";\n"
            "  }\n";
        }
        //change the size of seq length to prepare
        be_global->impl_ <<
          "  " << wrapper.seq_resize("new_length");
        //read the entire length of the writer's sequence
        be_global->impl_ <<
          "  for (CORBA::ULong i = 0; i < new_length; ++i) {\n";

        Intro intro;
        std::string stream_to;
        std::string classic_array_copy;
        if (!use_cxx11 && (elem_cls & CL_ARRAY)) {
          RefWrapper classic_array_wrapper(
            seq->base_type(), scoped(deepest_named_type(seq->base_type())->name()), elem_access);
          classic_array_wrapper.classic_array_copy_ = true;
          classic_array_wrapper.done(&intro);
          classic_array_copy = classic_array_wrapper.classic_array_copy();
          stream_to = classic_array_wrapper.ref();
        } else if (elem_cls & CL_STRING) {
          if (elem_cls & CL_BOUNDED) {
            const string args = elem_access + (use_cxx11 ? ", " : ".out(), ") + bounded_arg(elem);
            stream_to = getWrapper(args, elem, WD_INPUT);
          } else {
            const string getbuffer =
              (be_global->language_mapping() == BE_GlobalData::LANGMAP_NONE)
              ? ".get_buffer()" : "";
            stream_to = value_access + getbuffer + "[i]";
          }
        } else {
          RefWrapper elem_wrapper(elem, cxx_elem, value_access + "[i]", false);
          elem_wrapper.nested_key_only_ = nested_key_only;
          elem_wrapper.done(&intro);
          stream_to = elem_wrapper.ref();
        }
        const std::string indent = "    ";
        intro.join(be_global->impl_, indent);
        be_global->impl_ <<
          indent << " if (!(strm >> " << stream_to << ")) {\n";

        if (try_construct == tryconstructfailaction_use_default) {
          be_global->impl_ <<
            type_to_default("        ", elem, elem_access) <<
            "        strm.set_construction_status(Serializer::ConstructionSuccessful);\n";
        } else if ((try_construct == tryconstructfailaction_trim) && (elem_cls & CL_BOUNDED) &&
                   (elem_cls & (CL_STRING | CL_SEQUENCE))) {
          if (elem_cls & CL_STRING){
            const std::string check_not_empty =
              use_cxx11 ? "!" + elem_access + ".empty()" : elem_access + ".in()";
            const std::string get_length =
              use_cxx11 ? elem_access + ".length()" : "ACE_OS::strlen(" + elem_access + ".in())";
            const string inout = use_cxx11 ? "" : ".inout()";
            be_global->impl_ <<
              "        if (" + construct_bound_fail + " && " << check_not_empty << " && (" <<
              bounded_arg(elem) << " < " << get_length << ")) {\n"
              "          "  << elem_access << inout <<
              (use_cxx11 ? (".resize(" + bounded_arg(elem) +  ");\n") : ("[" + bounded_arg(elem) + "] = 0;\n")) <<
              "          strm.set_construction_status(Serializer::ConstructionSuccessful);\n"
              "        } else {\n"
              "          strm.set_construction_status(Serializer::ElementConstructionFailure);\n";
            skip_to_end_sequence("          ", "i", "length", named_as, use_cxx11, elem_cls, seq);
            be_global->impl_ <<
              "        return false;\n"
              "      }\n";
          } else if (elem_cls & CL_SEQUENCE) {
            be_global->impl_ <<
              "      if (" + construct_elem_fail + ") {\n";
            skip_to_end_sequence("          ", "i", "length", named_as, use_cxx11, elem_cls, seq);
            be_global->impl_ <<
              "        return false;\n"
              "      }\n"
              "      strm.set_construction_status(Serializer::ConstructionSuccessful);\n";
          }
        } else {
          //discard/default
          be_global->impl_ <<
            "      strm.set_construction_status(Serializer::ElementConstructionFailure);\n";
          skip_to_end_sequence("      ", "i", "length", named_as, use_cxx11, elem_cls, seq);
          be_global->impl_ <<
            "      return false;\n";
        }
        be_global->impl_ <<
          "    }\n";
        if (classic_array_copy.size()) {
          be_global->impl_ <<
            "    " << classic_array_copy << "\n";
        }
        be_global->impl_ << "  }\n";
      }
      if (!primitive) {
        be_global->impl_ <<
          "  if (new_length != length) {\n";
        skip_to_end_sequence("    ", "new_length", "length", named_as, use_cxx11, elem_cls, seq);
        be_global->impl_ <<
          "    strm.set_construction_status(Serializer::BoundConstructionFailure);\n"
          "    return false;\n"
          "  }\n";
      }
      be_global->impl_ <<
        "  return true;\n";
    }
  }

  void gen_sequence(UTL_ScopedName* name, AST_Typedef* typedef_node, AST_Sequence* seq)
  {
    gen_sequence_i(name, seq, false, typedef_node);
    if (needs_nested_key_only(seq)) {
      gen_sequence_i(name, seq, true, typedef_node);
    }
  }

  void gen_anonymous_sequence(const FieldInfo& sf)
  {
    gen_sequence_i(0, 0, false, 0, &sf);
    if (needs_nested_key_only(sf.type_)) {
      gen_sequence_i(0, 0, true, 0, &sf);
    }
  }

  void gen_array_i(
    UTL_ScopedName* name, AST_Array* arr, bool nested_key_only, const FieldInfo* anonymous = 0)
  {
    be_global->add_include("dds/DCPS/Serializer.h");
    if (anonymous) {
      arr = dynamic_cast<AST_Array*>(anonymous->type_);
    }
    const std::string named_as = anonymous ? anonymous->scoped_type_ : scoped(name);
    RefWrapper base_wrapper(arr, named_as, "arr");
    base_wrapper.nested_key_only_ = nested_key_only;
    NamespaceGuard ng(!anonymous);

    const bool use_cxx11 = be_global->language_mapping() == BE_GlobalData::LANGMAP_CXX11;

    AST_Type* elem = resolveActualType(arr->base_type());
    TryConstructFailAction try_construct = be_global->array_element_try_construct(arr);
    const Classification elem_cls = classify(elem);
    const bool primitive = elem_cls & CL_PRIMITIVE;
    if (!elem->in_main_file()
        && elem->node_type() != AST_Decl::NT_pre_defined) {
      be_global->add_referenced(elem->file_name().c_str());
    }
    const std::string cxx_elem =
      anonymous ? anonymous->scoped_elem_ : scoped(deepest_named_type(arr->base_type())->name());
    const ACE_CDR::ULong n_elems = array_element_count(arr);

    RefWrapper(base_wrapper).done().generate_tag();

    if (!nested_key_only) {
      RefWrapper wrapper(base_wrapper);
      wrapper.is_const_ = false;
      wrapper.done();
      Function set_default("set_default", "void", "");
      set_default.addArg("arr", wrapper.wrapped_type_name());
      set_default.endArgs();
      string indent = "  ";
      NestedForLoops nfl("CORBA::ULong", "i", arr, indent);
      be_global->impl_ << type_to_default(indent, elem, wrapper.value_access() + nfl.index_);
    }

    {
      RefWrapper wrapper(base_wrapper);
      wrapper.done();

      Function serialized_size("serialized_size", "void");
      serialized_size.addArg("encoding", "const Encoding&");
      serialized_size.addArg("size", "size_t&");
      serialized_size.addArg("arr", wrapper.wrapped_type_name());
      serialized_size.endArgs();

      marshal_generator::generate_dheader_code("    serialized_size_delimiter(encoding, size);\n", !primitive, false);

      if (elem_cls & CL_ENUM) {
        be_global->impl_ <<
          "  primitive_serialized_size_ulong(encoding, size, " << n_elems << ");\n";
      } else if (elem_cls & CL_PRIMITIVE) {
        std::ostringstream n_elems_ss;
        n_elems_ss << n_elems;
        be_global->impl_ <<
          "  " << getSizeExprPrimitive(elem, n_elems_ss.str()) << ";\n";
      } else { // String, Struct, Array, Sequence, Union
        string indent = "  ";
        NestedForLoops nfl("CORBA::ULong", "i", arr, indent);
        if (elem_cls & CL_STRING) {
          be_global->impl_ <<
            indent << "primitive_serialized_size_ulong(encoding, size);\n" <<
            indent;
          if (use_cxx11) {
            be_global->impl_ << "size += " << wrapper.value_access() << nfl.index_ << ".size()";
          } else {
            be_global->impl_ << "size += ACE_OS::strlen(" << wrapper.value_access()
              << nfl.index_ << ".in())";
          }
          be_global->impl_ << ((elem_cls & CL_WIDE) ? " * char16_cdr_size;\n" : " + 1;\n");
        } else {
          RefWrapper elem_wrapper(elem, cxx_elem, wrapper.value_access() + nfl.index_);
          elem_wrapper.nested_key_only_ = nested_key_only;
          Intro intro;
          elem_wrapper.done(&intro);
          intro.join(be_global->impl_, indent);
          be_global->impl_ <<
            indent << "serialized_size(encoding, size, " << elem_wrapper.ref() << ");\n";
        }
      }
    }

    {
      RefWrapper wrapper(base_wrapper);
      wrapper.done();
      Function insertion("operator<<", "bool");
      insertion.addArg("strm", "Serializer&");
      insertion.addArg("arr", wrapper.wrapped_type_name());
      insertion.endArgs();

      be_global->impl_ <<
        "  const Encoding& encoding = strm.encoding();\n"
        "  ACE_UNUSED_ARG(encoding);\n";

      marshal_generator::generate_dheader_code(
        "serialized_size(encoding, total_size, arr);"
        "if (!strm.write_delimiter(total_size)) {"
        "  return false;"
        "}", !primitive);
      const std::string accessor = wrapper.value_access() + (use_cxx11 ? ".data()" : ".in()");
      if (elem_cls & CL_PRIMITIVE) {
        string suffix;
        for (unsigned int i = 1; i < arr->n_dims(); ++i)
          suffix += use_cxx11 ? "->data()" : "[0]";
        be_global->impl_ <<
          "  return strm.write_" << getSerializerName(elem)
          << "_array(" << accessor << suffix << ", " << n_elems << ");\n";
      } else { // Enum, String, Struct, Array, Sequence, Union
        {
          string indent = "  ";
          NestedForLoops nfl("CORBA::ULong", "i", arr, indent);
          RefWrapper elem_wrapper(elem, cxx_elem, wrapper.value_access() + nfl.index_);
          elem_wrapper.nested_key_only_ = nested_key_only;
          Intro intro;
          elem_wrapper.done(&intro);
          intro.join(be_global->impl_, indent);
          be_global->impl_ << streamAndCheck("<< " + elem_wrapper.ref(), indent.size());
        }
        be_global->impl_ << "  return true;\n";
      }
    }

    {
      RefWrapper wrapper(base_wrapper);
      wrapper.is_const_ = false;
      wrapper.done();
      Function extraction("operator>>", "bool");
      extraction.addArg("strm", "Serializer&");
      extraction.addArg("arr", wrapper.wrapped_type_name());
      extraction.endArgs();

      if (!primitive) {
        be_global->impl_ << "  bool discard_flag = false;\n";
      }
      be_global->impl_ <<
        "  const Encoding& encoding = strm.encoding();\n"
        "  ACE_UNUSED_ARG(encoding);\n";
      marshal_generator::generate_dheader_code(
        "    if (!strm.read_delimiter(total_size)) {\n"
        "      return false;\n"
        "    }\n", !primitive);

      if (!primitive && (try_construct != tryconstructfailaction_use_default)) {
        be_global->impl_ << "  const size_t end_of_arr = strm.rpos() + total_size;\n";
      }

      const std::string accessor = wrapper.value_access() + (use_cxx11 ? ".data()" : ".out()");
      if (primitive) {
        string suffix;
        for (unsigned int i = 1; i < arr->n_dims(); ++i)
          suffix += use_cxx11 ? "->data()" : "[0]";
        be_global->impl_ <<
          "  return strm.read_" << getSerializerName(elem)
          << "_array(" << accessor << suffix << ", " << n_elems << ");\n";
      } else { // Enum, String, Struct, Array, Sequence, Union
        {
          string indent = "  ";
          NestedForLoops nfl("CORBA::ULong", "i", arr, indent);
          const std::string elem_access = wrapper.value_access() + nfl.index_;

          Intro intro;
          std::string stream;
          std::string classic_array_copy;
          if (!use_cxx11 && (elem_cls & CL_ARRAY)) {
            RefWrapper classic_array_wrapper(
              arr->base_type(), scoped(deepest_named_type(arr->base_type())->name()),
              wrapper.value_access() + nfl.index_);
            classic_array_wrapper.classic_array_copy_ = true;
            classic_array_wrapper.done(&intro);
            classic_array_copy = classic_array_wrapper.classic_array_copy();
            stream = "(strm >> " + classic_array_wrapper.ref() + ")";
          } else {
            stream = streamCommon(
              indent, 0, "", arr->base_type(), ">> " + elem_access, nested_key_only, intro);
          }
          intro.join(be_global->impl_, indent);
          be_global->impl_ <<
            indent << "if (!" << stream << ") {\n";

          indent += "  ";
          if (try_construct == tryconstructfailaction_use_default) {
            be_global->impl_ <<
              type_to_default(indent, elem, elem_access) <<
              indent << "strm.set_construction_status(Serializer::ConstructionSuccessful);\n";
          } else if ((try_construct == tryconstructfailaction_trim) && (elem_cls & CL_BOUNDED) &&
                     (elem_cls & (CL_STRING | CL_SEQUENCE))) {
            if (elem_cls & CL_STRING) {
              const std::string check_not_empty =
                use_cxx11 ? "!" + elem_access + ".empty()" : elem_access + ".in()";
              const std::string get_length =
                use_cxx11 ? elem_access + ".length()" : "ACE_OS::strlen(" + elem_access + ".in())";
              const string inout = use_cxx11 ? "" : ".inout()";
              be_global->impl_ <<
                indent << "if (" << construct_bound_fail << " && " <<
                  check_not_empty << " && (" << bounded_arg(elem) << " < " << get_length << ")) {\n" <<
                indent << "  " << wrapper.value_access() + nfl.index_ << inout <<
                  (use_cxx11 ? (".resize(" + bounded_arg(elem) +  ")") : ("[" + bounded_arg(elem) + "] = 0")) << ";\n" <<
                indent << "  strm.set_construction_status(Serializer::ConstructionSuccessful);\n" <<
                indent << "} else {\n";
              skip_to_end_array(indent);
              be_global->impl_ <<
                indent << "}\n";
            } else if (elem_cls & CL_SEQUENCE) {
              be_global->impl_ <<
                indent << "if (" + construct_elem_fail + ") {\n";
              skip_to_end_array(indent);
              be_global->impl_ <<
                indent << "} else {\n" <<
                indent << "  strm.set_construction_status(Serializer::ConstructionSuccessful);\n" <<
                indent << "}\n";
            }
          } else {
            //discard/default
            skip_to_end_array(indent);
          }
          if (classic_array_copy.size()) {
            be_global->impl_ <<
              indent << "} else {\n" <<
              indent << "  " << classic_array_copy << "\n";
          }
          indent.erase(0, 2);
          be_global->impl_ <<
            indent << "}\n";
        }
        be_global->impl_ <<
          "  if (discard_flag) {\n"
          "    strm.set_construction_status(Serializer::ElementConstructionFailure);\n"
          "    return false;\n"
          "  }\n"
          "  return true;\n";
      }
    }
  }

  void gen_array(UTL_ScopedName* name, AST_Array* arr)
  {
    gen_array_i(name, arr, false);
    if (needs_nested_key_only(arr)) {
      gen_array_i(name, arr, true);
    }
  }

  void gen_anonymous_array(const FieldInfo& af)
  {
    gen_array_i(0, 0, false, &af);
    if (needs_nested_key_only(af.type_)) {
      gen_array_i(0, 0, true, &af);
    }
  }

  // This function looks through the fields of a struct for the key
  // specified and returns the AST_Type associated with that key.
  // Because the key name can contain indexed arrays and nested
  // structures, things can get interesting.
  AST_Type* find_type_i(AST_Structure* struct_node, const string& key)
  {
    string key_base = key;   // the field we are looking for here
    string key_rem;          // the sub-field we will look for recursively
    bool is_array = false;
    size_t pos = key.find_first_of(".[");
    if (pos != string::npos) {
      key_base = key.substr(0, pos);
      if (key[pos] == '[') {
        is_array = true;
        size_t l_brack = key.find("]");
        if (l_brack == string::npos) {
          throw string("Missing right bracket");
        } else if (l_brack != key.length()) {
          key_rem = key.substr(l_brack+1);
        }
      } else {
        key_rem = key.substr(pos+1);
      }
    }

    const Fields fields(struct_node);
    const Fields::Iterator fields_end = fields.end();
    for (Fields::Iterator i = fields.begin(); i != fields_end; ++i) {
      AST_Field* const field = *i;
      if (key_base == field->local_name()->get_string()) {
        AST_Type* field_type = resolveActualType(field->field_type());
        if (!is_array && key_rem.empty()) {
          // The requested key field matches this one.  We do not allow
          // arrays (must be indexed specifically) or structs (must
          // identify specific sub-fields).
          AST_Structure* sub_struct = dynamic_cast<AST_Structure*>(field_type);
          if (sub_struct != 0) {
            throw string("Structs not allowed as keys");
          }
          AST_Array* array_node = dynamic_cast<AST_Array*>(field_type);
          if (array_node != 0) {
            throw string("Arrays not allowed as keys");
          }
          return field_type;
        } else if (is_array) {
          // must be a typedef of an array
          AST_Array* array_node = dynamic_cast<AST_Array*>(field_type);
          if (array_node == 0) {
            throw string("Indexing for non-array type");
          }
          if (array_node->n_dims() > 1) {
            throw string("Only single dimension arrays allowed in keys");
          }
          if (key_rem == "") {
            return array_node->base_type();
          } else {
            // This must be a struct...
            if ((key_rem[0] != '.') || (key_rem.length() == 1)) {
              throw string("Unexpected characters after array index");
            } else {
              // Set up key_rem and field_type and let things fall into
              // the struct code below
              key_rem = key_rem.substr(1);
              field_type = array_node->base_type();
            }
          }
        }

        // nested structures
        AST_Structure* sub_struct = dynamic_cast<AST_Structure*>(field_type);
        if (sub_struct == 0) {
          throw string("Expected structure field for ") + key_base;
        }

        // find type of nested struct field
        return find_type_i(sub_struct, key_rem);
      }
    }
    throw string("Field not found.");
  }

  AST_Type* find_type(AST_Structure* struct_node, const string& key)
  {
    try {
      return find_type_i(struct_node, key);
    } catch (const string& error) {
      const std::string struct_name = scoped(struct_node->name());
      be_util::misc_error_and_abort(
        "Invalid key specification for " + struct_name + " (" + key + "): " + error,
        struct_node);
    }
    return 0;
  }

  bool is_bounded_type(AST_Type* type, Encoding::Kind encoding)
  {
    bool bounded = true;
    static std::vector<AST_Type*> type_stack;
    type = resolveActualType(type);
    for (size_t i = 0; i < type_stack.size(); ++i) {
      // If we encounter the same type recursively, then we are unbounded
      if (type == type_stack[i]) return false;
    }
    type_stack.push_back(type);
    const Classification fld_cls = classify(type);
    if ((fld_cls & CL_STRING) && !(fld_cls & CL_BOUNDED)) {
      bounded = false;
    } else if (fld_cls & CL_STRUCTURE) {
      const ExtensibilityKind exten = be_global->extensibility(type);
      if (exten != extensibilitykind_final && encoding != Encoding::KIND_UNALIGNED_CDR) {
        /*
         * TODO(iguessthislldo): This is a workaround for not properly
         * implementing serialized_size_bound for XCDR. See XTYPE-83.
         */
        bounded = false;
      } else {
        const Fields fields(dynamic_cast<AST_Structure*>(type));
        const Fields::Iterator fields_end = fields.end();
        for (Fields::Iterator i = fields.begin(); i != fields_end; ++i) {
          if (!is_bounded_type((*i)->field_type(), encoding)) {
            bounded = false;
            break;
          }
        }
      }
    } else if (fld_cls & CL_SEQUENCE) {
      if (fld_cls & CL_BOUNDED) {
        AST_Sequence* seq_node = dynamic_cast<AST_Sequence*>(type);
        if (!is_bounded_type(seq_node->base_type(), encoding)) bounded = false;
      } else {
        bounded = false;
      }
    } else if (fld_cls & CL_ARRAY) {
      AST_Array* array_node = dynamic_cast<AST_Array*>(type);
      if (!is_bounded_type(array_node->base_type(), encoding)) bounded = false;
    } else if (fld_cls & CL_UNION) {
      const ExtensibilityKind exten = be_global->extensibility(type);
      if (exten != extensibilitykind_final && encoding != Encoding::KIND_UNALIGNED_CDR) {
        /*
         * TODO(iguessthislldo): This is a workaround for not properly
         * implementing serialized_size_bound for XCDR. See XTYPE-83.
         */
        bounded = false;
      } else {
        const Fields fields(dynamic_cast<AST_Union*>(type));
        const Fields::Iterator fields_end = fields.end();
        for (Fields::Iterator i = fields.begin(); i != fields_end; ++i) {
          if (!is_bounded_type((*i)->field_type(), encoding)) {
            bounded = false;
            break;
          }
        }
      }
    }
    type_stack.pop_back();
    return bounded;
  }

  ExtensibilityKind max_extensibility_kind(AST_Type* type)
  {
    ExtensibilityKind exten = extensibilitykind_final;
    static std::vector<AST_Type*> type_stack;

    type = resolveActualType(type);
    for (size_t i = 0; i < type_stack.size(); ++i) {
      // If we encounter the same type recursively, then it has already been considered
      if (type == type_stack[i]) return extensibilitykind_final;
    }
    type_stack.push_back(type);

    const Classification fld_cls = classify(type);
    if (fld_cls & CL_STRUCTURE || fld_cls & CL_UNION) {
      exten = be_global->extensibility(type);
      if (exten == extensibilitykind_mutable) {
        type_stack.pop_back();
        return extensibilitykind_mutable;
      }

      const Fields fields(fld_cls & CL_STRUCTURE ? dynamic_cast<AST_Structure*>(type) : dynamic_cast<AST_Union*>(type));
      const Fields::Iterator fields_end = fields.end();
      ExtensibilityKind this_exten = extensibilitykind_final;
      for (Fields::Iterator i = fields.begin(); i != fields_end; ++i) {
        if (extensibilitykind_mutable == (this_exten = max_extensibility_kind(type))) {
          type_stack.pop_back();
          return extensibilitykind_mutable;
        }
        if (exten < this_exten) {
          exten = this_exten;
        }
      }
      type_stack.pop_back();
      return exten;
    } else if (fld_cls & CL_ARRAY) {
      type_stack.pop_back();
      return max_extensibility_kind(dynamic_cast<AST_Array*>(type)->base_type());
    } else if (fld_cls & CL_SEQUENCE) {
      type_stack.pop_back();
      return max_extensibility_kind(dynamic_cast<AST_Sequence*>(type)->base_type());
    } else {
      type_stack.pop_back();
      return extensibilitykind_final;
    }
  }

  /**
   * Convert a compiler Encoding value to the string name of the corresponding
   * OpenDDS::DCPS::Encoding::XcdrVersion.
   */
  std::string encoding_to_xcdr_version(Encoding::Kind encoding)
  {
    switch (encoding) {
    case Encoding::KIND_XCDR1:
      return "Encoding::XCDR_VERSION_1";
    case Encoding::KIND_XCDR2:
      return "Encoding::XCDR_VERSION_2";
    default:
      return "Encoding::XCDR_VERSION_NONE";
    }
  }

  /**
   * Convert a compiler Encoding value to the string name of the corresponding
   * OpenDDS::DCPS::Encoding::Kind.
   */
  std::string encoding_to_encoding_kind(Encoding::Kind encoding)
  {
    switch (encoding) {
    case Encoding::KIND_XCDR1:
      return "Encoding::KIND_XCDR1";
    case Encoding::KIND_XCDR2:
      return "Encoding::KIND_XCDR2";
    default:
      return "Encoding::KIND_UNALIGNED_CDR";
    }
  }

  void align(Encoding::Kind encoding, size_t& value, size_t by)
  {
    const Encoding enc(encoding);
    enc.align(value, by);
  }

  void idl_max_serialized_size_dheader(
    Encoding::Kind encoding, ExtensibilityKind exten, size_t& size)
  {
    if (exten != extensibilitykind_final && encoding == Encoding::KIND_XCDR2) {
      align(encoding, size, 4);
      size += 4;
    }
  }

  void idl_max_serialized_size(Encoding::Kind encoding, size_t& size, AST_Type* type);

  // Max marshaled size of repeating 'type' 'n' times in the stream
  // (for an array or sequence)
  void idl_max_serialized_size_repeating(
    Encoding::Kind encoding, size_t& size, AST_Type* type, size_t n)
  {
    if (n > 0) {
      // 1st element may need padding relative to whatever came before
      idl_max_serialized_size(encoding, size, type);
    }
    if (n > 1) {
      // subsequent elements may need padding relative to prior element
      // TODO(iguessthislldo): https://github.com/OpenDDS/OpenDDS/pull/1668#discussion_r432521888
      const size_t prev_size = size;
      idl_max_serialized_size(encoding, size, type);
      size += (n - 2) * (size - prev_size);
    }
  }

  /// Should only be called on bounded types
  void idl_max_serialized_size(Encoding::Kind encoding, size_t& size, AST_Type* type)
  {
    type = resolveActualType(type);
    const ExtensibilityKind exten = be_global->extensibility(type);
    switch (type->node_type()) {
    case AST_Decl::NT_pre_defined: {
      AST_PredefinedType* p = dynamic_cast<AST_PredefinedType*>(type);
      switch (p->pt()) {
      case AST_PredefinedType::PT_char:
      case AST_PredefinedType::PT_boolean:
      case AST_PredefinedType::PT_octet:
#if OPENDDS_HAS_EXPLICIT_INTS
      case AST_PredefinedType::PT_uint8:
      case AST_PredefinedType::PT_int8:
#endif
        size += 1;
        break;
      case AST_PredefinedType::PT_short:
      case AST_PredefinedType::PT_ushort:
        align(encoding, size, 2);
        size += 2;
        break;
      case AST_PredefinedType::PT_wchar:
        align(encoding, size, 2);
        size += 2;
        break;
      case AST_PredefinedType::PT_long:
      case AST_PredefinedType::PT_ulong:
      case AST_PredefinedType::PT_float:
        align(encoding, size, 4);
        size += 4;
        break;
      case AST_PredefinedType::PT_longlong:
      case AST_PredefinedType::PT_ulonglong:
      case AST_PredefinedType::PT_double:
        align(encoding, size, 8);
        size += 8;
        break;
      case AST_PredefinedType::PT_longdouble:
        align(encoding, size, 16);
        size += 16;
        break;
      default:
        // Anything else shouldn't be in a DDS type or is unbounded.
        break;
      }
      break;
    }
    case AST_Decl::NT_enum:
      align(encoding, size, 4);
      size += 4;
      break;
    case AST_Decl::NT_string:
    case AST_Decl::NT_wstring: {
      AST_String* string_node = dynamic_cast<AST_String*>(type);
      align(encoding, size, 4);
      size += 4;
      const int width = (string_node->width() == 1) ? 1 : 2 /*UTF-16*/;
      size += width * string_node->max_size()->ev()->u.ulval;
      if (type->node_type() == AST_Decl::NT_string) {
        size += 1; // narrow string includes the null terminator
      }
      break;
    }
    case AST_Decl::NT_struct: {
      const Fields fields(dynamic_cast<AST_Structure*>(type));
      const Fields::Iterator fields_end = fields.end();
      idl_max_serialized_size_dheader(encoding, exten, size);
      // TODO(iguessthislldo) Handle Parameter List
      for (Fields::Iterator i = fields.begin(); i != fields_end; ++i) {
        idl_max_serialized_size(encoding, size, (*i)->field_type());
      }
      break;
    }
    case AST_Decl::NT_sequence: {
      AST_Sequence* seq_node = dynamic_cast<AST_Sequence*>(type);
      AST_Type* base_node = seq_node->base_type();
      idl_max_serialized_size_dheader(encoding, exten, size);
      size_t bound = seq_node->max_size()->ev()->u.ulval;
      align(encoding, size, 4);
      size += 4;
      idl_max_serialized_size_repeating(encoding, size, base_node, bound);
      break;
    }
    case AST_Decl::NT_array: {
      AST_Array* array_node = dynamic_cast<AST_Array*>(type);
      AST_Type* base_node = array_node->base_type();
      idl_max_serialized_size_dheader(encoding, exten, size);
      idl_max_serialized_size_repeating(
        encoding, size, base_node, array_element_count(array_node));
      break;
    }
    case AST_Decl::NT_union: {
      AST_Union* union_node = dynamic_cast<AST_Union*>(type);
      idl_max_serialized_size_dheader(encoding, exten, size);
      // TODO(iguessthislldo) Handle Parameter List
      idl_max_serialized_size(encoding, size, union_node->disc_type());
      size_t largest_field_size = 0;
      const size_t starting_size = size;
      const Fields fields(union_node);
      const Fields::Iterator fields_end = fields.end();
      for (Fields::Iterator i = fields.begin(); i != fields_end; ++i) {
        idl_max_serialized_size(encoding, size, (*i)->field_type());
        size_t field_size = size - starting_size;
        if (field_size > largest_field_size) {
          largest_field_size = field_size;
        }
        // rewind:
        size = starting_size;
      }
      size += largest_field_size;
      break;
    }
    default:
      // Anything else should be not here or is unbounded
      break;
    }
  }
}

bool marshal_generator::gen_typedef(AST_Typedef* node, UTL_ScopedName* name, AST_Type* base, const char*)
{
  switch (base->node_type()) {
  case AST_Decl::NT_sequence:
    gen_sequence(name, node, dynamic_cast<AST_Sequence*>(base));
    break;
  case AST_Decl::NT_array:
    gen_array(name, dynamic_cast<AST_Array*>(base));
    break;
  default:
    return true;
  }
  return true;
}

namespace {
  // common to both fields (in structs) and branches (in unions)
  string findSizeCommon(const std::string& indent, AST_Decl* field, const string& name,
                        AST_Type* type, const string& prefix, bool wrap_nested_key_only,
                        Intro& intro, const string& = "") // same sig as streamCommon
  {
    const bool use_cxx11 = be_global->language_mapping() == BE_GlobalData::LANGMAP_CXX11;
    const bool is_union_member = prefix == "uni";
    const bool is_optional = field != 0 ? be_global->is_optional(field) : false;

    AST_Type* const actual_type = resolveActualType(type);
    const Classification fld_cls = classify(actual_type);

    const std::string field_name = prefix + '.' + insert_cxx11_accessor_parens(name, is_union_member);
    const string qual = prefix + '.' + insert_cxx11_accessor_parens(name, is_union_member)
                        + (is_optional ? ".value()" : "");

    std::string line = "";
    if (is_optional) {
      line += indent + "primitive_serialized_size_boolean(encoding, size);\n"
            + indent + "if (" + field_name + ") {\n";
    }

    if (fld_cls & CL_ENUM) {
      line += indent + "primitive_serialized_size_ulong(encoding, size);\n";
    } else if (fld_cls & CL_STRING) {
      const string suffix = is_union_member ? "" : ".in()";
      const string get_size = use_cxx11 ? (qual + ".size()")
        : ("ACE_OS::strlen(" + qual + suffix + ")");

      line += indent + "primitive_serialized_size_ulong(encoding, size);\n" +
        indent + "size += " + get_size
        + ((fld_cls & CL_WIDE) ? " * char16_cdr_size;\n"
                               : " + 1;\n");
    } else if (fld_cls & CL_PRIMITIVE) {
      AST_PredefinedType* const p = dynamic_cast<AST_PredefinedType*>(actual_type);
      if (p->pt() == AST_PredefinedType::PT_longdouble) {
        // special case use to ACE's NONNATIVE_LONGDOUBLE in CDR_Base.h
        line += indent +
          "primitive_serialized_size(encoding, size, ACE_CDR::LongDouble());\n";
      } else {
        line += indent + "primitive_serialized_size(encoding, size, " +
          getWrapper(qual, actual_type, WD_OUTPUT) + ");\n";
      }
    } else if (fld_cls == CL_UNKNOWN) {
      return ""; // warning will be issued for the serialize functions
    } else { // sequence, struct, union, array
      RefWrapper wrapper(type, field_type_name(dynamic_cast<AST_Field*>(field), type),
        prefix + "." + insert_cxx11_accessor_parens(name, is_union_member) + (is_optional ? ".value()" : ""));
      wrapper.nested_key_only_ = wrap_nested_key_only;
      wrapper.done(&intro);
      line += indent + "serialized_size(encoding, size, " + wrapper.ref() + ");\n";
    }

    if (is_optional) {
      line += indent + "}\n";
    }

    return line;
  }

  string findSizeMutableUnion(const string& indent, AST_Decl* node, const string& name, AST_Type* type,
                              const string& prefix, bool wrap_nested_key_only, Intro& intro,
                              const string & = "") // same sig as streamCommon
  {
    return indent + "serialized_size_parameter_id(encoding, size, mutable_running_total);\n"
      + findSizeCommon(indent, node, name, type, prefix, wrap_nested_key_only, intro);
  }

  std::string generate_field_serialized_size(
    const std::string& indent, AST_Field* field, const std::string& prefix,
    bool wrap_nested_key_only, Intro& intro)
  {
    FieldInfo af(*field);
    if (af.anonymous()) {
      RefWrapper wrapper(af.type_, af.scoped_type_,
        prefix + "." + insert_cxx11_accessor_parens(af.name_));
      wrapper.nested_key_only_ = wrap_nested_key_only;
      wrapper.done(&intro);
      return indent + "serialized_size(encoding, size, " + wrapper.ref() + ");\n";
    }
    return findSizeCommon(
      indent, field, field->local_name()->get_string(), field->field_type(), prefix,
      wrap_nested_key_only, intro);
  }

  // common to both fields (in structs) and branches (in unions)
  string streamCommon(const std::string& /*indent*/, AST_Decl* field, const string& name,
    AST_Type* type, const string& prefix, bool wrap_nested_key_only, Intro& intro,
    const string& stru)
  {
    const bool use_cxx11 = be_global->language_mapping() == BE_GlobalData::LANGMAP_CXX11;
    const bool is_union_member = prefix.substr(3) == "uni";
    const bool is_optional = field != 0 ? be_global->is_optional(field) : false;

    AST_Type* const actual_type = resolveActualType(type);
    const Classification fld_cls = classify(actual_type);

    string qual = prefix + '.' + insert_cxx11_accessor_parens(name, is_union_member);
    // if there is a stray '.' on the end, strip it off
    if (qual[qual.length() - 1] == '.') {
      qual.erase(qual.length() - 1);
    }
    const string shift = prefix.substr(0, 2),
                 expr = qual.substr(3);

    WrapDirection dir = (shift == ">>") ? WD_INPUT : WD_OUTPUT;
    if ((fld_cls & CL_STRING) && (dir == WD_INPUT)) {
      if ((fld_cls & CL_BOUNDED)) {
        const string args = expr + (use_cxx11 ? ", " : ".out(), ") + bounded_arg(actual_type);
        return "(strm " + shift + ' ' + getWrapper(args, actual_type, WD_INPUT) + ')';
      }
      return "(strm " + qual + (use_cxx11 ? "" : ".out()") + ')';
    } else if (fld_cls & CL_PRIMITIVE) {
      return "(strm " + shift + ' ' + getWrapper(expr, actual_type, dir) + ')';
    } else if (fld_cls == CL_UNKNOWN) {
      if (dir == WD_INPUT) { // no need to warn twice
        std::cerr << "WARNING: field " << name << " can not be serialized.  "
          "The struct or union it belongs to (" << stru <<
          ") can not be used in an OpenDDS topic type." << std::endl;
      }
      return "false";
    } else { // sequence, struct, union, array, enum, string(insertion)
      string fieldref = prefix, local = insert_cxx11_accessor_parens(name, is_union_member);
      const bool accessor = local.size() > 2 && local.substr(local.size() - 2) == "()";
      if (fld_cls & CL_STRING) {
        if (!accessor && !use_cxx11) {
          if (is_optional) {
            local += ".value()";
          }

          local += ".in()";
        }
        if ((fld_cls & CL_BOUNDED)) {
          const string args = (fieldref + '.' + local).substr(3) + ", " + bounded_arg(actual_type);
          return "(strm " + shift + ' ' + getWrapper(args, actual_type, WD_OUTPUT) + ')';
        }
      }
      RefWrapper wrapper(type, field_type_name(dynamic_cast<AST_Field*>(field), type),
        fieldref, local, dir == WD_OUTPUT);
      wrapper.nested_key_only_ = wrap_nested_key_only;
      wrapper.done(&intro);
      return "(strm " + shift + " " + wrapper.ref() + ")";
    }
  }

  std::string generate_field_stream(
    const std::string& indent, AST_Field* field, const std::string& prefix, const std::string& field_name,
    bool wrap_nested_key_only, Intro& intro)
  {
    FieldInfo af(*field);

    if (af.anonymous()) {
      RefWrapper wrapper(af.type_, af.scoped_type_,
        prefix + "." + insert_cxx11_accessor_parens(af.name_));
      wrapper.nested_key_only_ = wrap_nested_key_only;
      wrapper.done(&intro);
      return "(strm " + wrapper.stream() + ")";
    }
    return streamCommon(
      indent, field, field_name, field->field_type(), prefix,
      wrap_nested_key_only, intro);
  }

  bool genBinaryProperty_t(const string& cxx)
  {
    {
      Function serialized_size("serialized_size", "void");
      serialized_size.addArg("encoding", "const Encoding&");
      serialized_size.addArg("size", "size_t&");
      serialized_size.addArg("stru", "const " + cxx + "&");
      serialized_size.endArgs();
      be_global->impl_ <<
        "  if (stru.propagate) {\n"
        "    primitive_serialized_size_ulong(encoding, size);\n"
        "    size += ACE_OS::strlen(stru.name.in()) + 1;\n"
        "    serialized_size(encoding, size, stru.value);\n"
        "  }\n";
    }
    {
      Function insertion("operator<<", "bool");
      insertion.addArg("strm", "Serializer&");
      insertion.addArg("stru", "const " + cxx + "&");
      insertion.endArgs();
      be_global->impl_ <<
        "  if (stru.propagate) {\n"
        "    return (strm << stru.name.in()) && (strm << stru.value);\n"
        "  }\n"
        "  return true;\n";
    }
    {
      Function extraction("operator>>", "bool");
      extraction.addArg("strm", "Serializer&");
      extraction.addArg("stru", cxx + "&");
      extraction.endArgs();
      be_global->impl_ <<
        "  stru.propagate = true;\n"
        "  return (strm >> stru.name.out()) && (strm >> stru.value);\n";
    }
    return true;
  }

  bool genProperty_t(const string& cxx)
  {
    {
      Function serialized_size("serialized_size", "void");
      serialized_size.addArg("encoding", "const Encoding&");
      serialized_size.addArg("size", "size_t&");
      serialized_size.addArg("stru", "const " + cxx + "&");
      serialized_size.endArgs();
      be_global->impl_ <<
        "  if (stru.propagate) {\n"
        "    primitive_serialized_size_ulong(encoding, size);\n"
        "    size += ACE_OS::strlen(stru.name.in()) + 1;\n"
        "    primitive_serialized_size_ulong(encoding, size);\n"
        "    size += ACE_OS::strlen(stru.value.in()) + 1;\n"
        "  }\n";
    }
    {
      Function insertion("operator<<", "bool");
      insertion.addArg("strm", "Serializer&");
      insertion.addArg("stru", "const " + cxx + "&");
      insertion.endArgs();
      be_global->impl_ <<
        "  if (stru.propagate) {\n"
        "    return (strm << stru.name.in()) && (strm << stru.value.in());\n"
        "  }\n"
        "  return true;\n";
    }
    {
      Function extraction("operator>>", "bool");
      extraction.addArg("strm", "Serializer&");
      extraction.addArg("stru", cxx + "&");
      extraction.endArgs();
      be_global->impl_ <<
        "  stru.propagate = true;\n"
        "  return (strm >> stru.name.out()) && (strm >> stru.value.out());\n";
    }
    return true;
  }

  bool genPropertyQosPolicy(const string& cxx)
  {
    {
      Function serialized_size("serialized_size", "void");
      serialized_size.addArg("encoding", "const Encoding&");
      serialized_size.addArg("size", "size_t&");
      serialized_size.addArg("stru", "const " + cxx + "&");
      serialized_size.endArgs();
      be_global->impl_ <<
        "  serialized_size(encoding, size, stru.value);\n"
        "  serialized_size(encoding, size, stru.binary_value);\n";
    }
    {
      Function insertion("operator<<", "bool");
      insertion.addArg("strm", "Serializer&");
      insertion.addArg("stru", "const " + cxx + "&");
      insertion.endArgs();
      be_global->impl_ <<
        "  return (strm << stru.value)\n"
        "    && (strm << stru.binary_value);\n";
    }
    {
      Function extraction("operator>>", "bool");
      extraction.addArg("strm", "Serializer&");
      extraction.addArg("stru", cxx + "&");
      extraction.endArgs();
      be_global->impl_ <<
        "  if (!(strm >> stru.value)) {\n"
        "    return false;\n"
        "  }\n"
        "  if (!strm.length() || !strm.skip(0, 4) || !strm.length()) {\n"
        "    return true; // optional member missing\n"
        "  }\n"
        "  return strm >> stru.binary_value;\n";
    }
    return true;
  }

  bool genSecuritySubmessage(const string& cxx)
  {
    {
      Function serialized_size("serialized_size", "void");
      serialized_size.addArg("encoding", "const Encoding&");
      serialized_size.addArg("size", "size_t&");
      serialized_size.addArg("stru", "const " + cxx + "&");
      serialized_size.endArgs();
      be_global->impl_ <<
        "  serialized_size(encoding, size, stru.smHeader);\n"
        "  primitive_serialized_size_octet(encoding, size, stru.content.length());\n";
    }
    {
      Function insertion("operator<<", "bool");
      insertion.addArg("strm", "Serializer&");
      insertion.addArg("stru", "const " + cxx + "&");
      insertion.endArgs();
      be_global->impl_ <<
        "  return (strm << stru.smHeader)\n"
        "    && strm.write_octet_array(stru.content.get_buffer(), "
        "stru.content.length());\n";
    }
    {
      Function extraction("operator>>", "bool");
      extraction.addArg("strm", "Serializer&");
      extraction.addArg("stru", cxx + "&");
      extraction.endArgs();
      be_global->impl_ <<
        "  if (strm >> stru.smHeader) {\n"
        "    stru.content.length(stru.smHeader.submessageLength);\n"
        "    if (strm.read_octet_array(stru.content.get_buffer(),\n"
        "                              stru.smHeader.submessageLength)) {\n"
        "      return true;\n"
        "    }\n"
        "  }\n"
        "  return false;\n";
    }
    return true;
  }

  bool genRtpsSpecialStruct(const string& cxx)
  {
    {
      Function serialized_size("serialized_size", "void");
      serialized_size.addArg("encoding", "const Encoding&");
      serialized_size.addArg("size", "size_t&");
      serialized_size.addArg("stru", "const " + cxx + "&");
      serialized_size.endArgs();
      be_global->impl_ <<
        "  size += "
        << ((cxx == RtpsNamespace + "SequenceNumberSet") ? "12" : "8")
        << " + 4 * ((stru.numBits + 31) / 32); // RTPS Custom\n";
    }
    {
      Function insertion("operator<<", "bool");
      insertion.addArg("strm", "Serializer&");
      insertion.addArg("stru", "const " + cxx + "&");
      insertion.endArgs();
      be_global->impl_ <<
        "  if ((strm << stru.bitmapBase) && (strm << stru.numBits)) {\n"
        "    const CORBA::ULong M = (stru.numBits + 31) / 32;\n"
        "    if (stru.bitmap.length() < M) {\n"
        "      return false;\n"
        "    }\n"
        "    for (CORBA::ULong i = 0; i < M; ++i) {\n"
        "      if (!(strm << stru.bitmap[i])) {\n"
        "        return false;\n"
        "      }\n"
        "    }\n"
        "    return true;\n"
        "  }\n"
        "  return false;\n";
    }
    {
      Function extraction("operator>>", "bool");
      extraction.addArg("strm", "Serializer&");
      extraction.addArg("stru", cxx + "&");
      extraction.endArgs();
      be_global->impl_ <<
        "  if ((strm >> stru.bitmapBase) && (strm >> stru.numBits)) {\n"
        "    const CORBA::ULong M = (stru.numBits + 31) / 32;\n"
        "    if (M > 8) {\n"
        "      return false;\n"
        "    }\n"
        "    stru.bitmap.length(M);\n"
        "    for (CORBA::ULong i = 0; i < M; ++i) {\n"
        "      if (!(strm >> stru.bitmap[i])) {\n"
        "        return false;\n"
        "      }\n"
        "    }\n"
        "    return true;\n"
        "  }\n"
        "  return false;\n";
    }
    return true;
  }

  struct RtpsFieldCustomizer {

    explicit RtpsFieldCustomizer(const string& cxx)
    {
      if (cxx == RtpsNamespace + "DataSubmessage") {
        cst_["inlineQos"] = "stru.smHeader.flags & 2";
        iQosOffset_ = "16";

      } else if (cxx == RtpsNamespace + "DataFragSubmessage") {
        cst_["inlineQos"] = "stru.smHeader.flags & 2";
        iQosOffset_ = "28";

      } else if (cxx == RtpsNamespace + "InfoReplySubmessage") {
        cst_["multicastLocatorList"] = "stru.smHeader.flags & 2";

      } else if (cxx == RtpsNamespace + "InfoTimestampSubmessage") {
        cst_["timestamp"] = "!(stru.smHeader.flags & 2)";

      } else if (cxx == RtpsNamespace + "InfoReplyIp4Submessage") {
        cst_["multicastLocator"] = "stru.smHeader.flags & 2";

      } else if (cxx == RtpsNamespace + "SubmessageHeader") {
        intro_.insert("strm.swap_bytes(ACE_CDR_BYTE_ORDER != (stru.flags & 1));");
      }
    }

    string getConditional(const string& field_name) const
    {
      if (cst_.empty()) {
        return "";
      }
      std::map<string, string>::const_iterator it = cst_.find(field_name);
      if (it != cst_.end()) {
        return it->second;
      }
      return "";
    }

    string preFieldRead(const string& field_name) const
    {
      if (cst_.empty() || field_name != "inlineQos" || iQosOffset_.empty()) {
        return "";
      }
      return "strm.skip(stru.octetsToInlineQos - " + iQosOffset_ + ")\n"
        "    && ";
    }

    std::map<string, string> cst_;
    string iQosOffset_;
    Intro intro_;
  };

  typedef void (*KeyIterationFn)(
    const std::string& indent,
    Encoding::Kind encoding,
    const string& key_name, AST_Type* ast_type,
    size_t* size,
    string* expr, Intro* intro);

  bool
  iterate_over_keys(
    const std::string& indent,
    Encoding::Kind encoding,
    AST_Structure* node,
    const std::string& struct_name,
    IDL_GlobalData::DCPS_Data_Type_Info* info,
    TopicKeys* keys,
    KeyIterationFn fn,
    size_t* size,
    string* expr, Intro* intro)
  {
    if (keys && keys->root_type() != TopicKeys::InvalidType) {
      const TopicKeys::Iterator finished = keys->end();
      for (TopicKeys::Iterator i = keys->begin(); i != finished; ++i) {
        string key_access = i.path();
        AST_Type* straight_ast_type = i.get_ast_type();
        AST_Type* ast_type;
        if (i.root_type() == TopicKeys::UnionType) {
          AST_Union* union_type = dynamic_cast<AST_Union*>(straight_ast_type);
          if (!union_type) {
            std::cerr << "ERROR: Invalid key iterator for: " << struct_name;
            return false;
          }
          ast_type = dynamic_cast<AST_Type*>(union_type->disc_type());
          key_access.append("._d()");
        } else {
          ast_type = straight_ast_type;
        }
        fn(indent, encoding, key_access, ast_type, size, expr, intro);
      }
      return true;
    }

    if (info) {
      IDL_GlobalData::DCPS_Data_Type_Info_Iter iter(info->key_list_);
      for (ACE_TString* kp = 0; iter.next(kp) != 0; iter.advance()) {
        const string key_name = ACE_TEXT_ALWAYS_CHAR(kp->c_str());
        fn(indent, encoding, key_name, find_type(node, key_name), size, expr, intro);
      }
    }

    return true;
  }

  // Args must match KeyIterationFn.
  void idl_max_serialized_size_iteration(
    const std::string&, Encoding::Kind encoding, const string&, AST_Type* ast_type,
    size_t* size, string*, Intro*)
  {
    idl_max_serialized_size(encoding, *size, ast_type);
  }

  void serialized_size_iteration(
    const std::string& indent, Encoding::Kind, const string& key_name, AST_Type* ast_type,
    size_t*, string* expr, Intro* intro)
  {
    *expr += findSizeCommon(indent, 0, key_name, ast_type, "stru.value", false, *intro);
  }

  std::string fill_datareprseq(
    const OpenDDS::DataRepresentation& repr,
    const std::string& name,
    const std::string& indent)
  {
    std::vector<std::string> values;
    if (repr.xcdr1) {
      values.push_back("DDS::XCDR_DATA_REPRESENTATION");
    }
    if (repr.xcdr2) {
      values.push_back("DDS::XCDR2_DATA_REPRESENTATION");
    }
    if (repr.xml) {
      values.push_back("DDS::XML_DATA_REPRESENTATION");
    }
    if (repr.unaligned) {
      values.push_back("OpenDDS::DCPS::UNALIGNED_CDR_DATA_REPRESENTATION");
    }

    std::ostringstream ss;
    ss << indent << name << ".length(" << values.size() << ");\n";
    for (size_t i = 0; i < values.size(); ++i) {
      ss << indent << name << "[" << i << "] = " << values[i] << ";\n";
    }
    return ss.str();
  }

  bool is_bounded_topic_struct(AST_Type* type, Encoding::Kind encoding, bool key_only,
    TopicKeys& keys, IDL_GlobalData::DCPS_Data_Type_Info* info = 0)
  {
    /*
     * TODO(iguessthislldo): This is a workaround for not properly implementing
     * serialized_size_bound for XCDR. See XTYPE-83.
     */
    const ExtensibilityKind exten = be_global->extensibility(type);
    if (exten != extensibilitykind_final && encoding != Encoding::KIND_UNALIGNED_CDR) {
      return false;
    }

    bool bounded = true;
    if (key_only) {
      if (info) {
        IDL_GlobalData::DCPS_Data_Type_Info_Iter iter(info->key_list_);
        AST_Structure* const struct_type = dynamic_cast<AST_Structure*>(type);
        for (ACE_TString* kp = 0; iter.next(kp) != 0; iter.advance()) {
          const string key_name = ACE_TEXT_ALWAYS_CHAR(kp->c_str());
          AST_Type* const field_type = find_type(struct_type, key_name);
          if (!is_bounded_type(field_type, encoding)) {
            bounded = false;
            break;
          }
        }
      } else {
        const TopicKeys::Iterator finished = keys.end();
        for (TopicKeys::Iterator i = keys.begin(); i != finished; ++i) {
          if (!is_bounded_type(i.get_ast_type(), encoding)) {
            bounded = false;
            break;
          }
        }
      }
    } else {
      bounded = is_bounded_type(type, encoding);
    }
    return bounded;
  }

  bool generate_marshal_traits_struct_bounds_functions(AST_Structure* node,
    TopicKeys& keys, IDL_GlobalData::DCPS_Data_Type_Info* info, bool key_only)
  {
    const char* function_prefix = key_only ? "key_only_" : "";
    AST_Type* const type_node = dynamic_cast<AST_Type*>(node);
    const Fields fields(node);
    const Fields::Iterator fields_end = fields.end();
    const std::string name = scoped(node->name());
    const ExtensibilityKind exten = be_global->extensibility(node);

    be_global->header_ <<
      "  static SerializedSizeBound " << function_prefix <<
        "serialized_size_bound(const Encoding& encoding)\n"
      "  {\n"
      "    switch (encoding.kind()) {\n";
    for (unsigned e = 0; e <= Encoding::KIND_UNALIGNED_CDR; ++e) {
      const Encoding::Kind encoding = static_cast<Encoding::Kind>(e);
      be_global->header_ <<
        "    case " << encoding_to_encoding_kind(encoding) << ":\n"
        "      return SerializedSizeBound(";
      if (is_bounded_topic_struct(type_node, encoding, key_only, keys, info)) {
        size_t size = 0;
        if (key_only) {
          idl_max_serialized_size_dheader(encoding, exten, size);
          if (!iterate_over_keys("", encoding, node, name, info, &keys,
                idl_max_serialized_size_iteration, &size, 0, 0)) {
            return false;
          }
        } else {
          for (Fields::Iterator i = fields.begin(); i != fields_end; ++i) {
            idl_max_serialized_size(encoding, size, (*i)->field_type());
          }
        }
        be_global->header_ << size;
      }
      be_global->header_ << ");\n";
    }
    be_global->header_ <<
      "    default:\n"
      "      OPENDDS_ASSERT(false);\n"
      "      return SerializedSizeBound();\n"
      "    }\n"
      "  }\n"
      "\n";

    return true;
  }

  bool generate_marshal_traits_struct(AST_Structure* node,
    TopicKeys& keys, IDL_GlobalData::DCPS_Data_Type_Info* info = 0)
  {
    return
      generate_marshal_traits_struct_bounds_functions(node, keys, info, false) && // All Fields
      generate_marshal_traits_struct_bounds_functions(node, keys, info, true); // Key Fields
  }

  bool generate_marshal_traits_union(AST_Union* node, bool has_key, ExtensibilityKind exten)
  {
    be_global->header_ <<
      "  static SerializedSizeBound serialized_size_bound(const Encoding& encoding)\n"
      "  {\n"
      "    switch (encoding.kind()) {\n";
    for (unsigned e = 0; e <= Encoding::KIND_UNALIGNED_CDR; ++e) {
      const Encoding::Kind encoding = static_cast<Encoding::Kind>(e);
      be_global->header_ <<
        "    case " << encoding_to_encoding_kind(encoding) << ":\n"
        "      return SerializedSizeBound(";
      if (is_bounded_type(node, encoding)) {
        size_t size = 0;
        idl_max_serialized_size(encoding, size, node);
        be_global->header_ << size;
      }
      be_global->header_ << ");\n";
    }
    be_global->header_ <<
      "    default:\n"
      "      OPENDDS_ASSERT(false);\n"
      "      return SerializedSizeBound();\n"
      "    }\n"
      "  }\n"
      "\n";

    be_global->header_ <<
      "  static SerializedSizeBound key_only_serialized_size_bound(const Encoding& encoding)\n"
      "  {\n"
      "    switch (encoding.kind()) {\n";
    for (unsigned e = 0; e <= Encoding::KIND_UNALIGNED_CDR; ++e) {
      const Encoding::Kind encoding = static_cast<Encoding::Kind>(e);
      be_global->header_ <<
        "    case " << encoding_to_encoding_kind(encoding) << ":\n"
        "      return SerializedSizeBound(";
      /*
       * TODO(iguessthislldo): This is a workaround for not properly implementing
       * serialized_size_bound for Mutable. See XTYPE-83.
       */
      if (exten == extensibilitykind_final || encoding == Encoding::KIND_UNALIGNED_CDR) {
        size_t size = 0;
        if (has_key) {
          idl_max_serialized_size(encoding, size, node->disc_type());
        }
        be_global->header_ << size;
      }
      be_global->header_ << ");\n";
    }
    be_global->header_ <<
      "    default:\n"
      "      OPENDDS_ASSERT(false);\n"
      "      return SerializedSizeBound();\n"
      "    }\n"
      "  }\n"
      "\n";

    return true;
  }

  bool generate_marshal_traits(
    AST_Decl* node, const std::string& cxx, ExtensibilityKind exten,
    TopicKeys& keys, IDL_GlobalData::DCPS_Data_Type_Info* info = 0)
  {
    AST_Structure* const struct_node =
      node->node_type() == AST_Decl::NT_struct ? dynamic_cast<AST_Structure*>(node) : 0;
    AST_Union* const union_node =
      node->node_type() == AST_Decl::NT_union ? dynamic_cast<AST_Union*>(node) : 0;
    if (!struct_node && !union_node) {
      idl_global->err()->misc_error("Can't generate MarshalTraits for this node", node);
      return false;
    }

    std::string octetSeqOnly;
    if (struct_node && struct_node->nfields() == 1) {
      AST_Field* const field = get_struct_field(struct_node, 0);
      AST_Type* const type = resolveActualType(field->field_type());
      const Classification fld_cls = classify(type);
      if (fld_cls & CL_SEQUENCE) {
        AST_Sequence* const seq = dynamic_cast<AST_Sequence*>(type);
        AST_Type* const base = resolveActualType(seq->base_type());
        if (classify(base) & CL_PRIMITIVE) {
          AST_PredefinedType* const pt = dynamic_cast<AST_PredefinedType*>(base);
          if (pt->pt() == AST_PredefinedType::PT_octet) {
            octetSeqOnly = field->local_name()->get_string();
          }
        }
      }
    }

    std::string export_string;
    if (octetSeqOnly.size()) {
      const ACE_CString exporter = be_global->export_macro();
      if (exporter != "") {
        export_string = string(" ") + exporter.c_str();
      }
    }

    be_global->add_include("dds/DCPS/TypeSupportImpl.h");

    be_global->header_ <<
      "template <>\n"
      "struct" << export_string << " MarshalTraits<" << cxx << "> {\n"
      "  static void representations_allowed_by_type(DDS::DataRepresentationIdSeq& seq)\n"
      "  {\n"
        << fill_datareprseq(be_global->data_representations(node), "seq", "    ") <<
      "  }\n"
      "\n";

    if (struct_node) {
      if (!generate_marshal_traits_struct(struct_node, keys, info)) {
        return false;
      }
    } else if (union_node) {
      if (!generate_marshal_traits_union(union_node, keys.count(), exten)) {
        return false;
      }
    }

    const char* msg_block_fn_decl_end = " { return false; }";
    if (octetSeqOnly.size()) {
      const char* get_len;
      const char* set_len;
      const char* get_buffer;
      const char* buffer_pre = "";
      if (be_global->language_mapping() == BE_GlobalData::LANGMAP_CXX11) {
        get_len = "size";
        set_len = "resize";
        get_buffer = "[0]";
        buffer_pre = "&";
        octetSeqOnly += "()";
      } else {
        get_len = set_len = "length";
        get_buffer = ".get_buffer()";
      }

      be_global->impl_ <<
        "bool MarshalTraits<" << cxx << ">::to_message_block(ACE_Message_Block& mb, "
        "const " << cxx << "& stru)\n"
        "{\n"
        "  if (mb.size(stru." << octetSeqOnly << "." << get_len << "()) != 0) {\n"
        "    return false;\n"
        "  }\n"
        "  return mb.copy(reinterpret_cast<const char*>(" << buffer_pre << "stru."
          << octetSeqOnly << get_buffer << "), stru." << octetSeqOnly << "." << get_len
          << "()) == 0;\n"
        "}\n\n"
        "bool MarshalTraits<" << cxx << ">::from_message_block(" << cxx << "& stru, "
        "const ACE_Message_Block& mb)\n"
        "{\n"
        "  stru." << octetSeqOnly << "." << set_len << "(static_cast<unsigned>(mb.total_length()));\n"
        "  ACE_CDR::Octet* dst = " << buffer_pre << "stru." << octetSeqOnly << get_buffer << ";\n"
        "  for (const ACE_Message_Block* m = &mb; m; m = m->cont()) {\n"
        "    std::memcpy(dst, m->rd_ptr(), m->length());\n"
        "    dst += m->length();\n"
        "  }\n"
        "  return true;\n"
        "}\n\n";

      msg_block_fn_decl_end = ";";
    }
    be_global->header_ <<
      "  static bool to_message_block(ACE_Message_Block&, const " << cxx << "&)"
        << msg_block_fn_decl_end << "\n"
      "  static bool from_message_block(" << cxx << "&, const ACE_Message_Block&)"
        << msg_block_fn_decl_end << "\n";

    /*
     * This is used for the CDR header.
     * This is just for the base type, nested types can have different
     * extensibilities.
     */
    be_global->header_ <<
      "  static Extensibility extensibility() { return ";
    switch (exten) {
    case extensibilitykind_final:
      be_global->header_ << "FINAL";
      break;
    case extensibilitykind_appendable:
      be_global->header_ << "APPENDABLE";
      break;
    case extensibilitykind_mutable:
      be_global->header_ << "MUTABLE";
      break;
    default:
      idl_global->err()->misc_error(
        "Unexpected extensibility while generating MarshalTraits", node);
      return false;
    }
    be_global->header_ << "; }\n";

    /*
     * This is used for topic type extensibility level.
     */
    const ExtensibilityKind ek = max_extensibility_kind(dynamic_cast<AST_Type*>(node));
    be_global->header_ <<
      "  static Extensibility max_extensibility_level() { return ";
    switch (ek) {
    case extensibilitykind_final:
      be_global->header_ << "FINAL";
      break;
    case extensibilitykind_appendable:
      be_global->header_ << "APPENDABLE";
      break;
    case extensibilitykind_mutable:
      be_global->header_ << "MUTABLE";
      break;
    default:
      idl_global->err()->misc_error(
        "Unexpected extensibility level while generating MarshalTraits", node);
      return false;
    }
    be_global->header_ << "; }\n"
      "};\n";

    return true;
  }

  bool generate_struct_deserialization(
    AST_Structure* node, FieldFilter field_type)
  {
    const std::string actual_cpp_name = scoped(node->name());
    std::string cpp_name = actual_cpp_name;
    std::string const_cpp_name;
    switch (field_type) {
    case FieldFilter_All:
      const_cpp_name = "const" + actual_cpp_name + "&";
      break;
    case FieldFilter_NestedKeyOnly:
      cpp_name = "const NestedKeyOnly<" + actual_cpp_name + ">";
      const_cpp_name = "const NestedKeyOnly<const" + actual_cpp_name + ">&";
      break;
    case FieldFilter_KeyOnly:
      cpp_name = "const KeyOnly<" + actual_cpp_name + ">";
      const_cpp_name = "const KeyOnly<const" + actual_cpp_name + ">&";
      break;
    }
    const std::string value_access = field_type == FieldFilter_All ? "" : ".value";
    const bool wrap_nested_key_only = field_type != FieldFilter_All;
    const Fields fields(node, field_type);
    const Fields::Iterator fields_end = fields.end();
    RtpsFieldCustomizer rtpsCustom(cpp_name);

    const bool use_cxx11 = be_global->language_mapping() == BE_GlobalData::LANGMAP_CXX11;

    const ExtensibilityKind exten = be_global->extensibility(node);
    const bool not_final = exten != extensibilitykind_final;
    const bool is_mutable = exten == extensibilitykind_mutable;
    const bool is_appendable = exten == extensibilitykind_appendable;

    {
      Function extraction("operator>>", "bool");
      extraction.addArg("strm", "Serializer&");
      extraction.addArg("stru", cpp_name + "&");
      extraction.endArgs();
      Intro intro;
      string expr;
      const std::string indent = "  ";

      be_global->impl_ <<
        "  const Encoding& encoding = strm.encoding();\n"
        "  ACE_UNUSED_ARG(encoding);\n";
      if (is_appendable) {
        be_global->impl_ <<
          "  bool reached_end_of_struct = false;\n"
          "  ACE_UNUSED_ARG(reached_end_of_struct);\n";
      }
      marshal_generator::generate_dheader_code(
        "    if (!strm.read_delimiter(total_size)) {\n"
        "      return false;\n"
        "    }\n", not_final);

      if (not_final) {
        be_global->impl_ <<
          "  const size_t end_of_struct = strm.rpos() + total_size;\n"
          "\n";
      }

      if (is_mutable) {
        be_global->impl_ <<
          "  if (encoding.xcdr_version() != Encoding::XCDR_VERSION_NONE) {\n"
          "    set_default(stru" << (wrap_nested_key_only ? ".value" : "") << ");\n"
          "\n"
          "    unsigned member_id;\n"
          "    size_t field_size;\n"
          "    while (true) {\n";

        /*
        * Get the Member ID and see if we're done. In XCDR1 we use a special
        * member ID to stop the loop. We don't have a PID marking the end in
        * XCDR2 parameter lists, but we have the size, so we need to stop
        * after we hit the offset marked by the delimiter, but before trying
        * to read a non-existent member id.
        */
        be_global->impl_ <<
          "      if (encoding.xcdr_version() == Encoding::XCDR_VERSION_2 && strm.rpos() >= end_of_struct) {\n"
          "        return true;\n"
          "      }\n"
          "      bool must_understand = false;\n"
          "      if (!strm.read_parameter_id(member_id, field_size, must_understand)) {\n"
          "        return false;\n"
          "      }\n"
          "      if (encoding.xcdr_version() == Encoding::XCDR_VERSION_1 && member_id == Serializer::pid_list_end) {\n"
          "        return true;\n"
          "      }\n"
          "      const size_t end_of_field = strm.rpos() + field_size;\n"
          "      ACE_UNUSED_ARG(end_of_field);\n"
          "\n";

        std::ostringstream cases;
        for (Fields::Iterator i = fields.begin(); i != fields_end; ++i) {
          AST_Field* const field = *i;
          const OpenDDS::XTypes::MemberId id = be_global->get_id(field);
          string field_name =
            string("stru") + value_access + "." + field->local_name()->get_string();
          cases <<
            "      case " << id << ": {\n"
            "        if (!" << generate_field_stream(
            indent, field, ">> stru" + value_access, field->local_name()->get_string(), wrap_nested_key_only, intro) << ") {\n";
          AST_Type* const field_type = resolveActualType(field->field_type());
          const Classification fld_cls = classify(field_type);

          if (use_cxx11) {
            field_name += "()";
          }
          const TryConstructFailAction try_construct = be_global->try_construct(field);
          if (try_construct == tryconstructfailaction_use_default) {
            cases <<
              type_to_default("          ", field_type, field_name, field->field_type()->anonymous()) <<
              "          strm.set_construction_status(Serializer::ConstructionSuccessful);\n";
            if (!(fld_cls & CL_STRING)) cases << "        strm.skip(end_of_field - strm.rpos());\n";
          } else if ((try_construct == tryconstructfailaction_trim) && (fld_cls & CL_BOUNDED) &&
                    (fld_cls & (CL_STRING | CL_SEQUENCE))) {
            if ((fld_cls & CL_STRING) && (fld_cls & CL_BOUNDED)) {
              const std::string check_not_empty = use_cxx11 ? "!" + field_name + ".empty()" : field_name + ".in()";
              const std::string get_length = use_cxx11 ? field_name + ".length()" : "ACE_OS::strlen(" + field_name + ".in())";
              const std::string inout = use_cxx11 ? "" : ".inout()";
              cases <<
                "        if (" + construct_bound_fail + " && " << check_not_empty << " && ("
                        << bounded_arg(field_type) << " < " << get_length << ")) {\n"
                "          " << field_name << inout;
              if (use_cxx11) {
                cases <<
                  ".resize(" << bounded_arg(field_type) <<  ");\n";
              } else {
                cases <<
                  "[" << bounded_arg(field_type) << "] = 0;\n";
              }
              cases <<
                "        } else {\n"
                "          strm.set_construction_status(Serializer::ElementConstructionFailure);\n"
                "          return false;\n"
                "        }\n";
            } else if (fld_cls & CL_SEQUENCE) {
              cases <<
                "          if (" + construct_elem_fail + ") {\n"
                "            return false;\n"
                "          }\n"
                "          strm.set_construction_status(Serializer::ConstructionSuccessful);\n"
                "          strm.skip(end_of_field - strm.rpos());\n";
            }

          } else { //discard/default
            cases <<
              "          strm.set_construction_status(Serializer::ElementConstructionFailure);\n";
            if (!(fld_cls & CL_STRING)) {
              cases <<
                "          strm.skip(end_of_field - strm.rpos());\n";
            }
            cases <<
              "          return false;\n";
          }
          cases <<
            "        }\n"
            "        break;\n"
            "      }\n";
        }
        intro.join(be_global->impl_, indent);
        const string switch_cases = cases.str();
        string sw_indent = "        ";
        if (switch_cases.empty()) {
          sw_indent = "      ";
        } else {
          be_global->impl_ <<
            "      switch (member_id) {\n"
            << switch_cases <<
            "      default:\n";
        }

        be_global->impl_ <<
          sw_indent << "if (must_understand) {\n" <<
          sw_indent << "  if (DCPS_debug_level >= 8) {\n" <<
          sw_indent << "    ACE_DEBUG((LM_DEBUG, ACE_TEXT(\"(%P|%t) unknown must_understand field(%u) in "
          << cpp_name << "\\n\"), member_id));\n" <<
          sw_indent << "  }\n" <<
          sw_indent << "  return false;\n" <<
          sw_indent << "}\n" <<
          sw_indent << "strm.skip(field_size);\n";

        if (!switch_cases.empty()) {
          be_global->impl_ <<
            "        break;\n"
            "      }\n";
        }

        be_global->impl_ <<
          "    }\n"
          "    return false;\n"
          "  }\n"
          "\n";
      }

      expr = "";
      for (Fields::Iterator i = fields.begin(); i != fields_end; ++i) {
        AST_Field* const field = *i;
        const bool is_optional = be_global->is_optional(field);

        if (expr.size() && exten != extensibilitykind_appendable) {
          expr += "\n    && ";
        }
        if (is_appendable) {
          expr +=
            "  reached_end_of_struct |= (encoding.xcdr_version() == Encoding::XCDR_VERSION_2 && strm.rpos() >= end_of_struct);\n";
        }
        const string field_name = field->local_name()->get_string();
        const string cond = rtpsCustom.getConditional(field_name);
        if (!cond.empty()) {
          string prefix = rtpsCustom.preFieldRead(field_name);
          if (is_appendable) {
            if (!prefix.empty()) {
              prefix = prefix.substr(0, prefix.length() - 8);
              expr +=
                "  if (!" + prefix + ") {\n"
                "    return false;\n"
                "  }\n";
            }
            expr += "  if ((" + cond + ") && !";
          } else {
            expr += prefix + "(!(" + cond + ") || ";
          }
        } else if (is_appendable) {
          AST_Type* const type = field->field_type();
          string stru_field_name = "stru" + value_access + "." + field_name;
          if (use_cxx11) {
            stru_field_name += "()";
          }
          expr +=
            "  if (reached_end_of_struct) {\n" +
            type_to_default("    ", type, stru_field_name, type->anonymous(), false, is_optional) +
            "  } else {\n";
          if (!is_optional) {
            expr += "    if (!";
          }
        }

        // TODO(tyler) This feels kind of hacky
        // Can the optional branch be isolated completely
        // Assigns strm value to temporary values
        if (is_optional) {
            //TODO(tyler) Is there an easier way to deal with strings here
            AST_Type* const field_type = resolveActualType(field->field_type());
            Classification fld_cls = classify(field_type);
            const std::string type_name = fld_cls & CL_STRING ? "String" : field->field_type()->full_name();
            const std::string has_value_name = field_name + "_has_value";
            expr += "    bool " + field_name + "_has_value = false;\n";
            expr += "    strm >> ACE_InputCDR::to_boolean(" + has_value_name + ");\n";
            expr += "    " + type_name + " " + field_name + "_tmp;\n";
            expr += "    if (" + has_value_name + " && !";
            expr += "(strm >> " + field_name + "_tmp)";
        } else {
          expr += generate_field_stream(
            indent, field, ">> stru" + value_access, field->local_name()->get_string(), wrap_nested_key_only, intro);
        }
        if (is_appendable) {
          expr += ") {\n"
            "      return false;\n"
            "    }\n";

          // Copy temporaries to the struct
          if (is_optional) {
            string stru_field_name = "stru" + value_access + "." + field_name;
            if (use_cxx11) {
              stru_field_name += "()";
            }
            expr += "    if (" + field_name + "_has_value) " + stru_field_name + " = " + field_name + "_tmp;\n";
          }
          if (cond.empty()) {
            expr +=
            "  }\n";
          }
        } else if (!cond.empty()) {
          expr += ")";
        }
      }
      intro.join(be_global->impl_, indent);
      if (is_appendable) {
        expr +=
          "  if (encoding.xcdr_version() == Encoding::XCDR_VERSION_2 && strm.rpos() < end_of_struct) {\n"
          "    strm.skip(end_of_struct - strm.rpos());\n"
          "  }\n"
          "  return true;\n";
        be_global->impl_ << expr;
      } else if (expr.empty()) {
        be_global->impl_ << "  return true;\n";
      } else {
        be_global->impl_ << "  return " << expr << ";\n";
      }
    }
    return true;
  }

  bool generate_struct_serialization_functions(AST_Structure* node, FieldFilter field_type)
  {
    const std::string actual_cpp_name = scoped(node->name());
    std::string cpp_name = actual_cpp_name;
    std::string const_cpp_name;
    switch (field_type) {
    case FieldFilter_All:
      const_cpp_name = "const" + actual_cpp_name + "&";
      break;
    case FieldFilter_NestedKeyOnly:
      cpp_name = "const NestedKeyOnly<" + actual_cpp_name + ">";
      const_cpp_name = "const NestedKeyOnly<const" + actual_cpp_name + ">&";
      break;
    case FieldFilter_KeyOnly:
      cpp_name = "const KeyOnly<" + actual_cpp_name + ">";
      const_cpp_name = "const KeyOnly<const" + actual_cpp_name + ">&";
      break;
    }
    const std::string value_access = field_type == FieldFilter_All ? "" : ".value";
    const bool wrap_nested_key_only = field_type != FieldFilter_All;
    const Fields fields(node, field_type);
    const Fields::Iterator fields_end = fields.end();
    RtpsFieldCustomizer rtpsCustom(cpp_name);

    const ExtensibilityKind exten = be_global->extensibility(node);
    const bool not_final = exten != extensibilitykind_final;
    const bool is_mutable = exten == extensibilitykind_mutable;

    {
      Function serialized_size("serialized_size", "void");
      serialized_size.addArg("encoding", "const Encoding&");
      serialized_size.addArg("size", "size_t&");
      serialized_size.addArg("stru", const_cpp_name);
      serialized_size.endArgs();

      if (is_mutable) {
        /*
         * For parameter lists this is used to hold the total size while
         * size is hijacked for field sizes because of alignment resets.
         */
        be_global->impl_ <<
          "  size_t mutable_running_total = 0;\n";
      }

      marshal_generator::generate_dheader_code("    serialized_size_delimiter(encoding, size);\n", not_final, false);

      std::string expr;
      Intro intro;
      const std::string indent = "  ";
      for (Fields::Iterator i = fields.begin(); i != fields_end; ++i) {
        AST_Field* const field = *i;
        AST_Type* field_type = resolveActualType(field->field_type());
        if (!field_type->in_main_file() && field_type->node_type() != AST_Decl::NT_pre_defined) {
          be_global->add_referenced(field_type->file_name().c_str());
        }
        const string field_name = field->local_name()->get_string(),
          cond = rtpsCustom.getConditional(field_name);
        if (!cond.empty()) {
          expr += "  if (" + cond + ") {\n  ";
        }
        if (is_mutable) {
          expr +=
            "  serialized_size_parameter_id(encoding, size, mutable_running_total);\n";
        }
        expr += generate_field_serialized_size(
          indent, field, "stru" + value_access, wrap_nested_key_only, intro);
        if (!cond.empty()) {
          expr += "  }\n";
        }
      }
      intro.join(be_global->impl_, indent);
      be_global->impl_ << expr;

      if (is_mutable) {
        be_global->impl_ <<
          "  serialized_size_list_end_parameter_id(encoding, size, mutable_running_total);\n";
      }
    }

    {
      Function insertion("operator<<", "bool");
      insertion.addArg("strm", "Serializer&");
      insertion.addArg("stru", const_cpp_name);
      insertion.endArgs();
      be_global->impl_ <<
        "  const Encoding& encoding = strm.encoding();\n"
        "  ACE_UNUSED_ARG(encoding);\n";
      marshal_generator::generate_dheader_code(
        "    serialized_size(encoding, total_size, stru);\n"
        "    if (!strm.write_delimiter(total_size)) {\n"
        "      return false;\n"
        "    }\n", not_final);

      // Mutable Code
      std::ostringstream mutable_fields;
      Intro intro = rtpsCustom.intro_;
      const std::string indent = "  ";
      if (is_mutable) {
        const std::string mutable_indent = indent + "  ";
        mutable_fields <<
          "  if (encoding.xcdr_version() != Encoding::XCDR_VERSION_NONE) {\n"
          "    size_t size = 0;\n"
          "    ACE_UNUSED_ARG(size);\n";
        for (Fields::Iterator i = fields.begin(); i != fields_end; ++i) {
          AST_Field* const field = *i;
          const OpenDDS::XTypes::MemberId id = be_global->get_id(field);
          const bool must_understand = be_global->is_effectively_must_understand(field);

          mutable_fields
            << generate_field_serialized_size(
              mutable_indent, field, "stru" + value_access, wrap_nested_key_only, intro)
            << "\n"
            "    if (!strm.write_parameter_id("
              << id << ", size" << (must_understand ? ", true" : "") << ")) {\n"
            "      return false;\n"
            "    }\n"
            "    size = 0;\n"
            "    if (!" << generate_field_stream(
              mutable_indent, field, "<< stru" + value_access, field->local_name()->get_string(), wrap_nested_key_only, intro)
            << ") {\n"
            "      return false;\n"
            "    }\n";
        }
        mutable_fields << "\n"
          "    if (!strm.write_list_end_parameter_id()) {\n"
          "      return false;\n"
          "    }\n"
          "    return true;\n"
          "  }\n";
      }

      // Non-Mutable Code
      string expr;
      for (Fields::Iterator i = fields.begin(); i != fields_end; ++i) {
        AST_Field* const field = *i;
        const bool is_optional = be_global->is_optional(field);;
        if (expr.size()) expr += "\n    && ";
        const string field_name = field->local_name()->get_string(),
          cond = rtpsCustom.getConditional(field_name);
        if (!cond.empty()) {
          expr += "(!(" + cond + ") || ";
        }

        if (is_optional) {
          expr += "(strm << ACE_OutputCDR::from_boolean(stru" + value_access + "." + field_name + "().has_value()) && ";
          expr += "stru" + value_access + "." + field_name + "().has_value() ? ";
          expr += generate_field_stream(
            indent, field, "<< stru" + value_access, field_name + ".value", wrap_nested_key_only, intro);
          expr += " : true)";
        } else {
          expr += generate_field_stream(
            indent, field, "<< stru" + value_access, field_name, wrap_nested_key_only, intro);
        }

        if (!cond.empty()) {
          expr += ")";
        }
      }

      intro.join(be_global->impl_, indent);
      if (expr.empty()) {
        expr = "true";
      }
      be_global->impl_ << mutable_fields.str() << "  return " << expr << ";\n";
    }

    return generate_struct_deserialization(node, field_type);
  }

} // anonymous namespace


void marshal_generator::generate_dheader_code(const std::string& code, bool dheader_required,
                                              bool is_ser_func, const char* indent)
{
  const std::string indents(indent);
  //DHeader appears on aggregated types that are mutable or appendable in XCDR2
  //DHeader also appears on ALL sequences and arrays of non-primitives
  if (dheader_required) {
    if (is_ser_func) {
      be_global->impl_ <<
        indents << "size_t total_size = 0;\n";
    }
    be_global->impl_ <<
      indents << "if (encoding.xcdr_version() == Encoding::XCDR_VERSION_2) {\n"
      << code <<
      indents << "}\n";
  }
}

bool marshal_generator::gen_struct(AST_Structure* node,
                                   UTL_ScopedName* name,
                                   const std::vector<AST_Field*>& fields,
                                   AST_Type::SIZE_TYPE /* size */,
                                   const char* /* repoid */)
{
  NamespaceGuard ng;
  be_global->add_include("dds/DCPS/Serializer.h");
  const string cxx = scoped(name); // name as a C++ class
  const bool use_cxx11 = be_global->language_mapping() == BE_GlobalData::LANGMAP_CXX11;

  {
    Function set_default("set_default", "void", "");
    set_default.addArg("stru", cxx + "&");
    set_default.endArgs();
    std::ostringstream contents;
    Intro intro;
    for (size_t i = 0; i < fields.size(); ++i) {
      AST_Field* const field = fields[i];
      AST_Type* const type = field->field_type();
      string field_name = string("stru.") + field->local_name()->get_string();
      if (use_cxx11) {
        field_name += "()";
      }
      contents << type_to_default("  ", type, field_name, type->anonymous(), false, be_global->is_optional(field));
    }
    intro.join(be_global->impl_, "  ");
    be_global->impl_ << contents.str();
  }

  bool special_result;
  if (generate_special_struct(node, cxx, special_result)) {
    return special_result;
  }

  FieldInfo::EleLenSet anonymous_seq_generated;
  for (size_t i = 0; i < fields.size(); ++i) {
    if (fields[i]->field_type()->anonymous()) {
      FieldInfo af(*fields[i]);
      if (af.arr_) {
        gen_anonymous_array(af);
      } else if (af.seq_ && af.is_new(anonymous_seq_generated)) {
        gen_anonymous_sequence(af);
      }
    }
  }

  if (!generate_struct_serialization_functions(node, FieldFilter_All) ||
      !generate_struct_serialization_functions(node, FieldFilter_NestedKeyOnly)) {
    return false;
  }

  IDL_GlobalData::DCPS_Data_Type_Info* info = idl_global->is_dcps_type(name);
  const bool is_topic_type = be_global->is_topic_type(node);
  TopicKeys keys;
  if (is_topic_type) {
    keys = TopicKeys(node);
    info = 0; // Annotations Override DCPS_DATA_TYPE

    if (!generate_struct_serialization_functions(node, FieldFilter_KeyOnly)) {
      return false;
    }
  }

  if ((info || is_topic_type) &&
      !generate_marshal_traits(node, cxx, be_global->extensibility(node), keys, info)) {
    return false;
  }

  if (info && !is_topic_type) {
    {
      Function serialized_size("serialized_size", "void");
      serialized_size.addArg("encoding", "const Encoding&");
      serialized_size.addArg("size", "size_t&");
      serialized_size.addArg("stru", "const KeyOnly<const " + cxx + ">");
      serialized_size.endArgs();

      be_global->impl_ <<
        "  switch (encoding.xcdr_version()) {\n";
      const char* indent = "    ";
      for (unsigned e = 0; e <= Encoding::KIND_UNALIGNED_CDR; ++e) {
        string expr;
        Intro intro;
        const Encoding::Kind encoding = static_cast<Encoding::Kind>(e);
        if (!iterate_over_keys(indent, encoding, node, cxx, info, 0,
              serialized_size_iteration, 0, &expr, &intro)) {
          return false;
        }
        be_global->impl_ <<
          "  case " << encoding_to_xcdr_version(encoding) << ":\n"
          "    {\n";
        intro.join(be_global->impl_, indent);
        be_global->impl_
          << expr <<
          "    break;\n"
          "    }\n";
      }
      be_global->impl_ <<
        "  }\n";
    }

    {
      Function insertion("operator<<", "bool");
      insertion.addArg("strm", "Serializer&");
      insertion.addArg("stru", "KeyOnly<const " + cxx + ">");
      insertion.endArgs();

      bool first = true;
      std::string expr;
      Intro intro;
      const char* indent = "  ";

      IDL_GlobalData::DCPS_Data_Type_Info_Iter iter(info->key_list_);
      for (ACE_TString* kp = 0; iter.next(kp) != 0; iter.advance()) {
        const string key_name = ACE_TEXT_ALWAYS_CHAR(kp->c_str());
        AST_Type* const field_type = find_type(node, key_name);
        if (first) {
          first = false;
        } else {
          expr += "\n    && ";
        }
        expr += streamCommon(indent, 0, key_name, field_type, "<< stru.value", false, intro);
      }

      intro.join(be_global->impl_, indent);
      be_global->impl_ << "  return " << (first ? "true" : expr) << ";\n";
    }

    {
      Function extraction("operator>>", "bool");
      extraction.addArg("strm", "Serializer&");
      extraction.addArg("stru", "KeyOnly<" + cxx + ">");
      extraction.endArgs();

      bool first = true;
      Intro intro;
      std::string expr;
      const std::string indent = "  ";

      IDL_GlobalData::DCPS_Data_Type_Info_Iter iter(info->key_list_);
      for (ACE_TString* kp = 0; iter.next(kp) != 0; iter.advance()) {
        const string key_name = ACE_TEXT_ALWAYS_CHAR(kp->c_str());
        AST_Type* const field_type = find_type(node, key_name);
        if (first) {
          first = false;
        } else {
          expr += "\n    && ";
        }
        expr += streamCommon(indent, 0, key_name, field_type, ">> stru.value", false, intro);
      }

      intro.join(be_global->impl_, indent);
      be_global->impl_ << "  return " << (first ? "true" : expr) << ";\n";
    }
  }

  return true;
}

void
marshal_generator::gen_field_getValueFromSerialized(AST_Structure* node, const std::string& clazz)
{
  //loop through meta struct
  //check the id for a match to our id
  //if we are not a match, skip to the next field
  //if we are a match, deserialize the field
  const bool use_cxx11 = be_global->language_mapping() == BE_GlobalData::LANGMAP_CXX11;
  std::string expr;
  const ExtensibilityKind exten = be_global->extensibility(node);
  const bool not_final = exten != extensibilitykind_final;
  const bool is_mutable = exten == extensibilitykind_mutable;
  const std::string actual_cpp_name = scoped(node->name());
  std::string cpp_name = actual_cpp_name;
  const Fields fields(node);
  const Fields::Iterator fields_end = fields.end();
  RtpsFieldCustomizer rtpsCustom(cpp_name);

  be_global->impl_ <<
    "  Value getValue(Serializer& strm, const char* field, const TypeSupportImpl* = 0) const\n"
    "  {\n"
    "    const Encoding& encoding = strm.encoding();\n"
    "    ACE_UNUSED_ARG(encoding);\n";
  marshal_generator::generate_dheader_code(
    "      if (!strm.read_delimiter(total_size)) {\n"
    "        throw std::runtime_error(\"Unable to reader delimiter in getValue\");\n"
    "      }\n", not_final, true, "    ");
  be_global->impl_ <<
    "    std::string base_field = field;\n"
    "    const size_t index = base_field.find('.');\n"
    "    std::string subfield;\n"
    "    if (index != std::string::npos) {\n"
    "      subfield = base_field.substr(index + 1);\n"
    "      base_field = base_field.substr(0, index);\n"
    "    }\n";

  if (is_mutable) {
    be_global->impl_ <<
      "    if (encoding.xcdr_version() != Encoding::XCDR_VERSION_NONE) {\n"
      "      unsigned field_id = map_name_to_id(base_field.c_str());\n"
      "      ACE_UNUSED_ARG(field_id);\n"
      "      unsigned member_id;\n"
      "      size_t field_size;\n"
      "      const size_t end_of_struct = strm.rpos() + total_size;\n"
      "      while (true) {\n"
      "        if (encoding.xcdr_version() == Encoding::XCDR_VERSION_2 &&\n"
      "            strm.rpos() >= end_of_struct) {\n"
      "          break;\n"
      "        }\n"
      "        bool must_understand = false;\n"
      "        if (!strm.read_parameter_id(member_id, field_size, must_understand)) {\n"
      "          throw std::runtime_error(\"Field \" + OPENDDS_STRING(field) + \" Deserialization "
      "Error for struct " << clazz << "\");\n"
      "        }\n"
      "        if (encoding.xcdr_version() == Encoding::XCDR_VERSION_1 &&\n"
      "            member_id == Serializer::pid_list_end) {\n"
      "          throw std::runtime_error(\"Field \" + OPENDDS_STRING(field) + \" not "
      "valid for struct " << clazz << "\");\n"
      "        }\n"
      "        const size_t end_of_field = strm.rpos() + field_size;\n"
      "        ACE_UNUSED_ARG(end_of_field);\n"
      "\n";

    std::ostringstream cases;
    for (Fields::Iterator i = fields.begin(); i != fields_end; ++i) {
      AST_Field* const field = *i;
      size_t size = 0;
      const OpenDDS::XTypes::MemberId id = be_global->get_id(field);
      std::string field_name = field->local_name()->get_string();
      AST_Type* const field_type = resolveActualType(field->field_type());
      const Classification fld_cls = classify(field_type);

      cases << "        case " << id << ": {\n";
      if (fld_cls & CL_SCALAR) {
        const std::string cxx_type = to_cxx_type(field_type, size);
        const std::string val = (fld_cls & CL_STRING) ? (use_cxx11 ? "val" : "val.out()")
          : getWrapper("val", field_type, WD_INPUT);
        std::string boundsCheck, transformPrefix, transformSuffix;
        if (fld_cls & CL_ENUM) {
          const std::string enumName = dds_generator::scoped_helper(field_type->name(), "_");
          boundsCheck = "            if (!gen_" + enumName + "_helper->valid(val)) {\n"
                        "              throw std::runtime_error(\"Enum value invalid\");\n"
                        "            }\n";
          transformPrefix = "gen_" + enumName + "_helper->get_name(";
          transformSuffix = ")";
        }
        cases <<
          "          if (field_id == member_id) {\n"
          "            " << cxx_type << " val;\n" <<
          "            if (!(strm >> " << val << ")) {\n"
          "              throw std::runtime_error(\"Field '" << field_name << "' could not be deserialized\");\n" <<
          "            }\n" <<
          boundsCheck <<
          "            return " << transformPrefix << "val" << transformSuffix << ";\n"
          "          } else {\n"
          "            strm.skip(field_size);\n"
          "          }\n"
          "          break;\n"
          "        }\n";
      } else if (fld_cls & CL_STRUCTURE) {
        cases <<
          "          if (field_id == member_id) {\n"
          "            return getMetaStruct<" << scoped(field_type->name()) << ">().getValue(strm, subfield.c_str());\n"
          "          } else {\n"
          "            strm.skip(field_size);\n"
          "          }\n"
          "          break;\n"
          "        }\n";
      } else { // array, sequence, union:
        cases <<
          "          strm.skip(field_size);\n"
          "          break;\n"
          "        }\n";
      }
    }
    const std::string switch_cases = cases.str();
    std::string sw_indent = "          ";
    if (switch_cases.empty()) {
      sw_indent = "        ";
    } else {
      be_global->impl_ <<
        "        switch (member_id) {\n"
        << switch_cases <<
        "        default:\n";
    }
    be_global->impl_ <<
      sw_indent << "if (must_understand) {\n" <<
      sw_indent << "  if (DCPS_debug_level >= 8) {\n" <<
      sw_indent << "    ACE_DEBUG((LM_DEBUG, ACE_TEXT(\"(%P|%t) unknown must_understand field(%u) in "
      << cpp_name << "\\n\"), member_id));\n" <<
      sw_indent << "  }\n" <<
      sw_indent << "  throw std::runtime_error(\"member id did not exist in getValue\");\n" <<
      sw_indent << "}\n" <<
      sw_indent << "strm.skip(field_size);\n";
    if (!switch_cases.empty()) {
      be_global->impl_ <<
        "          break;\n"
        "        }\n";
    }
    be_global->impl_ <<
      "      }\n"
      "      if (!field[0]) {\n"   // if 'field' is the empty string...
      "        return 0;\n"        //    the return value is ignored
      "      }\n"
      "      throw std::runtime_error(\"Did not find field in getValue\");\n"
      "    }\n";
  }
  //The following is Appendable/Final
  //It is also used when Mutable but not in XCDR1 or XCDR2
  expr = "";
  for (Fields::Iterator i = fields.begin(); i != fields_end; ++i) {
    AST_Field* const field = *i;
    size_t size = 0;
    const std::string idl_name = canonical_name(field);
    AST_Type* const field_type = resolveActualType(field->field_type());
    const Classification fld_cls = classify(field_type);
    if (fld_cls & CL_SCALAR) {
      const std::string cxx_type = to_cxx_type(field_type, size);
      const std::string val = (fld_cls & CL_STRING) ? (use_cxx11 ? "val" : "val.out()")
          : getWrapper("val", field_type, WD_INPUT);
      std::string boundsCheck, transformPrefix, transformSuffix;
      if (fld_cls & CL_ENUM) {
        const std::string enumName = dds_generator::scoped_helper(field_type->name(), "_");
        boundsCheck = "      if (!gen_" + enumName + "_helper->valid(val)) {\n"
                      "        throw std::runtime_error(\"Enum value invalid\");\n"
                      "      }\n";
        transformPrefix = "gen_" + enumName + "_helper->get_name(";
        transformSuffix = ")";
      }
      expr +=
        "    if (base_field == \"" + idl_name + "\") {\n"
        "      " + cxx_type + " val;\n"
        "      if (!(strm >> " + val + ")) {\n"
        "        throw std::runtime_error(\"Field '" + idl_name + "' could "
        "not be deserialized\");\n"
        "      }\n"
        + boundsCheck +
        "      return " + transformPrefix + "val" + transformSuffix + ";\n"
        "    } else {\n";
      if (fld_cls & CL_STRING) {
        expr +=
          "      ACE_CDR::ULong len;\n"
          "      if (!(strm >> len)) {\n"
          "        throw std::runtime_error(\"String '" + idl_name +
          "' length could not be deserialized\");\n"
          "      }\n"
          "      if (!strm.skip(len)) {\n"
          "        throw std::runtime_error(\"String '" + idl_name +
          "' contents could not be skipped\");\n"
          "      }\n"
          "    }\n";
      } else {
        expr +=
          "      if (!strm.skip(1,  " + OpenDDS::DCPS::to_dds_string(size) + " )) {\n"
          "        throw std::runtime_error(\"Field '" + idl_name +
          "' could not be skipped\");\n"
          "      }\n"
          "    }\n";
      }
    } else if (fld_cls & CL_STRUCTURE) {
        expr +=
          "    if (base_field == \"" + idl_name + "\") {\n"
          "      return getMetaStruct<" + scoped(field_type->name()) + ">().getValue(strm, subfield.c_str());\n"
          "    } else {\n"
          "      if (!gen_skip_over(strm, static_cast<" + scoped(field_type->name()) + "*>(0))) {\n"
          "        throw std::runtime_error(\"Field '" + idl_name + "' could not be skipped\");\n"
          "      }\n"
          "    }\n";
    } else { // array, sequence, union:
      std::string pre, post;
      if (!use_cxx11 && (fld_cls & CL_ARRAY)) {
        post = "_forany";
      } else if (use_cxx11 && (fld_cls & (CL_ARRAY | CL_SEQUENCE))) {
        pre = "IDL::DistinctType<";
        post = ", " + dds_generator::get_tag_name(scoped(deepest_named_type(field->field_type())->name())) + ">";
      }
      const std::string ptr = field->field_type()->anonymous() ?
        FieldInfo(*field).ptr_ : (pre + field_type_name(field) + post + '*');
      expr +=
        "    if (!gen_skip_over(strm, static_cast<" + ptr + ">(0))) {\n"
        "      throw std::runtime_error(\"Field \" + OPENDDS_STRING(field) + \" could not be skipped\");\n"
        "    }\n";
    }
  }
  be_global->impl_ <<
    expr <<
    "    if (!field[0]) {\n"   // if 'field' is the empty string...
    "      return 0;\n"        //    the return value is ignored
    "    }\n"
    "    throw std::runtime_error(\"Did not find field in getValue\");\n"
    "  }\n\n";
}

namespace {
  bool genRtpsParameter(const string&, AST_Union* u, AST_Type* discriminator,
                        const std::vector<AST_UnionBranch*>& branches)
  {
    const string cxx = RtpsNamespace + "Parameter";
    {
      Function serialized_size("serialized_size", "void");
      serialized_size.addArg("encoding", "const Encoding&");
      serialized_size.addArg("size", "size_t&");
      serialized_size.addArg("uni", "const " + cxx + "&");
      serialized_size.endArgs();
      generateSwitchForUnion(u, "uni._d()", findSizeCommon, branches,
                             discriminator, "", "", cxx.c_str());
      be_global->impl_ <<
        "  if (uni._d() == RTPS::PID_XTYPES_TYPE_INFORMATION) {\n"
        "    // Parameter union uses OctetSeq but this is not actually a sequence\n"
        "    size -= 4;\n"
        "  }\n"
        "  size += 4; // parameterId & length\n";
    }
    {
      Function insertion("operator<<", "bool");
      insertion.addArg("strm", "Serializer&");
      insertion.addArg("uni", "const " + cxx + "&");
      insertion.endArgs();
      be_global->impl_ <<
        "  if (!(strm << uni._d())) {\n"
        "    return false;\n"
        "  }\n"
        "  size_t size = serialized_size(strm.encoding(), uni);\n"
        "  size -= 4; // parameterId & length\n"
        "  const size_t post_pad = 4 - (size % 4);\n"
        "  const size_t total = size + ((post_pad < 4) ? post_pad : 0);\n"
        "  if (size > ACE_UINT16_MAX || !(strm << ACE_CDR::UShort(total))) {\n"
        "    return false;\n"
        "  }\n"
        "  const Serializer::ScopedAlignmentContext sac(strm);\n"
        "  if (uni._d() == RTPS::PID_XTYPES_TYPE_INFORMATION) {\n"
        "    if (!strm.write_octet_array(uni.type_information().get_buffer(), uni.type_information().length())) {\n"
        "      return false;\n"
        "    }\n"
        "  } else if (!insertParamData(strm, uni)) {\n"
        "    return false;\n"
        "  }\n"
        "  if (post_pad < 4 && strm.encoding().alignment() != Encoding::ALIGN_NONE) {\n"
        "    static const ACE_CDR::Octet padding[3] = {0};\n"
        "    return strm.write_octet_array(padding, ACE_CDR::ULong(post_pad));\n"
        "  }\n"
        "  return true;\n";
    }
    {
      Function insertData("insertParamData", "bool");
      insertData.addArg("strm", "Serializer&");
      insertData.addArg("uni", "const " + cxx + "&");
      insertData.endArgs();
      generateSwitchForUnion(u, "uni._d()", streamCommon, branches, discriminator,
                             "return", "<< ", cxx.c_str());
    }
    {
      Function extraction("operator>>", "bool");
      extraction.addArg("strm", "Serializer&");
      extraction.addArg("uni", cxx + "&");
      extraction.endArgs();
      be_global->impl_ <<
        "  ACE_CDR::UShort disc, size;\n"
        "  if (!(strm >> disc) || !(strm >> size)) {\n"
        "    return false;\n"
        "  }\n"
        "  if (disc == OpenDDS::RTPS::PID_SENTINEL) {\n"
        "    uni.unknown_data(DDS::OctetSeq());\n"
        "    uni._d(OpenDDS::RTPS::PID_SENTINEL);\n"
        "    return true;\n"
        "  }\n"
        "  if (size > strm.length()) {\n"
        "    return false;\n"
        "  }\n"
        "  if (disc == RTPS::PID_PROPERTY_LIST) {\n"
        "    // support special case deserialization of DDS::PropertyQosPolicy\n"
        "    Message_Block_Ptr param(strm.trim(size));\n"
        "    strm.skip(size);\n"
        "    Serializer strm2(param.get(), Encoding(Encoding::KIND_XCDR1, strm.swap_bytes()));\n"
        "    ::DDS::PropertyQosPolicy tmp;\n"
        "    if (strm2 >> tmp) {\n"
        "      uni.property(tmp);\n"
        "      return true;\n"
        "    } else {\n"
        "      return false;\n"
        "    }\n"
        "  }\n"
        "  const Serializer::ScopedAlignmentContext sac(strm, size);\n"
        "  if (disc == RTPS::PID_XTYPES_TYPE_INFORMATION) {\n"
        "    DDS::OctetSeq type_info(size);\n"
        "    type_info.length(size);\n"
        "    if (!strm.read_octet_array(type_info.get_buffer(), size)) {\n"
        "      return false;\n"
        "    }\n"
        "    uni.type_information(type_info);\n"
        "    return true;\n"
        "  }\n"
        "  switch (disc) {\n";
      generateSwitchBody(u, streamCommon, branches, discriminator,
                         "", ">> ", cxx.c_str(), true);
      be_global->impl_ <<
        "  default:\n"
        "    {\n"
        "      uni.unknown_data(DDS::OctetSeq(size));\n"
        "      uni.unknown_data().length(size);\n"
        "      if (!strm.read_octet_array(uni.unknown_data().get_buffer(), size)) {\n"
        "        return false;\n"
        "      }\n"
        "      uni._d(disc);\n"
        "    }\n"
        "  }\n"
        "  return true;\n";
    }
    return true;
  }

  bool genRtpsSubmessage(const string&, AST_Union* u, AST_Type* discriminator,
                         const std::vector<AST_UnionBranch*>& branches)
  {
    const string cxx = RtpsNamespace + "Submessage";
    {
      Function serialized_size("serialized_size", "void");
      serialized_size.addArg("encoding", "const Encoding&");
      serialized_size.addArg("size", "size_t&");
      serialized_size.addArg("uni", "const " + cxx + "&");
      serialized_size.endArgs();
      generateSwitchForUnion(u, "uni._d()", findSizeCommon, branches,
                             discriminator, "", "", cxx.c_str());
    }
    {
      Function insertion("operator<<", "bool");
      insertion.addArg("strm", "Serializer&");
      insertion.addArg("uni", "const " + cxx + "&");
      insertion.endArgs();
      generateSwitchForUnion(u, "uni._d()", streamCommon, branches,
                             discriminator, "return", "<< ", cxx.c_str());
    }
    {
      Function insertion("operator>>", "bool");
      insertion.addArg("strm", "Serializer&");
      insertion.addArg("uni", cxx + "&");
      insertion.endArgs();
      be_global->impl_ << "  // unused\n  return false;\n";
    }
    return true;
  }

  void gen_union_key_serializers(AST_Union* node, FieldFilter kind)
  {
    const string cxx = scoped(node->name()); // name as a C++ class
    AST_Type* const discriminator = node->disc_type();
    const Classification disc_cls = classify(discriminator);
    const string key_only_wrap_out = getWrapper("uni.value._d()", discriminator, WD_OUTPUT);
    const ExtensibilityKind exten = be_global->extensibility(node);
    const bool not_final = exten != extensibilitykind_final;
    const bool nested_key_only = kind == FieldFilter_NestedKeyOnly;
    const string wrapper = kind == FieldFilter_KeyOnly ? "KeyOnly"
      : nested_key_only ? "NestedKeyOnly"
      : "<<error from " __FILE__ ":" OPENDDS_IDL_STR(__LINE__) ">>";
    const bool has_key = be_global->union_discriminator_is_key(node) || nested_key_only;

    {
      Function serialized_size("serialized_size", "void");
      serialized_size.addArg("encoding", "const Encoding&");
      serialized_size.addArg("size", "size_t&");
      serialized_size.addArg("uni", "const " + wrapper + "<const " + cxx + ">");
      serialized_size.endArgs();

      marshal_generator::generate_dheader_code("    serialized_size_delimiter(encoding, size);\n", not_final, false);

      if (has_key) {
        if (exten == extensibilitykind_mutable) {
          be_global->impl_ <<
            "  size_t mutable_running_total = 0;\n"
            "  serialized_size_parameter_id(encoding, size, mutable_running_total);\n";
        }

        if (disc_cls & CL_ENUM) {
          be_global->impl_ <<
            "  primitive_serialized_size_ulong(encoding, size);\n";
        } else {
          be_global->impl_ <<
            "  primitive_serialized_size(encoding, size, " << key_only_wrap_out << ");\n";
        }

        if (exten == extensibilitykind_mutable) {
          be_global->impl_ <<
            "  serialized_size_list_end_parameter_id(encoding, size, mutable_running_total);\n";
        }
      }
    }

    {
      Function insertion("operator<<", "bool");
      insertion.addArg("strm", "Serializer&");
      insertion.addArg("uni", wrapper + "<const " + cxx + ">");
      insertion.endArgs();

      be_global->impl_ <<
        "  const Encoding& encoding = strm.encoding();\n"
        "  ACE_UNUSED_ARG(encoding);\n";
      marshal_generator::generate_dheader_code(
        "    serialized_size(encoding, total_size, uni);\n"
        "    if (!strm.write_delimiter(total_size)) {\n"
        "      return false;\n"
        "    }\n", not_final);

      if (has_key) {
        // EMHEADER for discriminator
        if (exten == extensibilitykind_mutable) {
          be_global->impl_ <<
            "  size_t size = 0;\n";

          if (disc_cls & CL_ENUM) {
            be_global->impl_ <<
              "  primitive_serialized_size_ulong(encoding, size);\n";
          } else {
            be_global->impl_ <<
              "  primitive_serialized_size(encoding, size, " << key_only_wrap_out << ");\n";
          }

          be_global->impl_ <<
            "  if (!strm.write_parameter_id(XTypes::DISCRIMINATOR_SERIALIZED_ID, size)) {\n"
            "    return false;\n"
            "  }\n"
            "  size = 0;\n";
        }

        be_global->impl_ << streamAndCheck("<< " + key_only_wrap_out);
      }

      be_global->impl_
        << "  return true;\n";
    }

    {
      Function extraction("operator>>", "bool");
      extraction.addArg("strm", "Serializer&");
      extraction.addArg("uni", wrapper + "<" + cxx + ">");
      extraction.endArgs();

      // DHEADER
      be_global->impl_ <<
        "  const Encoding& encoding = strm.encoding();\n"
        "  ACE_UNUSED_ARG(encoding);\n";
      marshal_generator::generate_dheader_code(
        "    if (!strm.read_delimiter(total_size)) {\n"
        "      return false;\n"
        "    }\n", not_final);

      if (has_key) {
        if (exten == extensibilitykind_mutable) {
          // EMHEADER for discriminator
          be_global->impl_ <<
            "  unsigned member_id;\n"
            "  size_t field_size;\n"
            "  bool must_understand = false;\n"
            "  if (!strm.read_parameter_id(member_id, field_size, must_understand)) {\n"
            "    return false;\n"
            "  }\n";
        }

        be_global->impl_
          << "  " << scoped(discriminator->name()) << " disc;\n"
          << streamAndCheck(">> " + getWrapper("disc", discriminator, WD_INPUT))
          << "  uni.value._d(disc);\n";
      }

      be_global->impl_
        << "  return true;\n";
    }
  }
}

bool marshal_generator::gen_union(AST_Union* node, UTL_ScopedName* name,
   const std::vector<AST_UnionBranch*>& branches, AST_Type* discriminator,
   const char*)
{
  NamespaceGuard ng;
  be_global->add_include("dds/DCPS/Serializer.h");
  string cxx = scoped(name); // name as a C++ class
  const Classification disc_cls = classify(discriminator);

  FieldInfo::EleLenSet anonymous_seq_generated;
  for (size_t i = 0; i < branches.size(); ++i) {
    if (branches[i]->field_type()->anonymous()) {
      FieldInfo af(*branches[i]);
      if (af.arr_) {
        gen_anonymous_array(af);
      } else if (af.seq_ && af.is_new(anonymous_seq_generated)) {
        gen_anonymous_sequence(af);
      }
    }
  }

  const ExtensibilityKind exten = be_global->extensibility(node);
  const bool not_final = exten != extensibilitykind_final;

  {
    // Define the set_default function in the header and implementation file.
    const std::string varname("uni");
    Function set_default("set_default", "void", "");
    set_default.addArg(varname.c_str(), cxx + "&");
    set_default.endArgs();

    // Add a reference to the idl file, if the descriminator is user defined.
    AST_Type* disc_type = resolveActualType(discriminator);
    const Classification disc_cls = classify(disc_type);
    if (!disc_type->in_main_file() && disc_type->node_type() != AST_Decl::NT_pre_defined) {
      be_global->add_referenced(disc_type->file_name().c_str());
    }

    // Determine the default enum value
    ACE_CDR::ULong default_enum_val = 0;
    if (disc_cls & CL_ENUM) {
      AST_Enum* enu = dynamic_cast<AST_Enum*>(disc_type);
      UTL_ScopeActiveIterator i(enu, UTL_Scope::IK_decls);
      AST_EnumVal* item = dynamic_cast<AST_EnumVal*>(i.item());
      default_enum_val = item->constant_value()->ev()->u.eval;
      // This doesn't look at @value annotations since it's only needed to find
      // a matching branch label below -- the integer value of the enumerator
      // isn't used in generated code.
    }

    // Search the union branches to find the default value according to
    // Table 9 of the XTypes spec v1.3
    bool found = false;
    for (std::vector<AST_UnionBranch*>::const_iterator itr = branches.begin(); itr < branches.end() && !found; ++itr) {
      AST_UnionBranch* branch = *itr;
      for (unsigned i = 0; i < branch->label_list_length(); ++i) {
        AST_UnionLabel* ul = branch->label(i);
        if (ul->label_kind() != AST_UnionLabel::UL_default) {
          AST_Expression::AST_ExprValue* ev = ul->label_val()->ev();
          if ((ev->et == AST_Expression::EV_enum && ev->u.eval == default_enum_val) ||
#if OPENDDS_HAS_EXPLICIT_INTS
              (ev->et == AST_Expression::EV_uint8 && ev->u.uint8val == 0) ||
              (ev->et == AST_Expression::EV_int8 && ev->u.int8val == 0) ||
#endif
              (ev->et == AST_Expression::EV_short && ev->u.sval == 0) ||
              (ev->et == AST_Expression::EV_ushort && ev->u.usval == 0) ||
              (ev->et == AST_Expression::EV_long && ev->u.lval == 0) ||
              (ev->et == AST_Expression::EV_ulong && ev->u.ulval == 0) ||
              (ev->et == AST_Expression::EV_longlong && ev->u.llval == 0) ||
              (ev->et == AST_Expression::EV_ulonglong && ev->u.ullval == 0) ||
              (ev->et == AST_Expression::EV_float && ev->u.fval == 0) ||
              (ev->et == AST_Expression::EV_double && ev->u.dval == 0) ||
              (ev->et == AST_Expression::EV_longdouble && ev->u.sval == 0) ||
              (ev->et == AST_Expression::EV_char && ev->u.cval == 0) ||
              (ev->et == AST_Expression::EV_wchar && ev->u.wcval == 0) ||
              (ev->et == AST_Expression::EV_octet && ev->u.oval == 0) ||
              (ev->et == AST_Expression::EV_bool && ev->u.bval == 0)) {
            gen_union_default(branch, varname);
            found = true;
            break;
          }
        }
      }
    }

    // If a default value was not found, just set the discriminator to the
    // default value.
    if (!found) {
      be_global->impl_ <<
        " " << scoped(discriminator->name()) << " temp;\n" <<
        type_to_default("", discriminator, "  temp") <<
        "  " << varname << "._d(temp);\n";
    }
  }

  bool special_result;
  if (generate_special_union(cxx, node, discriminator, branches, special_result)) {
    return special_result;
  }

  const string wrap_out = getWrapper("uni._d()", discriminator, WD_OUTPUT);
  {
    Function serialized_size("serialized_size", "void");
    serialized_size.addArg("encoding", "const Encoding&");
    serialized_size.addArg("size", "size_t&");
    serialized_size.addArg("uni", "const " + cxx + "&");
    serialized_size.endArgs();

    marshal_generator::generate_dheader_code("    serialized_size_delimiter(encoding, size);\n", not_final, false);

    if (exten == extensibilitykind_mutable) {
      be_global->impl_ <<
        "  size_t mutable_running_total = 0;\n"
        "  serialized_size_parameter_id(encoding, size, mutable_running_total);\n";
    }

    if (disc_cls & CL_ENUM) {
      be_global->impl_ <<
        "  primitive_serialized_size_ulong(encoding, size);\n";
    } else {
      be_global->impl_ <<
        "  primitive_serialized_size(encoding, size, " << wrap_out << ");\n";
    }

    generateSwitchForUnion(node, "uni._d()",
                           exten == extensibilitykind_mutable ? findSizeMutableUnion : findSizeCommon,
                           branches, discriminator, "", "", cxx.c_str());

    if (exten == extensibilitykind_mutable) {
      // TODO: XTypes B will need to edit this code to add the pid for the end of mutable unions.
      // Until this change is made, XCDR1 will NOT be functional
      be_global->impl_ <<
        "  serialized_size_list_end_parameter_id(encoding, size, mutable_running_total);\n";
    }
  }
  {
    Function insertion("operator<<", "bool");
    insertion.addArg("strm", "Serializer&");
    insertion.addArg("uni", "const " + cxx + "&");
    insertion.endArgs();

    be_global->impl_ <<
      "  const Encoding& encoding = strm.encoding();\n"
      "  ACE_UNUSED_ARG(encoding);\n";
    marshal_generator::generate_dheader_code(
      "    serialized_size(encoding, total_size, uni);\n"
      "    if (!strm.write_delimiter(total_size)) {\n"
      "      return false;\n"
      "    }\n", not_final);

    // EMHEADER for discriminator
    if (exten == extensibilitykind_mutable) {
      be_global->impl_ <<
        "  size_t size = 0;\n";

      if (disc_cls & CL_ENUM) {
        be_global->impl_ <<
          "  primitive_serialized_size_ulong(encoding, size);\n";
      } else {
        be_global->impl_ <<
          "  primitive_serialized_size(encoding, size, " << wrap_out << ");\n";
      }

      be_global->impl_ <<
        "  if (!strm.write_parameter_id(XTypes::DISCRIMINATOR_SERIALIZED_ID, size)) {\n"
        "    return false;\n"
        "  }\n"
        "  size = 0;\n";
    }

    be_global->impl_ <<
      streamAndCheck("<< " + wrap_out);
    if (generateSwitchForUnion(node, "uni._d()", streamCommon, branches,
                               discriminator, "return", "<< ", cxx.c_str(),
                               false, true, true,
                               exten == extensibilitykind_mutable ? findSizeCommon : 0)) {
      be_global->impl_ <<
        "  return true;\n";
    }
  }
  {
    Function extraction("operator>>", "bool");
    extraction.addArg("strm", "Serializer&");
    extraction.addArg("uni", cxx + "&");
    extraction.endArgs();

    be_global->impl_ <<
      "  const Encoding& encoding = strm.encoding();\n"
      "  ACE_UNUSED_ARG(encoding);\n";
    marshal_generator::generate_dheader_code(
      "    if (!strm.read_delimiter(total_size)) {\n"
      "      return false;\n"
      "    }\n", not_final);

    if (exten == extensibilitykind_mutable) {
      // EMHEADER for discriminator
      be_global->impl_ <<
        "  unsigned member_id;\n"
        "  size_t field_size;\n"
        "  bool must_understand = false;\n"
        "  if (!strm.read_parameter_id(member_id, field_size, must_understand)) {\n"
        "    return false;\n"
        "  }\n";
      TryConstructFailAction try_construct = be_global->union_discriminator_try_construct(node);
      be_global->impl_ <<
        "  " << scoped(discriminator->name()) << " disc;\n"
        "  if (!(strm >> disc)) {\n";
      if (try_construct == tryconstructfailaction_use_default) {
        be_global->impl_ <<
          "    set_default(uni);\n"
          "    if (!strm.read_parameter_id(member_id, field_size, must_understand)) {\n"
          "      return false;\n"
          "    }\n"
          "    strm.skip(field_size);\n"
          "    strm.set_construction_status(Serializer::ConstructionSuccessful);\n"
          "    return true;\n";
      } else {
        be_global->impl_ <<
          "    if (!strm.read_parameter_id(member_id, field_size, must_understand)) {\n"
          "      return false;\n"
          "    }\n"
          "    strm.skip(field_size);\n"
          "    strm.set_construction_status(Serializer::ElementConstructionFailure);\n"
          "    return false;\n";
      }
      be_global->impl_ << "  }\n";

      be_global->impl_ <<
        "  member_id = 0;\n"
        "  field_size = 0;\n"
        "  must_understand = false;\n";

      const char prefix[] =
        "    if (!strm.read_parameter_id(member_id, field_size, must_understand)) {\n"
        "      return false;\n"
        "    }\n";
      if (generateSwitchForUnion(node, "disc", streamCommon, branches,
                                 discriminator, prefix, ">> ", cxx.c_str())) {
        be_global->impl_ <<
          "  return true;\n";
      }
    } else {
      be_global->impl_ <<
        "  " << scoped(discriminator->name()) << " disc;\n" <<
        streamAndCheck(">> " + getWrapper("disc", discriminator, WD_INPUT));
      if (generateSwitchForUnion(node, "disc", streamCommon, branches,
          discriminator, "", ">> ", cxx.c_str())) {
        be_global->impl_ <<
          "  return true;\n";
      }
    }
  }

  gen_union_key_serializers(node, FieldFilter_NestedKeyOnly);
  if (be_global->is_topic_type(node)) {
    gen_union_key_serializers(node, FieldFilter_KeyOnly);
  }

  TopicKeys keys(node);
  return generate_marshal_traits(node, cxx, exten, keys);
}

void marshal_generator::gen_union_default(AST_UnionBranch* branch, const std::string& varname)
{
  AST_Type* br = resolveActualType(branch->field_type());
  const Classification br_cls = classify(br);
  const std::string tmpname = "tmp";

  if (br_cls & (CL_SEQUENCE | CL_ARRAY | CL_STRUCTURE | CL_UNION)) {
    be_global->impl_ << " " << scoped(branch->field_type()->name()) << " "
                     << getWrapper(tmpname, branch->field_type(), WD_INPUT) << ";\n";
  }

  if (br_cls & (CL_STRUCTURE | CL_UNION)) {
    be_global->impl_ << type_to_default("  ", branch->field_type(), tmpname);
    be_global->impl_ << "  " << varname << "." << branch->local_name()->get_string() << "(tmp);\n";
  } else {
    be_global->impl_ << type_to_default("  ", branch->field_type(),
                                        varname + "." + branch->local_name()->get_string(),
                                        false, true);
  }
}
