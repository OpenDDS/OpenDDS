/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "keys_generator.h"
#include "be_extern.h"
#include "topic_keys.h"

#include "utl_identifier.h"

#include <ast_array.h>
#include <ast_sequence.h>
#include <ast_structure.h>
#include <ast_union.h>

#include <sstream>
#include <string>
using std::string;
using namespace AstTypeClassification;

// Emit "if (E1 < E2) return true; if (E2 < E1) return false;" -- or,
// recursing into content, a per-leaf/per-element breakdown -- comparing two
// C++ access expressions of IDL type `type`. Used for content that's a key
// only because it's inside something that is (initially, a sequence's own
// elements; recursively, whatever nested structs, unions, arrays, or
// further sequences those elements themselves contain), so unlike a
// directly-@key-annotated field's single flat member access, this may
// recurse arbitrarily deep.
//
// A map can't be given an ordering (the same reason a map can never be
// marked @key directly -- see topic_keys.cpp), whether it's reached through
// a struct field (where TopicKeys::Iterator itself already throws) or, as
// below, directly as a sequence/array element type. And unlike a value,
// which is always finitely deep, the *type* graph reached this way can be
// cyclic (a struct that recursively contains a sequence of itself); expanding
// that inline, the way this function does for everything else, would never
// terminate. Both cases are rejected the same way: throw TopicKeys::Error
// rather than silently comparing nothing, and gen_struct() below turns that
// into a normal compile error instead of letting it escape uncaught.
void
compare_key_content(const string& e1, const string& e2, AST_Type* type, bool use_cxx11,
  size_t depth = 0)
{
  type = resolveActualType(type);
  static std::vector<AST_Type*> type_stack;
  for (size_t i = 0; i < type_stack.size(); ++i) {
    if (type == type_stack[i]) {
      throw TopicKeys::Error(type,
        "a recursive type can't be used as (or as part of) a sequence key element");
    }
  }
  struct TypeStackGuard {
    explicit TypeStackGuard(AST_Type* t) { type_stack.push_back(t); }
    ~TypeStackGuard() { type_stack.pop_back(); }
  } const guard(type);
  switch (TopicKeys::root_type(type)) {
  case TopicKeys::PrimitiveType:
    if (!use_cxx11 && (classify(type) & CL_STRING)) {
      // In the classic C++ mapping, a sequence's own string/wstring const
      // element access (unlike a TAO::String_Manager struct field) returns
      // a raw pointer with no content operator<, so `<` would compare
      // addresses, not content.
      be_global->header_ <<
        "    if (ACE_OS::strcmp(" << e1 << ", " << e2 << ") < 0) return true;\n"
        "    if (ACE_OS::strcmp(" << e2 << ", " << e1 << ") < 0) return false;\n";
    } else {
      be_global->header_ <<
        "    if (" << e1 << " < " << e2 << ") return true;\n"
        "    if (" << e2 << " < " << e1 << ") return false;\n";
    }
    break;
  case TopicKeys::UnionType:
    // A union's only key-relevant content is its discriminator, the same
    // "implied key" rule a directly-nested union field gets.
    compare_key_content(e1 + "._d()", e2 + "._d()",
      dynamic_cast<AST_Union*>(type)->disc_type(), use_cxx11, depth);
    break;
  case TopicKeys::StructureType: {
    // implied=true: no explicitly-@key fields means all fields are implied
    // keys, the same as a nested struct key field with no explicit keys of
    // its own (see TopicKeys's "implied" constructor parameter).
    TopicKeys keys(dynamic_cast<AST_Structure*>(type), true, true);
    const TopicKeys::Iterator finished = keys.end();
    for (TopicKeys::Iterator i = keys.begin(); i != finished; ++i) {
      string leaf = i.path();
      if (use_cxx11) {
        leaf = insert_cxx11_accessor_parens(leaf, false);
      }
      compare_key_content(e1 + "." + leaf, e2 + "." + leaf, i.get_ast_type(), use_cxx11, depth);
    }
    break;
  }
  case TopicKeys::SequenceType: {
    const char* const size_call = use_cxx11 ? ".size()" : ".length()";
    std::ostringstream oss;
    oss << "opendds_i" << depth;
    const string idx = oss.str();
    be_global->header_ <<
      "    if (" << e1 << size_call << " != " << e2 << size_call << ") {\n"
      "      return " << e1 << size_call << " < " << e2 << size_call << ";\n"
      "    }\n"
      "    for (size_t " << idx << " = 0; " << idx << " < " << e1 << size_call <<
        "; ++" << idx << ") {\n";
    compare_key_content(e1 + "[" + idx + "]", e2 + "[" + idx + "]",
      dynamic_cast<AST_Sequence*>(type)->base_type(), use_cxx11, depth + 1);
    be_global->header_ << "    }\n";
    break;
  }
  case TopicKeys::ArrayType: {
    AST_Array* const array_node = dynamic_cast<AST_Array*>(type);
    string index;
    size_t d = depth;
    for (unsigned long dim = 0; dim < array_node->n_dims(); ++dim, ++d) {
      std::ostringstream oss;
      oss << "opendds_i" << d;
      const string idx = oss.str();
      be_global->header_ <<
        "    for (unsigned long " << idx << " = 0; " << idx << " < " <<
          array_node->dims()[dim]->ev()->u.ulval << "; ++" << idx << ") {\n";
      index += "[" + idx + "]";
    }
    compare_key_content(e1 + index, e2 + index, array_node->base_type(), use_cxx11, d);
    for (unsigned long dim = 0; dim < array_node->n_dims(); ++dim) {
      be_global->header_ << "    }\n";
    }
    break;
  }
  case TopicKeys::MapType:
    throw TopicKeys::Error(type, "map types are not supported as keys");
  default:
    // e.g. InvalidType: not expected for a type that's actually reachable
    // as (or as part of) a sequence key element, but reject it rather than
    // silently comparing nothing if it somehow is.
    throw TopicKeys::Error(type, "this type is not supported as a key");
  }
}

struct KeyLessThanWrapper {
  size_t n_;
  const string cxx_name_;

  explicit KeyLessThanWrapper(UTL_ScopedName* name)
    : n_(0)
    , cxx_name_(scoped(name))
  {
    be_global->header_ << be_global->versioning_begin() << "\n";

    for (UTL_ScopedName* sn = name; sn && sn->tail();
        sn = static_cast<UTL_ScopedName*>(sn->tail())) {
      const string str = sn->head()->get_string();
      if (!str.empty()) {
        be_global->header_ << "namespace " << str << " {\n";
        ++n_;
      }
    }

    be_global->header_ <<
      "/// This structure supports use of std::map with one or more keys.\n"
      "struct " << be_global->export_macro() << ' ' <<
      name->last_component()->get_string() << "_OpenDDS_KeyLessThan {\n";
  }

  void
  has_no_keys_signature()
  {
    be_global->header_ <<
      "  bool operator()(const " << cxx_name_ << "&, const " << cxx_name_ << "&) const\n"
      "  {\n"
      "    // With no keys, return false to allow use of\n"
      "    // map with just one entry\n";
  }

  void
  has_keys_signature()
  {
    be_global->header_ <<
      "  bool operator()(const " << cxx_name_ << "& v1, const " << cxx_name_ << "& v2) const\n"
      "  {\n";
  }

  void
  key_compare(const string& member)
  {
    be_global->header_ <<
      "    if (v1." << member << " < v2." << member << ") return true;\n"
      "    if (v2." << member << " < v1." << member << ") return false;\n";
  }

  ~KeyLessThanWrapper()
  {
    be_global->header_ <<
      "    return false;\n"
      "  }\n};\n";

    for (size_t i = 0; i < n_; ++i) {
      be_global->header_ << "}\n";
    }

    be_global->header_ << be_global->versioning_end() << "\n";
  }
};

bool keys_generator::gen_struct(AST_Structure* node, UTL_ScopedName* name,
  const std::vector<AST_Field*>&, AST_Type::SIZE_TYPE, const char*)
{
  TopicKeys keys(node);
  size_t key_count = 0;
  const bool is_topic_type = be_global->is_topic_type(node);
  IDL_GlobalData::DCPS_Data_Type_Info* info = idl_global->is_dcps_type(name);
  if (is_topic_type) {
    key_count = keys.count();
  } else if (info) {
    key_count = info->key_list_.size();
  } else {
    return true;
  }

  try {
    KeyLessThanWrapper wrapper(name);

    if (key_count) {
      const bool use_cxx11 = be_global->language_mapping() == BE_GlobalData::LANGMAP_CXX11;

      wrapper.has_keys_signature();
      if (!use_cxx11) {
        be_global->header_ <<
          "    using ::operator<; // TAO::String_Manager's operator< is "
          "in global NS\n";
      }

      if (is_topic_type) {
        TopicKeys::Iterator finished = keys.end();
        for (TopicKeys::Iterator i = keys.begin(); i != finished; ++i) {
          string fname = i.path();
          if (use_cxx11) {
            fname = insert_cxx11_accessor_parens(fname, false);
          }
          if (i.root_type() == TopicKeys::SequenceType) {
            compare_key_content("v1." + fname, "v2." + fname, i.get_ast_type(), use_cxx11);
            continue;
          }
          if (i.root_type() == TopicKeys::UnionType) {
            fname += "._d()";
          }
          wrapper.key_compare(fname);
        }
      } else if (info) {
        IDL_GlobalData::DCPS_Data_Type_Info_Iter iter(info->key_list_);
        for (ACE_TString* kp = 0; iter.next(kp) != 0; iter.advance()) {
          string fname = ACE_TEXT_ALWAYS_CHAR(kp->c_str());
          if (use_cxx11) {
            fname = insert_cxx11_accessor_parens(fname, false);
          }
          wrapper.key_compare(fname);
        }
      }
    } else {
      wrapper.has_no_keys_signature();
    }
  } catch (TopicKeys::Error& error) {
    // From compare_key_content(): an unsupported (e.g. map) or recursive
    // type reachable through a sequence key's element type. The
    // KeyLessThanWrapper destructor still runs while unwinding into this
    // catch, so the header gets a truncated version of the struct; that's
    // fine since compilation is failing either way.
    idl_global->err()->misc_error(error.what(), error.node());
    return false;
  }

  return true;
}

bool keys_generator::gen_union(
  AST_Union* node, UTL_ScopedName* name,
  const std::vector<AST_UnionBranch*>&, AST_Type*, const char*)
{
  if (be_global->is_topic_type(node)) {
    KeyLessThanWrapper wrapper(name);
    if (be_global->union_discriminator_is_key(node)) {
      wrapper.has_keys_signature();
      wrapper.key_compare("_d()");
    } else {
      wrapper.has_no_keys_signature();
    }
  }
  return true;
}
