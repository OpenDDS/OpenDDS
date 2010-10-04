/*
 * $Id$
 *
 * Copyright 2010 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "marshal_generator.h"
#include "be_extern.h"

#include "utl_identifier.h"

#include <string>
#include <sstream>
#include <cctype>
using std::string;

using namespace AstTypeClassification;

bool marshal_generator::gen_enum(UTL_ScopedName* name,
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

  enum WrapDirection {WD_OUTPUT, WD_INPUT};

  string wrapPrefix(AST_Type* type, WrapDirection wd)
  {
    switch (type->node_type()) {
    case AST_Decl::NT_pre_defined:
      {
        AST_PredefinedType* p = AST_PredefinedType::narrow_from_decl(type);
        switch (p->pt()) {
        case AST_PredefinedType::PT_char:
          return (wd == WD_OUTPUT)
            ? "ACE_OutputCDR::from_char(" : "ACE_InputCDR::to_char(";
        case AST_PredefinedType::PT_wchar:
          return (wd == WD_OUTPUT)
            ? "ACE_OutputCDR::from_wchar(" : "ACE_InputCDR::to_wchar(";
        case AST_PredefinedType::PT_octet:
          return (wd == WD_OUTPUT)
            ? "ACE_OutputCDR::from_octet(" : "ACE_InputCDR::to_octet(";
        case AST_PredefinedType::PT_boolean:
          return (wd == WD_OUTPUT)
            ? "ACE_OutputCDR::from_boolean(" : "ACE_InputCDR::to_boolean(";
        default:
          return "";
        }
      }
    case AST_Decl::NT_string:
      return (wd == WD_OUTPUT)
        ? "ACE_OutputCDR::from_string(" : "ACE_InputCDR::to_string(";
    case AST_Decl::NT_wstring:
      return (wd == WD_OUTPUT)
        ? "ACE_OutputCDR::from_wstring(" : "ACE_InputCDR::to_wstring(";
    default:
      return "";
    }
  }

  string getWrapper(const string name, AST_Type* type, WrapDirection wd)
  {
    string pre = wrapPrefix(type, wd);
    return (pre.empty()) ? name : (pre + name + ')');
  }

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

  void gen_sequence(UTL_ScopedName* tdname, AST_Sequence* seq)
  {
    be_global->add_include("dds/DCPS/Serializer.h");
    NamespaceGuard ng;
    string cxx = scoped(tdname);
    AST_Type* elem = seq->base_type();
    unTypeDef(elem);
    Classification elem_cls = classify(elem);
    if (!elem->in_main_file()) {
      if (elem->node_type() == AST_Decl::NT_pre_defined) {
        be_global->add_include(("dds/CorbaSeq/" + nameOfSeqHeader(elem)
          + "SeqTypeSupportImpl.h").c_str(), BE_GlobalData::STREAM_CPP);
      } else {
        be_global->add_referenced(elem->file_name().c_str());
      }
    }
    string cxx_elem = scoped(elem->name());
    {
      Function is_bounded("gen_is_bounded_size", "bool");
      is_bounded.addArg("seq", "const " + cxx + "&");
      is_bounded.endArgs();
      if (seq->unbounded() || (elem_cls & CL_UNION)) {
        be_global->impl_ << "  return false;\n";
      } else if (elem_cls & CL_STRING) {
        be_global->impl_ << "  return "
          << ((elem_cls & CL_BOUNDED) ? "true" : "false") << ";\n";
      } else if (elem_cls & (CL_STRUCTURE | CL_SEQUENCE | CL_ARRAY)) {
        const bool arr = (elem_cls & CL_ARRAY);
        be_global->impl_ << "  " << scoped(seq->base_type()->name())
          << " element;\n";
        if (arr) {
          be_global->impl_ <<
            "  " << cxx_elem << "_forany element_forany(element);\n";
        }
        be_global->impl_ <<
          "  return gen_is_bounded_size(element" << (arr ? "_forany" : "")
          << ");\n";
      } else { // primitive
        be_global->impl_ << "  return true;\n";
      }
    }
    {
      Function is_bounded("gen_max_marshaled_size", "size_t");
      is_bounded.addArg("seq", "const " + cxx + "&");
      is_bounded.endArgs();
      std::ostringstream expr;
      string intro;
      if (elem_cls & CL_ENUM) {
        expr << "seq.maximum() * max_marshaled_size_ulong()";
      } else if (elem_cls & CL_STRING) {
        expr << "seq.maximum() * (max_marshaled_size_ulong()";
        AST_String* str = AST_String::narrow_from_decl(elem);
        if (elem_cls & CL_BOUNDED) {
          expr << " + " << str->max_size()->ev()->u.ulval;
        }
        expr << ")";
      } else if (elem_cls & CL_PRIMITIVE) {
        expr << "seq.maximum() * " << getMaxSizeExprPrimitive(elem);
      } else if (elem_cls & CL_UNION) {
        expr << "100000 /* from TAO_IDL_BE */";
      } else if (elem_cls & (CL_SEQUENCE | CL_STRUCTURE)) {
        expr << "seq.maximum() * gen_max_marshaled_size("
          << scoped(seq->base_type()->name()) << "())";
      } else if (elem_cls & CL_ARRAY) {
        intro += "  " + cxx_elem + " tmp;\n";
        expr << "seq.maximum() * gen_max_marshaled_size(" << cxx_elem
          << "_forany(tmp))";
      } else {
        expr << '0';
      }
      be_global->impl_ << intro <<
        "  return max_marshaled_size_ulong() + " << expr.str() << ";\n";
    }
    {
      Function find_size("gen_find_size", "size_t");
      find_size.addArg("seq", "const " + cxx + "&");
      find_size.endArgs();
      if (elem_cls & CL_ENUM) {
        be_global->impl_ << "  return max_marshaled_size_ulong() + "
          "seq.length() * max_marshaled_size_ulong();\n";
      } else if (elem_cls & CL_PRIMITIVE) {
        be_global->impl_ << "  return max_marshaled_size_ulong() + "
          "seq.length() * " << getMaxSizeExprPrimitive(elem) << ";\n";
      } else if (elem_cls & CL_INTERFACE) {
        be_global->impl_ <<
          "  return 0; // sequence of objrefs is not marshaled\n";
      } else { // String, Struct, Array, Sequence, Union
        be_global->impl_ <<
          "  size_t length = max_marshaled_size_ulong();\n"
          "  for (CORBA::ULong i = 0; i < seq.length(); ++i) {\n";
        if (elem_cls & CL_STRING) {
          be_global->impl_ <<
            "    length += max_marshaled_size_ulong() + "
            "(seq[i] ? ACE_OS::strlen(seq[i]) : 0)"
            << ((elem_cls & CL_WIDE) ? " * sizeof(ACE_CDR::WChar);\n" : ";\n");
        } else if (elem_cls & CL_ARRAY) {
          be_global->impl_ <<
            "    " << cxx_elem << "_var tmp_var = " << cxx_elem
            << "_dup(seq[i]);\n"
            "    " << cxx_elem << "_forany tmp = tmp_var.inout();\n"
            "    length += gen_find_size(tmp);\n";
        } else {
          be_global->impl_ <<
            "    length += gen_find_size(seq[i]);\n";
        }
        be_global->impl_ <<
          "  }\n"
          "  return length;\n";
      }
    }
    {
      Function insertion("operator<<", "bool");
      insertion.addArg("strm", "Serializer&");
      insertion.addArg("seq", "const " + cxx + "&");
      insertion.endArgs();
      be_global->impl_ <<
        "  const CORBA::ULong length = seq.length();\n"
        << streamAndCheck("<< length");
      if (elem_cls & CL_PRIMITIVE) {
        be_global->impl_ <<
          "  return strm.write_" << getSerializerName(elem)
          << "_array(seq.get_buffer(), length);\n";
      } else if (elem_cls & CL_INTERFACE) {
        be_global->impl_ <<
          "  return false; // sequence of objrefs is not marshaled\n";
      } else { // Enum, String, Struct, Array, Sequence, Union
        be_global->impl_ <<
          "  for (CORBA::ULong i = 0; i < length; ++i) {\n";
        if (elem_cls & CL_ARRAY) {
          const string typedefname = scoped(seq->base_type()->name());
          be_global->impl_ <<
            "    " << typedefname << "_var tmp_var = " << typedefname
            << "_dup(seq[i]);\n"
            "    " << typedefname << "_forany tmp = tmp_var.inout();\n"
            << streamAndCheck("<< tmp", 4);
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
      extraction.addArg("seq", cxx + "&");
      extraction.endArgs();
      be_global->impl_ <<
        "  CORBA::ULong length;\n"
        << streamAndCheck(">> length");
      if (!seq->unbounded()) {
        be_global->impl_ <<
          "  if (length > seq.maximum ()) {\n"
          "    return false;\n"
          "  }\n";
      }
      be_global->impl_ <<
        "  seq.length(length);\n";
      if (elem_cls & CL_PRIMITIVE) {
        be_global->impl_ <<
          "  if (length == 0) {\n"
          "    return true;\n"
          "  }\n"
          "  return strm.read_" << getSerializerName(elem)
          << "_array(seq.get_buffer(), length);\n";
      } else if (elem_cls & CL_INTERFACE) {
        be_global->impl_ <<
          "  return false; // sequence of objrefs is not marshaled\n";
      } else { // Enum, String, Struct, Array, Sequence, Union
        be_global->impl_ <<
          "  for (CORBA::ULong i = 0; i < length; ++i) {\n";
        if (elem_cls & CL_ARRAY) {
          const string typedefname = scoped(seq->base_type()->name());
          be_global->impl_ <<
            "    " << typedefname << "_var tmp = " << typedefname
            << "_alloc();\n"
            "    " << typedefname << "_forany fa = tmp.inout();\n"
            << streamAndCheck(">> fa", 4) <<
            "    " << typedefname << "_copy(seq[i], tmp.in());\n";
        } else if (elem_cls & CL_STRING) {
          if (elem_cls & CL_BOUNDED) {
            AST_String* str = AST_String::narrow_from_decl(elem);
            std::ostringstream args;
            args << "seq[i].out(), " << str->max_size()->ev()->u.ulval;
            be_global->impl_ <<
              streamAndCheck(">> " + getWrapper(args.str(), elem, WD_INPUT), 4);
          } else { // unbounded string
            be_global->impl_ << streamAndCheck(">> seq.get_buffer()[i]", 4);
          }
        } else { // Enum, Struct, Sequence, Union
          be_global->impl_ << streamAndCheck(">> seq[i]", 4);
        }
        be_global->impl_ <<
          "  }\n"
          "  return true;\n";
      }
    }
  }

  void gen_array(UTL_ScopedName* name, AST_Array* arr)
  {
    be_global->add_include("dds/DCPS/Serializer.h");
    NamespaceGuard ng;
    string cxx = scoped(name);
    AST_Type* elem = arr->base_type();
    unTypeDef(elem);
    Classification elem_cls = classify(elem);
    if (!elem->in_main_file()
        && elem->node_type() != AST_Decl::NT_pre_defined) {
      be_global->add_referenced(elem->file_name().c_str());
    }
    string cxx_elem = scoped(elem->name());
    {
      Function is_bounded("gen_is_bounded_size", "bool");
      is_bounded.addArg("arr", "const " + cxx + "_forany&");
      is_bounded.endArgs();
      if (elem_cls & CL_UNION) {
        be_global->impl_ << "  return false;\n";
      } else if (elem_cls & CL_STRING) {
        be_global->impl_ << "  return "
          << ((elem_cls & CL_BOUNDED) ? "true" : "false") << ";\n";
      } else if (elem_cls & (CL_STRUCTURE | CL_SEQUENCE)) {
        be_global->impl_ <<
          "  return gen_is_bounded_size(arr[CORBA::ULong(0)]);\n";
      } else if (elem_cls & CL_ARRAY) {
        be_global->impl_ <<
          "  " << cxx_elem << "_var tmp_var = " << cxx_elem << "_dup(arr";
        size_t ndims = AST_Array::narrow_from_decl(elem)->n_dims();
        for (size_t i = 0; i < ndims; ++i) {
          be_global->impl_ << "[CORBA::ULong(0)]";
        }
        be_global->impl_ << ");\n"
          "  " << cxx_elem << "_forany tmp = tmp_var.inout();\n"
          "  return gen_is_bounded_size(tmp);\n";
      } else { // primitive
        be_global->impl_ << "  return true;\n";
      }
    }
    size_t n_elems = 1;
    for (size_t i = 0; i < arr->n_dims(); ++i) {
      n_elems *= arr->dims()[i]->ev()->u.ulval;
    }
    {
      Function is_bounded("gen_max_marshaled_size", "size_t");
      is_bounded.addArg("arr", "const " + cxx + "_forany&");
      is_bounded.endArgs();
      std::ostringstream expr;
      string intro;
      if (elem_cls & CL_ENUM) {
        expr << "max_marshaled_size_ulong()";
      } else if (elem_cls & CL_STRING) {
        expr << "(max_marshaled_size_ulong()";
        AST_String* str = AST_String::narrow_from_decl(elem);
        if (elem_cls & CL_BOUNDED) {
          expr << " + " << str->max_size()->ev()->u.ulval;
        }
        expr << ")";
      } else if (elem_cls & CL_PRIMITIVE) {
        expr << getMaxSizeExprPrimitive(elem);
      } else if (elem_cls & CL_ARRAY) {
        intro += "  " + cxx_elem + "_var tmp_var = " + cxx_elem + "_dup(arr";
        size_t ndims = AST_Array::narrow_from_decl(elem)->n_dims();
        for (size_t i = 0; i < ndims; ++i) {
          intro += "[CORBA::ULong(0)]";
        }
        intro += ");\n"
          "  " + cxx_elem + "_forany tmp = tmp_var.inout();\n";
        expr << "gen_max_marshaled_size(tmp)";
      } else { // Struct, Sequence, Union
        expr << "gen_max_marshaled_size(" << scoped(arr->base_type()->name())
          << "())";
      }
      be_global->impl_ << intro <<
        "  return " << n_elems << " * " << expr.str() << ";\n";
    }
    {
      Function find_size("gen_find_size", "size_t");
      find_size.addArg("arr", "const " + cxx + "_forany&");
      find_size.endArgs();
      if (elem_cls & CL_ENUM) {
        be_global->impl_ <<
          "  return " << n_elems << " * max_marshaled_size_ulong();\n";
      } else if (elem_cls & CL_PRIMITIVE) {
        be_global->impl_ <<
          "  return " << n_elems << " * " << getMaxSizeExprPrimitive(elem)
          << ";\n";
      } else { // String, Struct, Array, Sequence, Union
        be_global->impl_ <<
          "  size_t length = 0;\n";
        {
          string indent = "  ";
          NestedForLoops nfl("CORBA::ULong", "i", arr, indent);
          if (elem_cls & CL_STRING) {
            be_global->impl_ <<
              indent << "length += max_marshaled_size_ulong() + "
              "ACE_OS::strlen(arr" << nfl.index_ << ")"
              << ((elem_cls & CL_WIDE) ? " * sizeof(ACE_CDR::WChar);\n"
              : ";\n");
          } else if (elem_cls & CL_ARRAY) {
            be_global->impl_ <<
              indent << cxx_elem << "_var tmp_var = " << cxx_elem
              << "_dup(arr" << nfl.index_ << ");\n" <<
              indent << cxx_elem << "_forany tmp = tmp_var.inout();\n" <<
              indent << "length += gen_find_size(tmp);\n";
          } else {
            be_global->impl_ <<
              "    length += gen_find_size(arr" << nfl.index_ << ");\n";
          }
        }
        be_global->impl_ << "  return length;\n";
      }
    }
    {
      Function insertion("operator<<", "bool");
      insertion.addArg("strm", "Serializer&");
      insertion.addArg("arr", "const " + cxx + "_forany&");
      insertion.endArgs();
      if (elem_cls & CL_PRIMITIVE) {
        be_global->impl_ <<
          "  return strm.write_" << getSerializerName(elem)
          << "_array(arr.in(), " << n_elems << ");\n";
      } else { // Enum, String, Struct, Array, Sequence, Union
        {
          string indent = "  ";
          NestedForLoops nfl("CORBA::ULong", "i", arr, indent);
          if (elem_cls & CL_ARRAY) {
            be_global->impl_ <<
              indent << cxx_elem << "_var tmp_var = " << cxx_elem
              << "_dup(arr" << nfl.index_ << ");\n" <<
              indent << cxx_elem << "_forany tmp = tmp_var.inout();\n" <<
              streamAndCheck("<< tmp", indent.size());
          } else {
            be_global->impl_ <<
              streamAndCheck("<< arr" + nfl.index_, indent.size());
          }
        }
        be_global->impl_ << "  return true;\n";
      }
    }
    {
      Function extraction("operator>>", "bool");
      extraction.addArg("strm", "Serializer&");
      extraction.addArg("arr", cxx + "_forany&");
      extraction.endArgs();
      if (elem_cls & CL_PRIMITIVE) {
        be_global->impl_ <<
          "  return strm.read_" << getSerializerName(elem)
          << "_array(arr.out(), " << n_elems << ");\n";
      } else { // Enum, String, Struct, Array, Sequence, Union
        {
          string indent = "  ";
          NestedForLoops nfl("CORBA::ULong", "i", arr, indent);
          if (elem_cls & CL_ARRAY) {
            const string typedefname = scoped(arr->base_type()->name());
            be_global->impl_ <<
              indent << typedefname << "_var tmp = " << typedefname
              << "_alloc();\n" <<
              indent << typedefname << "_forany fa = tmp.inout();\n"
              << streamAndCheck(">> fa", indent.size()) <<
              indent << typedefname << "_copy(arr" << nfl.index_ <<
              ", tmp.in());\n";
          } else {
            string suffix = (elem_cls & CL_STRING) ? ".out()" : "";
            be_global->impl_ <<
              streamAndCheck(">> arr" + nfl.index_ + suffix, indent.size());
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
}

bool marshal_generator::gen_typedef(UTL_ScopedName* name, AST_Type* base,
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
                        const string& prefix, string& intro)
  {
    AST_Type* typedeff = type;
    unTypeDef(type);
    Classification fld_cls = classify(type);
    const string qual = prefix + '.' + name;
    if (fld_cls & CL_ENUM) {
      return "max_marshaled_size_ulong()";
    } else if (fld_cls & CL_STRING) {
      return "max_marshaled_size_ulong() + ACE_OS::strlen(" + qual + ")"
        + ((fld_cls & CL_WIDE) ? " * sizeof(ACE_CDR::WChar)": "");
    } else if (fld_cls & CL_PRIMITIVE) {
      return "gen_max_marshaled_size(" + getWrapper(qual, type, WD_OUTPUT)
        + ')';
    } else { // sequence, struct, union, array
      string fieldref = prefix, local = name;
      if (fld_cls & CL_ARRAY) {
        intro += "  " + getArrayForany(prefix.c_str(), name.c_str(),
                          scoped(typedeff->name())) + '\n';
        fieldref += '_';
        if (local.size() > 2 && local.substr(local.size() - 2) == "()") {
          local.erase(local.size() - 2);
        }
      } else {
        fieldref += '.';
      }
      return "gen_find_size(" + fieldref + local + ')';
    }
  }

  // common to both fields (in structs) and branches (in unions)
  string streamCommon(const string& name, AST_Type* type,
                      const string& prefix, string& intro)
  {
    AST_Type* typedeff = type;
    unTypeDef(type);
    Classification fld_cls = classify(type);
    const string qual = prefix + '.' + name, shift = prefix.substr(0, 2);
    WrapDirection dir = (shift == ">>") ? WD_INPUT : WD_OUTPUT;
    if ((fld_cls & CL_STRING) && (dir == WD_INPUT)) {
      return "(strm " + qual + ".out())";
    } else if (fld_cls & CL_PRIMITIVE) {
      return "(strm " + shift + ' '
        + getWrapper(qual.substr(3), type, dir) + ')';
    } else { // sequence, struct, union, array, enum, string(insertion)
      string fieldref = prefix, local = name;
      if (fld_cls & CL_ARRAY) {
        string pre = prefix;
        if (shift == ">>" || shift == "<<") {
          pre.erase(0, 3);
        }
        if (local.size() > 2 && local.substr(local.size() - 2) == "()") {
          local.erase(local.size() - 2);
        }
        intro += "  " + getArrayForany(pre.c_str(), name.c_str(),
          scoped(typedeff->name())) + '\n';
        fieldref += '_';
      } else {
        fieldref += '.';
      }
      return "(strm " + fieldref + local + ')';
    }
  }
}

bool marshal_generator::gen_struct(UTL_ScopedName* name,
  const std::vector<AST_Field*>& fields, const char*)
{
  NamespaceGuard ng;
  be_global->add_include("dds/DCPS/Serializer.h");
  string cxx = scoped(name); // name as a C++ class
  {
    Function is_bounded("gen_is_bounded_size", "bool");
    is_bounded.addArg("stru", "const " + cxx + "&");
    is_bounded.endArgs();
    string intro, expr = "true";
    for (size_t i = 0; i < fields.size(); ++i) {
      AST_Type* field_type = fields[i]->field_type();
      unTypeDef(field_type);
      Classification fld_cls = classify(field_type);
      if ((fld_cls & CL_STRING) && !(fld_cls & CL_BOUNDED)) {
        expr = "false";
        intro = "";
        break;
      } else if (fld_cls & (CL_STRUCTURE | CL_SEQUENCE | CL_ARRAY | CL_UNION)) {
        if (expr == "true") expr = ""; else expr += " && ";
        const char* fname = fields[i]->local_name()->get_string();
        if (fld_cls & CL_ARRAY) {
          intro += "  " +
            getArrayForany("stru", fname,
              scoped(fields[i]->field_type()->name())) + "\n";
        }
        expr += string("gen_is_bounded_size(stru")
          + ((fld_cls & CL_ARRAY) ? '_' : '.') + fname + ")";
      }
    }
    be_global->impl_ << intro << "  return " << expr << ";\n";
  }
  {
    Function max_marsh("gen_max_marshaled_size", "size_t");
    max_marsh.addArg("stru", "const " + cxx + "&");
    max_marsh.endArgs();
    std::ostringstream expr;
    string intro;
    for (size_t i = 0; i < fields.size(); ++i) {
      if (i) expr << "\n    + ";
      const char* fname = fields[i]->local_name()->get_string();
      AST_Type* field_type = fields[i]->field_type();
      unTypeDef(field_type);
      Classification fld_cls = classify(field_type);
      if (!field_type->in_main_file()
          && field_type->node_type() != AST_Decl::NT_pre_defined) {
        be_global->add_referenced(field_type->file_name().c_str());
      }
      if (fld_cls & CL_ENUM) {
        expr << "max_marshaled_size_ulong()";
      } else if (fld_cls & CL_STRING) {
        AST_String* str = AST_String::narrow_from_decl(field_type);
        expr << "max_marshaled_size_ulong()";
        if (fld_cls & CL_BOUNDED) {
          expr << " + " << str->max_size()->ev()->u.ulval;
        }
      } else { // predefined, sequence, struct, union, array
        string fieldref = string("stru");
        if (fld_cls & CL_ARRAY) {
          intro += "  " + getArrayForany("stru", fname,
            scoped(field_type->name()))
            + "\n";
          fieldref += '_';
        } else {
          fieldref += '.';
        }
        expr << "gen_max_marshaled_size("
          << getWrapper(fieldref + fname, field_type, WD_OUTPUT) << ")";
      }
    }
    be_global->impl_ << intro << "  return " << expr.str() << ";\n";
  }
  {
    Function find_size("gen_find_size", "size_t");
    find_size.addArg("stru", "const " + cxx + "&");
    find_size.endArgs();
    string expr, intro;
    for (size_t i = 0; i < fields.size(); ++i) {
      if (i) expr += "\n    + ";
      expr += findSizeCommon(fields[i]->local_name()->get_string(),
        fields[i]->field_type(), "stru", intro);
    }
    be_global->impl_ << intro << "  return " << expr << ";\n";
  }
  {
    Function insertion("operator<<", "bool");
    insertion.addArg("strm", "Serializer&");
    insertion.addArg("stru", "const " + cxx + "&");
    insertion.endArgs();
    string expr, intro;
    for (size_t i = 0; i < fields.size(); ++i) {
      if (i) expr += "\n    && ";
      expr += streamCommon(fields[i]->local_name()->get_string(),
        fields[i]->field_type(), "<< stru", intro);
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
      expr += streamCommon(fields[i]->local_name()->get_string(),
        fields[i]->field_type(), ">> stru", intro);
    }
    be_global->impl_ << intro << "  return " << expr << ";\n";
  }
  return true;
}

namespace {
  // see TAO_IDL_BE be_union::gen_empty_default_label()
  bool needSyntheticDefault(AST_Type* disc, size_t n_labels)
  {
    AST_Decl::NodeType nt = disc->node_type ();
    if (nt == AST_Decl::NT_enum) return true;

    AST_PredefinedType* pdt = AST_PredefinedType::narrow_from_decl(disc);
    switch (pdt->pt()) {
    case AST_PredefinedType::PT_boolean:
      return n_labels < 2;
    case AST_PredefinedType::PT_char:
      return n_labels < ACE_OCTET_MAX;
    case AST_PredefinedType::PT_short:
    case AST_PredefinedType::PT_ushort:
      return n_labels < ACE_UINT16_MAX;
    case AST_PredefinedType::PT_long:
    case AST_PredefinedType::PT_ulong:
      return n_labels < ACE_UINT32_MAX;
    default:
      return true;
    }
  }

  std::ostream& operator<<(std::ostream& o, const AST_Expression& e)
  {
    // TAO_IDL_FE interfaces are not const-correct
    AST_Expression& e_nonconst = const_cast<AST_Expression&>(e);
    const AST_Expression::AST_ExprValue& ev = *e_nonconst.ev();
    switch (ev.et) {
    case AST_Expression::EV_short:
      return o << ev.u.sval;
    case AST_Expression::EV_ushort:
      return o << ev.u.usval;
    case AST_Expression::EV_long:
      return o << ev.u.lval;
    case AST_Expression::EV_ulong:
      return o << ev.u.ulval;
    case AST_Expression::EV_longlong:
      return o << ev.u.llval << "LL";
    case AST_Expression::EV_ulonglong:
      return o << ev.u.ullval << "ULL";
    case AST_Expression::EV_char:
      return o << '\'' << ev.u.cval << '\'';
    case AST_Expression::EV_bool:
      return o << std::boolalpha << static_cast<bool>(ev.u.bval);
    default:
      return o;
    }
  }

  string getEnumLabel(AST_Expression* label_val, AST_Type* disc)
  {
    string e = scoped(disc->name()),
      label = label_val->n()->last_component()->get_string();
    const size_t colon = e.rfind("::");
    if (colon == string::npos) {
      return label;
    }
    return e.replace(colon + 2, string::npos, label);
  }

  void generateBranchLabels(AST_UnionBranch* branch,AST_Type* discriminator,
                            size_t& n_labels, bool& has_default)
  {
    for (unsigned long j = 0; j < branch->label_list_length(); ++j) {
      ++n_labels;
      AST_UnionLabel* label = branch->label(j);
      if (label->label_kind() == AST_UnionLabel::UL_default) {
        be_global->impl_ << "  default:\n";
        has_default = true;
      } else if (discriminator->node_type() == AST_Decl::NT_enum) {
        be_global->impl_ << "  case "
          << getEnumLabel(label->label_val(), discriminator) << ":\n";
      } else {
        be_global->impl_ << "  case " << *label->label_val() << ":\n";
      }
    }
  }

  typedef string (*CommonFn)(const string& name, AST_Type* type,
                             const string& prefix, string& intro);
  void generateSwitchBodyForUnion(CommonFn commonFn,
    const std::vector<AST_UnionBranch*>& branches, AST_Type* discriminator,
    const char* statementPrefix, const char* namePrefix = "")
  {
    size_t n_labels = 0;
    bool has_default = false;
    for (size_t i = 0; i < branches.size(); ++i) {
      AST_UnionBranch* branch = branches[i];
      generateBranchLabels(branch, discriminator, n_labels, has_default);
      string intro, name = branch->local_name()->get_string();
      if (namePrefix == string(">> ")) {
        string brType = scoped(branch->field_type()->name()), forany;
        AST_Type* br = branch->field_type();
        unTypeDef(br);
        Classification br_cls = classify(br);
        if (!br->in_main_file()
            && br->node_type() != AST_Decl::NT_pre_defined) {
          be_global->add_referenced(br->file_name().c_str());
        }
        string rhs;
        if (br_cls & CL_STRING) {
          brType = string("CORBA::") + ((br_cls & CL_WIDE) ? "W" : "")
            + "String_var";
          rhs = "tmp.out()";
        } else if (br_cls & CL_ARRAY) {
          forany = "      " + brType + "_forany fa = tmp;\n";
          rhs = getWrapper("fa", br, WD_INPUT);
        } else {
          rhs = getWrapper("tmp", br, WD_INPUT);
        }
        be_global->impl_ <<
          "    {\n"
          "      " << brType << " tmp;\n" << forany <<
          "      if (strm >> " << rhs << ") {\n"
          "        uni." << name << "(tmp);\n"
          "        uni._d(disc);\n"
          "        return true;\n"
          "      }\n"
          "      return false;\n"
          "    }\n";
      } else {
        string expr = commonFn(name + "()", branch->field_type(),
          string(namePrefix) + "uni", intro);
        be_global->impl_ <<
          "    {\n" <<
          (intro.length() ? "    " : "") << intro <<
          "      " << statementPrefix << " " << expr << ";\n" <<
          (statementPrefix == string("return") ? "" : "      break;\n") <<
          "    }\n";
      }
    }
    if (!has_default && needSyntheticDefault(discriminator, n_labels)) {
      be_global->impl_ <<
        "  default:\n" <<
        ((namePrefix == string(">> ")) ? "    uni._d(disc);\n" : "") <<
        "    break;\n";
    }
  }
}

bool marshal_generator::gen_union(UTL_ScopedName* name,
   const std::vector<AST_UnionBranch*>& branches, AST_Type* discriminator,
   AST_Expression::ExprType, const AST_Union::DefaultValue&, const char*)
{
  NamespaceGuard ng;
  be_global->add_include("dds/DCPS/Serializer.h");
  string cxx = scoped(name); // name as a C++ class
  {
    Function is_bounded("gen_is_bounded_size", "bool");
    is_bounded.addArg("", "const " + cxx + "&");
    is_bounded.endArgs();
    be_global->impl_ << "  return false;\n";
  }
  {
    Function is_bounded("gen_max_marshaled_size", "size_t");
    is_bounded.addArg("", "const " + cxx + "&");
    is_bounded.endArgs();
    be_global->impl_ << "  return 100000; /* from TAO_IDL_BE */\n";
  }
  {
    Function is_bounded("gen_find_size", "size_t");
    is_bounded.addArg("uni", "const " + cxx + "&");
    is_bounded.endArgs();
    be_global->impl_ <<
      "  size_t result = gen_max_marshaled_size("
      << getWrapper("uni._d()", discriminator, WD_OUTPUT) << ");\n"
      "  switch (uni._d()) {\n";
    generateSwitchBodyForUnion(findSizeCommon, branches, discriminator,
      "result +=");
    be_global->impl_ <<
      "  }\n"
      "  return result;\n";
  }
  {
    Function insertion("operator<<", "bool");
    insertion.addArg("strm", "Serializer&");
    insertion.addArg("uni", "const " + cxx + "&");
    insertion.endArgs();
    be_global->impl_ << streamAndCheck("<< uni._d()") <<
      "  switch (uni._d()) {\n";
    generateSwitchBodyForUnion(streamCommon, branches, discriminator,
      "return", "<< ");
    be_global->impl_ <<
      "  }\n"
      "  return true;\n";
  }
  {
    Function extraction("operator>>", "bool");
    extraction.addArg("strm", "Serializer&");
    extraction.addArg("uni", cxx + "&");
    extraction.endArgs();
    be_global->impl_ <<
      "  " << scoped(discriminator->name()) << " disc;\n" <<
      streamAndCheck(">> disc") <<
      "  switch (disc) {\n";
    generateSwitchBodyForUnion(streamCommon, branches, discriminator,
      "if", ">> ");
    be_global->impl_ <<
      "  }\n"
      "  return true;\n";
  }
  return true;
}
