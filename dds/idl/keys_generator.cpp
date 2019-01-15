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

bool keys_generator::gen_struct(AST_Structure* node, UTL_ScopedName* name,
  const std::vector<AST_Field*>&, AST_Type::SIZE_TYPE, const char*)
{
#ifdef TAO_IDL_HAS_ANNOTATIONS
  TopicKeys keys(node);
#endif
  size_t key_count = 0;
  IDL_GlobalData::DCPS_Data_Type_Info* info = idl_global->is_dcps_type(name);
  if (info) {
    key_count = info->key_list_.size();
  }
#ifdef TAO_IDL_HAS_ANNOTATIONS
  else if (be_global->is_topic_type(node)) {
    key_count = keys.count();
  }
#endif
  else {
    return true;
  }

  be_global->header_ << be_global->versioning_begin() << "\n";

  {
    struct Namespaces {
      size_t n_;
      explicit Namespaces(UTL_ScopedName* name)
        : n_(0)
      {
        for (UTL_ScopedName* sn = name; sn && sn->tail();
            sn = static_cast<UTL_ScopedName*>(sn->tail())) {
          const string str = sn->head()->get_string();
          if (!str.empty()) {
            be_global->header_ << "namespace " << str << " {\n";
            ++n_;
          }
        }
      }
      ~Namespaces()
      {
        for (size_t i = 0; i < n_; ++i) be_global->header_ << "}\n";
      }
    } ns(name);

    be_global->header_ <<
      "// This structure supports use of std::map with one or more keys.\n"
      "struct " << be_global->export_macro() << ' ' <<
      name->last_component()->get_string() << "_OpenDDS_KeyLessThan {\n";

    const string cxx = scoped(name);
    if (!key_count) {
      be_global->header_ <<
        "  bool operator()(const " << cxx << "&, const " << cxx
        << "&) const\n"
        "  {\n"
        "    // With no keys, return false to allow use of\n"
        "    // map with just one entry\n";
    } else {
      const bool use_cxx11 = be_global->language_mapping() == BE_GlobalData::LANGMAP_CXX11;
      be_global->header_ <<
        "  bool operator()(const " << cxx << "& v1, const " << cxx
        << "& v2) const\n"
        "  {\n";
      if (!use_cxx11) {
        be_global->header_ <<
          "    using ::operator<; // TAO::String_Manager's operator< is "
          "in global NS\n";
      }

      std::vector<string> key_names;
      key_names.reserve(key_count);
      if (info) {
        IDL_GlobalData::DCPS_Data_Type_Info_Iter iter(info->key_list_);
        for (ACE_TString* kp = 0; iter.next(kp) != 0; iter.advance()) {
          key_names.push_back(ACE_TEXT_ALWAYS_CHAR(kp->c_str()));
        }
      } else {
        TopicKeys::Iterator finished = keys.end();
        for (TopicKeys::Iterator i = keys.begin(); i != finished; ++i) {
          key_names.push_back(i.path());
        }
      }

      for (size_t i = 0; i < key_count; i++) {
        string fname = key_names[i];
        if (use_cxx11) {
          fname += "()";
        }
        be_global->header_ <<
          "    if (v1." << fname << " < v2." << fname << ") return true;\n"
          "    if (v2." << fname << " < v1." << fname << ") return false;\n";
      }
    }
    be_global->header_ <<
      "    return false;\n"
      "  }\n};\n";
  } // close namespaces in generated code
  be_global->header_ << be_global->versioning_end() << "\n";
  return true;
}
