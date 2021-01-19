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

#include <string>
using std::string;

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

  {
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
          if (i.root_type() == TopicKeys::UnionType) {
            fname += "._d()";
          } else if (use_cxx11) {
            fname = insert_cxx11_accessor_parens(fname, false);
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
