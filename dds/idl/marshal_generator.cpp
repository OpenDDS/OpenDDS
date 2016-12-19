/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "marshal_generator.h"
#include "be_extern.h"

#include "utl_identifier.h"

#include <string>
#include <map>
#include <sstream>
#include <iostream>
#include <cctype>
using std::string;

using namespace AstTypeClassification;

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

  void gen_sequence(UTL_ScopedName* tdname, AST_Sequence* seq)
  {
    be_global->add_include("dds/DCPS/Serializer.h");
    NamespaceGuard ng;
    string cxx = scoped(tdname);
    if (isRtpsSpecialSequence(cxx)) {
      genRtpsSpecialSequence(cxx);
      return;
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
    string cxx_elem = scoped(elem->name());
    {
      Function find_size("gen_find_size", "void");
      find_size.addArg("seq", "const " + cxx + "&");
      find_size.addArg("size", "size_t&");
      find_size.addArg("padding", "size_t&");
      find_size.endArgs();
      be_global->impl_ <<
        "  find_size_ulong(size, padding);\n"
        "  if (seq.length() == 0) {\n"
        "    return;\n"
        "  }\n";
      if (elem_cls & CL_ENUM) {
        be_global->impl_ <<
          "  size += seq.length() * max_marshaled_size_ulong();\n";
      } else if (elem_cls & CL_PRIMITIVE) {
        be_global->impl_ << checkAlignment(elem) <<
          "  size += seq.length() * " << getMaxSizeExprPrimitive(elem) << ";\n";
      } else if (elem_cls & CL_INTERFACE) {
        be_global->impl_ <<
          "  // sequence of objrefs is not marshaled\n";
      } else if (elem_cls == CL_UNKNOWN) {
        be_global->impl_ <<
          "  // sequence of unknown/unsupported type\n";
      } else { // String, Struct, Array, Sequence, Union
        be_global->impl_ <<
          "  for (CORBA::ULong i = 0; i < seq.length(); ++i) {\n";
        if (elem_cls & CL_STRING) {
          be_global->impl_ <<
            "    find_size_ulong(size, padding);\n"
            "    if (seq[i]) {\n"
            "      size += ACE_OS::strlen(seq[i])"
            << ((elem_cls & CL_WIDE)
                ? " * OpenDDS::DCPS::Serializer::WCHAR_SIZE;\n"
                : " + 1;\n") <<
            "    }\n";
        } else if (elem_cls & CL_ARRAY) {
          be_global->impl_ <<
            "    " << cxx_elem << "_var tmp_var = " << cxx_elem
            << "_dup(seq[i]);\n"
            "    " << cxx_elem << "_forany tmp = tmp_var.inout();\n"
            "    gen_find_size(tmp, size, padding);\n";
        } else { // Struct, Sequence, Union
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
      insertion.addArg("seq", "const " + cxx + "&");
      insertion.endArgs();
      be_global->impl_ <<
        "  const CORBA::ULong length = seq.length();\n"
        << streamAndCheck("<< length") <<
        "  if (length == 0) {\n"
        "    return true;\n"
        "  }\n";
      if (elem_cls & CL_PRIMITIVE) {
        be_global->impl_ <<
          "  return strm.write_" << getSerializerName(elem)
          << "_array(seq.get_buffer(), length);\n";
      } else if (elem_cls & CL_INTERFACE) {
        be_global->impl_ <<
          "  return false; // sequence of objrefs is not marshaled\n";
      } else if (elem_cls == CL_UNKNOWN) {
        be_global->impl_ <<
          "  return false; // sequence of unknown/unsupported type\n";
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
          "  if (length > seq.maximum()) {\n"
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
      } else if (elem_cls == CL_UNKNOWN) {
        be_global->impl_ <<
          "  return false; // sequence of unknown/unsupported type\n";
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
            const string getbuffer =
              (be_global->language_mapping() == BE_GlobalData::LANGMAP_NONE)
              ? ".get_buffer()" : "";
            be_global->impl_ << streamAndCheck(">> seq" + getbuffer + "[i]", 4);
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
    string cxx = scoped(name);
    AST_Type* elem = resolveActualType(arr->base_type());
    Classification elem_cls = classify(elem);
    if (!elem->in_main_file()
        && elem->node_type() != AST_Decl::NT_pre_defined) {
      be_global->add_referenced(elem->file_name().c_str());
    }
    string cxx_elem = scoped(elem->name());
    size_t n_elems = 1;
    for (size_t i = 0; i < arr->n_dims(); ++i) {
      n_elems *= arr->dims()[i]->ev()->u.ulval;
    }
    {
      Function find_size("gen_find_size", "void");
      find_size.addArg("arr", "const " + cxx + "_forany&");
      find_size.addArg("size", "size_t&");
      find_size.addArg("padding", "size_t&");
      find_size.endArgs();
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
            indent << "size += ACE_OS::strlen(arr" << nfl.index_ << ".in())"
            << ((elem_cls & CL_WIDE)
                ? " * OpenDDS::DCPS::Serializer::WCHAR_SIZE;\n"
                : " + 1;\n");
        } else if (elem_cls & CL_ARRAY) {
          be_global->impl_ <<
            indent << cxx_elem << "_var tmp_var = " << cxx_elem
            << "_dup(arr" << nfl.index_ << ");\n" <<
            indent << cxx_elem << "_forany tmp = tmp_var.inout();\n" <<
            indent << "gen_find_size(tmp, size, padding);\n";
        } else { // Struct, Sequence, Union
          be_global->impl_ <<
            indent << "gen_find_size(arr" << nfl.index_
            << ", size, padding);\n";
        }
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
            string suffix = (elem_cls & CL_STRING) ? ".in()" : "";
            be_global->impl_ <<
              streamAndCheck("<< arr" + nfl.index_ + suffix , indent.size());
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

  // This function looks through the fields of a struct for the key
  // specified and returns the AST_Type associated with that key.
  // Because the key name can contain indexed arrays and nested
  // structures, things can get interesting.
  AST_Type* find_type(const std::vector<AST_Field*>& fields, const string& key)
  {
    string key_base = key;   // the field we are looking for here
    string key_rem;          // the sub-field we will look for recursively
    bool is_array = false;
    size_t pos = key.find_first_of(".[");
    if (pos != std::string::npos) {
      key_base = key.substr(0, pos);
      if (key[pos] == '[') {
        is_array = true;
        size_t l_brack = key.find("]");
        if (l_brack == std::string::npos) {
          throw std::string("Missing right bracket");
        } else if (l_brack != key.length()) {
          key_rem = key.substr(l_brack+1);
        }
      } else {
        key_rem = key.substr(pos+1);
      }
    }
    for (size_t i = 0; i < fields.size(); ++i) {
      string field_name = fields[i]->local_name()->get_string();
      if (field_name == key_base) {
        AST_Type* field_type = fields[i]->field_type();
        if (!is_array && key_rem.empty()) {
          // The requested key field matches this one.  We do not allow
          // arrays (must be indexed specifically) or structs (must
          // identify specific sub-fields).
          AST_Structure* sub_struct = dynamic_cast<AST_Structure*>(field_type);
          if (sub_struct != 0) {
            throw std::string("Structs not allowed as keys");
          }
          AST_Typedef* typedef_node = dynamic_cast<AST_Typedef*>(field_type);
          if (typedef_node != 0) {
            AST_Array* array_node =
              dynamic_cast<AST_Array*>(typedef_node->base_type());
            if (array_node != 0) {
              throw std::string("Arrays not allowed as keys");
            }
          }
          return field_type;
        } else if (is_array) {
          // must be a typedef of an array
          AST_Typedef* typedef_node = dynamic_cast<AST_Typedef*>(field_type);
          if (typedef_node == 0) {
            throw std::string("Indexing for non-array type");
          }
          AST_Array* array_node =
            dynamic_cast<AST_Array*>(typedef_node->base_type());
          if (array_node == 0) {
            throw std::string("Indexing for non-array type");
          }
          if (array_node->n_dims() > 1) {
            throw std::string("Only single dimension arrays allowed in keys");
          }
          if (key_rem == "") {
            return array_node->base_type();
          } else {
            // This must be a struct...
            if ((key_rem[0] != '.') || (key_rem.length() == 1)) {
              throw std::string("Unexpected characters after array index");
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
          throw std::string("Expected structure field for ") + key_base;
        }
        size_t nfields = sub_struct->nfields();
        std::vector<AST_Field*> sub_fields;
        sub_fields.reserve(nfields);

        for (unsigned long i = 0; i < nfields; ++i) {
          AST_Field** f;
          sub_struct->field(f, i);
          sub_fields.push_back(*f);
        }
        // find type of nested struct field
        return find_type(sub_fields, key_rem);
      }
    }
    throw std::string("Field not found.");
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
      AST_Structure* struct_node = dynamic_cast<AST_Structure*>(type);
      for (unsigned long i = 0; i < struct_node->nfields(); ++i) {
        AST_Field** f;
        struct_node->field(f, i);
        if (!is_bounded_type((*f)->field_type())) {
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
      AST_Union* union_node = dynamic_cast<AST_Union*>(type);
      for (unsigned long i = 0; i < union_node->nfields(); ++i) {
        AST_Field** f;
        union_node->field(f, i);
        if (!is_bounded_type((*f)->field_type())) {
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
        AST_Structure* struct_node = dynamic_cast<AST_Structure*>(type);
        for (unsigned long i = 0; i < struct_node->nfields(); ++i) {
          AST_Field** f;
          struct_node->field(f, i);
          AST_Type* field_type = (*f)->field_type();
          max_marshaled_size(field_type, size, padding);
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
        for (unsigned long i = 0; i < union_node->nfields(); ++i) {
          AST_Field** f;
          union_node->field(f, i);
          AST_Type* field_type = (*f)->field_type();
          max_marshaled_size(field_type, size, padding);
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
    AST_Type* typedeff = type;
    type = resolveActualType(type);
    Classification fld_cls = classify(type);
    const string qual = prefix + '.' + name;
    const string indent = (prefix == "uni") ? "      " : "  ";
    if (fld_cls & CL_ENUM) {
      return indent + "find_size_ulong(size, padding);\n";
    } else if (fld_cls & CL_STRING) {
      const string suffix = (prefix == "uni") ? "" : ".in()";
      return indent + "find_size_ulong(size, padding);\n" +
        indent + "size += ACE_OS::strlen(" + qual + suffix + ")"
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
      return align +
        indent + "size += gen_max_marshaled_size(" +
        getWrapper(qual, type, WD_OUTPUT) + ");\n";
    } else if (fld_cls == CL_UNKNOWN) {
      return ""; // warning will be issued for the serialize functions
    } else { // sequence, struct, union, array
      string fieldref = prefix, local = name;
      if (fld_cls & CL_ARRAY) {
        intro += indent + getArrayForany(prefix.c_str(), name.c_str(),
                                         scoped(typedeff->name())) + '\n';
        fieldref += '_';
        if (local.size() > 2 && local.substr(local.size() - 2) == "()") {
          local.erase(local.size() - 2);
        }
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
    AST_Type* typedeff = type;
    type = resolveActualType(type);
    Classification fld_cls = classify(type);
    const string qual = prefix + '.' + name, shift = prefix.substr(0, 2);
    WrapDirection dir = (shift == ">>") ? WD_INPUT : WD_OUTPUT;
    if ((fld_cls & CL_STRING) && (dir == WD_INPUT)) {
      return "(strm " + qual + ".out())";
    } else if (fld_cls & CL_PRIMITIVE) {
      return "(strm " + shift + ' '
        + getWrapper(qual.substr(3), type, dir) + ')';
    } else if (fld_cls == CL_UNKNOWN) {
      if (dir == WD_INPUT) { // no need to warn twice
        std::cerr << "WARNING: field " << name << " can not be serialized.  "
          "The struct or union it belongs to (" << stru <<
          ") can not be used in an OpenDDS topic type." << std::endl;
      }
      return "false";
    } else { // sequence, struct, union, array, enum, string(insertion)
      string fieldref = prefix, local = name;
      const bool accessor =
        local.size() > 2 && local.substr(local.size() - 2) == "()";
      if (fld_cls & CL_ARRAY) {
        string pre = prefix;
        if (shift == ">>" || shift == "<<") {
          pre.erase(0, 3);
        }
        if (accessor) {
          local.erase(local.size() - 2);
        }
        intro += "  " + getArrayForany(pre.c_str(), name.c_str(),
          scoped(typedeff->name())) + '\n';
        fieldref += '_';
      } else {
        fieldref += '.';
      }
      if ((fld_cls & CL_STRING) && !accessor) local += ".in()";
      return "(strm " + fieldref + local + ')';
    }
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
}

bool marshal_generator::gen_struct(AST_Structure*, UTL_ScopedName* name,
  const std::vector<AST_Field*>& fields, AST_Type::SIZE_TYPE, const char*)
{
  NamespaceGuard ng;
  be_global->add_include("dds/DCPS/Serializer.h");
  string cxx = scoped(name); // name as a C++ class
  if (isRtpsSpecialStruct(cxx)) {
    return genRtpsSpecialStruct(cxx);
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
      expr += findSizeCommon(field_name, field_type, "stru", intro);
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
      expr += streamCommon(field_name, fields[i]->field_type(),
                           "<< stru", intro, cxx);
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
      expr += streamCommon(field_name, fields[i]->field_type(),
                           ">> stru", intro, cxx);
      if (!cond.empty()) {
        expr += ")";
      }
    }
    be_global->impl_ << intro << "  return " << expr << ";\n";
  }

  IDL_GlobalData::DCPS_Data_Type_Info* info = idl_global->is_dcps_type(name);
  // Only generate these methods if this is a DCPS type
  if (info != 0) {
    bool is_bounded_struct = true;
    {
      for (size_t i = 0; i < fields.size(); ++i) {
        if (!is_bounded_type(fields[i]->field_type())) {
          is_bounded_struct = false;
          break;
        }
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
    {
      IDL_GlobalData::DCPS_Data_Type_Info_Iter iter(info->key_list_);
      for (ACE_TString* kp = 0; iter.next(kp) != 0; iter.advance()) {
        string key_name = ACE_TEXT_ALWAYS_CHAR(kp->c_str());
        AST_Type* field_type = 0;
        try {
          field_type = find_type(fields, key_name);
        } catch (const std::string& error) {
          std::cerr << "ERROR: Invalid key specification for " << cxx
                    << " (" << key_name << "). " << error << std::endl;
          return false;
        }
        if (!is_bounded_type(field_type)) {
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
        IDL_GlobalData::DCPS_Data_Type_Info_Iter iter(info->key_list_);
        size_t size = 0, padding = 0;
        for (ACE_TString* kp = 0; iter.next(kp) != 0; iter.advance()) {
          string key_name = ACE_TEXT_ALWAYS_CHAR(kp->c_str());
          AST_Type* field_type = 0;
          try {
            field_type = find_type(fields, key_name);
          } catch (const std::string& error) {
            std::cerr << "ERROR: Invalid key specification for " << cxx
                      << " (" << key_name << "). " << error << std::endl;
            return false;
          }
          max_marshaled_size(field_type, size, padding);
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
      IDL_GlobalData::DCPS_Data_Type_Info_Iter iter(info->key_list_);
      for (ACE_TString* kp = 0; iter.next(kp) != 0; iter.advance()) {
        string key_name = ACE_TEXT_ALWAYS_CHAR(kp->c_str());
        AST_Type* field_type = 0;
        try {
          field_type = find_type(fields, key_name);
        } catch (const std::string& error) {
          std::cerr << "ERROR: Invalid key specification for " << cxx
                    << " (" << key_name << "). " << error << std::endl;
          return false;
        }
        expr += findSizeCommon(key_name, field_type, "stru.t", intro);
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
      IDL_GlobalData::DCPS_Data_Type_Info_Iter iter(info->key_list_);
      for (ACE_TString* kp = 0; iter.next(kp) != 0; iter.advance()) {
        string key_name = ACE_TEXT_ALWAYS_CHAR(kp->c_str());
        AST_Type* field_type = 0;
        try {
          field_type = find_type(fields, key_name);
        } catch (const std::string& error) {
          std::cerr << "ERROR: Invalid key specification for " << cxx
                    << " (" << key_name << "). " << error << std::endl;
          return false;
        }
        if (first) first = false;
        else       expr += "\n    && ";
        expr += streamCommon(key_name, field_type, "<< stru.t", intro);
      }
      if (first) be_global->impl_ << intro << "  return true;\n";
      else be_global->impl_ << intro << "  return " << expr << ";\n";
    }

    {
      Function extraction("operator>>", "bool");
      extraction.addArg("strm", "Serializer&");
      extraction.addArg("stru", "KeyOnly<" + cxx + ">");
      extraction.endArgs();

      bool first = true;
      string expr, intro;
      IDL_GlobalData::DCPS_Data_Type_Info_Iter iter(info->key_list_);
      for (ACE_TString* kp = 0; iter.next(kp) != 0; iter.advance()) {
        string key_name = ACE_TEXT_ALWAYS_CHAR(kp->c_str());
        AST_Type* field_type = 0;
        try {
          field_type = find_type(fields, key_name);
        } catch (const std::string& error) {
          std::cerr << "ERROR: Invalid key specification for " << cxx
                    << " (" << key_name << "). " << error << std::endl;
          return false;
        }
        if (first) first = false;
        else       expr += "\n    && ";
        expr += streamCommon(key_name, field_type, ">> stru.t", intro);
      }
      if (first) be_global->impl_ << intro << "  return true;\n";
      else be_global->impl_ << intro << "  return " << expr << ";\n";
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
      generateSwitchForUnion("uni._d()", findSizeCommon, branches, discriminator,
                                 "", "", cxx);
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
                                 "return", "<< ", cxx);
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
      generateSwitchBodyForUnion(streamCommon, branches, discriminator,
                                 "return", ">> ", cxx, true);
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
      generateSwitchForUnion("uni._d()", findSizeCommon, branches, discriminator,
                                 "", "", cxx);
    }
    {
      Function insertion("operator<<", "bool");
      insertion.addArg("strm", "Serializer&");
      insertion.addArg("uni", "const " + cxx + "&");
      insertion.endArgs();
      generateSwitchForUnion("uni._d()", streamCommon, branches, discriminator,
                                 "return", "<< ", cxx);
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

bool marshal_generator::gen_union(AST_Union*, UTL_ScopedName* name,
   const std::vector<AST_UnionBranch*>& branches, AST_Type* discriminator,
   const char*)
{
  NamespaceGuard ng;
  be_global->add_include("dds/DCPS/Serializer.h");
  string cxx = scoped(name); // name as a C++ class
  if (isRtpsSpecialUnion(cxx)) {
    return genRtpsSpecialUnion(cxx, discriminator, branches);
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
    be_global->impl_ <<
      "  size += gen_max_marshaled_size(" << wrap_out << ");\n";
      generateSwitchForUnion("uni._d()", findSizeCommon, branches, discriminator,
                               "", "", cxx);
  }
  {
    Function insertion("operator<<", "bool");
    insertion.addArg("strm", "Serializer&");
    insertion.addArg("uni", "const " + cxx + "&");
    insertion.endArgs();
    be_global->impl_ <<
      streamAndCheck("<< " + wrap_out);
    generateSwitchForUnion("uni._d()", streamCommon, branches, discriminator,
                               "return", "<< ", cxx);
    be_global->impl_ <<
      "  return true;\n";
  }
  {
    Function extraction("operator>>", "bool");
    extraction.addArg("strm", "Serializer&");
    extraction.addArg("uni", cxx + "&");
    extraction.endArgs();
    be_global->impl_ <<
      "  " << scoped(discriminator->name()) << " disc;\n" <<
      streamAndCheck(">> " + getWrapper("disc", discriminator, WD_INPUT));
    generateSwitchForUnion("disc", streamCommon, branches, discriminator,
                           "if", ">> ", cxx);
    be_global->impl_ <<
      "  return true;\n";
  }
  return true;
}
