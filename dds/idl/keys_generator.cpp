/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "keys_generator.h"
#include "be_extern.h"

#include "utl_identifier.h"

#include <string>
using std::string;

bool keys_generator::gen_struct(AST_Structure*, UTL_ScopedName* name,
  const std::vector<AST_Field*>&, AST_Type::SIZE_TYPE, const char*)
{
  string cxx = scoped(name), under = scoped_helper(name, "_");
  IDL_GlobalData::DCPS_Data_Type_Info* info = idl_global->is_dcps_type(name);
  if (!info) {
    return true;
  }

  const bool empty = info->key_list_.is_empty();

  be_global->header_ << be_global->versioning_begin() << "\n";

    {
      struct Namespaces {
        size_t n_;
        Namespaces(UTL_ScopedName* name)
          : n_(0)
        {
          for (UTL_ScopedName* sn = name; sn && sn->tail();
              sn = static_cast<UTL_ScopedName*>(sn->tail())) {
            string str = sn->head()->get_string();
            if (str.size()) {
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
        "// This structure supports use of std::map with a key\n"
        "// defined by one or more #pragma DCPS_DATA_KEY lines.\n"
        "struct " << be_global->export_macro() << ' ' <<
        name->last_component()->get_string() << "_OpenDDS_KeyLessThan {\n";

      if (empty)
      {
        be_global->header_ <<
          "  bool operator()(const " << cxx << "&, const " << cxx
          << "&) const\n"
          "  {\n"
          "    // Eith no DCPS_DATA_KEYs, return false\n"
          "    // to allow use of map with just one entry\n";
      }
      else
      {
        be_global->header_ <<
          "  bool operator()(const " << cxx << "& v1, const " << cxx
          << "& v2) const\n"
          "  {\n"
          "    using ::operator<; // TAO::String_Manager's operator< is "
          "in global NS\n";

          IDL_GlobalData::DCPS_Data_Type_Info_Iter iter(info->key_list_);

          for (ACE_TString* kp = 0; iter.next(kp) != 0; iter.advance()) {
            string fname = ACE_TEXT_ALWAYS_CHAR(kp->c_str());
            be_global->header_ <<
              "    if (v1." << fname << " < v2." << fname << ") return true;\n"
              "    if (v2." << fname << " < v1." << fname << ") return false;\n";
          }
      }
      be_global->header_ <<
         "    return false;\n"
         "  }\n};\n";
    }
  be_global->header_ << be_global->versioning_end() << "\n";
  return true;
}
