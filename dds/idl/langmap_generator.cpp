/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "langmap_generator.h"

#include "field_info.h"
#include "be_extern.h"

#include <dds/DCPS/Definitions.h>

#include <ast_fixed.h>
#include <utl_identifier.h>

#include <ace/CDR_Base.h>

#include <map>
#include <iostream>

using namespace AstTypeClassification;

struct GeneratorBase;

namespace {
  std::string string_ns = "::CORBA";

  GeneratorBase* generator_ = 0;

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

  std::string exporter()
  {
    return be_global->export_macro().empty() ? ""
      : be_global->export_macro().c_str() + std::string(" ");
  }

  std::string array_zero_indices(AST_Array* arr)
  {
    std::string indices;
    for (ACE_CDR::ULong i = 1; i < arr->n_dims(); ++i) {
      indices += "[0]";
    }
    return indices;
  }

  std::string array_dims(AST_Type* type, ACE_CDR::ULong& elems)
  {
    AST_Array* const arr = dynamic_cast<AST_Array*>(type);
    std::string ret = array_zero_indices(arr);
    elems *= array_element_count(arr);
    AST_Type* base = resolveActualType(arr->base_type());
    if (dynamic_cast<AST_Array*>(base)) {
      ret += "[0]" + array_dims(base, elems);
    }
    return ret;
  }

  void gen_typecode(UTL_ScopedName* name)
  {
    if (be_global->suppress_typecode() || be_global->language_mapping() == BE_GlobalData::LANGMAP_CXX11) {
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

struct GeneratorBase
{
  virtual ~GeneratorBase() {}
  virtual void init() = 0;
  virtual void gen_sequence(UTL_ScopedName* tdname, AST_Sequence* seq) = 0;
  virtual void gen_map(UTL_ScopedName* /*tdnam*/, AST_Map* /*map*/) {}
  virtual bool gen_struct(AST_Structure* s, UTL_ScopedName* name, const std::vector<AST_Field*>& fields, AST_Type::SIZE_TYPE size, const char* x) = 0;

  virtual std::string const_keyword(AST_Expression::ExprType)
  {
    return "const";
  }

  std::string map_type(AST_Type* type)
  {
    if (dynamic_cast<AST_Typedef*>(type)) {
      return scoped(type->name());
    }
    const Classification cls = classify(type);
    if (cls & CL_PRIMITIVE) {
      AST_Type* actual = resolveActualType(type);
      return primtype_[dynamic_cast<AST_PredefinedType*>(actual)->pt()];
    }
    if (cls & CL_STRING) {
      const AST_PredefinedType::PredefinedType chartype = (cls & CL_WIDE)
        ? AST_PredefinedType::PT_wchar : AST_PredefinedType::PT_char;
      return map_type_string(chartype, false);
    }
    if (cls & (CL_STRUCTURE | CL_UNION | CL_SEQUENCE | CL_MAP | CL_ARRAY | CL_ENUM | CL_FIXED)) {
      return scoped(type->name());
    }
    if (cls & CL_INTERFACE) {
      return scoped(type->name()) + "_var";
    }
    return "<<unknown>>";
  }

  std::string map_type(AST_Field* field)
  {
    FieldInfo af(*field);
    if (af.type_->anonymous() && af.as_base_) {
      return af.type_name_;
    }

#if OPENDDS_HAS_IDL_MAP
    if (af.map_) {
      return af.type_name_;
    }
#endif

    return map_type(af.type_);
  }

  virtual std::string map_type_string(AST_PredefinedType::PredefinedType chartype, bool constant)
  {
    return primtype_[chartype] + '*' + (constant ? " const" : "");
  }

  std::string map_type(AST_Expression::ExprType type)
  {
    AST_PredefinedType::PredefinedType pt = AST_PredefinedType::PT_void;
    switch (type) {
#if OPENDDS_HAS_EXPLICIT_INTS
    case AST_Expression::EV_int8: pt = AST_PredefinedType::PT_int8; break;
    case AST_Expression::EV_uint8: pt = AST_PredefinedType::PT_uint8; break;
#endif
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
    case AST_Expression::EV_fixed:
      be_global->add_include("FACE/Fixed.h", BE_GlobalData::STREAM_LANG_H);
      return helpers_[HLP_FIXED_CONSTANT];
    default:
      be_util::misc_error_and_abort("Unhandled ExprType value in map_type");
    }

    if (type == AST_Expression::EV_string || type == AST_Expression::EV_wstring)
      return map_type_string(pt, true);

    return primtype_[pt];
  }

  virtual void gen_simple_out(const char* nm)
  {
    be_global->lang_header_ <<
      "typedef " << nm << "& " << nm << "_out;\n";
  }

  virtual bool scoped_enum() { return false; }
  virtual std::string enum_base() { return ""; }

  virtual void struct_decls(UTL_ScopedName* name, AST_Type::SIZE_TYPE size, const char* struct_or_class = "struct")
  {
    be_global->add_include("<tao/VarOut_T.h>", BE_GlobalData::STREAM_LANG_H);
    const char* const nm = name->last_component()->get_string();
    be_global->lang_header_ <<
      struct_or_class << ' ' << nm << ";\n";
    switch (size) {
    case AST_Type::SIZE_UNKNOWN:
      be_global->lang_header_ << "/* Unknown size */\n";
      break;
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

  static std::string generateDefaultValue(AST_Union* the_union)
  {
    std::stringstream first_label;
    AST_Union::DefaultValue dv;
    if (the_union->default_value(dv) == -1) {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("(%P|%t) ERROR: generateDefaultValue::")
        ACE_TEXT(" computing default value failed\n")));
      return "";
    }

    switch (the_union->udisc_type ())
      {
#if OPENDDS_HAS_EXPLICIT_INTS
      case AST_Expression::EV_int8:
        first_label << signed(dv.u.char_val);
        break;
      case AST_Expression::EV_uint8:
        first_label << unsigned(dv.u.char_val);
        break;
#endif
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
          AST_Enum* e = dynamic_cast<AST_Enum*>(the_union->disc_type());
          if (be_global->language_mapping() == BE_GlobalData::LANGMAP_CXX11 ||
              be_global->language_mapping() == BE_GlobalData::LANGMAP_FACE_CXX) {
            std::string prefix = scoped(e->name());
            if (be_global->language_mapping() == BE_GlobalData::LANGMAP_FACE_CXX) {
              size_t pos = prefix.rfind("::");
              if (pos == std::string::npos) {
                prefix = "";
              } else {
                prefix = prefix.substr(0, pos) + "::";
              }
            } else {
              prefix += "::";
            }
            first_label << prefix;
            UTL_ScopedName* default_name;
            if (dv.u.enum_val < static_cast<ACE_CDR::ULong>(e->member_count())) {
              default_name = e->value_to_name(dv.u.enum_val);
            } else {
              const Fields fields(the_union);
              AST_UnionBranch* ub = dynamic_cast<AST_UnionBranch*>(*(fields.begin()));
              AST_Expression::AST_ExprValue* ev = ub->label(0)->label_val()->ev();
              default_name = e->value_to_name(ev->u.eval);
            }
            first_label << default_name->last_component()->get_string();
          } else {
            first_label << scoped(e->value_to_name(dv.u.enum_val));
          }
          break;
        }
      case AST_Expression::EV_longlong:
        first_label << dv.u.longlong_val;
        break;
      case AST_Expression::EV_ulonglong:
        first_label << dv.u.ulonglong_val;
        break;
      default:
        be_util::misc_error_and_abort("Unhandled ExprType value in generateDefaultValue");
      }

    return first_label.str();
  }

  struct GenerateUnionAccessors
  {
    AST_Union* the_union;
    AST_Type* discriminator;

    GenerateUnionAccessors(AST_Union* u, AST_Type* d)
      : the_union(u)
      , discriminator(d)
    { }

    void operator()(AST_UnionBranch* branch)
    {
      const char* field_name = branch->local_name()->get_string();
      std::stringstream first_label;
      {
        AST_UnionLabel* label = branch->label(0);
        if (label->label_kind() == AST_UnionLabel::UL_default) {
          first_label << generateDefaultValue(the_union);
        } else if (discriminator->node_type() == AST_Decl::NT_enum) {
          first_label << getEnumLabel(label->label_val(), discriminator);
        } else {
          first_label << *label->label_val()->ev();
        }
      }

      AST_Type* field_type = branch->field_type();
      const std::string field_type_string = generator_->map_type(field_type);
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
          "    this->_u." << field_name << " = " << string_ns << "::string_dup(x);\n"
          "    _discriminator = " << first_label.str() << ";\n"
          "  }\n"
          "  void " << field_name << " (const " << helper << "& x) {\n"
          "    _reset();\n" <<
          "    this->_u." << field_name << " = " << string_ns << "::string_dup(x.in());\n"
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
        idl_global->err()->misc_warning("Unsupported type for union element", field_type);
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

  static void generate_union_field(AST_UnionBranch* branch)
  {
    AST_Type* field_type = branch->field_type();
    AST_Type* actual_field_type = resolveActualType(field_type);
    const Classification cls = classify(actual_field_type);
    const std::string lang_field_type = generator_->map_type(field_type);
    if (cls & (CL_PRIMITIVE | CL_ENUM)) {
      be_global->lang_header_ <<
        "    " << lang_field_type << ' ' << branch->local_name()->get_string() << ";\n";
    } else if (cls & CL_STRING) {
      const AST_PredefinedType::PredefinedType chartype = (cls & CL_WIDE)
        ? AST_PredefinedType::PT_wchar : AST_PredefinedType::PT_char;
      be_global->lang_header_ <<
        "    " << primtype_[chartype] << "* " << branch->local_name()->get_string() << ";\n";
    } else if (cls & CL_ARRAY) {
      be_global->lang_header_ <<
        "    " << lang_field_type << "_slice* " << branch->local_name()->get_string() << ";\n";
    } else if (cls & (CL_STRUCTURE | CL_UNION | CL_SEQUENCE | CL_FIXED)) {
      be_global->lang_header_ <<
        "    " << lang_field_type << "* " << branch->local_name()->get_string() << ";\n";
    } else {
      idl_global->err()->misc_warning("Unsupported type for union element", field_type);
    }
  }

  static std::string generateCopyCtor(const std::string&, AST_Decl*, const std::string& name, AST_Type* field_type,
                                      const std::string&, bool, Intro&,
                                      const std::string&)
  {
    std::stringstream ss;
    AST_Type* actual_field_type = resolveActualType(field_type);
    const Classification cls = classify(actual_field_type);
    const std::string lang_field_type = generator_->map_type(field_type);
    if (cls & (CL_PRIMITIVE | CL_ENUM)) {
      ss <<
        "    this->_u." << name << " = other._u." << name << ";\n";
    } else if (cls & CL_STRING) {
      ss <<
        "    this->_u." << name << " = (other._u." << name << ") ? " << string_ns << "::string_dup(other._u." << name << ") : 0 ;\n";
    } else if (cls & CL_ARRAY) {
      ss <<
        "    this->_u." << name << " = (other._u." << name << ") ? " << lang_field_type << "_dup(other._u." << name << ") : 0 ;\n";
    } else if (cls & (CL_STRUCTURE | CL_UNION | CL_SEQUENCE | CL_FIXED)) {
      ss <<
        "    this->_u." << name << " = (other._u." << name << ") ? new " << lang_field_type << "(*other._u." << name << ") : 0;\n";
    } else {
      idl_global->err()->misc_warning("Unsupported type for union element", field_type);
    }

    return ss.str();
  }

  static std::string generateAssign(const std::string&, AST_Decl*, const std::string& name, AST_Type* field_type,
                                    const std::string&, bool, Intro&,
                                    const std::string&)
  {
    std::stringstream ss;
    AST_Type* actual_field_type = resolveActualType(field_type);
    const Classification cls = classify(actual_field_type);
    const std::string lang_field_type = generator_->map_type(field_type);
    if (cls & (CL_PRIMITIVE | CL_ENUM)) {
      ss <<
        "    this->_u." << name << " = other._u." << name << ";\n";
    } else if (cls & CL_STRING) {
      ss <<
        "    this->_u." << name << " = (other._u." << name << ") ? " << string_ns << "::string_dup(other._u." << name << ") : 0 ;\n";
    } else if (cls & CL_ARRAY) {
      ss <<
        "    this->_u." << name << " = (other._u." << name << ") ? " << lang_field_type << "_dup(other._u." << name << ") : 0 ;\n";
    } else if (cls & (CL_STRUCTURE | CL_UNION | CL_SEQUENCE | CL_FIXED)) {
      ss <<
        "    this->_u." << name << " = (other._u." << name << ") ? new " << lang_field_type << "(*other._u." << name << ") : 0;\n";
    } else {
      idl_global->err()->misc_warning("Unsupported type for union element", field_type);
    }

    return ss.str();
  }

  static std::string generateEqual(const std::string&, AST_Decl*, const std::string& name, AST_Type* field_type,
                                   const std::string&, bool, Intro&,
                                   const std::string&)
  {
    std::stringstream ss;

    AST_Type* actual_field_type = resolveActualType(field_type);
    const Classification cls = classify(actual_field_type);
    if (cls & (CL_PRIMITIVE | CL_ENUM)) {
      ss <<
        "    return this->_u." << name << " == rhs._u." << name << ";\n";
    } else if (cls & CL_STRING) {
      ss <<
        "    return std::strcmp (this->_u." << name << ", rhs._u." << name << ") == 0 ;\n";
    } else if (cls & CL_ARRAY) {
      // TODO
      ss <<
        "    return false;\n";
    } else if (cls & (CL_STRUCTURE | CL_UNION | CL_SEQUENCE | CL_FIXED)) {
      ss <<
        "    return *this->_u." << name << " == *rhs._u." << name << ";\n";
    } else {
      idl_global->err()->misc_warning("Unsupported type for union element", field_type);
    }

    return ss.str();
  }

  static std::string generateEqualCxx11(const std::string&, AST_Decl*, const std::string& name, AST_Type* field_type,
                                        const std::string&, bool, Intro&,
                                        const std::string&)
  {
    std::stringstream ss;

    AST_Type* actual_field_type = resolveActualType(field_type);
    const Classification cls = classify(actual_field_type);
    if (cls & (CL_PRIMITIVE | CL_ENUM | CL_STRING | CL_ARRAY | CL_STRUCTURE | CL_UNION | CL_SEQUENCE | CL_FIXED)) {
      ss <<
        "    return this->_" << name << " == rhs._" << name << ";\n";
    } else {
      idl_global->err()->misc_warning("Unsupported type for union element", field_type);
    }

    return ss.str();
  }

  static std::string generateReset(const std::string&, AST_Decl*, const std::string& name, AST_Type* field_type,
                                   const std::string&, bool, Intro&,
                                   const std::string&)
  {
    std::stringstream ss;

    AST_Type* actual_field_type = resolveActualType(field_type);
    const Classification cls = classify(actual_field_type);
    if (cls & (CL_PRIMITIVE | CL_ENUM)) {
      // Do nothing.
    } else if (cls & CL_STRING) {
      ss <<
        "    " << string_ns << "::string_free(this->_u." << name << ");\n"
        "    this->_u." << name << " = 0;\n";
    } else if (cls & CL_ARRAY) {
      ss <<
        "    " << generator_->map_type(field_type) << "_free(this->_u." << name << ");\n"
        "    this->_u." << name << " = 0;\n";
    } else if (cls & (CL_STRUCTURE | CL_UNION | CL_SEQUENCE | CL_FIXED)) {
      ss <<
        "    delete this->_u." << name << ";\n"
        "    this->_u." << name << " = 0;\n";
    } else {
      idl_global->err()->misc_warning("Unsupported type for union element", field_type);
    }

    return ss.str();
  }

  virtual bool gen_union(AST_Union* u, UTL_ScopedName* name,
                         const std::vector<AST_UnionBranch*>& branches, AST_Type* discriminator)
  {
    const ScopedNamespaceGuard namespaces(name, be_global->lang_header_);
    const char* const nm = name->last_component()->get_string();
    struct_decls(name, u->size_type(), "class");
    be_global->lang_header_ <<
      "\n"
      "class " << exporter() << nm << "\n"
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

    std::for_each (branches.begin(), branches.end(), GenerateUnionAccessors(u, discriminator));

    if (needsDefault(branches, discriminator)) {
      be_global->lang_header_ <<
        "  void _default() {\n"
        "    _reset();\n"
        "    _discriminator = " << generateDefaultValue(u) << ";\n"
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

    be_global->add_include("dds/DCPS/PoolAllocationBase.h", BE_GlobalData::STREAM_LANG_H);
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
        nm << "::" << nm << "(const " << nm << "& other)\n"
        "{\n"
        "  this->_discriminator = other._discriminator;\n";
      generateSwitchForUnion(u, "this->_discriminator", generateCopyCtor, branches, discriminator, "", "", "", false, false);
      be_global->impl_ <<
        "}\n\n";

      be_global->impl_ <<
        nm << "& " << nm << "::operator=(const " << nm << "& other)\n"
        "{\n" <<
        "  if (this == &other) {\n"
        "    return *this;\n"
        "  }\n\n"
        "  _reset();\n"
        "  this->_discriminator = other._discriminator;\n";
      generateSwitchForUnion(u, "this->_discriminator", generateAssign, branches, discriminator, "", "", "", false, false);
      be_global->impl_ <<
        "  return *this;\n"
        "}\n\n";

      be_global->impl_ <<
        "bool " << nm << "::operator==(const " << nm << "& rhs) const\n"
        "{\n"
        "  if (this->_discriminator != rhs._discriminator) return false;\n";
      if (generateSwitchForUnion(u, "this->_discriminator", generateEqual, branches, discriminator, "", "", "", false, false)) {
        be_global->impl_ <<
          "  return false;\n";
      }
      be_global->impl_ <<
        "}\n\n";

      be_global->impl_ <<
        "void " << nm << "::_reset()\n"
        "{\n";
      generateSwitchForUnion(u, "this->_discriminator", generateReset, branches, discriminator, "", "", "", false, false);
      be_global->impl_ <<
        "}\n\n";

      be_global->impl_ <<
        "ACE_CDR::Boolean operator<<(ACE_OutputCDR&, const " << nm << "&) { return true; }\n\n";
      be_global->impl_ <<
        "ACE_CDR::Boolean operator>>(ACE_InputCDR&, " << nm << "&) { return true; }\n\n";
    }

    gen_typecode(name);
    return true;
  }

  virtual void gen_array(UTL_ScopedName* tdname, AST_Array* arr)
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
    const std::string zeros = array_zero_indices(arr);
    for (ACE_CDR::ULong dim = 0; dim < arr->n_dims(); ++dim) {
      const ACE_CDR::ULong extent = arr->dims()[dim]->ev()->u.ulval;
      bound << '[' << extent << ']';
      if (dim) {
        nofirst << '[' << extent << ']';
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

  // Outside of user's namespace: add Traits for arrays so that they can be
  // used in Sequences and Array_var/_out/_forany.
  virtual void gen_array_traits(UTL_ScopedName* tdname, AST_Array* arr)
  {
    const std::string nm = scoped(tdname);
    const std::string zeros = array_zero_indices(arr);
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

  virtual void gen_array_typedef(const char* nm, AST_Type* base)
  {
    be_global->lang_header_ <<
      "typedef " << map_type(base) << "_var " << nm << "_var;\n" <<
      "typedef " << map_type(base) << "_slice " << nm << "_slice;\n" <<
      "typedef " << map_type(base) << "_forany " << nm << "_forany;\n\n" <<
      "inline " << nm << "_slice *" << nm << "_alloc() { return " << map_type(base) << "_alloc(); }\n" <<
      "inline " << nm << "_slice* " << nm << "_dup(" << nm << "_slice *a) { return " << map_type(base) << "_dup(a); }\n" <<
      "inline void " << nm << "_copy(" << nm << "_slice* to, const " << nm << "_slice* from) { " << map_type(base) << "_copy(to, from); }\n" <<
      "inline void " << nm << "_free(" << nm << "_slice *a) { " << map_type(base) << "_free(a); }\n";
  }

  virtual void gen_typedef_varout(const char* nm, AST_Type* base)
  {
    const Classification cls = classify(base);
    if (cls & CL_STRING) {
      const Helper var = (cls & CL_WIDE) ? HLP_WSTR_VAR : HLP_STR_VAR,
        out = (cls & CL_WIDE) ? HLP_WSTR_OUT : HLP_STR_OUT;
      be_global->lang_header_ <<
        "typedef " << helpers_[var] << ' ' << nm << "_var;\n"
        "typedef " << helpers_[out] << ' ' << nm << "_out;\n";
    } else {
      be_global->lang_header_ <<
        "typedef " << generator_->map_type(base) << "_out " << nm << "_out;\n";
    }
  }
};

struct FaceGenerator : GeneratorBase
{
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

    be_global->add_include("dds/DCPS/PoolAllocationBase.h", BE_GlobalData::STREAM_LANG_H);
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
            dynamic_cast<AST_Array*>(field_type), indent, true);
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

struct SafetyProfileGenerator : GeneratorBase
{
  virtual void init()
  {
    be_global->add_include("tao/String_Manager_T.h", BE_GlobalData::STREAM_LANG_H);
    be_global->add_include("tao/CORBA_String.h", BE_GlobalData::STREAM_LANG_H);
    primtype_[AST_PredefinedType::PT_long] = "CORBA::Long";
    primtype_[AST_PredefinedType::PT_ulong] = "CORBA::ULong";
    primtype_[AST_PredefinedType::PT_longlong] = "CORBA::LongLong";
    primtype_[AST_PredefinedType::PT_ulonglong] = "CORBA::ULongLong";
    primtype_[AST_PredefinedType::PT_short] = "CORBA::Short";
    primtype_[AST_PredefinedType::PT_ushort] = "CORBA::UShort";
#if OPENDDS_HAS_EXPLICIT_INTS
    primtype_[AST_PredefinedType::PT_int8] = "CORBA::Int8";
    primtype_[AST_PredefinedType::PT_uint8] = "CORBA::UInt8";
#endif
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

    be_global->add_include("dds/DCPS/PoolAllocationBase.h", BE_GlobalData::STREAM_LANG_H);
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
            dynamic_cast<AST_Array*>(field_type), indent, true);
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

struct Cxx11Generator : GeneratorBase
{
  void init()
  {
    be_global->add_include("<cstdint>", BE_GlobalData::STREAM_LANG_H);
    be_global->add_include("<string>", BE_GlobalData::STREAM_LANG_H);
    primtype_[AST_PredefinedType::PT_long] = "int32_t";
    primtype_[AST_PredefinedType::PT_ulong] = "uint32_t";
    primtype_[AST_PredefinedType::PT_longlong] = "int64_t";
    primtype_[AST_PredefinedType::PT_ulonglong] = "uint64_t";
    primtype_[AST_PredefinedType::PT_short] = "int16_t";
    primtype_[AST_PredefinedType::PT_ushort] = "uint16_t";
#if OPENDDS_HAS_EXPLICIT_INTS
    primtype_[AST_PredefinedType::PT_int8] = "int8_t";
    primtype_[AST_PredefinedType::PT_uint8] = "uint8_t";
#endif
    primtype_[AST_PredefinedType::PT_float] = "float";
    primtype_[AST_PredefinedType::PT_double] = "double";
    primtype_[AST_PredefinedType::PT_longdouble] = "long double";
    primtype_[AST_PredefinedType::PT_char] = "char";
    primtype_[AST_PredefinedType::PT_wchar] = "wchar_t";
    primtype_[AST_PredefinedType::PT_boolean] = "bool";
    primtype_[AST_PredefinedType::PT_octet] = "uint8_t";
    helpers_[HLP_STR_VAR] = "std::string";
    helpers_[HLP_STR_OUT] = "std::string";
    helpers_[HLP_WSTR_VAR] = "std::wstring";
    helpers_[HLP_WSTR_OUT] = "std::wstring";
    helpers_[HLP_STR_MGR] = "std::string";
    helpers_[HLP_WSTR_MGR] = "std::wstring";
    helpers_[HLP_FIX_VAR] = "<<fixed-size var>>";
    helpers_[HLP_VAR_VAR] = "<<variable-size var>>";
    helpers_[HLP_OUT] = "<<out>>";
    helpers_[HLP_SEQ] = "std::vector";
    helpers_[HLP_SEQ_NS] = "std";
    helpers_[HLP_SEQ_VAR_VAR] = "<<variable sequence var>>";
    helpers_[HLP_SEQ_FIX_VAR] = "<<fixed sequence var>>";
    helpers_[HLP_SEQ_OUT] = "<<sequence out>>";
    helpers_[HLP_ARR_VAR_VAR] = "<<variable array var>>";
    helpers_[HLP_ARR_FIX_VAR] = "<<fixed array var>>";
    helpers_[HLP_ARR_OUT] = "<<array out>>";
    helpers_[HLP_ARR_FORANY] = "<<array forany>>";
    helpers_[HLP_FIXED] = "IDL::Fixed_T";
    helpers_[HLP_FIXED_CONSTANT] = "IDL::Fixed_T";
  }

  static void gen_typecode_ptrs(const std::string& type)
  {
    if (!be_global->suppress_typecode()) {
      be_global->add_include("tao/Basic_Types.h", BE_GlobalData::STREAM_LANG_H);
      be_global->lang_header_ << "extern const ::CORBA::TypeCode_ptr _tc_" << type << ";\n";
      be_global->impl_ << "const ::CORBA::TypeCode_ptr _tc_" << type << " = nullptr;\n";
    }
  }

  std::string map_type_string(AST_PredefinedType::PredefinedType chartype, bool)
  {
    return chartype == AST_PredefinedType::PT_char ? "std::string" : "std::wstring";
  }

  std::string const_keyword(AST_Expression::ExprType type)
  {
    switch (type) {
    case AST_Expression::EV_string:
    case AST_Expression::EV_wstring:
      return "const";
    default:
      return "constexpr";
    }
  }

  void gen_simple_out(const char*) {}

  bool scoped_enum() { return true; }
  std::string enum_base() { return " : uint32_t"; }

  void gen_union_pragma_pre()
  {
    // Older versions of gcc will complain because it appears that a primitive
    // default constructor is not called for anonymous unions.
    be_global->lang_header_ <<
      "#if defined(__GNUC__) && !defined(__clang__)\n"
      "#  pragma GCC diagnostic push\n"
      "#  pragma GCC diagnostic ignored \"-Wmaybe-uninitialized\"\n"
      "#endif\n";
  }

  void gen_union_pragma_post()
  {
    be_global->lang_header_ <<
      "#if defined(__GNUC__) && !defined(__clang__)\n"
      "#  pragma GCC diagnostic pop\n"
      "#endif\n\n";
  }

  void struct_decls(UTL_ScopedName* name, AST_Type::SIZE_TYPE, const char*)
  {
    be_global->lang_header_ <<
      "class " << name->last_component()->get_string() << ";\n";
  }

  static void gen_array(AST_Array* arr, const std::string& type, const std::string& elem, const std::string& ind = "")
  {
    std::string array;
    std::ostringstream bounds;
    for (ACE_CDR::ULong dim = arr->n_dims(); dim; --dim) {
      array += "std::array<";
      bounds << ", " << arr->dims()[dim - 1]->ev()->u.ulval << '>';
    }
    be_global->add_include("<array>", BE_GlobalData::STREAM_LANG_H);
    be_global->lang_header_ << ind << "using " << type << " = " << array << elem << bounds.str() << ";\n";
  }

  void gen_array(UTL_ScopedName* tdname, AST_Array* arr)
  {
    gen_array(arr, tdname->last_component()->get_string(), map_type(arr->base_type()));
  }

  void gen_array_traits(UTL_ScopedName*, AST_Array*) {}
  void gen_array_typedef(const char*, AST_Type*) {}
  void gen_typedef_varout(const char*, AST_Type*) {}

  static void gen_sequence(const std::string& type, const std::string& elem,  const std::string& ind = "")
  {
    be_global->add_include("<vector>", BE_GlobalData::STREAM_LANG_H);
    be_global->lang_header_ << ind << "using " << type << " = std::vector<" << elem << ">;\n";
  }

  void gen_sequence(UTL_ScopedName* tdname, AST_Sequence* seq)
  {
    gen_sequence(tdname->last_component()->get_string(), map_type(seq->base_type()));
  }

  static void gen_map(const std::string& type, const std::string& key, const std::string& val, const std::string& ind = "")
  {
    be_global->add_include("<map>", BE_GlobalData::STREAM_LANG_H);
    be_global->lang_header_ << ind << "using " << type << " = std::map<" << key << "," << val << ">;\n";
  }

#if OPENDDS_HAS_IDL_MAP
  void gen_map(UTL_ScopedName* tdname, AST_Map* map)
  {
    gen_map(tdname->last_component()->get_string(), map_type(map->key_type()), map_type(map->value_type()));
  }
#endif

  static void gen_common_strunion_pre(const char* nm)
  {
    be_global->lang_header_ <<
      "\n"
      "class " << exporter() << nm << "\n"
      "{\n"
      "public:\n\n";
  }

  static void gen_common_strunion_post(const char* nm)
  {
    be_global->lang_header_ <<
      "};\n\n"
      "using " << nm << "_out = " << nm << "&; // for tao_idl compatibility\n\n"
      << exporter() << "void swap(" << nm << "& lhs, " << nm << "& rhs);\n\n";
  }

  static void gen_struct_members(AST_Field* field)
  {
    FieldInfo af(*field);
    if (af.type_->anonymous() && af.as_base_) {
      const std::string elem_type = generator_->map_type(af.as_base_);
      if (af.arr_) {
        gen_array(af.arr_, af.type_name_, elem_type, "  ");
      } else if (af.seq_) {
        gen_sequence(af.type_name_, elem_type, "  ");
      }
    }

#if OPENDDS_HAS_IDL_MAP
    if (af.map_) {
      const std::string key_type = generator_->map_type(af.map_->key_type());
      const std::string value_type = generator_->map_type(af.map_->value_type());
      gen_map(af.type_name_, key_type, value_type, "  ");
    }
#endif

    const std::string lang_field_type = generator_->map_type(field);
    const std::string assign_pre = "{ _" + af.name_ + " = ",
      assign = assign_pre + "val; }\n",
      move = assign_pre + "std::move(val); }\n",
      ret = "{ return _" + af.name_ + "; }\n";
    std::string initializer;
    if (af.cls_ & (CL_PRIMITIVE | CL_ENUM)) {
      be_global->lang_header_ <<
        "  void " << af.name_ << '(' << lang_field_type << " val) " << assign <<
        "  " << lang_field_type << ' ' << af.name_ << "() const " << ret <<
        "  " << lang_field_type << "& " << af.name_ << "() " << ret;
      if (af.cls_ & CL_ENUM) {
        AST_Enum* enu = dynamic_cast<AST_Enum*>(af.act_);
        for (UTL_ScopeActiveIterator it(enu, UTL_Scope::IK_decls); !it.is_done(); it.next()) {
          if (it.item()->node_type() == AST_Decl::NT_enum_val) {
            initializer = '{' + generator_->map_type(af.type_)
              + "::" + it.item()->local_name()->get_string() + '}';
            break;
          }
        }
      } else {
        initializer = "{}";
      }
    } else {
      if (af.cls_ & CL_ARRAY) {
        initializer = "{}";
      }
      be_global->add_include("<utility>", BE_GlobalData::STREAM_LANG_H);
      be_global->lang_header_ <<
        "  void " << af.name_ << "(const " << lang_field_type << "& val) " << assign <<
        "  void " << af.name_ << '(' << lang_field_type << "&& val) " << move <<
        "  const " << lang_field_type << "& " << af.name_ << "() const " << ret <<
        "  " << lang_field_type << "& " << af.name_ << "() " << ret;
    }
    be_global->lang_header_ <<
      "  " << lang_field_type << " _" << af.name_ << initializer << ";\n\n";
  }

  bool gen_struct(AST_Structure*, UTL_ScopedName* name,
                  const std::vector<AST_Field*>& fields,
                  AST_Type::SIZE_TYPE, const char*)
  {
    const ScopedNamespaceGuard namespaces(name, be_global->lang_header_);
    const ScopedNamespaceGuard namespaces2(name, be_global->impl_);
    const char* const nm = name->last_component()->get_string();
    gen_common_strunion_pre(nm);

    std::for_each(fields.begin(), fields.end(), gen_struct_members);

    be_global->lang_header_ <<
      "  " << nm << "() = default;\n"
      "  " << (fields.size() == 1 ? "explicit " : "") << nm << '(';
    be_global->impl_ <<
      nm << "::" << nm << '(';

    std::string init_list, swaps;
    for (size_t i = 0; i < fields.size(); ++i) {
      const std::string fn = fields[i]->local_name()->get_string();
      const std::string ft = map_type(fields[i]);
      const Classification cls = classify(fields[i]->field_type());
      const bool by_ref = (cls & (CL_PRIMITIVE | CL_ENUM)) == 0;
      const std::string param = (by_ref ? "const " : "") + ft + (by_ref ? "&" : "")
        + ' ' + fn + (i < fields.size() - 1 ? ",\n    " : ")");
      be_global->lang_header_ << param;
      be_global->impl_ << param;
      init_list += '_' + fn + '(' + fn + ')';
      if (i < fields.size() - 1) init_list += "\n  , ";
      swaps += "  swap(lhs._" + fn + ", rhs._" + fn + ");\n";
    }
    be_global->lang_header_ << ";\n\n";
    be_global->impl_ << "\n  : " << init_list << "\n{}\n\n";

    if (be_global->generate_equality()) {
      be_global->lang_header_ << "\n"
        "  bool operator==(const " << nm << "& rhs) const;\n"
        "  bool operator!=(const " << nm << "& rhs) const { return !(*this == rhs); }\n";

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
                             dynamic_cast<AST_Array*>(field_type), indent, true);
          be_global->impl_ <<
            indent << "if (_" << field_name << nfl.index_ << " != rhs._"
                   << field_name << nfl.index_ << ") {\n" <<
            indent << "  return false;\n" <<
            indent << "}\n";
        } else {
          be_global->impl_ <<
            "  if (_" << field_name << " != rhs._" << field_name << ") {\n"
            "    return false;\n"
            "  }\n";
        }
      }
      be_global->impl_ << "  return true;\n}\n\n";
    }

    gen_common_strunion_post(nm);
    be_global->impl_ <<
      "void swap(" << nm << "& lhs, " << nm << "& rhs)\n"
      "{\n"
      "  using std::swap;\n"
      << swaps << "}\n\n";

    gen_typecode_ptrs(nm);
    return true;
  }

  static void union_field(AST_UnionBranch* branch)
  {
    AST_Type* field_type = branch->field_type();
    const std::string lang_field_type = generator_->map_type(field_type);
    be_global->lang_header_ <<
      "    " << lang_field_type << " _" << branch->local_name()->get_string()
      << ";\n";
  }

  static void union_accessors(AST_UnionBranch* branch)
  {
    AST_Type* field_type = branch->field_type();
    AST_Type* actual_field_type = resolveActualType(field_type);
    const std::string lang_field_type = generator_->map_type(field_type);
    const Classification cls = classify(actual_field_type);
    const char* nm = branch->local_name()->get_string();

    AST_UnionLabel* label = branch->label(0);
    AST_Union* union_ = dynamic_cast<AST_Union*>(branch->defined_in());
    AST_Type* dtype = resolveActualType(union_->disc_type());
    const std::string disc_type = generator_->map_type(dtype);

    std::string dval;
    if (label->label_kind() == AST_UnionLabel::UL_default) {
      dval = generateDefaultValue(union_);
    } else if (dtype->node_type() == AST_Decl::NT_enum) {
      dval = getEnumLabel(label->label_val(), dtype);
    } else {
      std::ostringstream strm;
      strm << *label->label_val()->ev();
      dval = strm.str();
    }

    std::string disc_param, disc_name = dval;
    if (label->label_kind() == AST_UnionLabel::UL_default ||
        branch->label_list_length() > 1) {
      disc_name = "disc";
      disc_param = ", " + disc_type + " disc = " + dval;
    }

    const std::string assign_pre = "{ _activate(" + disc_name + "); _"
      + std::string(nm) + " = ",
      assign = assign_pre + "val; }\n",
      move = assign_pre + "std::move(val); }\n",
      ret = "{ return _" + std::string(nm) + "; }\n";
    if (cls & (CL_PRIMITIVE | CL_ENUM)) {
      be_global->lang_header_ <<
        "  void " << nm << '(' << lang_field_type << " val" << disc_param
        << ") " << assign <<
        "  " << lang_field_type << ' ' << nm << "() const " << ret <<
        "  " << lang_field_type << "& " << nm << "() " << ret << "\n";
    } else {
      be_global->add_include("<utility>", BE_GlobalData::STREAM_LANG_H);
      be_global->lang_header_ <<
        "  void " << nm << "(const " << lang_field_type << "& val" << disc_param
        << ") " << assign <<
        "  void " << nm << '(' << lang_field_type << "&& val" << disc_param
        << ") " << move <<
        "  const " << lang_field_type << "& " << nm << "() const " << ret <<
        "  " << lang_field_type << "& " << nm << "() " << ret << "\n";
    }
  }

  static std::string union_copy(const std::string&, AST_Decl*, const std::string& name, AST_Type*,
                                const std::string&, bool, Intro&,
                                const std::string&)
  {
    return "    _" + name + " = rhs._" + name + ";\n";
  }

  static std::string union_move(const std::string&, AST_Decl*, const std::string& name, AST_Type*,
                                const std::string&, bool, Intro&,
                                const std::string&)
  {
    return "    _" + name + " = std::move(rhs._" + name + ");\n";
  }

  static std::string union_assign(const std::string&, AST_Decl*, const std::string& name, AST_Type*,
                                  const std::string&, bool, Intro&,
                                  const std::string&)
  {
    return "    " + name + "(rhs._" + name + ");\n";
  }

  static std::string union_move_assign(const std::string&, AST_Decl*, const std::string& name, AST_Type*,
                                       const std::string&, bool, Intro&,
                                       const std::string&)
  {
    return "    " + name + "(std::move(rhs._" + name + "));\n";
  }

  static std::string union_activate(const std::string&, AST_Decl*, const std::string& name, AST_Type* type,
                                    const std::string&, bool, Intro&,
                                    const std::string&)
  {
    AST_Type* actual_field_type = resolveActualType(type);
    const std::string lang_field_type = generator_->map_type(type);
    const Classification cls = classify(actual_field_type);
    if (!(cls & (CL_PRIMITIVE | CL_ENUM))) {
      return "    new(&_" + name + ") " + lang_field_type + ";\n";
    }
    return "";
  }

  static std::string union_reset(const std::string&, AST_Decl*, const std::string& name, AST_Type* type,
                                 const std::string&, bool, Intro&,
                                 const std::string&)
  {
    AST_Type* actual_field_type = resolveActualType(type);
    const std::string lang_field_type = generator_->map_type(type);
    const Classification cls = classify(actual_field_type);
    if (cls & CL_STRING) {
      return "    _" + name + ".~basic_string();\n";
    } else if (!(cls & (CL_PRIMITIVE | CL_ENUM))) {
      const size_t idx = lang_field_type.rfind("::");
      const std::string dtor_name = (idx == std::string::npos) ? lang_field_type
        : lang_field_type.substr(idx + 2);
      return "    _" + name + ".~" + dtor_name + "();\n";
    }
    return "";
  }

  bool gen_union(AST_Union* u, UTL_ScopedName* name,
                 const std::vector<AST_UnionBranch*>& branches, AST_Type* discriminator)
  {
    const ScopedNamespaceGuard namespaces(name, be_global->lang_header_);
    const ScopedNamespaceGuard namespacesCpp(name, be_global->impl_);
    const char* const nm = name->last_component()->get_string();
    const std::string d_type = generator_->map_type(discriminator);
    const std::string defVal = generateDefaultValue(u);

    gen_union_pragma_pre();
    gen_common_strunion_pre(nm);

    be_global->lang_header_ <<
      "  " << nm << "() { _activate(" << defVal << "); }\n"
      "  " << nm << "(const " << nm << "& rhs);\n"
      "  " << nm << "(" << nm << "&& rhs);\n"
      "  " << nm << "& operator=(const " << nm << "& rhs);\n"
      "  " << nm << "& operator=(" << nm << "&& rhs);\n"
      "  ~" << nm << "() { _reset(); }\n\n"
      "  " << d_type << " _d() const { return _disc; }\n"
      "  void _d(" << d_type << " d) { _disc = d; }\n\n";

    std::for_each(branches.begin(), branches.end(), union_accessors);
    if (needsDefault(branches, discriminator)) {
      be_global->lang_header_ <<
        "  void _default() { _reset(); _activate(" << defVal << "); }\n\n";
    }

    if (be_global->generate_equality()) {
      be_global->lang_header_ << "\n"
        "  bool operator==(const " << nm << "& rhs) const;\n"
        "  bool operator!=(const " << nm << "& rhs) const { return !(*this == rhs); }\n";

      be_global->impl_ <<
        "bool " << nm << "::operator==(const " << nm << "& rhs) const\n"
        "{\n"
        "  if (this->_disc != rhs._disc) return false;\n";
      if (generateSwitchForUnion(u, "this->_disc", generateEqualCxx11, branches, discriminator, "", "", "", false, false)) {
        be_global->impl_ <<
          "  return false;\n";
      }
      be_global->impl_ <<
        "}\n\n";
    }

    be_global->lang_header_ <<
      "private:\n"
      "  bool _set = false;\n"
      "  " << d_type << " _disc;\n\n"
      "  union {\n";

    std::for_each(branches.begin(), branches.end(), union_field);

    be_global->lang_header_ <<
      "  };\n\n"
      "  void _activate(" << d_type << " d);\n"
      "  void _reset();\n";

    gen_common_strunion_post(nm);
    gen_union_pragma_post();

    be_global->impl_ <<
      nm << "::" << nm << "(const " << nm << "& rhs)\n"
      "{\n"
      "  _activate(rhs._disc);\n";
    generateSwitchForUnion(u, "_disc", union_copy, branches, discriminator, "", "", "", false, false);
    be_global->impl_ <<
      "}\n\n" <<
      nm << "::" << nm << '(' << nm << "&& rhs)\n"
      "{\n"
      "  _activate(rhs._disc);\n";
    generateSwitchForUnion(u, "_disc", union_move, branches, discriminator, "", "", "", false, false);
    be_global->impl_ <<
      "}\n\n" <<
      nm << "& " << nm << "::operator=(const " << nm << "& rhs)\n"
      "{\n"
      "  if (this == &rhs) {\n"
      "    return *this;\n"
      "  }\n";
    generateSwitchForUnion(u, "rhs._disc", union_assign, branches, discriminator, "", "", "", false, false);
    be_global->impl_ <<
      "  _disc = rhs._disc;\n"
      "  return *this;\n"
      "}\n\n" <<
      nm << "& " << nm << "::operator=(" << nm << "&& rhs)\n"
      "{\n"
      "  if (this == &rhs) {\n"
      "    return *this;\n"
      "  }\n";
    generateSwitchForUnion(u, "rhs._disc", union_move_assign, branches, discriminator, "", "", "", false, false);
    be_global->impl_ <<
      "  _disc = rhs._disc;\n"
      "  return *this;\n"
      "}\n\n" <<
      "void " << nm << "::_activate(" << d_type << " d)\n"
      "{\n"
      "  if (_set && d != _disc) {\n"
      "    _reset();\n"
      "  }\n";
    generateSwitchForUnion(u, "d", union_activate, branches, discriminator, "", "", "", false, false);
    be_global->impl_ <<
      "  _set = true;\n"
      "  _disc = d;\n"
      "}\n\n"
      "void " << nm << "::_reset()\n"
      "{\n"
      "  if (!_set) return;\n";
    generateSwitchForUnion(u, "_disc", union_reset, branches, discriminator, "", "", "", false, false);
    be_global->impl_ <<
      "  _set = false;\n"
      "}\n\n"
      "void swap(" << nm << "& lhs, " << nm << "& rhs)\n"
      "{\n"
      "  std::swap(lhs, rhs);\n"
      "}\n\n";

    gen_typecode_ptrs(nm);
    return true;
  }

  static Cxx11Generator instance;
};
Cxx11Generator Cxx11Generator::instance;

void langmap_generator::init()
{
  switch (be_global->language_mapping()) {
  case BE_GlobalData::LANGMAP_FACE_CXX:
    string_ns = "::FACE";
    generator_ = &FaceGenerator::instance;
    generator_->init();
    break;
  case BE_GlobalData::LANGMAP_SP_CXX:
    string_ns = "::CORBA";
    generator_ = &SafetyProfileGenerator::instance;
    generator_->init();
    break;
  case BE_GlobalData::LANGMAP_CXX11:
    string_ns = "::CORBA";
    generator_ = &Cxx11Generator::instance;
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
    ? scoped(constant->enum_full_name()) : generator_->map_type(type);
  be_global->lang_header_ <<
    generator_->const_keyword(type) << ' ' << type_name << ' ' << nm << " = ";

  if (is_enum) {
    UTL_ScopedName* const enumerator = constant->constant_value()->n();
    if (generator_->scoped_enum()) {
      be_global->lang_header_ << type_name << "::"
        << to_string(enumerator->last_component()) << ";\n";
    } else {
      be_global->lang_header_ << dds_generator::scoped_helper(enumerator, "::") << ";\n";
    }
  } else {
    be_global->lang_header_ << *constant->constant_value()->ev() << ";\n";
  }
  return true;
}

bool langmap_generator::gen_enum(AST_Enum*, UTL_ScopedName* name,
                                 const std::vector<AST_EnumVal*>& contents,
                                 const char*)
{
  const ScopedNamespaceGuard namespaces(name, be_global->lang_header_);
  const char* const nm = name->last_component()->get_string();
  const char* scoped_enum = generator_->scoped_enum() ? "class " : "";
  const std::string enum_base = generator_->enum_base();
  be_global->lang_header_ <<
    "enum " << scoped_enum << nm << enum_base << " {\n";
  for (size_t i = 0; i < contents.size(); ++i) {
    be_global->lang_header_ <<
      "  " << contents[i]->local_name()->get_string()
      << ((i < contents.size() - 1) ? ",\n" : "\n");
  }
  be_global->lang_header_ <<
    "};\n\n";
  generator_->gen_simple_out(nm);
  gen_typecode(name);
  return true;
}

bool langmap_generator::gen_struct_fwd(UTL_ScopedName* name,
                                       AST_Type::SIZE_TYPE size)
{
  const ScopedNamespaceGuard namespaces(name, be_global->lang_header_);
  generator_->struct_decls(name, size);
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
  void gen_fixed(UTL_ScopedName* name, AST_Fixed* fixed)
  {
    be_global->add_include("FACE/Fixed.h", BE_GlobalData::STREAM_LANG_H);
    const char* const nm = name->last_component()->get_string();
    be_global->lang_header_ <<
      "typedef " << helpers_[HLP_FIXED] << '<' << *fixed->digits()->ev()
      << ", " << *fixed->scale()->ev() << "> " << nm << ";\n"
      "typedef " << nm << "& " << nm << "_out;\n";
  }
}

bool langmap_generator::gen_typedef(AST_Typedef*, UTL_ScopedName* name, AST_Type* base,
                                    const char*)
{
  AST_Array* arr = 0;
  {
    const ScopedNamespaceGuard namespaces(name, be_global->lang_header_);
    const char* const nm = name->last_component()->get_string();

    switch (base->node_type()) {
    case AST_Decl::NT_sequence:
      generator_->gen_sequence(name, dynamic_cast<AST_Sequence*>(base));
      break;
#if OPENDDS_HAS_IDL_MAP
    case AST_Decl::NT_map:
      generator_->gen_map(name, dynamic_cast<AST_Map*>(base));
      break;
#endif
    case AST_Decl::NT_array:
      generator_->gen_array(name, arr = dynamic_cast<AST_Array*>(base));
      break;
    case AST_Decl::NT_fixed:
      gen_fixed(name, dynamic_cast<AST_Fixed*>(base));
      break;
    default:
      if (be_global->language_mapping() == BE_GlobalData::LANGMAP_CXX11) {
        be_global->lang_header_ <<
          "using "  << nm << " = " << generator_->map_type(base) << ";\n";
      } else {
        be_global->lang_header_ <<
          "typedef " << generator_->map_type(base) << ' ' << nm << ";\n";
      }

      generator_->gen_typedef_varout(nm, base);

      AST_Type* actual_base = resolveActualType(base);
      if (actual_base->node_type() == AST_Decl::NT_array) {
        generator_->gen_array_typedef(nm, base);
      }

      break;
    }

    gen_typecode(name);
  }
  if (arr) generator_->gen_array_traits(name, arr);
  return true;
}

bool langmap_generator::gen_union_fwd(AST_UnionFwd* node,
                                      UTL_ScopedName* name,
                                      AST_Type::SIZE_TYPE)
{
  const ScopedNamespaceGuard namespaces(name, be_global->lang_header_);
  generator_->struct_decls(name, node->full_definition()->size_type(), "class");
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
  if (be_global->language_mapping() == BE_GlobalData::LANGMAP_CXX11) {
    return true;
  }

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
