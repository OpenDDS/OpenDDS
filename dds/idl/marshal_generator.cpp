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

#include <utl_identifier.h>

#include <string>
#include <sstream>
#include <iostream>
#include <cctype>
#include <map>

#define OPENDDS_IDL_STR(X) #X

using std::string;
using namespace AstTypeClassification;

namespace {
  template <typename Type, size_t length>
  size_t array_length(Type(&)[length])
  {
    return length;
  }

  const string RtpsNamespace = " ::OpenDDS::RTPS::", DdsNamespace = " ::DDS::";

  typedef bool (*is_special_case)(const string& cxx);
  typedef bool (*gen_special_case)(const string& cxx);

  typedef is_special_case is_special_sequence;
  typedef gen_special_case gen_special_sequence;

  typedef is_special_case is_special_struct;
  typedef gen_special_case gen_special_struct;

  typedef is_special_case is_special_union;
  typedef bool (*gen_special_union)(const string& cxx,
                                    AST_Union* u,
                                    AST_Type* discriminator,
                                    const std::vector<AST_UnionBranch*>& branches);

  struct special_sequence {
    is_special_sequence check;
    gen_special_sequence gen;
  };

  struct special_struct {
    is_special_struct check;
    gen_special_struct gen;
  };

  struct special_union {
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
                           AST_Union* u,
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

  const special_struct* get_special_struct(const std::string& name)
  {
    for (size_t i = 0; i < array_length(special_structs); ++i) {
      if (special_structs[i].check(name)) {
        return &special_structs[i];
      }
    }
    return 0;
  }

  const special_union special_unions[] = {
    {
      isRtpsSpecialUnion,
      genRtpsSpecialUnion,
    },
  };

  // TODO(iguessthislldo): Replace with Encoding::Kind from libOpenDDS_Util. See XTYPE-140
  enum Encoding {
    encoding_unaligned_cdr,
    encoding_xcdr1,
    encoding_xcdr2,
    encoding_count
  };

  string streamCommon(const std::string& indent, const string& name, AST_Type* type,
                      const string& prefix, bool wrap_nested_key_only, Intro& intro,
                      const string& stru = "", bool printing = false);

  const std::string construct_bound_fail =
    "strm.get_construction_status() == Serializer::BoundConstructionFailure";
  const std::string construct_elem_fail =
    "strm.get_construction_status() == Serializer::ElementConstructionFailure";
} /* namespace */

bool marshal_generator::gen_enum(AST_Enum*, UTL_ScopedName* name,
  const std::vector<AST_EnumVal*>& vals, const char*)
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
      "    if (CORBA::ULong(enumval) >= " << vals.size() << ") {\n"
      "      ACE_DEBUG((LM_DEBUG, ACE_TEXT(\"(%P|%t) Invalid enumerated value for " << cxx << " (%u)\\n\"), enumval));\n"
      "      return false;\n"
      "    }\n";
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
      "  if (strm >> temp) {\n";
    be_global->impl_ <<
      "    if (temp >= " << vals.size() << ") {\n"
      "      strm.set_construction_status(Serializer::ElementConstructionFailure);\n"
      "      return false;\n"
      "    }\n";
    be_global->impl_ <<
      "    enumval = static_cast<" << cxx << ">(temp);\n"
      "    return true;\n"
      "  }\n"
      "  return false;\n";
  }
  return true;
}

namespace {

  /**
   * Returns true for a type if nested key serialization is different from
   * normal serialization.
   */
  bool needs_nested_key_only(AST_Type* type)
  {
    static std::vector<AST_Type*> type_stack;
    type = resolveActualType(type);
    // Check if we have encountered the same type recursively
    for (size_t i = 0; i < type_stack.size(); ++i) {
      if (type == type_stack[i]) {
        return true;
      }
    }
    type_stack.push_back(type);
    bool result = false;
    if (get_special_struct(scoped(type->name()))) {
      result = false;
    } else {
      const Classification type_class = classify(type);
      if (type_class & CL_ARRAY) {
        result = needs_nested_key_only(dynamic_cast<AST_Array*>(type)->base_type());
      } else if (type_class & CL_SEQUENCE) {
        result = needs_nested_key_only(dynamic_cast<AST_Sequence*>(type)->base_type());
      } else if (type_class & CL_STRUCTURE) {
        AST_Structure* const struct_node = dynamic_cast<AST_Structure*>(type);
        // TODO(iguessthislldo): Possible optimization: If everything in a struct
        // was a key recursively, then we could return false.
        if (struct_has_explicit_keys(struct_node)) {
          result = true;
        } else {
          const Fields fields(struct_node);
          const Fields::Iterator fields_end = fields.end();
          for (Fields::Iterator i = fields.begin(); i != fields_end; ++i) {
            if (needs_nested_key_only((*i)->field_type())) {
              result = true;
              break;
            }
          }
        }
      } else if (type_class & CL_UNION) {
        result = be_global->union_discriminator_is_key(dynamic_cast<AST_Union*>(type));
      }
    }
    type_stack.pop_back();
    return result;
  }

  bool needs_forany(AST_Type* type)
  {
    const Classification type_class = classify(resolveActualType(type));
    return be_global->language_mapping() != BE_GlobalData::LANGMAP_CXX11 &&
      type_class & CL_ARRAY;
  }

  bool needs_distinct_type(AST_Type* type)
  {
    const Classification type_class = classify(resolveActualType(type));
    return be_global->language_mapping() == BE_GlobalData::LANGMAP_CXX11 &&
      type_class & (CL_SEQUENCE | CL_ARRAY);
  }

  const char* const shift_out = "<< ";
  const char* const shift_in = ">> ";

  std::string strip_shift_op(const std::string& s)
  {
    std::string rv = s;
    const size_t shift_len = 3;
    if (rv.size() > shift_len) {
      const std::string first3 = rv.substr(0, shift_len);
      if (first3 == shift_out || first3 == shift_in) {
        rv.erase(0, 3);
      }
    }
    return rv;
  }

  const char* get_shift_op(const std::string& s)
  {
    const size_t shift_len = 3;
    if (s.size() > shift_len) {
      const std::string first3 = s.substr(0, shift_len);
      if (first3 == shift_in) {
        return shift_in;
      }
      if (first3 == shift_out) {
        return shift_out;
      }
    }
    return "";
  }

  /// Handling wrapping and unwrapping references in the wrapper types:
  /// NestedKeyOnly, IDL::DistinctType, and *_forany.
  struct Wrapper {
    AST_Type* const type_;
    const std::string type_name_;
    const std::string to_wrap_;
    const char* const shift_op_;
    const std::string fieldref_;
    const std::string local_;
    bool is_const_;
    bool nested_key_only_;
    bool classic_array_copy_;
    std::string classic_array_copy_var_;

    Wrapper(AST_Type* type, const std::string& type_name,
      const std::string& to_wrap, bool is_const = true)
      : type_(type)
      , type_name_(type_name)
      , to_wrap_(strip_shift_op(to_wrap))
      , shift_op_(get_shift_op(to_wrap))
      , is_const_(is_const)
      , nested_key_only_(false)
      , classic_array_copy_(false)
      , done_(false)
    {
    }

    Wrapper(AST_Type* type, const std::string& type_name,
      const std::string& fieldref, const std::string& local, bool is_const = true)
      : type_(type)
      , type_name_(type_name)
      , shift_op_("")
      , fieldref_(strip_shift_op(fieldref))
      , local_(local)
      , is_const_(is_const)
      , nested_key_only_(false)
      , classic_array_copy_(false)
      , done_(false)
    {
    }

    std::string get_tag_name()
    {
      return dds_generator::get_tag_name(type_name_, nested_key_only_);
    }

    void generate_tag()
    {
      if (be_global->language_mapping() == BE_GlobalData::LANGMAP_CXX11) {
        be_global->header_ << "struct " << get_tag_name() << " {};\n\n";
      }
    }

    void done(Intro* intro = 0)
    {
      ACE_ASSERT(!done_);

      if (is_const_ && !std::strcmp(shift_op_, shift_in)) {
        is_const_ = false;
      }
      const std::string const_str = is_const_ ? "const " : "";
      const bool forany = classic_array_copy_ || needs_forany(type_);
      nested_key_only_ = nested_key_only_ && needs_nested_key_only(type_);
      wrapped_type_name_ = type_name_;
      bool by_ref = true;

      if (to_wrap_.size()) {
        ref_ = to_wrap_;
      } else {
        ref_ = fieldref_;
        if (local_.size()) {
          ref_ += '.' + local_;
        }
      }

      if (forany) {
        const std::string forany_type = type_name_ + "_forany";
        if (classic_array_copy_) {
          const std::string var_name = dds_generator::valid_var_name(ref_) + "_tmp_var";
          classic_array_copy_var_ = var_name;
          if (intro) {
            intro->insert(type_name_ + "_var " + var_name + "= " + type_name_ + "_alloc();");
          }
          ref_ = var_name;
        }
        const std::string var_name = dds_generator::valid_var_name(ref_) + "_forany";
        wrapped_type_name_ = forany_type;
        if (intro) {
          std::string line = forany_type + " " + var_name;
          if (classic_array_copy_) {
            line += " = " + ref_ + ".inout();";
          } else {
            line += "(const_cast<" + type_name_ + "_slice*>(" + ref_ + "));";
          }
          intro->insert(line);
        }
        ref_ = var_name;
      }

      if (nested_key_only_) {
        wrapped_type_name_ =
          std::string("NestedKeyOnly<") + const_str + wrapped_type_name_ + ">";
        value_access_post_ = ".value" + value_access_post_;
        const std::string nko_arg = "(" + ref_ + ")";
        if (is_const_) {
          ref_ = wrapped_type_name_ + nko_arg;
        } else {
          ref_ = dds_generator::valid_var_name(ref_) + "_nested_key_only";
          if (intro) {
            intro->insert(wrapped_type_name_ + " " + ref_ + nko_arg + ";");
          }
        }
      }

      if (needs_distinct_type(type_)) {
        wrapped_type_name_ =
          std::string("IDL::DistinctType<") + const_str + wrapped_type_name_ +
          ", " + get_tag_name() + ">";
        value_access_pre_ += "(*";
        value_access_post_ = ".val_)" + value_access_post_;
        const std::string idt_arg = "(" + ref_ + ")";
        if (is_const_) {
          ref_ = wrapped_type_name_ + idt_arg;
        } else {
          ref_ = dds_generator::valid_var_name(ref_) + "_distinct_type";
          if (intro) {
            intro->insert(wrapped_type_name_ + " " + ref_ + idt_arg + ";");
          }
        }
        by_ref = false;
      }

      wrapped_type_name_ = const_str + wrapped_type_name_ + (by_ref ? "&" : "");
      done_ = true;
    }

    std::string ref() const
    {
      ACE_ASSERT(done_);
      return ref_;
    }

    std::string wrapped_type_name() const
    {
      ACE_ASSERT(done_);
      return wrapped_type_name_;
    }

    std::string get_var_name(const std::string& var_name) const
    {
      return var_name.size() ? var_name : to_wrap_;
    }

    std::string value_access(const std::string& var_name = "") const
    {
      return value_access_pre_ + get_var_name(var_name) + value_access_post_;
    }

    std::string seq_check_empty() const
    {
      const bool use_cxx11 = be_global->language_mapping() == BE_GlobalData::LANGMAP_CXX11;
      return value_access() + (use_cxx11 ? ".empty()" : ".length() == 0");
    }

    std::string seq_get_length() const
    {
      const bool use_cxx11 = be_global->language_mapping() == BE_GlobalData::LANGMAP_CXX11;
      const std::string value = value_access();
      return use_cxx11 ? "static_cast<uint32_t>(" + value + ".size())" : value + ".length()";
    }

    std::string seq_get_buffer() const
    {
      const bool use_cxx11 = be_global->language_mapping() == BE_GlobalData::LANGMAP_CXX11;
      return value_access() + (use_cxx11 ? ".data()" : ".get_buffer()");
    }

    std::string stream() const
    {
      return shift_op_ + ref();
    }

    std::string classic_array_copy() const
    {
      return type_name_ + "_copy(" + to_wrap_ + ", " + classic_array_copy_var_ + ".in());";
    }

  private:
    bool done_;
    std::string wrapped_type_name_;
    std::string ref_;
    std::string value_access_pre_;
    std::string value_access_post_;
  };

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

  bool isRtpsSpecialSequence(const string& cxx)
  {
    return cxx == RtpsNamespace + "ParameterList";
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
        "    const CORBA::ULong len = seq.length();\n"
        "    // Improves growth behavior. See note in ParameterListConverter's add_param()\n"
        "    if (len && !(len & (len - 1))) {\n"
        "      seq.length(2 * len);\n"
        "    }\n"
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
    return cxx == DdsNamespace + "PropertySeq"
      || cxx == DdsNamespace + "BinaryPropertySeq";
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
    const std::string seq_resize_func = use_cxx11 ? "resize" : "length";
    std::string tempvar = "tempvar";
    be_global->impl_ <<
      indent << "if (encoding.xcdr_version() == Encoding::XCDR_VERSION_2) {\n" <<
      indent << "  strm.skip(end_of_seq - strm.rpos());\n" <<
      indent << "} else {\n" <<
      indent << "  " << seq_type_name << " " << tempvar << ";\n" <<
      indent << "  " << tempvar << "." << seq_resize_func << "(1);\n" <<
      indent << "  for (CORBA::ULong j = " << start << " + 1; j < " << end << "; ++j) {\n";

    std::string stream_to = tempvar + "[0]";
    if (cls & CL_STRING) {
      if (cls & CL_BOUNDED) {
        AST_Type* elem = resolveActualType(seq->base_type());
        const string args = stream_to + (use_cxx11 ? ", " : ".out(), ") + bounded_arg(elem);
        stream_to = getWrapper(args, elem, WD_INPUT);
      } else {
        const string getbuffer =
          (be_global->language_mapping() == BE_GlobalData::LANGMAP_NONE)
          ? ".get_buffer()" : "";
        stream_to = tempvar + getbuffer + "[0];\n";
      }
    } else {
      Intro intro;
      const bool classic_array_copy = !use_cxx11 && (cls & CL_ARRAY);
      Wrapper wrapper(seq->base_type(), scoped(seq->base_type()->name()),
        classic_array_copy ? tempvar : stream_to, false);
      wrapper.classic_array_copy_ = classic_array_copy;
      wrapper.done(&intro);
      stream_to = wrapper.ref();
      intro.join(be_global->impl_, indent + "    ");
    }

    be_global->impl_ <<
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
    UTL_ScopedName* tdname, AST_Sequence* seq, bool nested_key_only, const FieldInfo* anonymous = 0)
  {
    be_global->add_include("dds/DCPS/Serializer.h");
    if (anonymous) {
      seq = dynamic_cast<AST_Sequence*>(anonymous->type_);
    }
    const std::string named_as = anonymous ? anonymous->scoped_type_ : scoped(tdname);
    Wrapper base_wrapper(seq, named_as, "seq");
    base_wrapper.nested_key_only_ = nested_key_only;

    NamespaceGuard ng(!anonymous);

    if (!anonymous) {
      for (size_t i = 0; i < array_length(special_sequences); ++i) {
        if (special_sequences[i].check(base_wrapper.type_name_)) {
          special_sequences[i].gen(base_wrapper.type_name_);
          return;
        }
      }
    }

    AST_Type* elem = resolveActualType(seq->base_type());
    TryConstructFailAction try_construct = be_global->sequence_element_try_construct(seq);

    Classification elem_cls = classify(elem);
    const bool primitive = elem_cls & CL_PRIMITIVE;
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

    const std::string cxx_elem =
      anonymous ? anonymous->scoped_elem_ : scoped(seq->base_type()->name());
    const bool use_cxx11 = be_global->language_mapping() == BE_GlobalData::LANGMAP_CXX11;

    base_wrapper.generate_tag();

    {
      Intro intro;
      Wrapper wrapper(base_wrapper);
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
          Wrapper elem_wrapper(elem, cxx_elem, value_access + "[i]");
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
      Wrapper wrapper(base_wrapper);
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
          Wrapper elem_wrapper(elem, cxx_elem, value_access + "[i]");
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
      Wrapper wrapper(base_wrapper);
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
            "  }\n";
          be_global->impl_ <<
            (use_cxx11 ? "  " + value_access + ".resize(" : "  " + value_access +
            ".length(") << "new_length);\n";
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
            (use_cxx11 ? "  " + value_access + ".resize(new_length);\n" : "  " + value_access +
            ".length(new_length);\n");
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
          (use_cxx11 ? "  " + value_access + ".resize(new_length);\n" : "  " + value_access + ".length(new_length);\n");
        //read the entire length of the writer's sequence
        be_global->impl_ <<
          "  for (CORBA::ULong i = 0; i < new_length; ++i) {\n";

        Intro intro;
        std::string stream_to;
        std::string classic_array_copy;
        if (!use_cxx11 && (elem_cls & CL_ARRAY)) {
          Wrapper classic_array_wrapper(
            seq->base_type(), scoped(seq->base_type()->name()), elem_access);
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
          Wrapper elem_wrapper(elem, cxx_elem, value_access + "[i]", false);
          elem_wrapper.nested_key_only_ = nested_key_only;
          elem_wrapper.done(&intro);
          stream_to = elem_wrapper.ref();
        }
        const std::string indent = "    ";
        intro.join(be_global->impl_, indent);
        be_global->impl_ <<
          indent << " if (!(strm >> " << stream_to << ")) {\n";

        const std::string seq_resize_func = use_cxx11 ? "resize" : "length";

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

  void gen_sequence(UTL_ScopedName* name, AST_Sequence* seq)
  {
    gen_sequence_i(name, seq, false);
    if (needs_nested_key_only(seq)) {
      gen_sequence_i(name, seq, true);
    }
  }

  void gen_anonymous_sequence(const FieldInfo& sf)
  {
    gen_sequence_i(0, 0, false, &sf);
    if (needs_nested_key_only(sf.type_)) {
      gen_sequence_i(0, 0, true, &sf);
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
    Wrapper base_wrapper(arr, named_as, "arr");
    base_wrapper.nested_key_only_ = nested_key_only;
    NamespaceGuard ng(!anonymous);

    const bool use_cxx11 = be_global->language_mapping() == BE_GlobalData::LANGMAP_CXX11;

    AST_Type* elem = resolveActualType(arr->base_type());
    TryConstructFailAction try_construct = be_global->array_element_try_construct(arr);
    Classification elem_cls = classify(elem);
    const bool primitive = elem_cls & CL_PRIMITIVE;
    if (!elem->in_main_file()
        && elem->node_type() != AST_Decl::NT_pre_defined) {
      be_global->add_referenced(elem->file_name().c_str());
    }
    const std::string cxx_elem =
      anonymous ? anonymous->scoped_elem_ : scoped(arr->base_type()->name());
    const ACE_CDR::ULong n_elems = array_element_count(arr);

    base_wrapper.generate_tag();

    if (!nested_key_only) {
      Wrapper wrapper(base_wrapper);
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
      Wrapper wrapper(base_wrapper);
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
          Wrapper elem_wrapper(elem, cxx_elem, wrapper.value_access() + nfl.index_);
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
      Wrapper wrapper(base_wrapper);
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
          Wrapper elem_wrapper(elem, cxx_elem, wrapper.value_access() + nfl.index_);
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
      Wrapper wrapper(base_wrapper);
      wrapper.is_const_ = false;
      wrapper.done();
      Function extraction("operator>>", "bool");
      extraction.addArg("strm", "Serializer&");
      extraction.addArg("arr", wrapper.wrapped_type_name());
      extraction.endArgs();
      be_global->impl_ <<
        "  bool discard_flag = false;\n"
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
      if (elem_cls & CL_PRIMITIVE) {
        string suffix;
        for (unsigned int i = 1; i < arr->n_dims(); ++i)
          suffix += use_cxx11 ? "->data()" : "[0]";
        be_global->impl_ <<
          "  return strm.read_" << getSerializerName(elem)
          << "_array(" << accessor << suffix << ", " << n_elems << ");\n";
      } else { // Enum, String, Struct, Array, Sequence, Union
        string indent = "  ";
        NestedForLoops nfl("CORBA::ULong", "i", arr, indent);
        const std::string elem_access = wrapper.value_access() + nfl.index_;

        Intro intro;
        std::string stream;
        std::string classic_array_copy;
        if (!use_cxx11 && (elem_cls & CL_ARRAY)) {
          Wrapper classic_array_wrapper(
            arr->base_type(), scoped(arr->base_type()->name()), wrapper.value_access() + nfl.index_);
          classic_array_wrapper.classic_array_copy_ = true;
          classic_array_wrapper.done(&intro);
          classic_array_copy = classic_array_wrapper.classic_array_copy();
          stream = "(strm >> " + classic_array_wrapper.ref() + ")";
        } else {
          stream = streamCommon(
            indent, "", arr->base_type(), ">> " + elem_access, nested_key_only, intro);
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

  bool is_bounded_type(AST_Type* type, Encoding encoding)
  {
    bool bounded = true;
    static std::vector<AST_Type*> type_stack;
    type = resolveActualType(type);
    for (size_t i = 0; i < type_stack.size(); ++i) {
      // If we encounter the same type recursively, then we are unbounded
      if (type == type_stack[i]) return false;
    }
    type_stack.push_back(type);
    Classification fld_cls = classify(type);
    if ((fld_cls & CL_STRING) && !(fld_cls & CL_BOUNDED)) {
      bounded = false;
    } else if (fld_cls & CL_STRUCTURE) {
      const ExtensibilityKind exten = be_global->extensibility(type);
      if (exten != extensibilitykind_final && encoding != encoding_unaligned_cdr) {
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
      if (exten != extensibilitykind_final && encoding != encoding_unaligned_cdr) {
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
  std::string encoding_to_xcdr_version(Encoding encoding)
  {
    switch (encoding) {
    case encoding_xcdr1:
      return "Encoding::XCDR_VERSION_1";
    case encoding_xcdr2:
      return "Encoding::XCDR_VERSION_2";
    default:
      return "Encoding::XCDR_VERSION_NONE";
    }
  }

  /**
   * Convert a compiler Encoding value to the string name of the corresponding
   * OpenDDS::DCPS::Encoding::Kind.
   */
  std::string encoding_to_encoding_kind(Encoding encoding)
  {
    switch (encoding) {
    case encoding_xcdr1:
      return "Encoding::KIND_XCDR1";
    case encoding_xcdr2:
      return "Encoding::KIND_XCDR2";
    default:
      return "Encoding::KIND_UNALIGNED_CDR";
    }
  }

  size_t max_alignment(Encoding encoding)
  {
    switch (encoding) {
    case encoding_xcdr1:
      return 8;
    case encoding_xcdr2:
      return 4;
    default:
      return 0;
    }
  }

  // TODO(iguessthislldo): Replace with align from libOpenDDS_Util. See XTYPE-140
  void align(Encoding encoding, size_t& value, size_t by)
  {
    const size_t align_by = std::min(max_alignment(encoding), by);
    if (align_by) {
      const size_t offset_by = value % align_by;
      if (offset_by) {
        value += align_by - offset_by;
      }
    }
  }

  void idl_max_serialized_size_dheader(
    Encoding encoding, ExtensibilityKind exten, size_t& size)
  {
    if (exten != extensibilitykind_final && encoding == encoding_xcdr2) {
      align(encoding, size, 4);
      size += 4;
    }
  }

  void idl_max_serialized_size(Encoding encoding, size_t& size, AST_Type* type);

  // Max marshaled size of repeating 'type' 'n' times in the stream
  // (for an array or sequence)
  void idl_max_serialized_size_repeating(
    Encoding encoding, size_t& size, AST_Type* type, size_t n)
  {
    if (n > 0) {
      // 1st element may need padding relative to whatever came before
      idl_max_serialized_size(encoding, size, type);
    }
    if (n > 1) {
      // subsequent elements may need padding relative to prior element
      // TODO(iguessthislldo): https://github.com/objectcomputing/OpenDDS/pull/1668#discussion_r432521888
      const size_t prev_size = size;
      idl_max_serialized_size(encoding, size, type);
      size += (n - 2) * (size - prev_size);
    }
  }

  /// Should only be called on bounded types
  void idl_max_serialized_size(Encoding encoding, size_t& size, AST_Type* type)
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

bool marshal_generator::gen_typedef(AST_Typedef*, UTL_ScopedName* name, AST_Type* base, const char*)
{
  switch (base->node_type()) {
  case AST_Decl::NT_sequence:
    gen_sequence(name, dynamic_cast<AST_Sequence*>(base));
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
  string findSizeCommon(const std::string& indent, const string& name, AST_Type* type,
                        const string& prefix, bool wrap_nested_key_only, Intro& intro,
                        const string& = "", bool = false) // same sig as streamCommon
  {
    const bool use_cxx11 = be_global->language_mapping() == BE_GlobalData::LANGMAP_CXX11;
    const bool is_union_member = prefix == "uni";

    AST_Type* const actual_type = resolveActualType(type);
    const Classification fld_cls = classify(actual_type);

    const string qual = prefix + '.' + insert_cxx11_accessor_parens(name, is_union_member);

    if (fld_cls & CL_ENUM) {
      return indent + "primitive_serialized_size_ulong(encoding, size);\n";
    } else if (fld_cls & CL_STRING) {
      const string suffix = is_union_member ? "" : ".in()";
      const string get_size = use_cxx11 ? (qual + ".size()")
        : ("ACE_OS::strlen(" + qual + suffix + ")");
      return indent + "primitive_serialized_size_ulong(encoding, size);\n" +
        indent + "size += " + get_size
        + ((fld_cls & CL_WIDE) ? " * char16_cdr_size;\n"
                               : " + 1;\n");
    } else if (fld_cls & CL_PRIMITIVE) {
      AST_PredefinedType* const p = dynamic_cast<AST_PredefinedType*>(actual_type);
      if (p->pt() == AST_PredefinedType::PT_longdouble) {
        // special case use to ACE's NONNATIVE_LONGDOUBLE in CDR_Base.h
        return indent +
          "primitive_serialized_size(encoding, size, ACE_CDR::LongDouble());\n";
      }
      return indent + "primitive_serialized_size(encoding, size, " +
        getWrapper(qual, actual_type, WD_OUTPUT) + ");\n";
    } else if (fld_cls == CL_UNKNOWN) {
      return ""; // warning will be issued for the serialize functions
    } else { // sequence, struct, union, array
      Wrapper wrapper(type, scoped(type->name()),
        prefix + "." + insert_cxx11_accessor_parens(name, is_union_member));
      wrapper.nested_key_only_ = wrap_nested_key_only;
      wrapper.done(&intro);
      return indent + "serialized_size(encoding, size, " + wrapper.ref() + ");\n";
    }
  }

  string findSizeMutableUnion(const string& indent, const string& name, AST_Type* type,
                              const string& prefix, bool wrap_nested_key_only, Intro& intro,
                              const string & = "", bool = false) // same sig as streamCommon
  {
    return indent + "serialized_size_parameter_id(encoding, size, mutable_running_total);\n"
      + findSizeCommon(indent, name, type, prefix, wrap_nested_key_only, intro);
  }

  std::string generate_field_serialized_size(
    const std::string& indent, AST_Field* field, const std::string& prefix,
    bool wrap_nested_key_only, Intro& intro)
  {
    FieldInfo af(*field);
    if (af.anonymous()) {
      Wrapper wrapper(af.type_, af.scoped_type_,
        prefix + "." + insert_cxx11_accessor_parens(af.name_));
      wrapper.nested_key_only_ = wrap_nested_key_only;
      wrapper.done(&intro);
      return indent + "serialized_size(encoding, size, " + wrapper.ref() + ");\n";
    }
    return findSizeCommon(
      indent, field->local_name()->get_string(), field->field_type(), prefix,
      wrap_nested_key_only, intro);
  }

  // common to both fields (in structs) and branches (in unions)
  string streamCommon(const std::string& /*indent*/, const string& name, AST_Type* type,
                      const string& prefix, bool wrap_nested_key_only, Intro& intro,
                      const string& stru, bool printing)
  {
    const bool use_cxx11 = be_global->language_mapping() == BE_GlobalData::LANGMAP_CXX11;
    const bool is_union_member = prefix.substr(3) == "uni";

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
      if ((fld_cls & CL_BOUNDED) && !printing) {
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
      const std::string type_name = scoped(type->name());
      string fieldref = prefix, local = insert_cxx11_accessor_parens(name, is_union_member);
      const bool accessor = local.size() > 2 && local.substr(local.size() - 2) == "()";
      if (fld_cls & CL_STRING) {
        if (!accessor && !use_cxx11) {
          local += ".in()";
        }
        if ((fld_cls & CL_BOUNDED) && !printing) {
          const string args = (fieldref + '.' + local).substr(3) + ", " + bounded_arg(actual_type);
          return "(strm " + shift + ' ' + getWrapper(args, actual_type, WD_OUTPUT) + ')';
        }
      }
      Wrapper wrapper(type, scoped(type->name()), fieldref, local, dir == WD_OUTPUT);
      wrapper.nested_key_only_ = wrap_nested_key_only;
      wrapper.done(&intro);
      return "(strm " + shift + " " + wrapper.ref() + ")";
    }
  }

  std::string generate_field_stream(
    const std::string& indent, AST_Field* field, const std::string& prefix,
    bool wrap_nested_key_only, Intro& intro)
  {
    FieldInfo af(*field);
    if (af.anonymous()) {
      Wrapper wrapper(af.type_, af.scoped_type_,
        prefix + "." + insert_cxx11_accessor_parens(af.name_));
      wrapper.nested_key_only_ = wrap_nested_key_only;
      wrapper.done(&intro);
      return "(strm " + wrapper.stream() + ")";
    }
    return streamCommon(
      indent, field->local_name()->get_string(), field->field_type(), prefix,
      wrap_nested_key_only, intro);
  }

  bool isBinaryProperty_t(const string& cxx)
  {
    return cxx == DdsNamespace + "BinaryProperty_t";
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

  bool isProperty_t(const string& cxx)
  {
    return cxx == DdsNamespace + "Property_t";
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

  bool isPropertyQosPolicy(const string& cxx)
  {
    return cxx == DdsNamespace + "PropertyQosPolicy";
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

  bool isSecuritySubmessage(const string& cxx)
  {
    return cxx == RtpsNamespace + "SecuritySubmessage";
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

  bool isRtpsSpecialStruct(const string& cxx)
  {
    return cxx == RtpsNamespace + "SequenceNumberSet"
      || cxx == RtpsNamespace + "FragmentNumberSet";
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
    Encoding encoding,
    const string& key_name, AST_Type* ast_type,
    size_t* size,
    string* expr, Intro* intro);

  bool
  iterate_over_keys(
    const std::string& indent,
    Encoding encoding,
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
    const std::string&, Encoding encoding, const string&, AST_Type* ast_type,
    size_t* size, string*, Intro*)
  {
    idl_max_serialized_size(encoding, *size, ast_type);
  }

  void serialized_size_iteration(
    const std::string& indent, Encoding, const string& key_name, AST_Type* ast_type,
    size_t*, string* expr, Intro* intro)
  {
    *expr += findSizeCommon(indent, key_name, ast_type, "stru.value", false, *intro);
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

    std::ostringstream ss;
    ss << indent << name << ".length(" << values.size() << ");\n";
    for (size_t i = 0; i < values.size(); ++i) {
      ss << indent << name << "[" << i << "] = " << values[i] << ";\n";
    }
    return ss.str();
  }

  bool is_bounded_topic_struct(AST_Type* type, Encoding encoding, bool key_only,
    TopicKeys& keys, IDL_GlobalData::DCPS_Data_Type_Info* info = 0)
  {
    /*
     * TODO(iguessthislldo): This is a workaround for not properly implementing
     * serialized_size_bound for XCDR. See XTYPE-83.
     */
    const ExtensibilityKind exten = be_global->extensibility(type);
    if (exten != extensibilitykind_final && encoding != encoding_unaligned_cdr) {
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
    for (unsigned e = 0; e < encoding_count; ++e) {
      const Encoding encoding = static_cast<Encoding>(e);
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
    for (unsigned e = 0; e < encoding_count; ++e) {
      const Encoding encoding = static_cast<Encoding>(e);
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
    for (unsigned e = 0; e < encoding_count; ++e) {
      const Encoding encoding = static_cast<Encoding>(e);
      be_global->header_ <<
        "    case " << encoding_to_encoding_kind(encoding) << ":\n"
        "      return SerializedSizeBound(";
      /*
       * TODO(iguessthislldo): This is a workaround for not properly implementing
       * serialized_size_bound for Mutable. See XTYPE-83.
       */
      if (exten == extensibilitykind_final || encoding == encoding_unaligned_cdr) {
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
        AST_Type* const base = seq->base_type();
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
      const_cpp_name = "const " + actual_cpp_name + "&";
      break;
    case FieldFilter_NestedKeyOnly:
      cpp_name = "const NestedKeyOnly<" + actual_cpp_name + ">";
      const_cpp_name = "const NestedKeyOnly<const " + actual_cpp_name + ">&";
      break;
    case FieldFilter_KeyOnly:
      cpp_name = "const KeyOnly<" + actual_cpp_name + ">";
      const_cpp_name = "const KeyOnly<const " + actual_cpp_name + ">&";
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
          "      if (encoding.xcdr_version() == Encoding::XCDR_VERSION_2 &&\n"
          "            strm.rpos() >= end_of_struct) {\n"
          "        return true;\n"
          "      }\n"
          "      bool must_understand = false;\n"
          "      if (!strm.read_parameter_id(member_id, field_size, must_understand)) {\n"
          "        return false;\n"
          "      }\n"
          "      if (encoding.xcdr_version() == Encoding::XCDR_VERSION_1 &&\n"
          "            member_id == Serializer::pid_list_end) {\n"
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
            indent, field, ">> stru" + value_access, wrap_nested_key_only, intro) << ") {\n";
          AST_Type* const field_type = resolveActualType(field->field_type());
          Classification fld_cls = classify(field_type);

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
        if (expr.size() && exten != extensibilitykind_appendable) {
          expr += "\n    && ";
        }
        // TODO (sonndinh): Integrate with try-construct for when the stream
        // ends before some fields on the reader side get their values.
        if (is_appendable) {
          expr +=
            "  if (encoding.xcdr_version() == Encoding::XCDR_VERSION_2 &&\n"
            "      strm.rpos() >= end_of_struct) {\n"
            "    return true;\n"
            "  }\n";
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
          expr += "  if (!";
        }
        expr += generate_field_stream(
          indent, field, ">> stru" + value_access, wrap_nested_key_only, intro);
        if (is_appendable) {
          expr += ") {\n"
            "    return false;\n"
            "  }\n";
        } else if (!cond.empty()) {
          expr += ")";
        }
      }
      intro.join(be_global->impl_, indent);
      if (is_appendable) {
        expr +=
          "  if (encoding.xcdr_version() == Encoding::XCDR_VERSION_2 &&\n"
          "      strm.rpos() < end_of_struct) {\n"
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
      const_cpp_name = "const " + actual_cpp_name + "&";
      break;
    case FieldFilter_NestedKeyOnly:
      cpp_name = "const NestedKeyOnly<" + actual_cpp_name + ">";
      const_cpp_name = "const NestedKeyOnly<const " + actual_cpp_name + ">&";
      break;
    case FieldFilter_KeyOnly:
      cpp_name = "const KeyOnly<" + actual_cpp_name + ">";
      const_cpp_name = "const KeyOnly<const " + actual_cpp_name + ">&";
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
          const string field_name = field->local_name()->get_string();
          const bool is_key = be_global->is_key(field);

          mutable_fields
            << generate_field_serialized_size(
              mutable_indent, field, "stru" + value_access, wrap_nested_key_only, intro)
            << "\n"
            "    if (!strm.write_parameter_id("
              << id << ", size" << (is_key ? ", true" : "") << ")) {\n"
            "      return false;\n"
            "    }\n"
            "    size = 0;\n"
            "    if (!" << generate_field_stream(
              mutable_indent, field, "<< stru" + value_access, wrap_nested_key_only, intro)
            << ") {\n"
            "    return false;\n"
            "  }\n";
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
        if (expr.size()) expr += "\n    && ";
        const string field_name = field->local_name()->get_string(),
          cond = rtpsCustom.getConditional(field_name);
        if (!cond.empty()) {
          expr += "(!(" + cond + ") || ";
        }
        expr += generate_field_stream(
          indent, field, "<< stru" + value_access, wrap_nested_key_only, intro);
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


void marshal_generator::generate_dheader_code(const std::string& code, bool dheader_required, bool is_ser_func)
{
  //DHeader appears on aggregated types that are mutable or appendable in XCDR2
  //DHeader also appears on ALL sequences and arrays of non-primitives
  if (dheader_required) {
    if (is_ser_func) {
      be_global->impl_ << "  size_t total_size = 0;\n";
    }
    be_global->impl_ << "  if (encoding.xcdr_version() == Encoding::XCDR_VERSION_2) {\n"
      << code <<
      "  }\n";
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
      contents << type_to_default("  ", type, field_name, type->anonymous());
    }
    intro.join(be_global->impl_, "  ");
    be_global->impl_ << contents.str();
  }

  const special_struct* special_struct_ptr = get_special_struct(cxx);
  if (special_struct_ptr) {
    return special_struct_ptr->gen(cxx);
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

  if (be_global->printer()) {
    be_global->add_include("dds/DCPS/Printer.h");
    PreprocessorIfGuard g("ndef OPENDDS_SAFETY_PROFILE");
    g.extra_newline(true);
    Function shift("operator<<", "std::ostream&");
    shift.addArg("strm", "Printable");
    shift.addArg("stru", "const " + cxx + "&");
    shift.endArgs();
    shift.extra_newline_ = false;
    Intro intro;
    const std::string indent = "  ";
    std::string expr = indent + "strm.push_indent();\n";
    for (size_t i = 0; i < fields.size(); ++i) {
      const string field_name = fields[i]->local_name()->get_string();
      AST_Type* const field_type = resolveActualType(fields[i]->field_type());
      const AST_Decl::NodeType node_type = field_type->node_type();
      const bool is_composite_type = node_type == AST_Decl::NT_struct;
      const bool is_string_type = node_type == AST_Decl::NT_string ||
        node_type == AST_Decl::NT_wstring;
      expr +=
        "\n"
        "  // Print " + field_name  + "\n"
        "  strm.print_indent();\n"
        "  if (strm.printer().print_field_names()) {\n"
        "    strm.os() << \"" + field_name + ":";
      if (is_composite_type) {
        expr += "\" << std::endl";
      } else {
        expr += " \"";
      }
      expr += ";\n"
        "  }\n";
      if (is_string_type) {
        expr +=
          "  strm.os() << '\"';\n";
      }
      expr += indent + streamCommon(
        indent, field_name, fields[i]->field_type(), "<< stru", false, intro, cxx, true);
      if (is_string_type) {
        expr += " << '\"'";
      }
      if (!is_composite_type) {
        expr += " << std::endl";
      }
      expr += ";\n";
    }
    intro.join(be_global->impl_, indent);
    be_global->impl_ << expr << "\n"
      "  return strm.os();\n";
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
      for (unsigned e = 0; e < encoding_count; ++e) {
        string expr;
        Intro intro;
        const Encoding encoding = static_cast<Encoding>(e);
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
        expr += streamCommon(indent, key_name, field_type, "<< stru.value", false, intro);
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
        expr += streamCommon(indent, key_name, field_type, ">> stru.value", false, intro);
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
    "  Value getValue(Serializer& strm, const char* field) const\n"
    "  {\n"
    "    const Encoding& encoding = strm.encoding();\n"
    "    ACE_UNUSED_ARG(encoding);\n";
  marshal_generator::generate_dheader_code(
    "    if (!strm.read_delimiter(total_size)) {\n"
    "      throw std::runtime_error(\"Unable to reader delimiter in getValue\");\n"
    "    }\n", not_final);
  be_global->impl_ <<
    "    std::string base_field = field;\n"
    "    size_t index = base_field.find('.');\n"
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
      Classification fld_cls = classify(field_type);

      cases << "        case " << id << ": {\n";
      if (fld_cls & CL_SCALAR) {
        const std::string cxx_type = to_cxx_type(field_type, size);
        const std::string val = (fld_cls & CL_STRING) ? (use_cxx11 ? "val" : "val.out()")
          : getWrapper("val", field_type, WD_INPUT);
        cases <<
          "          if (field_id == member_id) {\n"
          "            " << cxx_type << " val;\n" <<
          "            if (!(strm >> " << val << ")) {\n"
          "              throw std::runtime_error(\"Field '" << field_name << "' could not be deserialized\");\n" <<
          "            }\n"
          "            return val;\n"
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
    std::string field_name = field->local_name()->get_string();
    AST_Type* const field_type = resolveActualType(field->field_type());
    Classification fld_cls = classify(field_type);
    if (fld_cls & CL_SCALAR) {
      const std::string cxx_type = to_cxx_type(field_type, size);
      const std::string val = (fld_cls & CL_STRING) ? (use_cxx11 ? "val" : "val.out()")
          : getWrapper("val", field_type, WD_INPUT);
      expr +=
        "    if (base_field == \"" + field_name + "\") {\n"
        "      " + cxx_type + " val;\n"
        "      if (!(strm >> " + val + ")) {\n"
        "        throw std::runtime_error(\"Field '" + field_name + "' could "
        "not be deserialized\");\n"
        "      }\n"
        "      return val;\n"
        "    } else {\n";
      if (fld_cls & CL_STRING) {
        expr +=
          "      ACE_CDR::ULong len;\n"
          "      if (!(strm >> len)) {\n"
          "        throw std::runtime_error(\"String '" + field_name +
          "' length could not be deserialized\");\n"
          "      }\n"
          "      if (!strm.skip(len)) {\n"
          "        throw std::runtime_error(\"String '" + field_name +
          "' contents could not be skipped\");\n"
          "      }\n"
          "    }\n";
      } else {
        expr +=
          "      if (!strm.skip(1,  " + OpenDDS::DCPS::to_dds_string(size) + " )) {\n"
          "        throw std::runtime_error(\"Field '" + field_name +
          "' could not be skipped\");\n"
          "      }\n"
          "    }\n";
      }
    } else if (fld_cls & CL_STRUCTURE) {
        expr +=
          "    if (base_field == \"" + field_name + "\") {\n"
          "      return getMetaStruct<" + scoped(field_type->name()) + ">().getValue(strm, subfield.c_str());\n"
          "    } else {\n"
          "      if (!gen_skip_over(strm, static_cast<" + scoped(field_type->name()) + "*>(0))) {\n"
          "        throw std::runtime_error(\"Field '" + field_name + "' could not be skipped\");\n"
          "      }\n"
          "    }\n";
    } else { // array, sequence, union:
      std::string pre, post;
      if (!use_cxx11 && (fld_cls & CL_ARRAY)) {
        post = "_forany";
      } else if (use_cxx11 && (fld_cls & (CL_ARRAY | CL_SEQUENCE))) {
        pre = "IDL::DistinctType<";
        post = ", " + dds_generator::get_tag_name(scoped(field->field_type()->name())) + ">";
      }
      const std::string ptr = field->field_type()->anonymous() ?
        FieldInfo(*field).ptr_ : (pre + scoped(field->field_type()->name()) + post + '*');
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

  bool isRtpsSpecialUnion(const string& cxx)
  {
    return cxx == RtpsNamespace + "Parameter"
      || cxx == RtpsNamespace + "Submessage";
  }

  bool genRtpsParameter(AST_Union* u, AST_Type* discriminator,
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
        "    uni._d(OpenDDS::RTPS::PID_SENTINEL);\n"
        "    return true;\n"
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

  bool genRtpsSubmessage(AST_Union* u, AST_Type* discriminator,
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

  bool genRtpsSpecialUnion(const string& cxx, AST_Union* u, AST_Type* discriminator,
                           const std::vector<AST_UnionBranch*>& branches)
  {
    if (cxx == RtpsNamespace + "Parameter") {
      return genRtpsParameter(u, discriminator, branches);
    } else if (cxx == RtpsNamespace + "Submessage") {
      return genRtpsSubmessage(u, discriminator, branches);
    } else {
      return false;
    }
  }

  void gen_union_key_serializers(AST_Union* node, FieldFilter kind)
  {
    const string cxx = scoped(node->name()); // name as a C++ class
    AST_Type* const discriminator = node->disc_type();
    const Classification disc_cls = classify(discriminator);
    const bool has_key = be_global->union_discriminator_is_key(node);
    const string key_only_wrap_out = getWrapper("uni.value._d()", discriminator, WD_OUTPUT);
    const ExtensibilityKind exten = be_global->extensibility(node);
    const bool not_final = exten != extensibilitykind_final;
    const string wrapper = kind == FieldFilter_KeyOnly ? "KeyOnly"
      : kind == FieldFilter_NestedKeyOnly ? "NestedKeyOnly"
      : "<<error from " __FILE__ ":" OPENDDS_IDL_STR(__LINE__) ">>";

    {
      Function serialized_size("serialized_size", "void");
      serialized_size.addArg("encoding", "const Encoding&");
      serialized_size.addArg("size", "size_t&");
      serialized_size.addArg("uni", "const " + wrapper + "<const " + cxx + ">");
      serialized_size.endArgs();

      if (has_key) {
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

      if (has_key) {
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
              "  primitive_serialized_size(encoding, size, " << key_only_wrap_out << ");\n";
          }

          be_global->impl_ <<
            "  if (!strm.write_parameter_id(0, size)) {\n"
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

      if (has_key) {
        // DHEADER
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
  Classification disc_cls = classify(discriminator);

  const ExtensibilityKind exten = be_global->extensibility(node);
  const bool not_final = exten != extensibilitykind_final;

  {
    Function set_default("set_default", "void", "");
    set_default.addArg("uni", cxx + "&");
    set_default.endArgs();
    be_global->impl_ << "  " << scoped(discriminator->name()) << " temp;\n";
    be_global->impl_ << type_to_default("  ", discriminator, "  temp");
    be_global->impl_ << "  uni._d(temp);\n";
    AST_Type* disc_type = resolveActualType(discriminator);
    Classification disc_cls = classify(disc_type);
    if (!disc_type->in_main_file() && disc_type->node_type() != AST_Decl::NT_pre_defined) {
      be_global->add_referenced(disc_type->file_name().c_str());
    }
    ACE_CDR::ULong default_enum_val = 0;
    if (disc_cls & CL_ENUM) {
      AST_Enum* enu = dynamic_cast<AST_Enum*>(disc_type);
      UTL_ScopeActiveIterator i(enu, UTL_Scope::IK_decls);
      AST_EnumVal *item = dynamic_cast<AST_EnumVal*>(i.item());
      default_enum_val = item->constant_value()->ev()->u.eval;
    }
    bool found = false;
    for (std::vector<AST_UnionBranch*>::const_iterator itr = branches.begin(); itr < branches.end() && !found; ++itr) {
      AST_UnionBranch* branch = *itr;
      for (unsigned i = 0; i < branch->label_list_length(); ++i) {
        AST_UnionLabel* ul = branch->label(i);
        if (ul->label_kind() != AST_UnionLabel::UL_default) {
          AST_Expression::AST_ExprValue* ev = branch->label(i)->label_val()->ev();
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
              (ev->et == AST_Expression::EV_bool && ev->u.bval == 0))
          {
            AST_Type* br = resolveActualType(branch->field_type());
            Classification br_cls = classify(br);
            if (br_cls & (CL_SEQUENCE | CL_ARRAY)) {
              be_global->impl_ << scoped(branch->field_type()->name()) << " " << getWrapper("tmp", branch->field_type(), WD_INPUT) << ";\n";
            }
            be_global->impl_ << type_to_default("  ", branch->field_type(), string("uni.")
              + branch->local_name()->get_string(), false, true);
            found = true;
            break;
          }
        }
      }
    }
  }

  for (size_t i = 0; i < array_length(special_unions); ++i) {
    if (special_unions[i].check(cxx)) {
      return special_unions[i].gen(cxx, node, discriminator, branches);
    }
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
        "  if (!strm.write_parameter_id(0, size)) {\n"
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
  gen_union_key_serializers(node, FieldFilter_KeyOnly);

  TopicKeys keys(node);
  return generate_marshal_traits(node, cxx, exten, keys);
}
