/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "langmap_generator.h"
#include "be_extern.h"

#include "utl_identifier.h"

#include "ace/Version.h"
#include "ace/CDR_Base.h"
#ifdef ACE_HAS_CDR_FIXED
#include "ast_fixed.h"
#endif

#include <map>
#include <iostream>

using namespace AstTypeClassification;

namespace {
  std::map<AST_PredefinedType::PredefinedType, std::string> primtype_;

  enum Helper {
    HLP_STR_VAR, HLP_STR_OUT, HLP_WSTR_VAR, HLP_WSTR_OUT,
    HLP_STR_MGR, HLP_WSTR_MGR,
    HLP_FIX_VAR, HLP_VAR_VAR, HLP_OUT,
    HLP_SEQ, HLP_SEQ_NS, HLP_SEQ_VAR_VAR, HLP_SEQ_FIX_VAR, HLP_SEQ_OUT,
    HLP_ARR_VAR_VAR, HLP_ARR_FIX_VAR, HLP_ARR_OUT, HLP_ARR_FORANY,
    HLP_FIXED, HLP_FIXED_CONSTANT
  };
  std::map<Helper, std::string> helpers_;

  std::string map_type(AST_Type* type)
  {
    if (AST_Typedef::narrow_from_decl(type)) {
      return scoped(type->name());
    }
    const Classification cls = classify(type);
    if (cls & CL_PRIMITIVE) {
      AST_Type* actual = resolveActualType(type);
      return primtype_[AST_PredefinedType::narrow_from_decl(actual)->pt()];
    }
    if (cls & CL_STRING) {
      const AST_PredefinedType::PredefinedType chartype = (cls & CL_WIDE)
        ? AST_PredefinedType::PT_wchar : AST_PredefinedType::PT_char;
      return primtype_[chartype] + '*';
    }
    if (cls & (CL_STRUCTURE | CL_UNION | CL_SEQUENCE | CL_ARRAY | CL_ENUM | CL_FIXED)) {
      return scoped(type->name());
    }
    if (cls & CL_INTERFACE) {
      return scoped(type->name()) + "_var";
    }
    return "<<unknown>>";
  }

  std::string map_type(AST_Expression::ExprType type)
  {
    AST_PredefinedType::PredefinedType pt = AST_PredefinedType::PT_void;
    switch (type)
    {
    case AST_Expression::EV_short: pt = AST_PredefinedType::PT_short; break;
    case AST_Expression::EV_ushort: pt = AST_PredefinedType::PT_ushort; break;
    case AST_Expression::EV_long: pt = AST_PredefinedType::PT_long; break;
    case AST_Expression::EV_ulong: pt = AST_PredefinedType::PT_ulong; break;
    case AST_Expression::EV_longlong: pt = AST_PredefinedType::PT_longlong; break;
    case AST_Expression::EV_ulonglong: pt = AST_PredefinedType::PT_ulonglong; break;
    case AST_Expression::EV_float: pt = AST_PredefinedType::PT_float; break;
    case AST_Expression::EV_double: pt = AST_PredefinedType::PT_double; break;
    case AST_Expression::EV_longdouble: pt = AST_PredefinedType::PT_longdouble; break;
    case AST_Expression::EV_char: pt = AST_PredefinedType::PT_char; break;
    case AST_Expression::EV_wchar: pt = AST_PredefinedType::PT_wchar; break;
    case AST_Expression::EV_octet: pt = AST_PredefinedType::PT_octet; break;
    case AST_Expression::EV_bool: pt = AST_PredefinedType::PT_boolean; break;
    case AST_Expression::EV_string: pt = AST_PredefinedType::PT_char; break;
    case AST_Expression::EV_wstring: pt = AST_PredefinedType::PT_wchar; break;
#ifdef ACE_HAS_CDR_FIXED
    case AST_Expression::EV_fixed:
      be_global->add_include("FACE/Fixed.h", BE_GlobalData::STREAM_LANG_H);
      return helpers_[HLP_FIXED_CONSTANT];
#endif
    default: break;
    }
    if (type == AST_Expression::EV_string || type == AST_Expression::EV_wstring)
      return primtype_[pt] + "* const";
    return primtype_[pt];
  }

  std::string exporter() {
    return be_global->export_macro().empty() ? ""
      : be_global->export_macro().c_str() + std::string(" ");
  }

  void struct_decls(UTL_ScopedName* name, AST_Type::SIZE_TYPE size, const char* struct_or_class = "struct")
  {
    be_global->add_include("<tao/VarOut_T.h>", BE_GlobalData::STREAM_LANG_H);
    const char* const nm = name->last_component()->get_string();
    be_global->lang_header_ <<
      struct_or_class << ' ' << nm << ";\n";
    switch (size) {
    case AST_Type::SIZE_UNKNOWN:
      be_global->lang_header_ << "/* Unknown size */\n";
    case AST_Type::FIXED:
      be_global->lang_header_ <<
        "typedef " << helpers_[HLP_FIX_VAR] << '<' << nm << "> " << nm << "_var;\n" <<
        "typedef " << nm << "& " << nm << "_out;\n";
      break;
    case AST_Type::VARIABLE:
      be_global->lang_header_ <<
        "typedef " << helpers_[HLP_VAR_VAR] << '<' << nm << "> " << nm << "_var;\n" <<
        "typedef " << helpers_[HLP_OUT] << '<' << nm << "> " << nm << "_out;\n";
      break;
    }
  }

  std::string array_dims(AST_Type* type, ACE_CDR::ULong& elems) {
    AST_Array* const arr = AST_Array::narrow_from_decl(type);
    std::string ret;
    for (ACE_CDR::ULong dim = 0; dim < arr->n_dims(); ++dim) {
      elems *= arr->dims()[dim]->ev()->u.ulval;
      if (dim) ret += "[0]";
    }
    AST_Type* base = resolveActualType(arr->base_type());
    if (AST_Array::narrow_from_decl(base)) {
      ret += "[0]" + array_dims(base, elems);
    }
    return ret;
  }

  void gen_typecode(UTL_ScopedName* name)
  {
    if (be_global->suppress_typecode()) {
      return;
    }
    const char* const nm = name->last_component()->get_string();
    be_global->lang_header_ <<
      "extern " << exporter() << "const ::CORBA::TypeCode_ptr _tc_" << nm
      << ";\n";
    const ScopedNamespaceGuard cppNs(name, be_global->impl_);
    be_global->impl_ <<
      "const ::CORBA::TypeCode_ptr _tc_" << nm << " = 0;\n";
  }

}

class GeneratorBase
{
public:
  virtual ~GeneratorBase() {}
  virtual void init() = 0;
  virtual void gen_sequence(UTL_ScopedName* tdname, AST_Sequence* seq) = 0;
  virtual bool gen_struct(AST_Structure* s, UTL_ScopedName* name, const std::vector<AST_Field*>& fields, AST_Type::SIZE_TYPE size, const char* x) = 0;

  static std::string generateDefaultValue(AST_Union* the_union, AST_Type* discriminator)
  {
    std::stringstream first_label;
    AST_Union::DefaultValue dv;
    the_union->default_value(dv);

    switch (the_union->udisc_type ())
      {
      case AST_Expression::EV_short:
        first_label << dv.u.short_val;
        break;
      case AST_Expression::EV_ushort:
        first_label << dv.u.ushort_val;
        break;
      case AST_Expression::EV_long:
        first_label << dv.u.long_val;
        break;
      case AST_Expression::EV_ulong:
        first_label << dv.u.ulong_val;
        break;
      case AST_Expression::EV_char:
        first_label << (int)dv.u.char_val;
        break;
      case AST_Expression::EV_bool:
        first_label << (dv.u.bool_val == 0 ? "false" : "true");
        break;
      case AST_Expression::EV_enum:
        {
          AST_Enum* e = AST_Enum::narrow_from_decl(discriminator);
          first_label << scoped(e->value_to_name(dv.u.enum_val));
          break;
        }
      case AST_Expression::EV_longlong:
        first_label << dv.u.longlong_val;
        break;
      case AST_Expression::EV_ulonglong:
        first_label << dv.u.ulonglong_val;
        break;
      default:
        std::cerr << "Illegal discriminator for union\n";
        break;
      }

    return first_label.str();
  }

  struct GenerateGettersAndSetters
  {
    AST_Union* the_union;
    AST_Type* discriminator;

    GenerateGettersAndSetters (AST_Union* u, AST_Type* d)
      : the_union(u)
      , discriminator(d)
    { }

    void operator() (AST_UnionBranch* branch)
    {
      const char* field_name = branch->local_name()->get_string();
      std::stringstream first_label;
      {
        AST_UnionLabel* label = branch->label(0);
        if (label->label_kind() == AST_UnionLabel::UL_default) {
          first_label << generateDefaultValue(the_union, discriminator);
        } else if (discriminator->node_type() == AST_Decl::NT_enum) {
          first_label << getEnumLabel(label->label_val(), discriminator);
        } else {
          first_label << *label->label_val()->ev();
        }
      }

      AST_Type* field_type = branch->field_type();
      const std::string field_type_string = map_type(field_type);
      AST_Type* actual_field_type = resolveActualType(field_type);
      const Classification cls = classify(actual_field_type);
      if (cls & (CL_PRIMITIVE | CL_ENUM)) {
        be_global->lang_header_ <<
          "  void " << field_name << " (" << field_type_string << " x) {\n"
          "    _reset();\n"
          "    this->_u." << field_name << " = x;\n"
          "    _discriminator = " << first_label.str() << ";\n"
          "  }\n"
          "  " << field_type_string << ' ' << field_name << " () const {\n"
          "    return this->_u." << field_name << ";\n"
          "  }\n";
      } else if (cls & CL_STRING) {
        const std::string& primtype = (cls & CL_WIDE) ? primtype_[AST_PredefinedType::PT_wchar] : primtype_[AST_PredefinedType::PT_char];
        const std::string& helper = (cls & CL_WIDE) ? helpers_[HLP_WSTR_VAR] : helpers_[HLP_STR_VAR];
        be_global->lang_header_ <<
          "  void " << field_name << " (" << primtype << "* x) {\n"
          "    _reset();\n" <<
          "    this->_u." << field_name << " = x;\n"
          "    _discriminator = " << first_label.str() << ";\n"
          "  }\n"
          "  void " << field_name << " (const " << primtype << "* x) {\n"
          "    _reset();\n"
          "    this->_u." << field_name << " = ::CORBA::string_dup(x);\n"
          "    _discriminator = " << first_label.str() << ";\n"
          "  }\n"
          "  void " << field_name << " (const " << helper << "& x) {\n"
          "    _reset();\n" <<
          "    this->_u." << field_name << " = ::CORBA::string_dup(x.in());\n"
          "    _discriminator = " << first_label.str() << ";\n"
          "  }\n"
          "  const " << primtype << "* " << field_name << " () const {\n"
          "    return this->_u." << field_name << ";\n"
          "  }\n";
      } else if (cls & CL_ARRAY) {
        be_global->lang_header_ <<
          "  void " << field_name << " (" << field_type_string << " x) {\n"
          "    _reset();\n" <<
          "    this->_u." << field_name << " = " << field_type_string << "_dup(x);\n"
          "    _discriminator = " << first_label.str() << ";\n"
          "  }\n"
          "  " << field_type_string << "_slice* " << field_name << " () const {\n"
          "    return this->_u." << field_name << ";\n"
          "  }\n";
      } else if (cls & (CL_STRUCTURE | CL_UNION | CL_SEQUENCE | CL_FIXED)) {
        be_global->lang_header_ <<
          "  void " << field_name << " (const " << field_type_string << "& x) {\n"
          "    _reset();\n"
          "    this->_u." << field_name << " = new " << field_type_string << "(x);\n"
          "    _discriminator = " << first_label.str() << ";\n"
          "  }\n"
          "  const " << field_type_string << "& " << field_name << " () const {\n"
          "    return *this->_u." << field_name << ";\n"
          "  }\n"
          "  " << field_type_string << "& " << field_name << " () {\n"
          "    return *this->_u." << field_name << ";\n"
          "  }\n";
      } else {
        std::cerr << "Unsupported type for union element\n";
      }
    }
  };

  static bool hasDefaultLabel (const std::vector<AST_UnionBranch*>& branches)
  {
    for (std::vector<AST_UnionBranch*>::const_iterator pos = branches.begin(), limit = branches.end();
         pos != limit;
         ++pos) {
      AST_UnionBranch* branch = *pos;
      for (unsigned long j = 0; j < branch->label_list_length(); ++j) {
        AST_UnionLabel* label = branch->label(j);
        if (label->label_kind() == AST_UnionLabel::UL_default) {
          return true;
        }
      }
    }
    return false;
  }

  static size_t countLabels (const std::vector<AST_UnionBranch*>& branches)
  {
    size_t count = 0;

    for (std::vector<AST_UnionBranch*>::const_iterator pos = branches.begin(), limit = branches.end();
         pos != limit;
         ++pos) {
      count += (*pos)->label_list_length();
    }

    return count;
  }

  static bool needsDefault (const std::vector<AST_UnionBranch*>& branches, AST_Type* discriminator)
  {
    return !hasDefaultLabel(branches) && needSyntheticDefault(discriminator, countLabels(branches));
  }

  static void generate_union_field (AST_UnionBranch* branch)
  {
    AST_Type* field_type = branch->field_type();
    AST_Type* actual_field_type = resolveActualType(field_type);
    const Classification cls = classify(actual_field_type);
    if (cls & (CL_PRIMITIVE | CL_ENUM)) {
      be_global->lang_header_ <<
        "    " << map_type(field_type) << ' ' << branch->local_name()->get_string() << ";\n";
    } else if (cls & CL_STRING) {
      const AST_PredefinedType::PredefinedType chartype = (cls & CL_WIDE)
        ? AST_PredefinedType::PT_wchar : AST_PredefinedType::PT_char;
      be_global->lang_header_ <<
        "    " << primtype_[chartype] << "* " << branch->local_name()->get_string() << ";\n";
    } else if (cls & CL_ARRAY) {
      be_global->lang_header_ <<
        "    " << map_type(field_type) << "_slice* " << branch->local_name()->get_string() << ";\n";
    } else if (cls & (CL_STRUCTURE | CL_UNION | CL_SEQUENCE | CL_FIXED)) {
      be_global->lang_header_ <<
        "    " << map_type(field_type) << "* " << branch->local_name()->get_string() << ";\n";
    } else {
      std::cerr << "Unsupported type for union element\n";
    }
  }

  static std::string generateCopyCtor(const std::string& name, AST_Type* field_type,
                                      const std::string&, std::string&,
                                      const std::string&)
  {
    std::stringstream ss;
    AST_Type* actual_field_type = resolveActualType(field_type);
    const Classification cls = classify(actual_field_type);
    if (cls & (CL_PRIMITIVE | CL_ENUM)) {
      ss <<
        "    this->_u." << name << " = other._u." << name << ";\n";
    } else if (cls & CL_STRING) {
      ss <<
        "    this->_u." << name << " = (other._u." << name << ") ? ::CORBA::string_dup(other._u." << name << ") : 0 ;\n";
    } else if (cls & CL_ARRAY) {
      ss <<
        "    this->_u." << name << " = (other._u." << name << ") ? " << map_type(field_type) << "_dup(other._u." << name << ") : 0 ;\n";
    } else if (cls & (CL_STRUCTURE | CL_UNION | CL_SEQUENCE | CL_FIXED)) {
      ss <<
        "    this->_u." << name << " = (other._u." << name << ") ? new " << map_type(field_type) << "(*other._u." << name << ") : 0;\n";
    } else {
      std::cerr << "Unsupported type for union element\n";
    }

    return ss.str();
  }

  static std::string generateAssign(const std::string& name, AST_Type* field_type,
                                    const std::string&, std::string&,
                                    const std::string&)
  {
    std::stringstream ss;
    AST_Type* actual_field_type = resolveActualType(field_type);
    const Classification cls = classify(actual_field_type);
    if (cls & (CL_PRIMITIVE | CL_ENUM)) {
      ss <<
        "      this->_u." << name << " = other._u." << name << ";\n";
    } else if (cls & CL_STRING) {
      ss <<
        "      this->_u." << name << " = (other._u." << name << ") ? ::CORBA::string_dup(other._u." << name << ") : 0 ;\n";
    } else if (cls & CL_ARRAY) {
      ss <<
        "    this->_u." << name << " = (other._u." << name << ") ? " << map_type(field_type) << "_dup(other._u." << name << ") : 0 ;\n";
    } else if (cls & (CL_STRUCTURE | CL_UNION | CL_SEQUENCE | CL_FIXED)) {
      ss <<
        "      this->_u." << name << " = (other._u." << name << ") ? new " << map_type(field_type) << "(*other._u." << name << ") : 0;\n";
    } else {
      std::cerr << "Unsupported type for union element\n";
    }

    return ss.str();
  }

  static std::string generateEqual(const std::string& name, AST_Type* field_type,
                                   const std::string&, std::string&,
                                   const std::string&)
  {
    std::stringstream ss;

    AST_Type* actual_field_type = resolveActualType(field_type);
    const Classification cls = classify(actual_field_type);
    if (cls & (CL_PRIMITIVE | CL_ENUM)) {
      ss <<
        "      return this->_u." << name << " == rhs._u." << name << ";\n";
    } else if (cls & CL_STRING) {
      ss <<
        "      return std::strcmp (this->_u." << name << ", rhs._u." << name << ") == 0 ;\n";
    } else if (cls & CL_ARRAY) {
      // TODO
      ss <<
        "      return false;\n";
    } else if (cls & (CL_STRUCTURE | CL_UNION | CL_SEQUENCE | CL_FIXED)) {
      ss <<
        "      return *this->_u." << name << " == *rhs._u." << name << ";\n";
    } else {
      std::cerr << "Unsupported type for union element\n";
    }

    return ss.str();
  }

  static std::string generateReset(const std::string& name, AST_Type* field_type,
                                   const std::string&, std::string&,
                                   const std::string&)
  {
    std::stringstream ss;

    AST_Type* actual_field_type = resolveActualType(field_type);
    const Classification cls = classify(actual_field_type);
    if (cls & (CL_PRIMITIVE | CL_ENUM)) {
      // Do nothing.
    } else if (cls & CL_STRING) {
      ss <<
        "      ::CORBA::string_free(this->_u." << name << ");\n"
        "      this->_u." << name << " = 0;\n";
    } else if (cls & CL_ARRAY) {
      ss <<
        "      " << map_type(field_type) << "_free(this->_u." << name << ");\n"
        "      this->_u." << name << " = 0;\n";
    } else if (cls & (CL_STRUCTURE | CL_UNION | CL_SEQUENCE | CL_FIXED)) {
      ss <<
        "      delete this->_u." << name << ";\n"
        "      this->_u." << name << " = 0;\n";
    } else {
      std::cerr << "Unsupported type for union element\n";
    }

    return ss.str();
  }

  bool gen_union(AST_Union* u, UTL_ScopedName* name, const std::vector<AST_UnionBranch*>& branches, AST_Type* discriminator)
  {
    const ScopedNamespaceGuard namespaces(name, be_global->lang_header_);
    const char* const nm = name->last_component()->get_string();
    struct_decls(name, u->size_type(), "class");
    be_global->lang_header_ <<
      "\n"
      "class " << exporter() << nm << " \n"
      "{\n"
      " public:\n"
      "  typedef " << nm << "_var _var_type;\n"
      "  typedef " << nm << "_out _out_type;\n"
      "  " << nm << "();\n"
      "  " << nm << "(const " << nm << "&);\n"
      "  ~" << nm << "() { _reset(); };\n"
      "  " << nm << "& operator=(const " << nm << "&);\n"
      "  void _d(" << scoped(discriminator->name()) << " d) { _discriminator = d; }\n"
      "  " << scoped(discriminator->name()) << " _d() const { return _discriminator; }\n";

    std::for_each (branches.begin(), branches.end(), GenerateGettersAndSetters(u, discriminator));

    if (needsDefault(branches, discriminator)) {
      be_global->lang_header_ <<
        "  void _default() {\n"
        "    _reset();\n"
        "    _discriminator = " << generateDefaultValue(u, discriminator) << ";\n"
        "  }\n";
    }

    be_global->lang_header_ <<
      "  bool operator==(const " << nm << "& rhs) const;\n"
      "  bool operator!=(const " << nm << "& rhs) const { return !(*this == rhs); }\n"
      "  OPENDDS_POOL_ALLOCATION_HOOKS\n";

    be_global->lang_header_ <<
      " private:\n"
      "  " << scoped(discriminator->name()) << " _discriminator;\n"
      "  union {\n";

    std::for_each (branches.begin(), branches.end(), generate_union_field);

    be_global->lang_header_ <<
      "  } _u;\n";

    be_global->lang_header_ <<
      "  void _reset();\n"
      "};\n\n";

    be_global->add_include("dds/DCPS/PoolAllocationBase.h");
    be_global->add_include("<ace/CDR_Stream.h>", BE_GlobalData::STREAM_LANG_H);

    be_global->lang_header_ <<
      exporter() << "ACE_CDR::Boolean operator<< (ACE_OutputCDR& os, const " << nm << "& x);\n\n";
    be_global->lang_header_ <<
      exporter() << "ACE_CDR::Boolean operator>> (ACE_InputCDR& os, " << nm << "& x);\n\n";

    {
      const ScopedNamespaceGuard guard(name, be_global->impl_);

      be_global->impl_ <<
        nm << "::" << nm << "() { std::memset (this, 0, sizeof (" << nm << ")); }\n\n";

      be_global->impl_ <<
        nm << "::" << nm << "(const " << nm << "& other) {\n" <<
        "  this->_discriminator = other._discriminator;\n";
      generateSwitchForUnion("this->_discriminator", generateCopyCtor, branches, discriminator, "", "", "", false, false);
      be_global->impl_ <<
        "}\n\n";

      be_global->impl_ <<
        nm << "& " << nm << "::operator=(const " << nm << "& other) {\n" <<
        "  if (this != &other) {\n" <<
        "    _reset();\n" <<
        "    this->_discriminator = other._discriminator;\n";
      generateSwitchForUnion("this->_discriminator", generateAssign, branches, discriminator, "", "", "", false, false);
      be_global->impl_ <<
        "  }\n"
        "  return *this;\n"
        "}\n\n";

      be_global->impl_ <<
        "bool " << nm << "::operator==(const " << nm << "& rhs) const\n"
        "{\n"
        "  if (this->_discriminator != rhs._discriminator) return false;\n";
      generateSwitchForUnion("this->_discriminator", generateEqual, branches, discriminator, "", "", "", false, false);
      be_global->impl_ <<
        "    return false;\n"
        "  }\n";

      be_global->impl_ <<
        "void " << nm << "::_reset()\n"
        "{\n";
      generateSwitchForUnion("this->_discriminator", generateReset, branches, discriminator, "", "", "", false, false);
      be_global->impl_ <<
        "  }\n";

      be_global->impl_ <<
        "ACE_CDR::Boolean operator<< (ACE_OutputCDR &, const " << nm << "&) { return true; }\n\n";
      be_global->impl_ <<
        "ACE_CDR::Boolean operator>> (ACE_InputCDR &, " << nm << "&) { return true; }\n\n";
    }

    gen_typecode(name);
    return true;
  }
};

class FaceGenerator : public GeneratorBase
{
public:
  virtual void init()
  {
    be_global->add_include("FACE/types.hpp", BE_GlobalData::STREAM_LANG_H);
    be_global->add_include("FACE/StringManager.h", BE_GlobalData::STREAM_LANG_H);
    primtype_[AST_PredefinedType::PT_long] = "::FACE::Long";
    primtype_[AST_PredefinedType::PT_ulong] = "::FACE::UnsignedLong";
    primtype_[AST_PredefinedType::PT_longlong] = "::FACE::LongLong";
    primtype_[AST_PredefinedType::PT_ulonglong] = "::FACE::UnsignedLongLong";
    primtype_[AST_PredefinedType::PT_short] = "::FACE::Short";
    primtype_[AST_PredefinedType::PT_ushort] = "::FACE::UnsignedShort";
    primtype_[AST_PredefinedType::PT_float] = "::FACE::Float";
    primtype_[AST_PredefinedType::PT_double] = "::FACE::Double";
    primtype_[AST_PredefinedType::PT_longdouble] = "::FACE::LongDouble";
    primtype_[AST_PredefinedType::PT_char] = "::FACE::Char";
    primtype_[AST_PredefinedType::PT_wchar] = "::FACE::WChar";
    primtype_[AST_PredefinedType::PT_boolean] = "::FACE::Boolean";
    primtype_[AST_PredefinedType::PT_octet] = "::FACE::Octet";
    helpers_[HLP_STR_VAR] = "::FACE::String_var";
    helpers_[HLP_STR_OUT] = "::FACE::String_out";
    helpers_[HLP_WSTR_VAR] = "::FACE::WString_var";
    helpers_[HLP_WSTR_OUT] = "::FACE::WString_out";
    helpers_[HLP_STR_MGR] = "::OpenDDS::FaceTypes::String_mgr";
    helpers_[HLP_WSTR_MGR] = "::OpenDDS::FaceTypes::WString_mgr";
    helpers_[HLP_FIX_VAR] = "::TAO_Fixed_Var_T";
    helpers_[HLP_VAR_VAR] = "::TAO_Var_Var_T";
    helpers_[HLP_OUT] = "::TAO_Out_T";
    helpers_[HLP_SEQ] = "::OpenDDS::FaceTypes::Sequence";
    helpers_[HLP_SEQ_NS] = "::OpenDDS::FaceTypes";
    helpers_[HLP_SEQ_VAR_VAR] = "::OpenDDS::FaceTypes::SequenceVar";
    helpers_[HLP_SEQ_FIX_VAR] = "::OpenDDS::FaceTypes::SequenceVar";
    helpers_[HLP_SEQ_OUT] = "::TAO_Seq_Out_T";
    helpers_[HLP_ARR_VAR_VAR] = "::TAO_VarArray_Var_T";
    helpers_[HLP_ARR_FIX_VAR] = "::TAO_FixedArray_Var_T";
    helpers_[HLP_ARR_OUT] = "::TAO_Array_Out_T";
    helpers_[HLP_ARR_FORANY] = "::TAO_Array_Forany_T";
    helpers_[HLP_FIXED] = "::OpenDDS::FaceTypes::Fixed_T";
    helpers_[HLP_FIXED_CONSTANT] = "::OpenDDS::FaceTypes::Fixed";
  }

  void gen_sequence(UTL_ScopedName* tdname, AST_Sequence* seq)
  {
    be_global->add_include("<tao/Seq_Out_T.h>", BE_GlobalData::STREAM_LANG_H);
    be_global->add_include("FACE/Sequence.h", BE_GlobalData::STREAM_LANG_H);
    be_global->add_include("FACE/SequenceVar.h", BE_GlobalData::STREAM_LANG_H);
    be_global->add_include("<ace/CDR_Stream.h>", BE_GlobalData::STREAM_LANG_H);
    const char* const nm = tdname->last_component()->get_string();
    AST_Type* elem = seq->base_type();
    const Classification elem_cls = classify(elem);
    const bool bounded = !seq->unbounded();
    const Helper var = (elem->size_type() == AST_Type::VARIABLE)
                        ? HLP_SEQ_VAR_VAR : HLP_SEQ_FIX_VAR,
      out = HLP_SEQ_OUT;

    std::string elem_type = map_type(elem), extra_tmp_args;
    if (elem_cls & CL_STRING) {
      const AST_Expression::ExprType char_type = (elem_cls & CL_WIDE)
        ? AST_Expression::EV_wchar : AST_Expression::EV_char;
      extra_tmp_args = ", " + helpers_[HLP_SEQ_NS] + "::StringEltPolicy< "
        + map_type(char_type) + ">";
    } else if (elem_cls & CL_ARRAY) {
      extra_tmp_args = ", " + helpers_[HLP_SEQ_NS] + "::ArrayEltPolicy<"
        + elem_type + "_forany>";
    } else if (elem->size_type() == AST_Type::VARIABLE) {
      extra_tmp_args = ", " +  helpers_[HLP_SEQ_NS] + "::VariEltPolicy<"
        + elem_type + ">";
    }

    std::string bound = helpers_[HLP_SEQ_NS] + "::Unbounded";
    if (bounded) {
      std::ostringstream oss;
      oss << helpers_[HLP_SEQ_NS] << "::Bounded<"
        << seq->max_size()->ev()->u.ulval << '>';
      bound = oss.str();
    }

    const std::string base = helpers_[HLP_SEQ] + "< " + elem_type + ", " + bound
                           + extra_tmp_args + " >",
      len_type = primtype_[AST_PredefinedType::PT_ulong],
      flag_type = primtype_[AST_PredefinedType::PT_boolean];

    be_global->lang_header_ <<
      "class " << nm << ";\n"
      "typedef " << helpers_[var] << '<' << nm << "> " << nm << "_var;\n"
      "typedef " << helpers_[out] << '<' << nm << "> " << nm << "_out;\n\n"
      "class " << exporter() << nm << " : public " << base << " {\n"
      "public:\n"
      "  typedef " << nm << "_var _var_type;\n"
      "  typedef " << nm << "_out _out_type;\n\n"
      "  " << nm << "() {}\n"
      "  " << nm << "(const " << nm << "& seq) : " << base << "(seq) {}\n"
      "  friend void swap(" << nm << "& a, " << nm << "& b) { a.swap(b); }\n"
      "  " << nm << "& operator=(const " << nm << "& rhs)\n"
      "  {\n"
      "    " << nm << " tmp(rhs);\n"
      "    swap(tmp);\n"
      "    return *this;\n"
      "  }\n";

    if (bounded) {
      be_global->lang_header_ <<
        "  " << nm << "(" << len_type << " length, " << elem_type << "* data, "
        << flag_type << " release = false)\n"
        "    : " << base << "(0u, length, data, release) {}\n";
    } else {
      be_global->lang_header_ <<
        "  " << nm << "(" << len_type << " maximum)\n"
        "    : " << base << "(maximum, 0u, 0, true) {}\n"
        "  " << nm << "(" << len_type << " maximum, " << len_type << " length, "
        << elem_type << "* data, " << flag_type << " release = false)\n"
        "    : " << base << "(maximum, length, data, release) {}\n";
    }
    be_global->lang_header_ <<
      "};\n\n";

    be_global->lang_header_ <<
      "inline ACE_CDR::Boolean operator<< (ACE_OutputCDR&, const " << nm << "&) { return true; }\n\n";

    be_global->lang_header_ <<
      "inline ACE_CDR::Boolean operator>> (ACE_InputCDR&, " << nm << "&) { return true; }\n\n";
  }

  bool gen_struct(AST_Structure*, UTL_ScopedName* name,
                  const std::vector<AST_Field*>& fields,
                  AST_Type::SIZE_TYPE size,
                  const char*)
  {
    const ScopedNamespaceGuard namespaces(name, be_global->lang_header_);
    const char* const nm = name->last_component()->get_string();
    struct_decls(name, size);
    be_global->lang_header_ <<
      "\n"
      "struct " << exporter() << nm << "\n"
      "{\n"
      "  typedef " << nm << "_var _var_type;\n"
      "  typedef " << nm << "_out _out_type;\n\n";

    for (size_t i = 0; i < fields.size(); ++i) {
      AST_Type* field_type = fields[i]->field_type();
      const std::string field_name = fields[i]->local_name()->get_string();
      std::string type_name = map_type(field_type);
      const Classification cls = classify(field_type);
      if (cls & CL_STRING) {
        type_name = helpers_[(cls & CL_WIDE) ? HLP_WSTR_MGR : HLP_STR_MGR];
      }
      be_global->lang_header_ <<
        "  " << type_name << ' ' << field_name << ";\n";
    }

    be_global->lang_header_ << "\n"
      "  bool operator==(const " << nm << "& rhs) const;\n"
      "  bool operator!=(const " << nm << "& rhs) const { return !(*this == rhs); }\n"
      "  OPENDDS_POOL_ALLOCATION_HOOKS\n"
      "};\n\n";

    be_global->add_include("dds/DCPS/PoolAllocationBase.h");
    be_global->add_include("<ace/CDR_Stream.h>", BE_GlobalData::STREAM_LANG_H);

    if (size == AST_Type::VARIABLE) {
      be_global->lang_header_ <<
        exporter() << "void swap(" << nm << "& lhs, " << nm << "& rhs);\n\n";
    }

    be_global->lang_header_ <<
      exporter() << "ACE_CDR::Boolean operator<< (ACE_OutputCDR& os, const " << nm << "& x);\n\n";
    be_global->lang_header_ <<
      exporter() << "ACE_CDR::Boolean operator>> (ACE_InputCDR& os, " << nm << "& x);\n\n";

    {
      const ScopedNamespaceGuard guard(name, be_global->impl_);
      be_global->impl_ <<
        "bool " << nm << "::operator==(const " << nm << "& rhs) const\n"
        "{\n";
      for (size_t i = 0; i < fields.size(); ++i) {
        const std::string field_name = fields[i]->local_name()->get_string();
        AST_Type* field_type = resolveActualType(fields[i]->field_type());
        const Classification cls = classify(field_type);
        if (cls & CL_ARRAY) {
          std::string indent("  ");
          NestedForLoops nfl("int", "i",
            AST_Array::narrow_from_decl(field_type), indent, true);
          be_global->impl_ <<
            indent << "if (" << field_name << nfl.index_ << " != rhs."
            << field_name << nfl.index_ << ") {\n" <<
            indent << "  return false;\n" <<
            indent << "}\n";
        } else {
          be_global->impl_ <<
            "  if (" << field_name << " != rhs." << field_name << ") {\n"
            "    return false;\n"
            "  }\n";
        }
      }
      be_global->impl_ << "  return true;\n}\n\n";

      if (size == AST_Type::VARIABLE) {
        be_global->impl_ <<
          "void swap(" << nm << "& lhs, " << nm << "& rhs)\n"
          "{\n"
          "  using std::swap;\n";
        for (size_t i = 0; i < fields.size(); ++i) {
          const std::string fn = fields[i]->local_name()->get_string();
          AST_Type* field_type = resolveActualType(fields[i]->field_type());
          const Classification cls = classify(field_type);
          if (cls & CL_ARRAY) {
            ACE_CDR::ULong elems = 1;
            const std::string flat_fn = fn + array_dims(field_type, elems);
            be_global->add_include("<algorithm>", BE_GlobalData::STREAM_CPP);
            be_global->impl_ <<
              "  std::swap_ranges(lhs." << flat_fn << ", lhs." << flat_fn
                                        << " + " << elems << ", rhs." << flat_fn << ");\n";
          } else {
            be_global->impl_ <<
              "  swap(lhs." << fn << ", rhs." << fn << ");\n";
          }
        }
        be_global->impl_ << "}\n\n";
      }

      be_global->impl_ <<
        "ACE_CDR::Boolean operator<< (ACE_OutputCDR &, const " << nm << "&) { return true; }\n\n";
      be_global->impl_ <<
        "ACE_CDR::Boolean operator>> (ACE_InputCDR &, " << nm << "&) { return true; }\n\n";
    }

    gen_typecode(name);
    return true;
  }

  static FaceGenerator instance;
};
FaceGenerator FaceGenerator::instance;

class SafetyProfileGenerator : public GeneratorBase
{
public:
  virtual void init()
  {
    be_global->add_include("tao/String_Manager_T.h", BE_GlobalData::STREAM_LANG_H);
    be_global->add_include("tao/CORBA_String.h", BE_GlobalData::STREAM_LANG_H);
    primtype_[AST_PredefinedType::PT_long] = "CORBA::Long";
    primtype_[AST_PredefinedType::PT_ulong] = "CORBA::ULong";
    primtype_[AST_PredefinedType::PT_longlong] = "CORBA::LongLong";
    primtype_[AST_PredefinedType::PT_ulonglong] = "CORBA::UnsignedLongLong";
    primtype_[AST_PredefinedType::PT_short] = "CORBA::Short";
    primtype_[AST_PredefinedType::PT_ushort] = "CORBA::UShort";
    primtype_[AST_PredefinedType::PT_float] = "CORBA::Float";
    primtype_[AST_PredefinedType::PT_double] = "CORBA::Double";
    primtype_[AST_PredefinedType::PT_longdouble] = "CORBA::LongDouble";
    primtype_[AST_PredefinedType::PT_char] = "CORBA::Char";
    primtype_[AST_PredefinedType::PT_wchar] = "CORBA::WChar";
    primtype_[AST_PredefinedType::PT_boolean] = "CORBA::Boolean";
    primtype_[AST_PredefinedType::PT_octet] = "CORBA::Octet";
    helpers_[HLP_STR_VAR] = "CORBA::String_var";
    helpers_[HLP_STR_OUT] = "CORBA::String_out";
    helpers_[HLP_WSTR_VAR] = "CORBA::WString_var";
    helpers_[HLP_WSTR_OUT] = "CORBA::WString_out";
    helpers_[HLP_STR_MGR] = "::TAO::String_Manager";
    helpers_[HLP_WSTR_MGR] = "CORBA::WString_mgr";
    helpers_[HLP_FIX_VAR] = "::TAO_Fixed_Var_T";
    helpers_[HLP_VAR_VAR] = "::TAO_Var_Var_T";
    helpers_[HLP_OUT] = "::TAO_Out_T";
    helpers_[HLP_SEQ] = "::OpenDDS::SafetyProfile::Sequence";
    helpers_[HLP_SEQ_NS] = "::OpenDDS::SafetyProfile";
    helpers_[HLP_SEQ_VAR_VAR] = "::OpenDDS::SafetyProfile::SequenceVar";
    helpers_[HLP_SEQ_FIX_VAR] = "::OpenDDS::SafetyProfile::SequenceVar";
    helpers_[HLP_SEQ_OUT] = "::TAO_Seq_Out_T";
    helpers_[HLP_ARR_VAR_VAR] = "::TAO_VarArray_Var_T";
    helpers_[HLP_ARR_FIX_VAR] = "::TAO_FixedArray_Var_T";
    helpers_[HLP_ARR_OUT] = "::TAO_Array_Out_T";
    helpers_[HLP_ARR_FORANY] = "::TAO_Array_Forany_T";
  }

  virtual void gen_sequence(UTL_ScopedName* tdname, AST_Sequence* seq)
  {
    be_global->add_include("<tao/Seq_Out_T.h>", BE_GlobalData::STREAM_LANG_H);
    be_global->add_include("dds/DCPS/SafetyProfileSequence.h", BE_GlobalData::STREAM_LANG_H);
    be_global->add_include("dds/DCPS/SafetyProfileSequenceVar.h", BE_GlobalData::STREAM_LANG_H);
    const char* const nm = tdname->last_component()->get_string();
    AST_Type* elem = seq->base_type();
    const Classification elem_cls = classify(elem);
    const bool bounded = !seq->unbounded();
    const Helper var = (elem->size_type() == AST_Type::VARIABLE)
                        ? HLP_SEQ_VAR_VAR : HLP_SEQ_FIX_VAR,
      out = HLP_SEQ_OUT;

    std::string elem_type = map_type(elem), extra_tmp_args;
    if (elem_cls & CL_STRING) {
      const AST_Expression::ExprType char_type = (elem_cls & CL_WIDE)
        ? AST_Expression::EV_wchar : AST_Expression::EV_char;
      extra_tmp_args = ", " + helpers_[HLP_SEQ_NS] + "::StringEltPolicy< "
        + map_type(char_type) + ">";
    } else if (elem_cls & CL_ARRAY) {
      extra_tmp_args = ", " + helpers_[HLP_SEQ_NS] + "::ArrayEltPolicy<"
        + elem_type + "_forany>";
    } else if (elem->size_type() == AST_Type::VARIABLE) {
      extra_tmp_args = ", " +  helpers_[HLP_SEQ_NS] + "::VariEltPolicy<"
        + elem_type + ">";
    }

    std::string bound = helpers_[HLP_SEQ_NS] + "::Unbounded";
    if (bounded) {
      std::ostringstream oss;
      oss << helpers_[HLP_SEQ_NS] << "::Bounded<"
        << seq->max_size()->ev()->u.ulval << '>';
      bound = oss.str();
    }

    const std::string base = helpers_[HLP_SEQ] + "< " + elem_type + ", " + bound
                           + extra_tmp_args + " >",
      len_type = primtype_[AST_PredefinedType::PT_ulong],
      flag_type = primtype_[AST_PredefinedType::PT_boolean];

    be_global->lang_header_ <<
      "class " << nm << ";\n"
      "typedef " << helpers_[var] << '<' << nm << "> " << nm << "_var;\n"
      "typedef " << helpers_[out] << '<' << nm << "> " << nm << "_out;\n\n"
      "class " << exporter() << nm << " : public " << base << " {\n"
      "public:\n"
      "  typedef " << nm << "_var _var_type;\n"
      "  typedef " << nm << "_out _out_type;\n\n"
      "  " << nm << "() {}\n"
      "  " << nm << "(const " << nm << "& seq) : " << base << "(seq) {}\n"
      "  friend void swap(" << nm << "& a, " << nm << "& b) { a.swap(b); }\n"
      "  " << nm << "& operator=(const " << nm << "& rhs)\n"
      "  {\n"
      "    " << nm << " tmp(rhs);\n"
      "    swap(tmp);\n"
      "    return *this;\n"
      "  }\n";

    if (bounded) {
      be_global->lang_header_ <<
        "  " << nm << "(" << len_type << " length, " << elem_type << "* data, "
        << flag_type << " release = false)\n"
        "    : " << base << "(0u, length, data, release) {}\n";
    } else {
      be_global->lang_header_ <<
        "  " << nm << "(" << len_type << " maximum)\n"
        "    : " << base << "(maximum, 0u, 0, true) {}\n"
        "  " << nm << "(" << len_type << " maximum, " << len_type << " length, "
        << elem_type << "* data, " << flag_type << " release = false)\n"
        "    : " << base << "(maximum, length, data, release) {}\n";
    }
    be_global->lang_header_ <<
      "};\n\n";

    be_global->lang_header_ <<
      "inline ACE_CDR::Boolean operator<< (ACE_OutputCDR&, const " << nm << "&) { return true; }\n\n";

    be_global->lang_header_ <<
      "inline ACE_CDR::Boolean operator>> (ACE_InputCDR&, " << nm << "&) { return true; }\n\n";
  }

  bool gen_struct(AST_Structure*, UTL_ScopedName* name,
                  const std::vector<AST_Field*>& fields,
                  AST_Type::SIZE_TYPE size,
                  const char*)
  {
    const ScopedNamespaceGuard namespaces(name, be_global->lang_header_);
    const char* const nm = name->last_component()->get_string();
    struct_decls(name, size);
    be_global->lang_header_ <<
      "\n"
      "struct " << exporter() << nm << " \n"
      "{\n"
      "  typedef " << nm << "_var _var_type;\n"
      "  typedef " << nm << "_out _out_type;\n\n";

    for (size_t i = 0; i < fields.size(); ++i) {
      AST_Type* field_type = fields[i]->field_type();
      const std::string field_name = fields[i]->local_name()->get_string();
      std::string type_name = map_type(field_type);
      const Classification cls = classify(field_type);
      if (cls & CL_STRING) {
        type_name = helpers_[(cls & CL_WIDE) ? HLP_WSTR_MGR : HLP_STR_MGR];
      }
      be_global->lang_header_ <<
        "  " << type_name << ' ' << field_name << ";\n";
    }

    be_global->lang_header_ << "\n"
      "  bool operator==(const " << nm << "& rhs) const;\n"
      "  bool operator!=(const " << nm << "& rhs) const { return !(*this == rhs); }\n"
      "  OPENDDS_POOL_ALLOCATION_HOOKS\n"
      "};\n\n";

    be_global->add_include("dds/DCPS/PoolAllocationBase.h");
    be_global->add_include("<ace/CDR_Stream.h>", BE_GlobalData::STREAM_LANG_H);

    if (size == AST_Type::VARIABLE) {
      be_global->lang_header_ <<
        exporter() << "void swap(" << nm << "& lhs, " << nm << "& rhs);\n\n";
    }

    be_global->lang_header_ <<
      exporter() << "ACE_CDR::Boolean operator<< (ACE_OutputCDR& os, const " << nm << "& x);\n\n";
    be_global->lang_header_ <<
      exporter() << "ACE_CDR::Boolean operator>> (ACE_InputCDR& os, " << nm << "& x);\n\n";

    {
      const ScopedNamespaceGuard guard(name, be_global->impl_);
      be_global->impl_ <<
        "bool " << nm << "::operator==(const " << nm << "& rhs) const\n"
        "{\n";
      for (size_t i = 0; i < fields.size(); ++i) {
        const std::string field_name = fields[i]->local_name()->get_string();
        AST_Type* field_type = resolveActualType(fields[i]->field_type());
        const Classification cls = classify(field_type);
        if (cls & CL_ARRAY) {
          std::string indent("  ");
          NestedForLoops nfl("int", "i",
            AST_Array::narrow_from_decl(field_type), indent, true);
          be_global->impl_ <<
            indent << "if (" << field_name << nfl.index_ << " != rhs."
            << field_name << nfl.index_ << ") {\n" <<
            indent << "  return false;\n" <<
            indent << "}\n";
        } else {
          be_global->impl_ <<
            "  if (" << field_name << " != rhs." << field_name << ") {\n"
            "    return false;\n"
            "  }\n";
        }
      }
      be_global->impl_ << "  return true;\n}\n\n";

      if (size == AST_Type::VARIABLE) {
        be_global->impl_ <<
          "void swap(" << nm << "& lhs, " << nm << "& rhs)\n"
          "{\n"
          "  using std::swap;\n";
        for (size_t i = 0; i < fields.size(); ++i) {
          const std::string fn = fields[i]->local_name()->get_string();
          AST_Type* field_type = resolveActualType(fields[i]->field_type());
          const Classification cls = classify(field_type);
          if (cls & CL_ARRAY) {
            ACE_CDR::ULong elems = 1;
            const std::string flat_fn = fn + array_dims(field_type, elems);
            be_global->add_include("<algorithm>", BE_GlobalData::STREAM_CPP);
            be_global->impl_ <<
              "  std::swap_ranges(lhs." << flat_fn << ", lhs." << flat_fn
                                        << " + " << elems << ", rhs." << flat_fn << ");\n";
          } else {
            be_global->impl_ <<
              "  swap(lhs." << fn << ", rhs." << fn << ");\n";
          }
        }
        be_global->impl_ << "}\n\n";
      }

      be_global->impl_ <<
        "ACE_CDR::Boolean operator<< (ACE_OutputCDR &, const " << nm << "&) { return true; }\n\n";
      be_global->impl_ <<
        "ACE_CDR::Boolean operator>> (ACE_InputCDR &, " << nm << "&) { return true; }\n\n";
    }

    gen_typecode(name);
    return true;
  }

  static SafetyProfileGenerator instance;
};
SafetyProfileGenerator SafetyProfileGenerator::instance;

void langmap_generator::init()
{
  switch (be_global->language_mapping()) {
  case BE_GlobalData::LANGMAP_FACE_CXX:
    generator_ = &FaceGenerator::instance;
    generator_->init();
    break;
  case BE_GlobalData::LANGMAP_SP_CXX:
    generator_ = &SafetyProfileGenerator::instance;
    generator_->init();
    break;
  default: break;
  }
}

bool langmap_generator::gen_const(UTL_ScopedName* name, bool,
                                  AST_Constant* constant)
{
  const ScopedNamespaceGuard namespaces(name, be_global->lang_header_);
  const char* const nm = name->last_component()->get_string();

  const AST_Expression::ExprType type = constant->et();
  const bool is_enum = (type == AST_Expression::EV_enum);
  const std::string type_name = is_enum
    ? scoped(constant->enum_full_name()) : map_type(type);
  be_global->lang_header_ <<
    "const " << type_name << ' ' << nm << " = ";

  if (is_enum) {
    be_global->lang_header_ << scoped(constant->constant_value()->n()) << ";\n";
  } else {
    be_global->lang_header_ << *constant->constant_value()->ev() << ";\n";
  }
  return true;
}

namespace {
  void gen_array(UTL_ScopedName* tdname, AST_Array* arr)
  {
    be_global->add_include("<tao/Array_VarOut_T.h>", BE_GlobalData::STREAM_LANG_H);
    be_global->add_include("dds/DCPS/SafetyProfilePool.h", BE_GlobalData::STREAM_LANG_H);
    const char* const nm = tdname->last_component()->get_string();
    AST_Type* elem = arr->base_type();
    const Classification elem_cls = classify(elem);
    const Helper var = (elem->size_type() == AST_Type::VARIABLE)
                        ? HLP_ARR_VAR_VAR : HLP_ARR_FIX_VAR,
      out = HLP_ARR_OUT,
      forany = HLP_ARR_FORANY;

    std::ostringstream bound, nofirst, total;
    std::string zeros;
    for (ACE_CDR::ULong dim = 0; dim < arr->n_dims(); ++dim) {
      const ACE_CDR::ULong extent = arr->dims()[dim]->ev()->u.ulval;
      bound << '[' << extent << ']';
      if (dim) {
        nofirst << '[' << extent << ']';
        zeros += "[0]";
        total << " * ";
      }
      total << extent;
    }

    std::string elem_type = map_type(elem);
    if (elem_cls & CL_STRING) {
      elem_type = helpers_[(elem_cls & CL_WIDE) ? HLP_WSTR_MGR : HLP_STR_MGR];
    }

    std::string out_type = nm;
    if (elem->size_type() == AST_Type::VARIABLE) {
      out_type = helpers_[out] + '<' + nm + ", " + nm + "_var, " + nm +
        "_slice, " + nm + "_tag>";
    }

    be_global->lang_header_ <<
      "typedef " << elem_type << ' ' << nm << bound.str() << ";\n"
      "typedef " << elem_type << ' ' << nm << "_slice" << nofirst.str() << ";\n"
      "struct " << nm << "_tag {};\n"
      "typedef " << helpers_[var] << '<' << nm << ", " << nm << "_slice, "
      << nm << "_tag> " << nm << "_var;\n"
      "typedef " << out_type << ' ' << nm << "_out;\n"
      "typedef " << helpers_[forany] << '<' << nm << ", " << nm << "_slice, "
      << nm << "_tag> " << nm << "_forany;\n\n" <<
      exporter() << nm << "_slice* " << nm << "_alloc();\n" <<
      exporter() << "void " << nm << "_init_i(" << elem_type << "* begin);\n" <<
      exporter() << "void " << nm << "_fini_i(" << elem_type << "* begin);\n" <<
      exporter() << "void " << nm << "_free(" << nm << "_slice* slice);\n" <<
      exporter() << nm << "_slice* " << nm << "_dup(const " << nm
      << "_slice* slice);\n" <<
      exporter() << "void " << nm << "_copy(" << nm << "_slice* dst, const "
      << nm << "_slice* src);\n\n";
    const ScopedNamespaceGuard namespaces(tdname, be_global->impl_);
    be_global->impl_ <<
      nm << "_slice* " << nm << "_alloc()\n"
      "{\n"
      "  void* const raw = ACE_Allocator::instance()->malloc"
      "(sizeof(" << nm << "));\n"
      "  " << nm << "_slice* const slice = static_cast<" << nm << "_slice*"
      << ">(raw);\n"
      "  " << nm << "_init_i(slice" << zeros << ");\n"
      "  return slice;\n"
      "}\n\n"
      "void " << nm << "_init_i(" << elem_type << "* begin)\n"
      "{\n";

    if (elem_cls & (CL_PRIMITIVE | CL_ENUM)) {
      be_global->impl_ << "  ACE_UNUSED_ARG(begin);\n";
    } else if (elem_cls & CL_ARRAY) {
      std::string indent = "  ";
      const NestedForLoops nfl("ACE_CDR::ULong", "i", arr, indent);
      be_global->impl_ <<
        indent << elem_type << "_init_i(begin" << nfl.index_ << ");\n";
    } else {
      be_global->impl_ <<
        "  std::uninitialized_fill_n(begin, " << total.str() << ", "
        << elem_type << "());\n";
    }

    be_global->impl_ <<
      "}\n\n"
      "void " << nm << "_fini_i(" << elem_type << "* begin)\n"
      "{\n";

    if (elem_cls & (CL_PRIMITIVE | CL_ENUM)) {
      be_global->impl_ << "  ACE_UNUSED_ARG(begin);\n";
    } else if (elem_cls & CL_ARRAY) {
      std::string indent = "  ";
      const NestedForLoops nfl("ACE_CDR::ULong", "i", arr, indent);
      be_global->impl_ <<
        indent << elem_type << "_fini_i(begin" << nfl.index_ << ");\n";
    } else {
      const std::string::size_type idx_last = elem_type.rfind("::");
      const std::string elem_last =
        (elem_cls & CL_STRING) ? "StringManager" :
        ((idx_last == std::string::npos) ? elem_type
          : elem_type.substr(idx_last + 2));
      be_global->impl_ <<
        "  for (int i = 0; i < " << total.str() << "; ++i) {\n"
        "    begin[i]."
#ifdef __SUNPRO_CC
        << elem_type << "::"
#endif
        "~" << elem_last << "();\n"
        "  }\n";
    }

    be_global->impl_ <<
      "}\n\n"
      "void " << nm << "_free(" << nm << "_slice* slice)\n"
      "{\n"
      "  if (!slice) return;\n"
      "  " << nm << "_fini_i(slice" << zeros << ");\n"
      "  ACE_Allocator::instance()->free(slice);\n"
      "}\n\n" <<
      nm << "_slice* " << nm << "_dup(const " << nm << "_slice* slice)\n"
      "{\n"
      "  " << nm << "_slice* const arr = " << nm << "_alloc();\n"
      "  if (arr) " << nm << "_copy(arr, slice);\n"
      "  return arr;\n"
      "}\n\n"
      "void " << nm << "_copy(" << nm << "_slice* dst, const " << nm
      << "_slice* src)\n"
      "{\n"
      "  if (!src || !dst) return;\n";

    {
      std::string indent = "  ";
      const NestedForLoops nfl("ACE_CDR::ULong", "i", arr, indent);
      if (elem_cls & CL_ARRAY) {
        be_global->impl_ <<
          indent << elem_type << "_copy(dst" << nfl.index_ << ", src"
          << nfl.index_ << ");\n";
      } else {
        be_global->impl_ <<
          indent << "dst" << nfl.index_ << " = src" << nfl.index_ << ";\n";
      }
    }

    be_global->impl_ <<
      "}\n\n";


    be_global->lang_header_ <<
      "inline ACE_CDR::Boolean operator<<(ACE_OutputCDR &, const " << nm << "_forany&) { return true; }\n\n"
      "inline ACE_CDR::Boolean operator>>(ACE_InputCDR &, " << nm << "_forany&) { return true; }\n\n";
  }
}

bool langmap_generator::gen_enum(AST_Enum*, UTL_ScopedName* name,
                                 const std::vector<AST_EnumVal*>& contents,
                                 const char*)
{
  const ScopedNamespaceGuard namespaces(name, be_global->lang_header_);
  const char* const nm = name->last_component()->get_string();
  be_global->lang_header_ <<
    "enum " << nm << " {\n";
  for (size_t i = 0; i < contents.size(); ++i) {
    be_global->lang_header_ <<
      "  " << contents[i]->local_name()->get_string()
      << ((i < contents.size() - 1) ? ",\n" : "\n");
  }
  be_global->lang_header_ <<
    "};\n\n"
    "typedef " << nm << "& " << nm << "_out;\n";
  gen_typecode(name);
  return true;
}

bool langmap_generator::gen_struct_fwd(UTL_ScopedName* name,
                                       AST_Type::SIZE_TYPE size)
{
  const ScopedNamespaceGuard namespaces(name, be_global->lang_header_);
  struct_decls(name, size);
  return true;
}

bool langmap_generator::gen_struct(AST_Structure* s, UTL_ScopedName* name,
                                   const std::vector<AST_Field*>& fields,
                                   AST_Type::SIZE_TYPE size,
                                   const char* x)
{
  return generator_->gen_struct(s, name, fields, size, x);
}

namespace {

  // Outside of user's namespace: add Traits for arrays so that they can be
  // used in Sequences and Array_var/_out/_forany.
  void gen_array_traits(UTL_ScopedName* tdname, AST_Array* arr)
  {
    const std::string nm = scoped(tdname);
    std::string zeros;
    for (ACE_CDR::ULong i = 1; i < arr->n_dims(); ++i) zeros += "[0]";
    be_global->lang_header_ <<
      "TAO_BEGIN_VERSIONED_NAMESPACE_DECL\nnamespace TAO {\n"
      "template <>\n"
      "struct " << exporter() << "Array_Traits<" << nm << "_forany>\n"
      "{\n"
      "  static void free(" << nm << "_slice* slice)\n"
      "  {\n"
      "    " << nm << "_free(slice);\n"
      "  }\n\n"
      "  static " << nm << "_slice* dup(const " << nm << "_slice* slice)\n"
      "  {\n"
      "    return " << nm << "_dup(slice);\n"
      "  }\n\n"
      "  static void copy(" << nm << "_slice* dst, const " << nm
      << "_slice* src)\n"
      "  {\n"
      "    " << nm << "_copy(dst, src);\n"
      "  }\n\n"
      "  static " << nm << "_slice* alloc()\n"
      "  {\n"
      "    return " << nm << "_alloc();\n"
      "  }\n\n"
      "  static void zero(" << nm << "_slice* slice)\n"
      "  {\n"
      "    " << nm << "_fini_i(slice" << zeros << ");\n"
      "    " << nm << "_init_i(slice" << zeros << ");\n"
      "  }\n"
      "  static void construct(" << nm << "_slice* slice)\n"
      "  {\n"
      "    " << nm << "_init_i(slice" << zeros << ");\n"
      "  }\n"
      "  static void destroy(" << nm << "_slice* slice)\n"
      "  {\n"
      "    " << nm << "_fini_i(slice" << zeros << ");\n"
      "  }\n"
      "};\n}\nTAO_END_VERSIONED_NAMESPACE_DECL\n\n";
  }

#ifdef ACE_HAS_CDR_FIXED
  void gen_fixed(UTL_ScopedName* name, AST_Fixed* fixed)
  {
    be_global->add_include("FACE/Fixed.h", BE_GlobalData::STREAM_LANG_H);
    const char* const nm = name->last_component()->get_string();
    be_global->lang_header_ <<
      "typedef " << helpers_[HLP_FIXED] << '<' << *fixed->digits()->ev()
      << ", " << *fixed->scale()->ev() << "> " << nm << ";\n"
      "typedef " << nm << "& " << nm << "_out;\n";
  }
#endif
}

bool langmap_generator::gen_typedef(AST_Typedef*, UTL_ScopedName* name, AST_Type* base,
                                    const char*)
{
  AST_Array* arr = 0;
  {
    const ScopedNamespaceGuard namespaces(name, be_global->lang_header_);
    const char* const nm = name->last_component()->get_string();

    const Classification cls = classify(base);

    switch (base->node_type()) {
    case AST_Decl::NT_sequence:
      generator_->gen_sequence(name, AST_Sequence::narrow_from_decl(base));
      break;
    case AST_Decl::NT_array:
      gen_array(name, arr = AST_Array::narrow_from_decl(base));
      break;
    case AST_Decl::NT_fixed:
# ifdef ACE_HAS_CDR_FIXED
      gen_fixed(name, AST_Fixed::narrow_from_decl(base));
      break;
# else
      std::cerr << "ERROR: fixed data type (for " << nm << ") is not supported"
        " with this version of ACE+TAO\n";
      return false;
# endif
    default:
      be_global->lang_header_ <<
        "typedef " << map_type(base) << ' ' << nm << ";\n";
      if ((cls & CL_STRING) == 0) {
          be_global->lang_header_ <<
            "typedef " << map_type(base) << "_out " << nm << "_out;\n";
        }

      AST_Type* actual_base = resolveActualType(base);
      if (actual_base->node_type() == AST_Decl::NT_array) {
        be_global->lang_header_ <<
          "typedef " << map_type(base) << "_var " << nm << "_var;\n" <<
          "typedef " << map_type(base) << "_slice " << nm << "_slice;\n" <<
          "typedef " << map_type(base) << "_forany " << nm << "_forany;\n\n" <<
          "inline " << nm << "_slice *" << nm << "_alloc() { return " << map_type(base) << "_alloc(); }\n" <<
          "inline " << nm << "_slice* " << nm << "_dup(" << nm << "_slice *a) { return " << map_type(base) << "_dup(a); }\n" <<
          "inline void " << nm << "_copy(" << nm << "_slice* to, const " << nm << "_slice* from) { " << map_type(base) << "_copy(to, from); }\n" <<
          "inline void " << nm << "_free(" << nm << "_slice *a) { " << map_type(base) << "_free(a); }\n";
      }

      break;
    }

    if (cls & CL_STRING) {
      const Helper var = (cls & CL_WIDE) ? HLP_WSTR_VAR : HLP_STR_VAR,
        out = (cls & CL_WIDE) ? HLP_WSTR_OUT : HLP_STR_OUT;
      be_global->lang_header_ <<
        "typedef " << helpers_[var] << ' ' << nm << "_var;\n"
        "typedef " << helpers_[out] << ' ' << nm << "_out;\n";
    }

    gen_typecode(name);
  }
  if (arr) gen_array_traits(name, arr);
  return true;
}

bool langmap_generator::gen_union_fwd(AST_UnionFwd* node,
                                      UTL_ScopedName* name,
                                      AST_Type::SIZE_TYPE)
{
  const ScopedNamespaceGuard namespaces(name, be_global->lang_header_);
  struct_decls(name, node->full_definition()->size_type());
  return true;
}

bool langmap_generator::gen_union(AST_Union* u, UTL_ScopedName* name,
                                  const std::vector<AST_UnionBranch*>& branches,
                                  AST_Type* discriminator,
                                  const char*)
{
  return generator_->gen_union(u, name, branches, discriminator);
}

bool langmap_generator::gen_interf_fwd(UTL_ScopedName* name)
{
  const ScopedNamespaceGuard namespaces(name, be_global->lang_header_);

  be_global->add_include("<tao/Objref_VarOut_T.h>", BE_GlobalData::STREAM_LANG_H);
  const char* const nm = name->last_component()->get_string();
  be_global->lang_header_ <<
    "class " << nm << ";\n"
    "typedef " << nm << '*' << nm << "_ptr;\n"
    "typedef TAO_Objref_Var_T<" << nm << "> " << nm << "_var;\n"
    "typedef TAO_Objref_Out_T<" << nm << "> " << nm << "_out;\n";

  return true;
}
