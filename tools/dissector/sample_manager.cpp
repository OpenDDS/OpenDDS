/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "tools/dissector/sample_manager.h"

#include <ace/Basic_Types.h>
#include <ace/CDR_Base.h>
#include <ace/Message_Block.h>
#include <ace/Log_Msg.h>
#include <ace/ACE.h>
#include <ace/Dirent.h>
#include <ace/Configuration.h>
#include <ace/Configuration_Import_Export.h>

#ifndef NO_ITL
#include <itl/itl.hpp>
#endif

#include <cstring>

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <string>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS
{
  namespace DCPS
  {

#ifndef NO_ITL
    Sample_Dissector* get_dissector(std::map<itl::Type*, Sample_Dissector*>& map, itl::Type* type)
    {
      if (map.find(type) != map.end()) {
        return map[type];
      }

      struct visitor : public itl::TypeVisitor {
        Sample_Dissector* dissector;
        std::map<itl::Type*, Sample_Dissector*>& map;

        visitor(std::map<itl::Type*, Sample_Dissector*>& m)
          : dissector(NULL)
          , map(m)
        { }

        void visit (itl::Alias& a) {
          dissector = get_dissector(map, a.type());
        }

        void visit (itl::Int& i) {
          if (i.bits() == 32 && i.isUnsigned () && i.isConstrained) {
            std::map<unsigned int, std::string> x;
            for (itl::ConstrainedType::ValuesType::const_iterator pos = i.values.begin(),
                   limit = i.values.end();
                 pos != limit;
                 ++pos) {
              x[ACE_OS::atoi (pos->second.c_str())] = pos->first;
            }

            Sample_Enum* sample = new Sample_Enum();
            if (!x.empty()) {
              for (unsigned int i = 0, limit = x.rbegin()->first + 1; i != limit; ++i) {
                if (x.find(i) != x.end()) {
                  sample->add_value(x[i]);
                }
                else {
                  std::stringstream ss;
                  ss << i;
                  sample->add_value(ss.str());
                }
              }
            }
            dissector = sample;
            return;
          }

          dissector = new Sample_Dissector();
          switch (i.bits()) {
          case 0:
            dissector->add_field(new Sample_Field(Sample_Field::WChar, ""));
            break;
          case 1:
            dissector->add_field(new Sample_Field(Sample_Field::Boolean, ""));
            break;
          case 8:
            if (i.isUnsigned()) {
              dissector->add_field(new Sample_Field(Sample_Field::Octet, ""));
            }
            else {
              dissector->add_field(new Sample_Field(Sample_Field::Char, ""));
            }
            break;
          case 16:
            if (i.isUnsigned()) {
              dissector->add_field(new Sample_Field(Sample_Field::UShort, ""));
            }
            else {
              dissector->add_field(new Sample_Field(Sample_Field::Short, ""));
            }
            break;
          case 32:
            if (i.isUnsigned()) {
              dissector->add_field(new Sample_Field(Sample_Field::ULong, ""));
            }
            else {
              dissector->add_field(new Sample_Field(Sample_Field::Long, ""));
            }
            break;
          case 64:
            if (i.isUnsigned()) {
              dissector->add_field(new Sample_Field(Sample_Field::ULongLong, ""));
            }
            else {
              dissector->add_field(new Sample_Field(Sample_Field::LongLong, ""));
            }
            break;
          default:
            {
              ACE_DEBUG ((LM_WARNING, ACE_TEXT ("Unknown integer type\n")));

              const unsigned int seven = 7;
              const unsigned int bytes = ((i.bits() + seven) & (~seven)) / 8;
              for (unsigned int i = 0; i != bytes; ++i) {
                dissector->add_field(new Sample_Field(Sample_Field::Octet, ""));
              }
            }
            break;
          }
        }

        void visit (itl::Float& f) {
          dissector = new Sample_Dissector();
          switch (f.model()) {
          case itl::Float::Binary32:
            dissector->add_field(new Sample_Field(Sample_Field::Float, ""));
            break;
          case itl::Float::Binary64:
            dissector->add_field(new Sample_Field(Sample_Field::Double, ""));
            break;
          case itl::Float::Binary128:
            dissector->add_field(new Sample_Field(Sample_Field::LongDouble, ""));
            break;
          default:
            dissector->add_field(new Sample_Field(Sample_Field::Undefined, ""));
            ACE_DEBUG ((LM_WARNING, ACE_TEXT ("Unknown float model: %d\n"), f.model()));
            break;
          }
        }

        void visit (itl::Fixed&) {
          dissector = new Sample_Dissector();
          dissector->add_field(new Sample_Field(Sample_Field::Undefined, ""));
          ACE_DEBUG ((LM_WARNING, ACE_TEXT ("Fixed-point types are not supported\n")));
        }

        Sample_Dissector* do_array(std::vector<unsigned int>::const_iterator pos,
                                   std::vector<unsigned int>::const_iterator limit,
                                   itl::Type* element_type) {
          if (pos != limit) {
            return new Sample_Array(*pos, do_array(pos + 1, limit, element_type));
          }
          else {
            return get_dissector(map, element_type);
          }
        }

        void visit (itl::Sequence& s) {
          std::vector<unsigned int> size = s.size();
          if (size.empty()) {
            dissector = new Sample_Sequence(get_dissector(map, s.element_type()));
          }
          else {
            dissector = do_array(size.begin(), size.end(), s.element_type());
          }
        }

        void visit (itl::String& s) {
          dissector = new Sample_Dissector();
          const rapidjson::Value* note = s.note();
          if (note && note->IsObject() && note->HasMember("idl")) {
            const rapidjson::Value& idl = (*note)["idl"];
            if (idl.IsObject() && idl.HasMember("type")) {
              const rapidjson::Value& type = idl["type"];
              if (type.IsString() && type.GetString() == std::string("wchar")) {
                dissector->add_field(new Sample_Field(Sample_Field::WString, ""));
                return;
              }
            }
          }
          dissector->add_field(new Sample_Field(Sample_Field::String, ""));
        }

        void visit (itl::Record& r) {
          dissector = new Sample_Dissector();
          for (itl::Record::const_iterator pos = r.begin(), limit = r.end();
               pos != limit;
               ++pos) {
            const itl::Record::Field& field = *pos;
            dissector->add_field(get_dissector(map, field.type), field.name);
          }
        }

        void visit (itl::Union& u) {
          Sample_Union *s_union = new Sample_Union ();
          s_union->discriminator(get_dissector(map, u.discriminator()));
          for (itl::Union::const_iterator pos = u.begin(), limit = u.end();
               pos != limit;
               ++pos) {
            if (pos->labels.empty()) {
              s_union->add_default(new Sample_Field(get_dissector(map, pos->type), pos->name));
              break;
            }
          }

          for (itl::Union::const_iterator pos = u.begin(), limit = u.end();
               pos != limit;
               ++pos) {
            if (!pos->labels.empty()) {
              Sample_Field* field = s_union->add_field (get_dissector(map, pos->type), pos->name);
              for (itl::Union::Field::const_iterator label_pos = pos->begin(),
                     label_limit = pos->end();
                   label_pos != label_limit;
                   ++label_pos) {
                s_union->add_label(*label_pos, field);
              }
            }
          }

          dissector = s_union;
        }

        void visit (itl::TypeRef& tr) {
          dissector = get_dissector(map, tr.type());
        }
      };

      visitor v(map);
      type->accept(v);

      map[type] = v.dissector;
      return v.dissector;
    }
#endif

    //--------------------------------------------------------------------
    Sample_Manager
    Sample_Manager::instance_;

    Sample_Manager &
    Sample_Manager::instance ()
    {
      return instance_;
    }

    void
    Sample_Manager::init_from_file (const ACE_TCHAR *filename)
    {
#ifndef NO_ITL
      std::ifstream str(filename);
      itl::Dictionary d;
      try {
        d.fromJson(str);
        std::map<itl::Type*, Sample_Dissector*> map;
        for (itl::Dictionary::const_iterator pos = d.begin(), limit = d.end();
             pos != limit;
             ++pos) {
          itl::Alias* a = pos->second;
          dissectors_[a->name()] = get_dissector(map, a->type());
        }
      }
      catch(std::runtime_error e) {
        ACE_DEBUG ((LM_DEBUG,
                    ACE_TEXT("error parsing itl: %s\n"),
                    e.what()));
      }
#endif
    }

    void
    Sample_Manager::init ()
    {
      // - from env get directory for config files use "." by default
      // - use dirent to iterate over all *.ini files in config dir
      // - for each file load the configuration and iterate over sections
      //   - a section label defines a dissectable kind and typeid
      //   - "kind" is ("struct", "seq", "array", "enum", "union", "alias")
      //   -

      const ACE_TCHAR *ini_dir = ACE_OS::getenv (ACE_TEXT("OPENDDS_DISSECTORS"));
      if (ini_dir == 0 || ACE_OS::strlen (ini_dir) == 0)
        ini_dir = ACE_TEXT(".");

      ACE_Dirent directory (ini_dir);
      for (ACE_DIRENT *entry = directory.read();
           entry != 0;
           entry = directory.read())
        {
          const ACE_TCHAR *name = entry->d_name;
          size_t len = ACE_OS::strlen (name);
          if (len < 5) // must be at least "x.itl"
            continue;
          const ACE_TCHAR *ext_pos = ACE_OS::strstr (name, ACE_TEXT(".itl"));
          if (ext_pos == 0)
            continue;
          size_t pos = (ext_pos - name);
          if (pos < len - 4)
            continue;
          std::string path = std::string (ACE_TEXT_ALWAYS_CHAR (ini_dir));
          path += ACE_DIRECTORY_SEPARATOR_STR_A;
          path += ACE_TEXT_ALWAYS_CHAR (name);
          this->init_from_file (ACE_TEXT_CHAR_TO_TCHAR (path.c_str()));
        }
    }

    Sample_Dissector *
    Sample_Manager::find (const char *repo_id)
    {
      std::string key(repo_id);
      const std::string typesupport("TypeSupport");
      const size_t pos = key.find(typesupport);
      if (pos != std::string::npos) {
        key.erase(pos, typesupport.size());
      }

      const std::string prefix("IDL:");
      if (key.find(prefix) != 0) {
        for (size_t iter = key.find("::"); iter != std::string::npos;
             iter = key.find("::", iter)) {
          key.replace(iter, 2, 1, '/');
        }
        key = prefix + key + ":1.0";
      }

      DissectorsType::const_iterator dpos = dissectors_.find(key);
      if (dpos != dissectors_.end()) {
        return dpos->second;
      }
      else {
        return 0;
      }
    }
  }
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
