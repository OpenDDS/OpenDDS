/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "field_info.h"
#include <sstream>

using namespace AstTypeClassification;

FieldInfo::EleLen::EleLen(FieldInfo& af) : ele_(af.as_base_), len_(af.n_elems_)
{
}

bool FieldInfo::EleLen::operator<(const EleLen& o) const
{
  return ele_ < o.ele_ || (ele_ == o.ele_ && len_ < o.len_);
}

const std::string FieldInfo::scope_op = "::";

bool FieldInfo::cxx11() {
  return be_global->language_mapping() == BE_GlobalData::LANGMAP_CXX11;
}

std::string FieldInfo::at_pfx() {
  return cxx11() ? "T_" : "_";
}

std::string FieldInfo::get_type_name(AST_Type& field)
{
  std::string n = scoped(field.name());
  if (!field.anonymous()) {
    return n;
  }
  n = n.substr(0, n.rfind(scope_op) + 2) + at_pfx() + field.local_name()->get_string();
  return (field.node_type() == AST_Decl::NT_sequence) ? (n + "_seq") : n;
}

std::string FieldInfo::underscored_type_name(UTL_ScopedName* sn, AST_Type& field)
{
  std::string n = dds_generator::scoped_helper(sn, "_");
  if (!field.anonymous()) {
    return n;
  }
  std::size_t i = n.rfind(scope_op);
  return n.substr(0, i + 2) + at_pfx() + n.substr(i + 2);
}

std::string FieldInfo::underscore(const std::string& scoped_type)
{
  std::string s = scoped_type;
  for (std::size_t i = s.find(scope_op); i != s.npos; i = s.find(scope_op, i + 2)) {
    s.replace(i, 2, "_");
  }
  return s;
}

std::string FieldInfo::ref(const std::string& scoped_t, const std::string& underscored_t, const std::string& const_s)
{
  return "IDL::DistinctType<" + const_s + scoped_t + ", " + underscored_t + "_tag>";
}

// for anonymous types
FieldInfo::FieldInfo(AST_Field& field) :
  type_(field.field_type()),
  name_(field.local_name()->get_string()),
  underscored_(underscored_type_name(type_->name(), *type_))
{
  init();
}

FieldInfo::FieldInfo(UTL_ScopedName* sn, AST_Type* base) :
  type_(base),
  underscored_(underscored_type_name(sn, *type_)),
  scoped_type_(scoped(sn))
{
  init();
}

void FieldInfo::init()
{
  act_ = resolveActualType(type_);
  cls_ = classify(act_);
  arr_ = AST_Array::narrow_from_decl(type_);
  seq_ = AST_Sequence::narrow_from_decl(type_);
  as_base_ = arr_ ? arr_->base_type() : (seq_ ? seq_->base_type() : 0);
  as_act_ = as_base_ ? resolveActualType(as_base_) : 0;
  as_cls_ = as_act_ ? classify(as_act_) : CL_UNKNOWN;
  scoped_elem_ = as_base_ ? scoped(as_base_->name()) : "";
  underscored_elem_ = as_base_ ? underscore(scoped_elem_) : "";
  if (!scoped_elem_.empty() && !underscored_elem_.empty()) {
    elem_ref_ = ref(scoped_elem_, underscored_elem_, "");
    elem_const_ref_ = ref(scoped_elem_, underscored_elem_);
  }

  if (type_->anonymous() && as_base_) {
    scoped_type_ = scoped(type_->name());
    std::size_t i = scoped_type_.rfind(scope_op);
    struct_name_ = scoped_type_.substr(0, i);
    if (!name_.empty()) {
      type_name_ = at_pfx() + name_;
      if (seq_) { type_name_ += "_seq"; }
      scoped_type_ = struct_name_ + "::" + type_name_;
      underscored_ = underscore(scoped_type_);
    } else {
      type_name_ = scoped_type_.substr(i + 2);
    }
  } else {
    if (scoped_type_.empty()) {
      scoped_type_ = scoped(type_->name());
    }
    //name_
    type_name_ = scoped_type_;
    //struct_name_
  }

  n_elems_ = 1;
  if (arr_) {
    for (size_t i = 0; i < arr_->n_dims(); ++i) {
      n_elems_ *= arr_->dims()[i]->ev()->u.ulval;
    }
    std::ostringstream os;
    os << n_elems_;
    length_ = os.str();
    arg_ = "arr";
  } else if (seq_) {
    n_elems_ = !seq_->unbounded() ? seq_->max_size()->ev()->u.ulval : 0;
    length_ = "length";
    arg_ = "seq";
  }

  if (cxx11()) {
    unwrap_ = scoped_type_ + "& " + arg_ + " = wrap;\n  ACE_UNUSED_ARG(" + arg_ + ");\n";
    const_unwrap_ = "  const " + unwrap_;
    unwrap_ = "  " + unwrap_;
    arg_ = "wrap";
    ref_ = ref(scoped_type_, underscored_, "");
    const_ref_ = ref(scoped_type_, underscored_);
    ptr_ = "IDL::DistinctType<" + scoped_type_ + ", " + underscored_ + "_tag>*";
  } else {
    ref_ = scoped_type_ + (arr_ ? "_forany&" : "&");
    const_ref_ = "const " + ref_;
    ptr_ = scoped_type_ + (arr_ ? "_forany*" : "*");
  }
}

bool FieldInfo::is_new(EleLenSet& el_set)
{
  return cxx11() || el_set.insert(EleLen(*this)).second;
}
