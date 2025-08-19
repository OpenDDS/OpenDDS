/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "field_info.h"

#include <sstream>
#include <cstring>

using namespace AstTypeClassification;

namespace {
  AST_Type* seq_or_arr_base_type(AST_Type* type)
  {
    AST_Type* const act = AstTypeClassification::resolveActualType(type);
    AST_Sequence* const seq = dynamic_cast<AST_Sequence*>(act);
    AST_Array* const arr = dynamic_cast<AST_Array*>(act);
    if (seq) {
      return seq->base_type();
    } else if (arr) {
      return arr->base_type();
    }
    return 0;
  }
}

FieldInfo::EleLen::EleLen(AST_Type* type)
  : cls_(classify(type))
  , len_(container_element_limit(type))
  , map_key_(0)
{
  AST_Type* base;
  if (cls_ & CL_MAP) {
    AST_Map* const map = dynamic_cast<AST_Map*>(type);
    map_key_ = resolveActualType(map->key_type());
    base = resolveActualType(map->value_type());
  } else {
    base = resolveActualType(seq_or_arr_base_type(type));
  }
  const Classification base_cls = classify(base);
  base_cls_ = base_cls;
  base_name_ = base->full_name();
  if (base_cls_ & (CL_ARRAY | CL_SEQUENCE | CL_MAP)) {
    base_container_ = Container(new EleLen(base));
  }
}

bool FieldInfo::EleLen::operator<(const EleLen& o) const
{
  if (cls_ != o.cls_) {
    return cls_ < o.cls_;
  }
  if (len_ != o.len_) {
    return len_ < o.len_;
  }
  if (map_key_ != o.map_key_) {
    return map_key_ < o.map_key_;
  }
  if (bool(base_container_.get()) != bool(o.base_container_.get())) {
    return bool(base_container_.get()) < bool(o.base_container_.get());
  }
  if (base_container_.get()) {
    return *base_container_ < *o.base_container_;
  }
  if (base_cls_ != o.base_cls_) {
    return base_cls_ < o.base_cls_;
  }
  return std::strcmp(base_name_, o.base_name_) < 0;
}

const std::string FieldInfo::scope_op = "::";

bool FieldInfo::cxx11()
{
  return be_global->language_mapping() == BE_GlobalData::LANGMAP_CXX11;
}

std::string FieldInfo::anonymous_type_prefix()
{
  return cxx11() ? "AnonymousType_" : "_";
}

std::string FieldInfo::scoped_type(AST_Type& field_type, const std::string& field_name)
{
  std::string n = scoped(field_type.name());
  if (!field_type.anonymous()) {
    return n;
  }
  n = n.substr(0, n.rfind(scope_op) + 2) + anonymous_type_prefix() + field_name;

  switch (field_type.node_type()) {
  case AST_Decl::NT_sequence:
    return n + "_seq";
  case AST_Decl::NT_map:
    return n + "_map";
  default:
    return n;
  }
}

std::string FieldInfo::underscore(const std::string& scoped)
{
  std::string s = scoped;
  if (s.find(scope_op) == 1) {
    s = s.substr(3);
  }
  for (std::size_t i = s.find(scope_op); i != s.npos; i = s.find(scope_op, i + 2)) {
    s.replace(i, 2, "_");
  }
  return s;
}

std::string FieldInfo::ref(const std::string& scoped, const std::string& const_s)
{
  return "IDL::DistinctType<" + const_s + scoped + ", " + dds_generator::get_tag_name(scoped) + ">";
}

FieldInfo::FieldInfo(AST_Field& field)
  : type_(field.field_type())
  , name_(field.local_name()->get_string())
  , scoped_type_(scoped_type(*type_, name_))
  , underscored_(underscore(scoped_type_))
  , type_name_(scoped_type_.substr(scoped_type_.rfind(scope_op) + 2))
  , act_(resolveActualType(type_))
  , cls_(classify(act_))
  , arr_(dynamic_cast<AST_Array*>(type_))
  , seq_(dynamic_cast<AST_Sequence*>(type_))
  , map_(dynamic_cast<AST_Map*>(type_))
  , as_base_(seq_or_arr_base_type(type_))
  , as_act_(as_base_ ? resolveActualType(as_base_) : 0)
  , as_cls_(as_act_ ? classify(as_act_) : CL_UNKNOWN)
  , scoped_elem_(as_base_ ? scoped(as_base_->name()) : "")
  , underscored_elem_(as_base_ ? underscore(scoped_elem_) : "")
  , elem_ref_(as_base_ ? ref(scoped_elem_, "") : "")
  , elem_const_ref_(as_base_ ? ref(scoped_elem_) : "")
  , n_elems_(container_element_limit(type_))
  , is_optional_(be_global->is_optional(&field))
{
  if (arr_) {
    std::ostringstream os;
    os << n_elems_;
    length_ = os.str();
    arg_ = "arr";
  } else if (seq_) {
    length_ = "length";
    arg_ = "seq";
  } else if (map_) {
    arg_ = "map";
  }

  if (cxx11()) {
    unwrap_ = scoped_type_ + "& " + arg_ + " = wrap;\n  ACE_UNUSED_ARG(" + arg_ + ");\n";
    const_unwrap_ = "  const " + unwrap_;
    unwrap_ = "  " + unwrap_;
    arg_ = "wrap";
    ref_ = ref(scoped_type_, "");
    const_ref_ = ref(scoped_type_);
    ptr_ = ref_ + '*';
  } else {
    ref_ = scoped_type_ + (arr_ ? "_forany&" : "&");
    const_ref_ = "const " + ref_;
    ptr_ = scoped_type_ + (arr_ ? "_forany*" : "*");
  }
}

bool FieldInfo::is_new(EleLenSet& el_set) const
{
  return cxx11() || el_set.insert(EleLen(type_)).second;
}

bool FieldInfo::anonymous() const
{
  return type_->anonymous() && (cls_ & (CL_ARRAY | CL_SEQUENCE | CL_MAP));
}
