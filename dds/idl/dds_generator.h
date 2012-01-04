/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef dds_generator_H
#define dds_generator_H

#include "be_extern.h"

#include "utl_scoped_name.h"
#include "ast.h"
#include "ast_component_fwd.h"
#include "ast_eventtype_fwd.h"
#include "ast_structure_fwd.h"
#include "ast_union_fwd.h"
#include "ast_valuetype_fwd.h"

#include <string>
#include <vector>

class dds_generator {
public:
  virtual ~dds_generator() = 0;

  virtual bool gen_const(UTL_ScopedName* name, bool nestedInInteface,
                         AST_Expression::ExprType type,
                         AST_Expression::AST_ExprValue* value) = 0;

  virtual bool gen_enum(UTL_ScopedName* name,
                        const std::vector<AST_EnumVal*>& contents,
                        const char* repoid) = 0;

  virtual bool gen_struct(UTL_ScopedName* name,
                          const std::vector<AST_Field*>& fields,
                          const char* repoid) = 0;

  virtual bool gen_typedef(UTL_ScopedName* name, AST_Type* base,
                           const char* repoid) = 0;

  virtual bool gen_interf(UTL_ScopedName* name, bool local,
                          const std::vector<AST_Interface*>& inherits,
                          const std::vector<AST_Interface*>& inherits_flat,
                          const std::vector<AST_Attribute*>& attrs,
                          const std::vector<AST_Operation*>& ops,
                          const char* repoid) = 0;

  virtual bool gen_interf_fwd(UTL_ScopedName* name) = 0;

  virtual bool gen_native(UTL_ScopedName* name, const char* repoid) = 0;

  virtual bool gen_union(UTL_ScopedName* name,
                         const std::vector<AST_UnionBranch*>& branches,
                         AST_Type* discriminator,
                         AST_Expression::ExprType udisc_type,
                         const AST_Union::DefaultValue& default_value,
                         const char* repoid) = 0;

  static std::string scoped_helper(UTL_ScopedName* sn, const char* sep);
};

class composite_generator : public dds_generator {
public:
  bool gen_const(UTL_ScopedName* name, bool nestedInInteface,
                 AST_Expression::ExprType type,
                 AST_Expression::AST_ExprValue* value);

  bool gen_enum(UTL_ScopedName* name,
                const std::vector<AST_EnumVal*>& contents, const char* repoid);

  bool gen_struct(UTL_ScopedName* name,
                  const std::vector<AST_Field*>& fields, const char* repoid);

  bool gen_typedef(UTL_ScopedName* name, AST_Type* base, const char* repoid);

  bool gen_interf(UTL_ScopedName* name, bool local,
                  const std::vector<AST_Interface*>& inherits,
                  const std::vector<AST_Interface*>& inherits_flat,
                  const std::vector<AST_Attribute*>& attrs,
                  const std::vector<AST_Operation*>& ops, const char* repoid);

  bool gen_interf_fwd(UTL_ScopedName* name);

  bool gen_native(UTL_ScopedName* name, const char* repoid);

  bool gen_union(UTL_ScopedName* name,
                 const std::vector<AST_UnionBranch*>& branches,
                 AST_Type* discriminator,
                 AST_Expression::ExprType udisc_type,
                 const AST_Union::DefaultValue& default_value,
                 const char* repoid);

  template <typename InputIterator>
  composite_generator(InputIterator begin, InputIterator end)
  : components_(begin, end) {}

private:
  std::vector<dds_generator*> components_;
};

// common utilities for all "generator" derived classes

struct NamespaceGuard {
  NamespaceGuard()
  {
    be_global->header_ << "namespace OpenDDS { namespace DCPS {\n\n";
    be_global->impl_ << "namespace OpenDDS { namespace DCPS {\n\n";
  }
  ~NamespaceGuard()
  {
    be_global->header_ << "}  }\n\n";
    be_global->impl_ << "}  }\n\n";
  }
};

struct Function {
  bool has_arg_;
  std::string preamble_;

  Function(const char* name, const char* returntype)
    : has_arg_(false)
  {
    using std::string;
    ACE_CString ace_exporter = be_global->export_macro();
    bool use_exp = ace_exporter != "";
    string exporter = use_exp ? (string(" ") + ace_exporter.c_str()) : "";
    be_global->header_ << ace_exporter << (use_exp ? "\n" : "")
      << returntype << " " << name << "(";
    be_global->impl_ << returntype << " " << name << "(";
  }

  void addArg(const char* name, const std::string& type)
  {
    std::string sig = (has_arg_ ? ", " : "") + type + (name[0] ? " " : "")
      + name;
    be_global->header_ << sig;
    be_global->impl_ << sig;
    if (name[0]) {
      preamble_ += "  ACE_UNUSED_ARG(" + std::string(name) + ");\n";
    }
    has_arg_ = true;
  }

  void endArgs()
  {
    be_global->header_ << ");\n\n";
    be_global->impl_ << ")\n{\n" << preamble_;
  }

  ~Function()
  {
    be_global->impl_ << "}\n\n";
  }
};

inline std::string scoped(UTL_ScopedName* sn)
{
  return dds_generator::scoped_helper(sn, "::");
}

namespace AstTypeClassification {
  inline void resolveActualType(AST_Type*& element)
  {
    if (element->node_type() == AST_Decl::NT_typedef) {
      AST_Typedef* td = AST_Typedef::narrow_from_decl(element);
      element = td->primitive_base_type();
    }

    switch(element->node_type()) {
    case AST_Decl::NT_interface_fwd:
    {
      AST_InterfaceFwd* td = AST_InterfaceFwd::narrow_from_decl(element);
      element = td->full_definition();
      break;
    }
    case AST_Decl::NT_valuetype_fwd:
    {
      AST_ValueTypeFwd* td = AST_ValueTypeFwd::narrow_from_decl(element);
      element = td->full_definition();
      break;
    }
    case AST_Decl::NT_union_fwd:
    {
      AST_UnionFwd* td = AST_UnionFwd::narrow_from_decl(element);
      element = td->full_definition();
      break;
    }
    case AST_Decl::NT_struct_fwd:
    {
      AST_StructureFwd* td = AST_StructureFwd::narrow_from_decl(element);
      element = td->full_definition();
      break;
    }
    case AST_Decl::NT_component_fwd:
    {
      AST_ComponentFwd* td = AST_ComponentFwd::narrow_from_decl(element);
      element = td->full_definition();
      break;
    }
    case AST_Decl::NT_eventtype_fwd:
    {
      AST_EventTypeFwd* td = AST_EventTypeFwd::narrow_from_decl(element);
      element = td->full_definition();
      break;
    }
    }
  }

  typedef size_t Classification;
  const Classification CL_UNKNOWN = 0, CL_SCALAR = 1, CL_PRIMITIVE = 2,
    CL_STRUCTURE = 4, CL_STRING = 8, CL_ENUM = 16, CL_UNION = 32, CL_ARRAY = 64,
    CL_SEQUENCE = 128, CL_WIDE = 256, CL_BOUNDED = 512, CL_INTERFACE = 1024;

  inline Classification classify(AST_Type* type)
  {
    resolveActualType(type);
    switch (type->node_type()) {
    case AST_Decl::NT_pre_defined: {
      AST_PredefinedType* p = AST_PredefinedType::narrow_from_decl(type);
      return (p->pt() == AST_PredefinedType::PT_any
        || p->pt() == AST_PredefinedType::PT_object)
        ? CL_UNKNOWN : (CL_SCALAR | CL_PRIMITIVE);
    }
    case AST_Decl::NT_array:
      return CL_ARRAY;
    case AST_Decl::NT_union:
      return CL_UNION;
    case AST_Decl::NT_string:
    case AST_Decl::NT_wstring:
      return CL_SCALAR | CL_STRING |
        ((AST_String::narrow_from_decl(type)->max_size()->ev()->u.ulval == 0)
        ? 0 : CL_BOUNDED) |
        ((type->node_type() == AST_Decl::NT_wstring) ? CL_WIDE : 0);
    case AST_Decl::NT_sequence:
      return CL_SEQUENCE |
        ((AST_Sequence::narrow_from_decl(type)->unbounded()) ? 0 : CL_BOUNDED);
    case AST_Decl::NT_struct:
      return CL_STRUCTURE;
    case AST_Decl::NT_enum:
      return CL_SCALAR | CL_ENUM;
    case AST_Decl::NT_interface:
      return CL_INTERFACE;
    default:
      return CL_UNKNOWN;
    }
  }
}

struct NestedForLoops {
  NestedForLoops(const char* type, const char* prefix, AST_Array* arr,
                 std::string& indent)
    : n_(arr->n_dims()), indent_(indent)
  {
    std::ostringstream index_oss;
    for (size_t i = 0; i < n_; ++i) {
      be_global->impl_ <<
        indent << "for (" << type << ' ' << prefix << i << " = 0; " <<
        prefix << i << " < " << arr->dims()[i]->ev()->u.ulval << "; ++" <<
        prefix << i << ") {\n";
      indent += "  ";
      index_oss << "[i" << i << "]";
    }
    index_ = index_oss.str();
  }

  ~NestedForLoops()
  {
    for (size_t i = 0; i < n_; ++i) {
      indent_.resize(indent_.size() - 2);
      be_global->impl_ << indent_ << "}\n";
    }
  }

  size_t n_;
  std::string& indent_;
  std::string index_;
};


#endif
