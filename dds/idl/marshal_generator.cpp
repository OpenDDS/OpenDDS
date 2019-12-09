/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "marshal_generator.h"
#include "utl_identifier.h"
#include "topic_keys.h"

#include <string>
#include <sstream>
#include <iostream>
#include <cctype>
#include <map>

using std::string;
using namespace AstTypeClassification;

#define LENGTH(CARRAY) (sizeof(CARRAY)/sizeof(CARRAY[0]))

namespace {
  typedef bool (*is_special_case)(const string& cxx);
  typedef bool (*gen_special_case)(const string& cxx);

  typedef is_special_case is_special_sequence;
  typedef gen_special_case gen_special_sequence;

  typedef is_special_case is_special_struct;
  typedef gen_special_case gen_special_struct;

  typedef is_special_case is_special_union;
  typedef bool (*gen_special_union)(const string& cxx,
                                    AST_Type* discriminator,
                                    const std::vector<AST_UnionBranch*>& branches);

  struct special_sequence
  {
    is_special_sequence check;
    gen_special_sequence gen;
  };

  struct special_struct
  {
    is_special_struct check;
    gen_special_struct gen;
  };

  struct special_union
  {
    is_special_union check;
    gen_special_union gen;
  };

  bool isRtpsSpecialSequence(const string& cxx);
  bool genRtpsSpecialSequence(const string& cxx);

  bool isPropertySpecialSequence(const string& cxx);
  bool genPropertySpecialSequence(const string& cxx);

  bool isRtpsSpecialStruct(const string& cxx);
  bool genRtpsSpecialStruct(const string& cxx);

  bool isRtpsSpecialUnion(const string& cxx);
  bool genRtpsSpecialUnion(const string& cxx,
                           AST_Type* discriminator,
                           const std::vector<AST_UnionBranch*>& branches);

  bool isProperty_t(const string& cxx);
  bool genProperty_t(const string& cxx);

  bool isBinaryProperty_t(const string& cxx);
  bool genBinaryProperty_t(const string& cxx);

  bool isPropertyQosPolicy(const string& cxx);
  bool genPropertyQosPolicy(const string& cxx);

  bool isSecuritySubmessage(const string& cxx);
  bool genSecuritySubmessage(const string& cxx);

  const special_sequence special_sequences[] = {
    {
      isRtpsSpecialSequence,
      genRtpsSpecialSequence,
    },
    {
      isPropertySpecialSequence,
      genPropertySpecialSequence,
    },
  };

  const special_struct special_structs[] = {
    {
      isRtpsSpecialStruct,
      genRtpsSpecialStruct,
    },
    {
      isProperty_t,
      genProperty_t,
    },
    {
      isBinaryProperty_t,
      genBinaryProperty_t,
    },
    {
      isPropertyQosPolicy,
      genPropertyQosPolicy,
    },
    {
      isSecuritySubmessage,
      genSecuritySubmessage,
    },
  };

  const special_union special_unions[] = {
    {
      isRtpsSpecialUnion,
      genRtpsSpecialUnion,
    },
  };

} /* namespace */

bool marshal_generator::gen_enum(AST_Enum*, UTL_ScopedName* name,
  const std::vector<AST_EnumVal*>&, const char*)
{
  NamespaceGuard ng;
  be_global->add_include("dds/DCPS/Serializer.h");
  string cxx = scoped(name); // name as a C++ class
  {
    Function insertion("operator<<", "bool");
    insertion.addArg("strm", "Serializer&");
    insertion.addArg("enumval", "const " + cxx + "&");
    insertion.endArgs();
    be_global->impl_ <<
      "  return strm << static_cast<CORBA::ULong>(enumval);\n";
  }
  {
    Function extraction("operator>>", "bool");
    extraction.addArg("strm", "Serializer&");
    extraction.addArg("enumval", cxx + "&");
    extraction.endArgs();
    be_global->impl_ <<
      "  CORBA::ULong temp = 0;\n"
      "  if (strm >> temp) {\n"
      "    enumval = static_cast<" << cxx << ">(temp);\n"
      "    return true;\n"
      "  }\n"
      "  return false;\n";
  }
  return true;
}

namespace {

  string getMaxSizeExprPrimitive(AST_Type* type)
  {
    if (type->node_type() != AST_Decl::NT_pre_defined) {
      return "";
    }
    AST_PredefinedType* pt = AST_PredefinedType::narrow_from_decl(type);
    switch (pt->pt()) {
    case AST_PredefinedType::PT_octet:
      return "max_marshaled_size_octet()";
    case AST_PredefinedType::PT_char:
      return "max_marshaled_size_char()";
    case AST_PredefinedType::PT_wchar:
      return "max_marshaled_size_wchar()";
    case AST_PredefinedType::PT_boolean:
      return "max_marshaled_size_boolean()";
    default:
      return "gen_max_marshaled_size(" + scoped(type->name()) + "())";
    }
  }

  string getSerializerName(AST_Type* type)
  {
    switch (AST_PredefinedType::narrow_from_decl(type)->pt()) {
    case AST_PredefinedType::PT_long:
      return "long";
    case AST_PredefinedType::PT_ulong:
      return "ulong";
    case AST_PredefinedType::PT_short:
      return "short";
    case AST_PredefinedType::PT_ushort:
      return "ushort";
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
    switch (AST_PredefinedType::narrow_from_decl(elem)->pt()) {
    case AST_PredefinedType::PT_longlong:
    case AST_PredefinedType::PT_ulonglong:
    case AST_PredefinedType::PT_double:
    case AST_PredefinedType::PT_longdouble:
      return
        "  if ((size + padding) % 8) {\n"
        "    padding += 4;\n"
        "  }\n";
    default:
      return "";
    }
  }

  bool isRtpsSpecialSequence(const string& cxx)
  {
    return cxx == "OpenDDS::RTPS::ParameterList";
  }

  bool genRtpsSpecialSequence(const string& cxx)
  {
    {
      Function find_size("gen_find_size", "void");
      find_size.addArg("seq", "const " + cxx + "&");
      find_size.addArg("size", "size_t&");
      find_size.addArg("padding", "size_t&");
      find_size.endArgs();
      be_global->impl_ <<
        "  for (CORBA::ULong i = 0; i < seq.length(); ++i) {\n"
        "    if (seq[i]._d() == OpenDDS::RTPS::PID_SENTINEL) continue;\n"
        "    size_t param_size = 0, param_padding = 0;\n"
        "    gen_find_size(seq[i], param_size, param_padding);\n"
        "    size += param_size + param_padding;\n"
        "    if (size % 4) {\n"
        "      size += 4 - (size % 4);\n"
        "    }\n"
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
        "    const CORBA::ULong len = seq.length();\n"
        "    seq.length(len + 1);\n"
        "    if (!(strm >> seq[len])) {\n"
        "      return false;\n"
        "    }\n"
        "    if (seq[len]._d() == OpenDDS::RTPS::PID_SENTINEL) {\n"
        "      seq.length(len);\n"
        "      return true;\n"
        "    }\n"
        "  }\n";
    }
    return true;
  }

  bool isPropertySpecialSequence(const string& cxx)
  {
    return cxx == "DDS::PropertySeq"
      || cxx == "DDS::BinaryPropertySeq";
  }

  bool genPropertySpecialSequence(const string& cxx)
  {
    {
      Function find_size("gen_find_size", "void");
      find_size.addArg("seq", "const " + cxx + "&");
      find_size.addArg("size", "size_t&");
      find_size.addArg("padding", "size_t&");
      find_size.endArgs();
      be_global->impl_ <<
        "  find_size_ulong(size, padding);\n"
        "  for (CORBA::ULong i = 0; i < seq.length(); ++i) {\n"
        "    gen_find_size(seq[i], size, padding);\n"
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

  std::string bounded_arg(AST_Type* type)
  {
    std::ostringstream arg;
    const Classification cls = classify(type);
    if (cls & CL_STRING) {
      AST_String* const str = AST_String::narrow_from_decl(type);
      arg << str->max_size()->ev()->u.ulval;
    } else if (cls & CL_SEQUENCE) {
      AST_Sequence* const seq = AST_Sequence::narrow_from_decl(type);
      arg << seq->max_size()->ev()->u.ulval;
    }
    return arg.str();
  }

  void gen_sequence(UTL_ScopedName* tdname, AST_Sequence* seq)
  {
    be_global->add_include("dds/DCPS/Serializer.h");
    NamespaceGuard ng;
    string cxx = scoped(tdname);

    for (size_t i = 0; i < LENGTH(special_sequences); ++i) {
      if (special_sequences[i].check(cxx)) {
        special_sequences[i].gen(cxx);
        return;
      }
    }

    AST_Type* elem = resolveActualType(seq->base_type());
    Classification elem_cls = classify(elem);
    if (!elem->in_main_file()) {
      if (elem->node_type() == AST_Decl::NT_pre_defined) {
        if (be_global->language_mapping() != BE_GlobalData::LANGMAP_FACE_CXX &&
            be_global->language_mapping() != BE_GlobalData::LANGMAP_SP_CXX) {
          be_global->add_include(("dds/CorbaSeq/" + nameOfSeqHeader(elem)
                                  + "SeqTypeSupportImpl.h").c_str(), BE_GlobalData::STREAM_CPP);
        }
      } else {
        be_global->add_referenced(elem->file_name().c_str());
      }
    }

    const string cxx_elem = scoped(seq->base_type()->name()),
      elem_underscores = dds_generator::scoped_helper(seq->base_type()->name(), "_");
    const bool use_cxx11 = be_global->language_mapping() == BE_GlobalData::LANGMAP_CXX11;
    const string check_empty = use_cxx11 ? "seq.empty()" : "seq.length() == 0";
    const string get_length = use_cxx11 ? "static_cast<uint32_t>(seq.size())" : "seq.length()";
    const string get_buffer = use_cxx11 ? "seq.data()" : "seq.get_buffer()";
    string const_cxx = cxx, unwrap, const_unwrap;
    if (use_cxx11) {
      const string underscores = dds_generator::scoped_helper(tdname, "_");
      be_global->header_ <<
        "struct " << underscores << "_tag {};\n\n";
      unwrap = "  " + cxx + "& seq = wrap;\n  ACE_UNUSED_ARG(seq);\n";
      const_unwrap = "  const " + cxx + "& seq = wrap;\n  ACE_UNUSED_ARG(seq);\n";
      const_cxx = "IDL::DistinctType<const " + cxx + ", " + underscores + "_tag>";
      cxx = "IDL::DistinctType<" + cxx + ", " + underscores + "_tag>";
    } else {
      const_cxx = "const " + cxx + '&';
      cxx += '&';
    }

    {
      Function find_size("gen_find_size", "void");
      find_size.addArg(use_cxx11 ? "wrap" : "seq", const_cxx);
      find_size.addArg("size", "size_t&");
      find_size.addArg("padding", "size_t&");
      find_size.endArgs();
      be_global->impl_ << const_unwrap <<
        "  find_size_ulong(size, padding);\n"
        "  if (" << check_empty << ") {\n"
        "    return;\n"
        "  }\n";
      if (elem_cls & CL_ENUM) {
        be_global->impl_ <<
          "  size += " << get_length << " * max_marshaled_size_ulong();\n";
      } else if (elem_cls & CL_PRIMITIVE) {
        be_global->impl_ << checkAlignment(elem) <<
          "  size += " << get_length << " * " << getMaxSizeExprPrimitive(elem) << ";\n";
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
            "    find_size_ulong(size, padding);\n";
          const string strlen_suffix = (elem_cls & CL_WIDE)
            ? " * OpenDDS::DCPS::Serializer::WCHAR_SIZE;\n"
            : " + 1;\n";
          if (use_cxx11) {
            be_global->impl_ <<
              "    size += seq[i].size()" << strlen_suffix;
          } else {
            be_global->impl_ <<
              "    if (seq[i]) {\n"
              "      size += ACE_OS::strlen(seq[i])" << strlen_suffix <<
              "    }\n";
          }
        } else if (!use_cxx11 && (elem_cls & CL_ARRAY)) {
          be_global->impl_ <<
            "    " << cxx_elem << "_var tmp_var = " << cxx_elem
            << "_dup(seq[i]);\n"
            "    " << cxx_elem << "_forany tmp = tmp_var.inout();\n"
            "    gen_find_size(tmp, size, padding);\n";
        } else if (use_cxx11 && (elem_cls & (CL_ARRAY | CL_SEQUENCE))) {
          be_global->impl_ <<
            "    gen_find_size(IDL::DistinctType<const " << cxx_elem << ", " <<
            elem_underscores << "_tag>(seq[i]), size, padding);\n";
        } else { // Struct, Union, non-C++11 Sequence
          be_global->impl_ <<
            "    gen_find_size(seq[i], size, padding);\n";
        }
        be_global->impl_ <<
          "  }\n";
      }
    }
    {
      Function insertion("operator<<", "bool");
      insertion.addArg("strm", "Serializer&");
      insertion.addArg(use_cxx11 ? "wrap" : "seq", const_cxx);
      insertion.endArgs();
      be_global->impl_ << const_unwrap <<
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
      if (elem_cls & CL_PRIMITIVE) {
        AST_PredefinedType* predef = AST_PredefinedType::narrow_from_decl(elem);
        if (use_cxx11 && predef->pt() == AST_PredefinedType::PT_boolean) {
          be_global->impl_ <<
            "  for (CORBA::ULong i = 0; i < length; ++i) {\n" <<
            streamAndCheck("<< ACE_OutputCDR::from_boolean(seq[i])", 4) <<
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
        if (!use_cxx11 && (elem_cls & CL_ARRAY)) {
          const string typedefname = scoped(seq->base_type()->name());
          be_global->impl_ <<
            "    " << typedefname << "_var tmp_var = " << typedefname
            << "_dup(seq[i]);\n"
            "    " << typedefname << "_forany tmp = tmp_var.inout();\n"
            << streamAndCheck("<< tmp", 4);
        } else if ((elem_cls & (CL_STRING | CL_BOUNDED)) == (CL_STRING | CL_BOUNDED)) {
          const string args = "seq[i], " + bounded_arg(elem);
          be_global->impl_ <<
            streamAndCheck("<< " + getWrapper(args, elem, WD_OUTPUT), 4);
        } else if (use_cxx11 && (elem_cls & (CL_ARRAY | CL_SEQUENCE))) {
          be_global->impl_ <<
            streamAndCheck("<< IDL::DistinctType<const " + cxx_elem + ", " +
                           elem_underscores + "_tag>(seq[i])", 4);
        } else {
          be_global->impl_ << streamAndCheck("<< seq[i]", 4);
        }
        be_global->impl_ <<
          "  }\n"
          "  return true;\n";
      }
    }
    {
      Function extraction("operator>>", "bool");
      extraction.addArg("strm", "Serializer&");
      extraction.addArg(use_cxx11 ? "wrap" : "seq", cxx);
      extraction.endArgs();
      be_global->impl_ << unwrap <<
        "  CORBA::ULong length;\n"
        << streamAndCheck(">> length");
      if (!seq->unbounded()) {
        be_global->impl_ <<
          "  if (length > " << (use_cxx11 ? bounded_arg(seq) : "seq.maximum()") << ") {\n"
          "    return false;\n"
          "  }\n";
      }
      be_global->impl_ <<
        (use_cxx11 ? "  seq.resize(length);\n" : "  seq.length(length);\n");
      if (elem_cls & CL_PRIMITIVE) {
        AST_PredefinedType* predef = AST_PredefinedType::narrow_from_decl(elem);
        if (use_cxx11 && predef->pt() == AST_PredefinedType::PT_boolean) {
          be_global->impl_ <<
            "  for (CORBA::ULong i = 0; i < length; ++i) {\n"
            "    bool b;\n" <<
            streamAndCheck(">> ACE_InputCDR::to_boolean(b)", 4) <<
            "    seq[i] = b;\n"
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
      } else if (elem_cls & CL_INTERFACE) {
        be_global->impl_ <<
          "  return false; // sequence of objrefs is not marshaled\n";
      } else if (elem_cls == CL_UNKNOWN) {
        be_global->impl_ <<
          "  return false; // sequence of unknown/unsupported type\n";
      } else { // Enum, String, Struct, Array, Sequence, Union
        be_global->impl_ <<
          "  for (CORBA::ULong i = 0; i < length; ++i) {\n";
        if (!use_cxx11 && (elem_cls & CL_ARRAY)) {
          const string typedefname = scoped(seq->base_type()->name());
          be_global->impl_ <<
            "    " << typedefname << "_var tmp = " << typedefname
            << "_alloc();\n"
            "    " << typedefname << "_forany fa = tmp.inout();\n"
            << streamAndCheck(">> fa", 4) <<
            "    " << typedefname << "_copy(seq[i], tmp.in());\n";
        } else if (elem_cls & CL_STRING) {
          if (elem_cls & CL_BOUNDED) {
            const string args = string("seq[i]") + (use_cxx11 ? ", " : ".out(), ")
              + bounded_arg(elem);
            be_global->impl_ <<
              streamAndCheck(">> " + getWrapper(args, elem, WD_INPUT), 4);
          } else { // unbounded string
            const string getbuffer =
              (be_global->language_mapping() == BE_GlobalData::LANGMAP_NONE)
              ? ".get_buffer()" : "";
            be_global->impl_ << streamAndCheck(">> seq" + getbuffer + "[i]", 4);
          }
        } else if (use_cxx11 && (elem_cls & (CL_ARRAY | CL_SEQUENCE))) {
          be_global->impl_ <<
            streamAndCheck(">> IDL::DistinctType<" + cxx_elem + ", " +
                           elem_underscores + "_tag>(seq[i])", 4);
        } else { // Enum, Struct, Union, non-C++11 Array, non-C++11 Sequence
          be_global->impl_ << streamAndCheck(">> seq[i]", 4);
        }
        be_global->impl_ <<
          "  }\n"
          "  return true;\n";
      }
    }
  }

  string getAlignment(AST_Type* elem)
  {
    if (elem->node_type() == AST_Decl::NT_enum) {
      return "4";
    }
    switch (AST_PredefinedType::narrow_from_decl(elem)->pt()) {
    case AST_PredefinedType::PT_short:
    case AST_PredefinedType::PT_ushort:
      return "2";
    case AST_PredefinedType::PT_long:
    case AST_PredefinedType::PT_ulong:
    case AST_PredefinedType::PT_float:
      return "4";
    case AST_PredefinedType::PT_longlong:
    case AST_PredefinedType::PT_ulonglong:
    case AST_PredefinedType::PT_double:
    case AST_PredefinedType::PT_longdouble:
      return "8";
    default:
      return "";
    }
  }

  void gen_array(UTL_ScopedName* name, AST_Array* arr)
  {
    be_global->add_include("dds/DCPS/Serializer.h");
    NamespaceGuard ng;
    const bool use_cxx11 = be_global->language_mapping() == BE_GlobalData::LANGMAP_CXX11;
    string cxx = scoped(name);
    string const_cxx = cxx, unwrap, const_unwrap;
    if (use_cxx11) {
      const string underscores = dds_generator::scoped_helper(name, "_");
      be_global->header_ <<
        "struct " << underscores << "_tag {};\n\n";
      unwrap = "  " + cxx + "& arr = wrap;\n  ACE_UNUSED_ARG(arr);\n";
      const_unwrap = "  const " + cxx + "& arr = wrap;\n  ACE_UNUSED_ARG(arr);\n";
      const_cxx = "IDL::DistinctType<const " + cxx + ", " + underscores + "_tag>";
      cxx = "IDL::DistinctType<" + cxx + ", " + underscores + "_tag>";
    } else {
      const_cxx = "const " + cxx + "_forany&";
      cxx += "_forany&";
    }

    AST_Type* elem = resolveActualType(arr->base_type());
    Classification elem_cls = classify(elem);
    if (!elem->in_main_file()
        && elem->node_type() != AST_Decl::NT_pre_defined) {
      be_global->add_referenced(elem->file_name().c_str());
    }
    string cxx_elem = scoped(arr->base_type()->name());
    size_t n_elems = 1;
    for (size_t i = 0; i < arr->n_dims(); ++i) {
      n_elems *= arr->dims()[i]->ev()->u.ulval;
    }
    {
      Function find_size("gen_find_size", "void");
      find_size.addArg(use_cxx11 ? "wrap" : "arr", const_cxx);
      find_size.addArg("size", "size_t&");
      find_size.addArg("padding", "size_t&");
      find_size.endArgs();
      be_global->impl_ << const_unwrap;
      if (elem_cls & CL_ENUM) {
        be_global->impl_ <<
          "  find_size_ulong(size, padding);\n";
          if (n_elems > 1) {
            be_global->impl_ <<
              "  size += " << n_elems - 1 << " * max_marshaled_size_ulong();\n";
          }
      } else if (elem_cls & CL_PRIMITIVE) {
        const string align = getAlignment(elem);
        if (!align.empty()) {
          be_global->impl_ <<
            "  if ((size + padding) % " << align << ") {\n"
            "    padding += " << align << " - ((size + padding) % " << align
            << ");\n"
            "  }\n";
        }
        be_global->impl_ <<
          "  size += " << n_elems << " * " << getMaxSizeExprPrimitive(elem)
          << ";\n";
      } else { // String, Struct, Array, Sequence, Union
        string indent = "  ";
        NestedForLoops nfl("CORBA::ULong", "i", arr, indent);
        if (elem_cls & CL_STRING) {
          be_global->impl_ <<
            indent << "find_size_ulong(size, padding);\n" <<
            indent;
          if (use_cxx11) {
            be_global->impl_ << "size += arr" << nfl.index_ << ".size()";
          } else {
            be_global->impl_ << "size += ACE_OS::strlen(arr" << nfl.index_ << ".in())";
          }
          be_global->impl_ << ((elem_cls & CL_WIDE)
            ? " * OpenDDS::DCPS::Serializer::WCHAR_SIZE;\n"
            : " + 1;\n");
        } else if (!use_cxx11 && (elem_cls & CL_ARRAY)) {
          be_global->impl_ <<
            indent << cxx_elem << "_var tmp_var = " << cxx_elem
            << "_dup(arr" << nfl.index_ << ");\n" <<
            indent << cxx_elem << "_forany tmp = tmp_var.inout();\n" <<
            indent << "gen_find_size(tmp, size, padding);\n";
        } else { // Struct, Sequence, Union, C++11 Array
          string pre, post;
          if (use_cxx11 && (elem_cls & (CL_ARRAY | CL_SEQUENCE))) {
            pre = "IDL::DistinctType<const " + cxx_elem + ", " +
              dds_generator::scoped_helper(arr->base_type()->name(), "_") + "_tag>(";
            post = ')';
          }
          be_global->impl_ <<
            indent << "gen_find_size(" << pre << "arr" << nfl.index_
            << post << ", size, padding);\n";
        }
      }
    }
    {
      Function insertion("operator<<", "bool");
      insertion.addArg("strm", "Serializer&");
      insertion.addArg(use_cxx11 ? "wrap" : "arr", const_cxx);
      insertion.endArgs();
      be_global->impl_ << const_unwrap;
      const std::string accessor = use_cxx11 ? ".data()" : ".in()";
      if (elem_cls & CL_PRIMITIVE) {
        string suffix;
        for (unsigned int i = 1; i < arr->n_dims(); ++i)
          suffix += use_cxx11 ? accessor : "[0]";
        be_global->impl_ <<
          "  return strm.write_" << getSerializerName(elem)
          << "_array(arr" << accessor << suffix << ", " << n_elems << ");\n";
      } else { // Enum, String, Struct, Array, Sequence, Union
        {
          string indent = "  ";
          NestedForLoops nfl("CORBA::ULong", "i", arr, indent);
          if (!use_cxx11 && (elem_cls & CL_ARRAY)) {
            be_global->impl_ <<
              indent << cxx_elem << "_var tmp_var = " << cxx_elem
              << "_dup(arr" << nfl.index_ << ");\n" <<
              indent << cxx_elem << "_forany tmp = tmp_var.inout();\n" <<
              streamAndCheck("<< tmp", indent.size());
          } else {
            string suffix = (elem_cls & CL_STRING) ? (use_cxx11 ? "" : ".in()") : "";
            string pre;
            if (use_cxx11 && (elem_cls & (CL_ARRAY | CL_SEQUENCE))) {
              pre = "IDL::DistinctType<const " + cxx_elem + ", " +
                dds_generator::scoped_helper(arr->base_type()->name(), "_") + "_tag>(";
              suffix += ')';
            }
            be_global->impl_ <<
              streamAndCheck("<< " + pre + "arr" + nfl.index_ + suffix , indent.size());
          }
        }
        be_global->impl_ << "  return true;\n";
      }
    }
    {
      Function extraction("operator>>", "bool");
      extraction.addArg("strm", "Serializer&");
      extraction.addArg(use_cxx11 ? "wrap" : "arr", cxx);
      extraction.endArgs();
      be_global->impl_ << unwrap;
      const std::string accessor = use_cxx11 ? ".data()" : ".out()";
      if (elem_cls & CL_PRIMITIVE) {
        string suffix;
        for (unsigned int i = 1; i < arr->n_dims(); ++i)
          suffix += use_cxx11 ? accessor : "[0]";
        be_global->impl_ <<
          "  return strm.read_" << getSerializerName(elem)
          << "_array(arr" << accessor << suffix << ", " << n_elems << ");\n";
      } else { // Enum, String, Struct, Array, Sequence, Union
        {
          string indent = "  ";
          NestedForLoops nfl("CORBA::ULong", "i", arr, indent);
          if (!use_cxx11 && (elem_cls & CL_ARRAY)) {
            const string typedefname = scoped(arr->base_type()->name());
            be_global->impl_ <<
              indent << typedefname << "_var tmp = " << typedefname
              << "_alloc();\n" <<
              indent << typedefname << "_forany fa = tmp.inout();\n"
              << streamAndCheck(">> fa", indent.size()) <<
              indent << typedefname << "_copy(arr" << nfl.index_ <<
              ", tmp.in());\n";
          } else {
            string suffix = (elem_cls & CL_STRING) ? (use_cxx11 ? "" : ".out()") : "";
            string pre;
            if (use_cxx11 && (elem_cls & (CL_ARRAY | CL_SEQUENCE))) {
              pre = "IDL::DistinctType<" + cxx_elem + ", " +
                dds_generator::scoped_helper(arr->base_type()->name(), "_") + "_tag>(";
              suffix += ')';
            }
            be_global->impl_ <<
              streamAndCheck(">> " + pre + "arr" + nfl.index_ + suffix, indent.size());
          }
        }
        be_global->impl_ << "  return true;\n";
      }
    }
  }

  string getArrayForany(const char* prefix, const char* fname,
                        const string& cxx_fld)
  {
    string local = fname;
    if (local.size() > 2 && local.substr(local.size() - 2, 2) == "()") {
      local.erase(local.size() - 2);
    }
    return cxx_fld + "_forany " + prefix + '_' + local + "(const_cast<"
      + cxx_fld + "_slice*>(" + prefix + "." + fname + "));";
  }

  // This function looks through the fields of a struct for the key
  // specified and returns the AST_Type associated with that key.
  // Because the key name can contain indexed arrays and nested
  // structures, things can get interesting.
  AST_Type* find_type(AST_Structure* struct_node, const string& key)
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
      AST_Field* field = *i;
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
        return find_type(sub_struct, key_rem);
      }
    }
    throw string("Field not found.");
  }

  bool is_bounded_type(AST_Type* type)
  {
    bool bounded = true;
    static std::vector<AST_Type*> type_stack;
    type = resolveActualType(type);
    for (unsigned int i = 0; i < type_stack.size(); i++) {
      // If we encounter the same type recursively, then we are unbounded
      if (type == type_stack[i]) return false;
    }
    type_stack.push_back(type);
    Classification fld_cls = classify(type);
    if ((fld_cls & CL_STRING) && !(fld_cls & CL_BOUNDED)) {
      bounded = false;
    } else if (fld_cls & CL_STRUCTURE) {
      const Fields fields(dynamic_cast<AST_Structure*>(type));
      const Fields::Iterator fields_end = fields.end();
      for (Fields::Iterator i = fields.begin(); i != fields_end; ++i) {
        if (!is_bounded_type((*i)->field_type())) {
          bounded = false;
          break;
        }
      }
    } else if (fld_cls & CL_SEQUENCE) {
      if (fld_cls & CL_BOUNDED) {
        AST_Sequence* seq_node = dynamic_cast<AST_Sequence*>(type);
        if (!is_bounded_type(seq_node->base_type())) bounded = false;
      } else {
        bounded = false;
      }
    } else if (fld_cls & CL_ARRAY) {
      AST_Array* array_node = dynamic_cast<AST_Array*>(type);
      if (!is_bounded_type(array_node->base_type())) bounded = false;
    } else if (fld_cls & CL_UNION) {
      const Fields fields(dynamic_cast<AST_Union*>(type));
      const Fields::Iterator fields_end = fields.end();
      for (Fields::Iterator i = fields.begin(); i != fields_end; ++i) {
        if (!is_bounded_type((*i)->field_type())) {
          bounded = false;
          break;
        }
      }
    }
    type_stack.pop_back();
    return bounded;
  }

  void align(size_t alignment, size_t& size, size_t& padding)
  {
    if ((size + padding) % alignment) {
      padding += alignment - ((size + padding) % alignment);
    }
  }

  void max_marshaled_size(AST_Type* type, size_t& size, size_t& padding);

  // Max marshaled size of repeating 'type' 'n' times in the stream
  // (for an array or sequence)
  void mms_repeating(AST_Type* type, size_t n, size_t& size, size_t& padding)
  {
    if (n > 0) {
      // 1st element may need padding relative to whatever came before
      max_marshaled_size(type, size, padding);
    }
    if (n > 1) {
      // subsequent elements may need padding relative to prior element
      size_t prev_size = size, prev_pad = padding;
      max_marshaled_size(type, size, padding);
      size += (n - 2) * (size - prev_size);
      padding += (n - 2) * (padding - prev_pad);
    }
  }

  // Should only be called on bounded types (see above function)
  void max_marshaled_size(AST_Type* type, size_t& size, size_t& padding)
  {
    type = resolveActualType(type);
    switch (type->node_type()) {
    case AST_Decl::NT_pre_defined: {
      AST_PredefinedType* p = AST_PredefinedType::narrow_from_decl(type);
      switch (p->pt()) {
      case AST_PredefinedType::PT_char:
      case AST_PredefinedType::PT_boolean:
      case AST_PredefinedType::PT_octet:
        size += 1;
        break;
      case AST_PredefinedType::PT_short:
      case AST_PredefinedType::PT_ushort:
        align(2, size, padding);
        size += 2;
        break;
      case AST_PredefinedType::PT_wchar:
        size += 3; // see Serializer::max_marshaled_size_wchar()
        break;
      case AST_PredefinedType::PT_long:
      case AST_PredefinedType::PT_ulong:
      case AST_PredefinedType::PT_float:
        align(4, size, padding);
        size += 4;
        break;
      case AST_PredefinedType::PT_longlong:
      case AST_PredefinedType::PT_ulonglong:
      case AST_PredefinedType::PT_double:
        align(8, size, padding);
        size += 8;
        break;
      case AST_PredefinedType::PT_longdouble:
        align(8, size, padding);
        size += 16;
        break;
      default:
        // Anything else shouldn't be in a DDS type or is unbounded.
        break;
      }
      break;
    }
    case AST_Decl::NT_enum:
      align(4, size, padding);
      size += 4;
      break;
    case AST_Decl::NT_string:
    case AST_Decl::NT_wstring: {
      AST_String* string_node = dynamic_cast<AST_String*>(type);
      align(4, size, padding);
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
      for (Fields::Iterator i = fields.begin(); i != fields_end; ++i) {
        max_marshaled_size((*i)->field_type(), size, padding);
      }
      break;
    }
    case AST_Decl::NT_sequence: {
      AST_Sequence* seq_node = dynamic_cast<AST_Sequence*>(type);
      AST_Type* base_node = seq_node->base_type();
      size_t bound = seq_node->max_size()->ev()->u.ulval;
      align(4, size, padding);
      size += 4;
      mms_repeating(base_node, bound, size, padding);
      break;
    }
    case AST_Decl::NT_array: {
      AST_Array* array_node = dynamic_cast<AST_Array*>(type);
      AST_Type* base_node = array_node->base_type();
      size_t array_size = 1;
      AST_Expression** dims = array_node->dims();
      for (unsigned long i = 0; i < array_node->n_dims(); i++) {
        array_size *= dims[i]->ev()->u.ulval;
      }
      mms_repeating(base_node, array_size, size, padding);
      break;
    }
    case AST_Decl::NT_union: {
      AST_Union* union_node = dynamic_cast<AST_Union*>(type);
      max_marshaled_size(union_node->disc_type(), size, padding);
      size_t largest_field_size = 0, largest_field_pad = 0;
      const size_t starting_size = size, starting_pad = padding;
      const Fields fields(union_node);
      const Fields::Iterator fields_end = fields.end();
      for (Fields::Iterator i = fields.begin(); i != fields_end; ++i) {
        max_marshaled_size((*i)->field_type(), size, padding);
        size_t field_size = size - starting_size,
          field_pad = padding - starting_pad;
        if (field_size > largest_field_size) {
          largest_field_size = field_size;
          largest_field_pad = field_pad;
        }
        // rewind:
        size = starting_size;
        padding = starting_pad;
      }
      size += largest_field_size;
      padding += largest_field_pad;
      break;
    }
    default:
      // Anything else should be not here or is unbounded
      break;
    }
  }
}

bool marshal_generator::gen_typedef(AST_Typedef*, UTL_ScopedName* name, AST_Type* base,
  const char*)
{
  switch (base->node_type()) {
  case AST_Decl::NT_sequence:
    gen_sequence(name, AST_Sequence::narrow_from_decl(base));
    break;
  case AST_Decl::NT_array:
    gen_array(name, AST_Array::narrow_from_decl(base));
    break;
  default:
    return true;
  }
  return true;
}

namespace {
  // common to both fields (in structs) and branches (in unions)
  string findSizeCommon(const string& name, AST_Type* type,
                        const string& prefix, string& intro,
                        const string& = "") // same sig as streamCommon
  {
    const bool use_cxx11 = be_global->language_mapping() == BE_GlobalData::LANGMAP_CXX11;
    const bool is_union_member = prefix == "uni";

    AST_Type* typedeff = type;
    type = resolveActualType(type);
    Classification fld_cls = classify(type);

    const string qual = prefix + '.' + insert_cxx11_accessor_parens(name, is_union_member);
    const string indent = (is_union_member) ? "    " : "  ";

    if (fld_cls & CL_ENUM) {
      return indent + "find_size_ulong(size, padding);\n";
    } else if (fld_cls & CL_STRING) {
      const string suffix = is_union_member ? "" : ".in()";
      const string get_size = use_cxx11 ? (qual + ".size()")
        : ("ACE_OS::strlen(" + qual + suffix + ")");
      return indent + "find_size_ulong(size, padding);\n" +
        indent + "size += " + get_size
        + ((fld_cls & CL_WIDE) ? " * OpenDDS::DCPS::Serializer::WCHAR_SIZE;\n"
                               : " + 1;\n");
    } else if (fld_cls & CL_PRIMITIVE) {
      string align = getAlignment(type);
      if (!align.empty()) {
        align =
          indent + "if ((size + padding) % " + align + ") {\n" +
          indent + "  padding += " + align + " - ((size + padding) % "
          + align + ");\n" +
          indent + "}\n";
      }
      AST_PredefinedType* p = AST_PredefinedType::narrow_from_decl(type);
      if (p->pt() == AST_PredefinedType::PT_longdouble) {
        // special case use to ACE's NONNATIVE_LONGDOUBLE in CDR_Base.h
        return align +
          indent + "size += gen_max_marshaled_size(ACE_CDR::LongDouble());\n";
      }
      return align +
        indent + "size += gen_max_marshaled_size(" +
        getWrapper(qual, type, WD_OUTPUT) + ");\n";
    } else if (fld_cls == CL_UNKNOWN) {
      return ""; // warning will be issued for the serialize functions
    } else { // sequence, struct, union, array
      string fieldref = prefix,
             local = insert_cxx11_accessor_parens(name, is_union_member),
             tdname = scoped(typedeff->name());

      if (!use_cxx11 && (fld_cls & CL_ARRAY)) {
        intro += "  " + getArrayForany(prefix.c_str(), name.c_str(), tdname) + '\n';
        fieldref += '_';
        if (local.size() > 2 && local.substr(local.size() - 2) == "()") {
          local.erase(local.size() - 2);
        }
      } else if (use_cxx11 && (fld_cls & (CL_SEQUENCE | CL_ARRAY))) {
        fieldref = "IDL::DistinctType<const " + tdname + ", " +
          dds_generator::scoped_helper(typedeff->name(), "_") + "_tag>("
          + fieldref + '.';
        local += ')';
      } else {
        fieldref += '.';
      }
      return indent +
        "gen_find_size(" + fieldref + local + ", size, padding);\n";
    }
  }

  // common to both fields (in structs) and branches (in unions)
  string streamCommon(const string& name, AST_Type* type,
                      const string& prefix, string& intro,
                      const string& stru = "")
  {
    const bool use_cxx11 = be_global->language_mapping() == BE_GlobalData::LANGMAP_CXX11;
    const bool is_union_member = prefix.substr(3) == "uni";

    AST_Type* typedeff = type;
    const string tdname = scoped(typedeff->name());
    type = resolveActualType(type);
    Classification fld_cls = classify(type);

    const string qual = prefix + '.' + insert_cxx11_accessor_parens(name, is_union_member),
          shift = prefix.substr(0, 2),
          expr = qual.substr(3);

    WrapDirection dir = (shift == ">>") ? WD_INPUT : WD_OUTPUT;
    if ((fld_cls & CL_STRING) && (dir == WD_INPUT)) {
      if (fld_cls & CL_BOUNDED) {
        const string args = expr + (use_cxx11 ? ", " : ".out(), ") + bounded_arg(type);
        return "(strm " + shift + ' ' + getWrapper(args, type, WD_INPUT) + ')';
      }
      return "(strm " + qual + (use_cxx11 ? "" : ".out()") + ')';
    } else if (fld_cls & CL_PRIMITIVE) {
      return "(strm " + shift + ' ' + getWrapper(expr, type, dir) + ')';
    } else if (fld_cls == CL_UNKNOWN) {
      if (dir == WD_INPUT) { // no need to warn twice
        std::cerr << "WARNING: field " << name << " can not be serialized.  "
          "The struct or union it belongs to (" << stru <<
          ") can not be used in an OpenDDS topic type." << std::endl;
      }
      return "false";
    } else { // sequence, struct, union, array, enum, string(insertion)
      string fieldref = prefix,
             local = insert_cxx11_accessor_parens(name, is_union_member);

      const bool accessor =
        local.size() > 2 && local.substr(local.size() - 2) == "()";
      if (!use_cxx11 && (fld_cls & CL_ARRAY)) {
        string pre = prefix;
        if (shift == ">>" || shift == "<<") {
          pre.erase(0, 3);
        }
        if (accessor) {
          local.erase(local.size() - 2);
        }
        intro += "  " + getArrayForany(pre.c_str(), name.c_str(), tdname) + '\n';
        fieldref += '_';
      } else {
        fieldref += '.';
      }

      if (fld_cls & CL_STRING) {
        if (!accessor && !use_cxx11) {
          local += ".in()";
        }
        if (fld_cls & CL_BOUNDED) {
          const string args = (fieldref + local).substr(3) + ", " + bounded_arg(type);
          return "(strm " + shift + ' ' + getWrapper(args, type, WD_OUTPUT) + ')';
        }
      } else if (use_cxx11 && (fld_cls & (CL_ARRAY | CL_SEQUENCE))) {
        return "(strm " + shift + " IDL::DistinctType<" +
          (dir == WD_OUTPUT ? "const " : "") + tdname + ", " +
          dds_generator::scoped_helper(typedeff->name(), "_") + "_tag>("
          + (fieldref + local).substr(3) + "))";
      }
      return "(strm " + fieldref + local + ')';
    }
  }

  bool isBinaryProperty_t(const string& cxx)
  {
    return cxx == "DDS::BinaryProperty_t";
  }

  bool genBinaryProperty_t(const string& cxx)
  {
    {
      Function find_size("gen_find_size", "void");
      find_size.addArg("stru", "const " + cxx + "&");
      find_size.addArg("size", "size_t&");
      find_size.addArg("padding", "size_t&");
      find_size.endArgs();
      be_global->impl_ <<
        "  if (stru.propagate) {\n"
        "    find_size_ulong(size, padding);\n"
        "    size += ACE_OS::strlen(stru.name.in()) + 1;\n"
        "    gen_find_size(stru.value, size, padding);\n"
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

  bool isProperty_t(const string& cxx)
  {
    return cxx == "DDS::Property_t";
  }

  bool genProperty_t(const string& cxx)
  {
    {
      Function find_size("gen_find_size", "void");
      find_size.addArg("stru", "const " + cxx + "&");
      find_size.addArg("size", "size_t&");
      find_size.addArg("padding", "size_t&");
      find_size.endArgs();
      be_global->impl_ <<
        "  if (stru.propagate) {\n"
        "    find_size_ulong(size, padding);\n"
        "    size += ACE_OS::strlen(stru.name.in()) + 1;\n"
        "    find_size_ulong(size, padding);\n"
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

  bool isPropertyQosPolicy(const string& cxx)
  {
    return cxx == "DDS::PropertyQosPolicy";
  }

  bool genPropertyQosPolicy(const string& cxx)
  {
    {
      Function find_size("gen_find_size", "void");
      find_size.addArg("stru", "const " + cxx + "&");
      find_size.addArg("size", "size_t&");
      find_size.addArg("padding", "size_t&");
      find_size.endArgs();
      be_global->impl_ <<
        "  gen_find_size(stru.value, size, padding);\n"
        "  gen_find_size(stru.binary_value, size, padding);\n";
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

  bool isSecuritySubmessage(const string& cxx)
  {
    return cxx == "OpenDDS::RTPS::SecuritySubmessage";
  }

  bool genSecuritySubmessage(const string& cxx)
  {
    {
      Function find_size("gen_find_size", "void");
      find_size.addArg("stru", "const " + cxx + "&");
      find_size.addArg("size", "size_t&");
      find_size.addArg("padding", "size_t&");
      find_size.endArgs();
      be_global->impl_ <<
        "  gen_find_size(stru.smHeader, size, padding);\n"
        "  size += stru.content.length() * max_marshaled_size_octet();\n";
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

  bool isRtpsSpecialStruct(const string& cxx)
  {
    return cxx == "OpenDDS::RTPS::SequenceNumberSet"
      || cxx == "OpenDDS::RTPS::FragmentNumberSet";
  }

  bool genRtpsSpecialStruct(const string& cxx)
  {
    {
      Function find_size("gen_find_size", "void");
      find_size.addArg("stru", "const " + cxx + "&");
      find_size.addArg("size", "size_t&");
      find_size.addArg("padding", "size_t&");
      find_size.endArgs();
      be_global->impl_ <<
        "  size += "
        << ((cxx == "OpenDDS::RTPS::SequenceNumberSet") ? "12" : "8")
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
      if (cxx == "OpenDDS::RTPS::DataSubmessage") {
        cst_["inlineQos"] = "stru.smHeader.flags & 2";
        iQosOffset_ = "16";

      } else if (cxx == "OpenDDS::RTPS::DataFragSubmessage") {
        cst_["inlineQos"] = "stru.smHeader.flags & 2";
        iQosOffset_ = "28";

      } else if (cxx == "OpenDDS::RTPS::InfoReplySubmessage") {
        cst_["multicastLocatorList"] = "stru.smHeader.flags & 2";

      } else if (cxx == "OpenDDS::RTPS::InfoTimestampSubmessage") {
        cst_["timestamp"] = "!(stru.smHeader.flags & 2)";

      } else if (cxx == "OpenDDS::RTPS::InfoReplyIp4Submessage") {
        cst_["multicastLocator"] = "stru.smHeader.flags & 2";

      } else if (cxx == "OpenDDS::RTPS::SubmessageHeader") {
        preamble_ =
          "  strm.swap_bytes(ACE_CDR_BYTE_ORDER != (stru.flags & 1));\n";
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
    string iQosOffset_, preamble_;
  };

  typedef void (*KeyIterationFn)(
    const string& key_name, AST_Type* ast_type,
    size_t* size, size_t* padding,
    string* expr, string* intro);

  bool
  iterate_over_keys(
    AST_Structure* node,
    const std::string& struct_name,
    IDL_GlobalData::DCPS_Data_Type_Info* info,
    TopicKeys& keys,
    KeyIterationFn fn,
    size_t* size, size_t* padding,
    string* expr, string* intro)
  {
    if (!info) {
      TopicKeys::Iterator finished = keys.end();
      for (TopicKeys::Iterator i = keys.begin(); i != finished; ++i) {
        string key_name = i.path();
        AST_Type* straight_ast_type = i.get_ast_type();
        AST_Type* ast_type;
        if (i.root_type() == TopicKeys::UnionType) {
          AST_Union* union_type = dynamic_cast<AST_Union*>(straight_ast_type);
          if (!union_type) {
            std::cerr << "ERROR: Invalid key iterator for: " << struct_name;
            return false;
          }
          ast_type = dynamic_cast<AST_Type*>(union_type->disc_type());
          key_name.append("._d()");
        } else {
          ast_type = straight_ast_type;
        }
        fn(key_name, ast_type, size, padding, expr, intro);
      }
    } else {
      IDL_GlobalData::DCPS_Data_Type_Info_Iter iter(info->key_list_);
      for (ACE_TString* kp = 0; iter.next(kp) != 0; iter.advance()) {
        const string key_name = ACE_TEXT_ALWAYS_CHAR(kp->c_str());
        AST_Type* field_type = 0;
        try {
          field_type = find_type(node, key_name);
        } catch (const string& error) {
          std::cerr << "ERROR: Invalid key specification for " << struct_name
                    << " (" << key_name << "). " << error << std::endl;
          return false;
        }
        fn(key_name, field_type, size, padding, expr, intro);
      }
    }
    return true;
  }

  void
  gen_max_marshaled_size_iteration(
    const string&, AST_Type* ast_type,
    size_t* size, size_t* padding, string*, string*)
  {
    max_marshaled_size(ast_type, *size, *padding);
  }

  void
  gen_find_size_iteration(
    const string& key_name, AST_Type* ast_type,
    size_t*, size_t*, string* expr, string* intro)
  {
    *expr += findSizeCommon(key_name, ast_type, "stru.t", *intro);
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
  string cxx = scoped(name); // name as a C++ class

  for (size_t i = 0; i < LENGTH(special_structs); ++i) {
    if (special_structs[i].check(cxx)) {
      return special_structs[i].gen(cxx);
    }
  }

  RtpsFieldCustomizer rtpsCustom(cxx);
  {
    Function find_size("gen_find_size", "void");
    find_size.addArg("stru", "const " + cxx + "&");
    find_size.addArg("size", "size_t&");
    find_size.addArg("padding", "size_t&");
    find_size.endArgs();
    string expr, intro;
    for (size_t i = 0; i < fields.size(); ++i) {
      AST_Type* field_type = resolveActualType(fields[i]->field_type());
      if (!field_type->in_main_file()
          && field_type->node_type() != AST_Decl::NT_pre_defined) {
        be_global->add_referenced(field_type->file_name().c_str());
      }
      const string field_name = fields[i]->local_name()->get_string(),
        cond = rtpsCustom.getConditional(field_name);
      if (!cond.empty()) {
        expr += "  if (" + cond + ") {\n  ";
      }
      expr += findSizeCommon(field_name, fields[i]->field_type(), "stru", intro);
      if (!cond.empty()) {
        expr += "  }\n";
      }
    }
    be_global->impl_ << intro << expr;
  }
  {
    Function insertion("operator<<", "bool");
    insertion.addArg("strm", "Serializer&");
    insertion.addArg("stru", "const " + cxx + "&");
    insertion.endArgs();
    string expr, intro = rtpsCustom.preamble_;
    for (size_t i = 0; i < fields.size(); ++i) {
      if (i) expr += "\n    && ";
      const string field_name = fields[i]->local_name()->get_string(),
        cond = rtpsCustom.getConditional(field_name);
      if (!cond.empty()) {
        expr += "(!(" + cond + ") || ";
      }
      expr += streamCommon(field_name, fields[i]->field_type(), "<< stru", intro, cxx);
      if (!cond.empty()) {
        expr += ")";
      }
    }
    be_global->impl_ << intro << "  return " << expr << ";\n";
  }
  {
    Function extraction("operator>>", "bool");
    extraction.addArg("strm", "Serializer&");
    extraction.addArg("stru", cxx + "&");
    extraction.endArgs();
    string expr, intro;
    for (size_t i = 0; i < fields.size(); ++i) {
      if (i) expr += "\n    && ";
      const string field_name = fields[i]->local_name()->get_string(),
        cond = rtpsCustom.getConditional(field_name);
      if (!cond.empty()) {
        expr += rtpsCustom.preFieldRead(field_name);
        expr += "(!(" + cond + ") || ";
      }
      expr += streamCommon(field_name, fields[i]->field_type(), ">> stru", intro, cxx);
      if (!cond.empty()) {
        expr += ")";
      }
    }
    be_global->impl_ << intro << "  return " << expr << ";\n";
  }

  IDL_GlobalData::DCPS_Data_Type_Info* info = idl_global->is_dcps_type(name);
  const bool is_topic_type = be_global->is_topic_type(node);
  TopicKeys keys;
  if (is_topic_type) {
    keys = TopicKeys(node);
    info = 0; // Annotations Override DCPS_DATA_TYPE
  }

  // Only generate these methods if this is a topic type
  if (info || is_topic_type) {
    bool is_bounded_struct = true;
    for (size_t i = 0; i < fields.size(); ++i) {
      if (!is_bounded_type(fields[i]->field_type())) {
        is_bounded_struct = false;
        break;
      }
    }
    {
      Function max_marsh("gen_max_marshaled_size", "size_t");
      max_marsh.addArg("stru", "const " + cxx + "&");
      max_marsh.addArg("align", "bool");
      max_marsh.endArgs();
      if (is_bounded_struct) {
        size_t size = 0, padding = 0;
        for (size_t i = 0; i < fields.size(); ++i) {
          max_marshaled_size(fields[i]->field_type(), size, padding);
        }
        if (padding) {
          be_global->impl_
            << "  return align ? " << size + padding << " : " << size << ";\n";
        } else {
          be_global->impl_
            << "  return " << size << ";\n";
        }
      } else { // unbounded
        be_global->impl_
          << "  return 0;\n";
      }
    }

    // Generate key-related marshaling code
    bool bounded_key = true;
    if (info) {
      IDL_GlobalData::DCPS_Data_Type_Info_Iter iter(info->key_list_);
      for (ACE_TString* kp = 0; iter.next(kp) != 0; iter.advance()) {
        string key_name = ACE_TEXT_ALWAYS_CHAR(kp->c_str());
        AST_Type* field_type = 0;
        try {
          field_type = find_type(node, key_name);
        } catch (const string& error) {
          std::cerr << "ERROR: Invalid key specification for " << cxx
                    << " (" << key_name << "). " << error << std::endl;
          return false;
        }
        if (!is_bounded_type(field_type)) {
          bounded_key = false;
          break;
        }
      }
    } else {
      TopicKeys::Iterator finished = keys.end();
      for (TopicKeys::Iterator i = keys.begin(); i != finished; ++i) {
        if (!is_bounded_type(i.get_ast_type())) {
          bounded_key = false;
          break;
        }
      }
    }

    {
      Function max_marsh("gen_max_marshaled_size", "size_t");
      max_marsh.addArg("stru", "KeyOnly<const " + cxx + ">");
      max_marsh.addArg("align", "bool");
      max_marsh.endArgs();

      if (bounded_key) {  // Only generate a size if the key is bounded
        size_t size = 0, padding = 0;

        if (!iterate_over_keys(node, cxx, info, keys,
          gen_max_marshaled_size_iteration, &size, &padding, 0, 0)) {
          return false;
        }

        if (padding) {
          be_global->impl_
            << "  return align ? " << size + padding << " : " << size << ";\n";
        } else {
          be_global->impl_
            << "  return " << size << ";\n";
        }
      } else { // unbounded
        be_global->impl_
          << "  return 0;\n";
      }
    }

    {
      Function find_size("gen_find_size", "void");
      find_size.addArg("stru", "KeyOnly<const " + cxx + ">");
      find_size.addArg("size", "size_t&");
      find_size.addArg("padding", "size_t&");
      find_size.endArgs();
      string expr, intro;

      if (!iterate_over_keys(node, cxx, info, keys,
        gen_find_size_iteration, 0, 0, &expr, &intro)) {
        return false;
      }

      be_global->impl_ << intro << expr;
    }

    {
      Function insertion("operator<<", "bool");
      insertion.addArg("strm", "Serializer&");
      insertion.addArg("stru", "KeyOnly<const " + cxx + ">");
      insertion.endArgs();

      bool first = true;
      string expr, intro;

      if (info) {
        IDL_GlobalData::DCPS_Data_Type_Info_Iter iter(info->key_list_);
        for (ACE_TString* kp = 0; iter.next(kp) != 0; iter.advance()) {
          string key_name = ACE_TEXT_ALWAYS_CHAR(kp->c_str());
          AST_Type* field_type = 0;
          try {
            field_type = find_type(node, key_name);
          } catch (const string& error) {
            std::cerr << "ERROR: Invalid key specification for " << cxx
                      << " (" << key_name << "). " << error << std::endl;
            return false;
          }
          if (first) {
            first = false;
          } else {
            expr += "\n    && ";
          }
          expr += streamCommon(key_name, field_type, "<< stru.t", intro);
        }
      } else {
        TopicKeys::Iterator finished = keys.end();
        for (TopicKeys::Iterator i = keys.begin(); i != finished; ++i) {
          std::string key_name = i.path();
          if (first) {
            first = false;
          } else {
            expr += "\n    && ";
          }
          AST_Type* straight_ast_type = i.get_ast_type();
          AST_Type* ast_type;
          if (i.root_type() == TopicKeys::UnionType) {
            key_name.append("._d()");
            AST_Union* union_type = dynamic_cast<AST_Union*>(straight_ast_type);
            if (!union_type) {
              std::cerr << "ERROR: Invalid key iterator for: " << cxx;
              return false;
            }
            ast_type = dynamic_cast<AST_Type*>(union_type->disc_type());
          } else {
            ast_type = straight_ast_type;
          }
          if (!ast_type) {
            std::cerr << "ERROR: Invalid key iterator for: " << cxx;
            return false;
          }
          expr += streamCommon(key_name, ast_type, "<< stru.t", intro);
        }
      }

      if (first) {
        be_global->impl_ << intro << "  return true;\n";
      } else {
        be_global->impl_ << intro << "  return " << expr << ";\n";
      }
    }

    {
      Function extraction("operator>>", "bool");
      extraction.addArg("strm", "Serializer&");
      extraction.addArg("stru", "KeyOnly<" + cxx + ">");
      extraction.endArgs();

      bool first = true;
      string expr, intro;

      if (info) {
        IDL_GlobalData::DCPS_Data_Type_Info_Iter iter(info->key_list_);
        for (ACE_TString* kp = 0; iter.next(kp) != 0; iter.advance()) {
          string key_name = ACE_TEXT_ALWAYS_CHAR(kp->c_str());
          AST_Type* field_type = 0;
          try {
            field_type = find_type(node, key_name);
          } catch (const string& error) {
            std::cerr << "ERROR: Invalid key specification for " << cxx
                      << " (" << key_name << "). " << error << std::endl;
            return false;
          }
          if (first) {
            first = false;
          } else {
            expr += "\n    && ";
          }
          expr += streamCommon(key_name, field_type, ">> stru.t", intro);
        }
      } else {
        TopicKeys::Iterator finished = keys.end();
        for (TopicKeys::Iterator i = keys.begin(); i != finished; ++i) {
          std::string key_name = i.path();
          AST_Type* ast_type = i.get_ast_type();
          if (i.root_type() == TopicKeys::UnionType) {
            AST_Union* union_type = dynamic_cast<AST_Union*>(ast_type);
            if (!union_type) {
              std::cerr << "ERROR: Invalid key iterator for: " << cxx;
              return false;
            }
            AST_Type* disc_type = dynamic_cast<AST_Type*>(union_type->disc_type());
            if (!disc_type) {
              std::cerr << "ERROR: Invalid key iterator for: " << cxx;
              return false;
            }
            be_global->impl_ <<
              "  {\n"
              "    " << scoped(disc_type->name()) << " tmp;\n" <<
              "    if (strm >> " << getWrapper("tmp", disc_type, WD_INPUT) << ") {\n"
              "      stru.t." << key_name << "._d(tmp);\n"
              "    } else {\n"
              "      return false;\n"
              "    }\n"
              "  }\n"
              ;
          } else {
            if (first) {
              first = false;
            } else {
              expr += "\n    && ";
            }
            expr += streamCommon(key_name, ast_type, ">> stru.t", intro);
          }
        }
      }

      if (first) {
        be_global->impl_ << intro << "  return true;\n";
      } else {
        be_global->impl_ << intro << "  return " << expr << ";\n";
      }
    }

    be_global->header_ <<
      "template <>\n"
      "struct MarshalTraits<" << cxx << "> {\n"
      "  static bool gen_is_bounded_size() { return " << (is_bounded_struct ? "true" : "false") << "; }\n"
      "  static bool gen_is_bounded_key_size() { return " << (bounded_key ? "true" : "false") << "; }\n"
      "};\n";
  }

  return true;
}

namespace {

  bool isRtpsSpecialUnion(const string& cxx)
  {
    return cxx == "OpenDDS::RTPS::Parameter"
      || cxx == "OpenDDS::RTPS::Submessage";
  }

  bool genRtpsParameter(AST_Type* discriminator,
                        const std::vector<AST_UnionBranch*>& branches)
  {
    const string cxx = "OpenDDS::RTPS::Parameter";
    {
      Function find_size("gen_find_size", "void");
      find_size.addArg("uni", "const " + cxx + "&");
      find_size.addArg("size", "size_t&");
      find_size.addArg("padding", "size_t&");
      find_size.endArgs();
      generateSwitchForUnion("uni._d()", findSizeCommon, branches,
                             discriminator, "", "", cxx.c_str());
      be_global->impl_ <<
        "  size += 4; // parameterId & length\n";
    }
    {
      Function insertion("operator<<", "bool");
      insertion.addArg("outer_strm", "Serializer&");
      insertion.addArg("uni", "const " + cxx + "&");
      insertion.endArgs();
      be_global->impl_ <<
        "  if (!(outer_strm << uni._d())) {\n"
        "    return false;\n"
        "  }\n"
        "  size_t size = 0, pad = 0;\n"
        "  gen_find_size(uni, size, pad);\n"
        "  size -= 4; // parameterId & length\n"
        "  const size_t post_pad = 4 - ((size + pad) % 4);\n"
        "  const size_t total = size + pad + ((post_pad < 4) ? post_pad : 0);\n"
        "  if (size + pad > ACE_UINT16_MAX || "
        "!(outer_strm << ACE_CDR::UShort(total))) {\n"
        "    return false;\n"
        "  }\n"
        "  ACE_Message_Block param(size + pad);\n"
        "  Serializer strm(&param, outer_strm.swap_bytes(), "
        "outer_strm.alignment());\n"
        "  if (!insertParamData(strm, uni)) {\n"
        "    return false;\n"
        "  }\n"
        "  const ACE_CDR::Octet* data = reinterpret_cast<ACE_CDR::Octet*>("
        "param.rd_ptr());\n"
        "  if (!outer_strm.write_octet_array(data, ACE_CDR::ULong(param.length()))) {\n"
        "    return false;\n"
        "  }\n"
        "  if (post_pad < 4 && outer_strm.alignment() != "
        "Serializer::ALIGN_NONE) {\n"
        "    static const ACE_CDR::Octet padding[3] = {0};\n"
        "    return outer_strm.write_octet_array(padding, "
        "ACE_CDR::ULong(post_pad));\n"
        "  }\n"
        "  return true;\n";
    }
    {
      Function insertData("insertParamData", "bool");
      insertData.addArg("strm", "Serializer&");
      insertData.addArg("uni", "const " + cxx + "&");
      insertData.endArgs();
      generateSwitchForUnion("uni._d()", streamCommon, branches, discriminator,
                             "return", "<< ", cxx.c_str());
    }
    {
      Function extraction("operator>>", "bool");
      extraction.addArg("outer_strm", "Serializer&");
      extraction.addArg("uni", cxx + "&");
      extraction.endArgs();
      be_global->impl_ <<
        "  ACE_CDR::UShort disc, size;\n"
        "  if (!(outer_strm >> disc) || !(outer_strm >> size)) {\n"
        "    return false;\n"
        "  }\n"
        "  if (disc == OpenDDS::RTPS::PID_SENTINEL) {\n"
        "    uni._d(OpenDDS::RTPS::PID_SENTINEL);\n"
        "    return true;\n"
        "  }\n"
        "  ACE_Message_Block param(size);\n"
        "  ACE_CDR::Octet* data = reinterpret_cast<ACE_CDR::Octet*>("
        "param.wr_ptr());\n"
        "  if (!outer_strm.read_octet_array(data, size)) {\n"
        "    return false;\n"
        "  }\n"
        "  param.wr_ptr(size);\n"
        "  Serializer strm(&param, outer_strm.swap_bytes(), "
        "Serializer::ALIGN_CDR);\n"
        "  switch (disc) {\n";
      generateSwitchBody(streamCommon, branches, discriminator,
                         "return", ">> ", cxx.c_str(), true);
      be_global->impl_ <<
        "  default:\n"
        "    {\n"
        "      uni.unknown_data(DDS::OctetSeq(size));\n"
        "      uni.unknown_data().length(size);\n"
        "      std::memcpy(uni.unknown_data().get_buffer(), data, size);\n"
        "      uni._d(disc);\n"
        "    }\n"
        "  }\n"
        "  return true;\n";
    }
    return true;
  }

  bool genRtpsSubmessage(AST_Type* discriminator,
                         const std::vector<AST_UnionBranch*>& branches)
  {
    const string cxx = "OpenDDS::RTPS::Submessage";
    {
      Function find_size("gen_find_size", "void");
      find_size.addArg("uni", "const " + cxx + "&");
      find_size.addArg("size", "size_t&");
      find_size.addArg("padding", "size_t&");
      find_size.endArgs();
      generateSwitchForUnion("uni._d()", findSizeCommon, branches,
                             discriminator, "", "", cxx.c_str());
    }
    {
      Function insertion("operator<<", "bool");
      insertion.addArg("strm", "Serializer&");
      insertion.addArg("uni", "const " + cxx + "&");
      insertion.endArgs();
      generateSwitchForUnion("uni._d()", streamCommon, branches,
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

  bool genRtpsSpecialUnion(const string& cxx, AST_Type* discriminator,
                           const std::vector<AST_UnionBranch*>& branches)
  {
    if (cxx == "OpenDDS::RTPS::Parameter") {
      return genRtpsParameter(discriminator, branches);
    } else if (cxx == "OpenDDS::RTPS::Submessage") {
      return genRtpsSubmessage(discriminator, branches);
    } else {
      return false;
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
  Classification disc_cls = classify(discriminator);

  for (size_t i = 0; i < LENGTH(special_unions); ++i) {
    if (special_unions[i].check(cxx)) {
      return special_unions[i].gen(cxx, discriminator, branches);
    }
  }

  const string wrap_out = getWrapper("uni._d()", discriminator, WD_OUTPUT);
  {
    Function find_size("gen_find_size", "void");
    find_size.addArg("uni", "const " + cxx + "&");
    find_size.addArg("size", "size_t&");
    find_size.addArg("padding", "size_t&");
    find_size.endArgs();
    const string align = getAlignment(discriminator);
    if (!align.empty()) {
      be_global->impl_ <<
        "  if ((size + padding) % " << align << ") {\n"
        "    padding += " << align << " - ((size + padding) % " << align
        << ");\n"
        "  }\n";
    }
    if (disc_cls & CL_ENUM) {
      be_global->impl_ <<
        "  size += max_marshaled_size_ulong();\n";
    } else {
      be_global->impl_ <<
        "  size += gen_max_marshaled_size(" << wrap_out << ");\n";
    }
    generateSwitchForUnion("uni._d()", findSizeCommon, branches, discriminator,
                           "", "", cxx.c_str());
  }
  {
    Function insertion("operator<<", "bool");
    insertion.addArg("strm", "Serializer&");
    insertion.addArg("uni", "const " + cxx + "&");
    insertion.endArgs();
    be_global->impl_ <<
      streamAndCheck("<< " + wrap_out);
    if (generateSwitchForUnion("uni._d()", streamCommon, branches,
                               discriminator, "return", "<< ", cxx.c_str())) {
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
      "  " << scoped(discriminator->name()) << " disc;\n" <<
      streamAndCheck(">> " + getWrapper("disc", discriminator, WD_INPUT));
    if (generateSwitchForUnion("disc", streamCommon, branches,
                               discriminator, "if", ">> ", cxx.c_str())) {
      be_global->impl_ <<
        "  return true;\n";
    }
  }

  const bool has_key = be_global->has_key(node);
  const bool is_topic_type = be_global->is_topic_type(node);

  if (!is_topic_type) {
    if (has_key) {
      idl_global->err()->misc_warning(
        "Union has @key on its discriminator, "
        "but it's not a topic type, ignoring it...", node);
    }
    return true;
  }

  const string key_only_wrap_out = getWrapper("uni.t._d()", discriminator, WD_OUTPUT);

  const bool is_bounded = is_bounded_type(node);
  {
    Function max_marsh("gen_max_marshaled_size", "size_t");
    max_marsh.addArg("uni", "const " + cxx + "&");
    max_marsh.addArg("align", "bool");
    max_marsh.endArgs();
    if (is_bounded) {
      size_t size = 0, padding = 0;
      max_marshaled_size(node, size, padding);
      if (padding) {
        be_global->impl_
          << "  return align ? " << size + padding << " : " << size << ";\n";
      } else {
        be_global->impl_
          << "  return " << size << ";\n";
      }
    } else { // unbounded
      be_global->impl_
        << "  return 0;\n";
    }
  }

  {
    Function max_marsh("gen_max_marshaled_size", "size_t");
    max_marsh.addArg("uni", "KeyOnly<const " + cxx + ">");
    max_marsh.addArg("align", "bool");
    max_marsh.endArgs();

    if (has_key) {
      size_t size = 0, padding = 0;
      max_marshaled_size(node->disc_type(), size, padding);
      if (padding) {
        be_global->impl_
          << "  return align ? " << size + padding << " : " << size << ";\n";
      } else {
        be_global->impl_
          << "  return " << size << ";\n";
      }
    } else {
      be_global->impl_
        << "  return 0;\n";
    }
  }

  {
    Function find_size("gen_find_size", "void");
    find_size.addArg("uni", "KeyOnly<const " + cxx + ">");
    find_size.addArg("size", "size_t&");
    find_size.addArg("padding", "size_t&");
    find_size.endArgs();

    if (has_key) {
      const string align = getAlignment(discriminator);
      if (!align.empty()) {
        be_global->impl_ <<
          "  if ((size + padding) % " << align << ") {\n"
          "    padding += " << align << " - ((size + padding) % " << align
          << ");\n"
          "  }\n";
      }
      if (disc_cls & CL_ENUM) {
        be_global->impl_ <<
          "  size += max_marshaled_size_ulong();\n";
      } else {
        be_global->impl_ <<
          "  size += gen_max_marshaled_size(" << key_only_wrap_out << ");\n";
      }
    }
  }

  {
    Function insertion("operator<<", "bool");
    insertion.addArg("strm", "Serializer&");
    insertion.addArg("uni", "KeyOnly<const " + cxx + ">");
    insertion.endArgs();

    if (has_key) {
      be_global->impl_ << streamAndCheck("<< " + key_only_wrap_out);
    }

    be_global->impl_ << "  return true;\n";
  }

  {
    Function extraction("operator>>", "bool");
    extraction.addArg("strm", "Serializer&");
    extraction.addArg("uni", "KeyOnly<" + cxx + ">");
    extraction.endArgs();

    if (has_key) {
      be_global->impl_
        << "  " << scoped(discriminator->name()) << " disc;\n"
        << streamAndCheck(">> " + getWrapper("disc", discriminator, WD_INPUT))
        << "  uni.t._d(disc);\n";
    }

    be_global->impl_ << "  return true;\n";
  }

  be_global->header_ <<
    "template <>\n"
    "struct MarshalTraits<" << cxx << "> {\n"
    "  static bool gen_is_bounded_size() { return " << (is_bounded ? "true" : "false") << "; }\n"
    // Key is the discriminator, so it's always bounded
    "  static bool gen_is_bounded_key_size() { return true; }\n"
    "};\n";

  return true;
}
