/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "sample_manager.h"

#include "ws_common.h"

#ifndef NO_ITL
#  include <itl/itl.hpp>
#endif

#include <dds/DCPS/DirentWrapper.h>

#include <ace/Basic_Types.h>
#include <ace/CDR_Base.h>
#include <ace/Message_Block.h>
#include <ace/Log_Msg.h>
#include <ace/ACE.h>
#include <ace/Configuration.h>
#include <ace/Configuration_Import_Export.h>

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <string>
#include <list>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS
{
  namespace DCPS
  {

#ifndef NO_ITL
    typedef std::map<itl::Type*, Sample_Dissector*> ITL_Map;

    Sample_Dissector* get_dissector(ITL_Map& map, itl::Type* type);

    struct visitor : public itl::TypeVisitor {
      ITL_Map& map;
      Sample_Dissector* dissector;

      explicit visitor(ITL_Map& m) :
        map(m),
        dissector(0)
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

      void visit (itl::Fixed& fixed) {
#ifndef ACE_HAS_CDR_FIXED
        ACE_DEBUG((LM_WARNING, ACE_TEXT(
          "A Fixed type has been specified but ACE is missing Fixed.\n"
        )));
#endif
        dissector = new Sample_Fixed(fixed.digits(), fixed.scale());
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
            if (type.IsString() && type.GetString() == std::string("wstring")) {
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
        Sample_Union* s_union = new Sample_Union();
        s_union->discriminator(get_dissector(map, u.discriminator()));
        for (itl::Union::const_iterator pos = u.begin(), limit = u.end();
             pos != limit;
             ++pos) {
          if (pos->labels.empty()) {
            s_union->add_default(new Sample_Field(
              get_dissector(map, pos->type), pos->name));
            break;
          }
        }

        for (itl::Union::const_iterator pos = u.begin(), limit = u.end();
             pos != limit;
             ++pos) {
          if (!pos->labels.empty()) {
            Sample_Field* field = s_union->add_field(
              get_dissector(map, pos->type), pos->name);
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

    Sample_Dissector* get_dissector(ITL_Map& map, itl::Type* type)
    {
      if (map.find(type) != map.end()) {
        return map[type];
      }

      visitor v(map);
      type->accept(v);

      map[type] = v.dissector;
      return v.dissector;
    }
#endif // NO_ITL

    //--------------------------------------------------------------------

    Sample_Manager::~Sample_Manager()
    {
      if (hf_array_ != NULL) {
        delete [] hf_array_;
      }
    }

    Sample_Manager Sample_Manager::instance_;

    Sample_Manager& Sample_Manager::instance()
    {
      return instance_;
    }

    void Sample_Manager::init_from_file(const ACE_TCHAR* filename)
    {
#ifdef NO_ITL
      ACE_UNUSED_ARG(filename);
#else
      std::ifstream str(filename);
      itl::Dictionary d;
      bool no_dcps_data_types = true;

      /*
       * To reduce the number of fields we create in Wireshark, we just
       * want to initialize fields off based on the primary data types as
       * the bases. The TAO IDL compiler will mark them in ITL files after
       * OpenDDS 3.12. ITL files created at and before 3.12 will work but
       * will populate Wireshark with more fields in the auto complete than
       * necessary.
       */
      Dissector_Map primary_dissectors;

      Dissector_Map dissectors_to_add;

      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("Found Dissector ITL File: %C\n"),
        filename));
      try {
        d.fromJson(str);
        ITL_Map map;
        for (itl::Dictionary::const_iterator pos = d.begin(), limit = d.end();
             pos != limit;
             ++pos) {
          itl::Alias* a = pos->second;
          Sample_Dissector* sd = get_dissector(map, a->type());
          // Check if the dissector is for a primary data type
          const rapidjson::Value* note = a->note();
          if (
            note != NULL &&
            note->HasMember("is_dcps_data_type") &&
            (*note)["is_dcps_data_type"].IsBool()
          ) {
            if ((*note)["is_dcps_data_type"].GetBool()) {
                primary_dissectors[a->name()] = sd;
                no_dcps_data_types = false;
            }
            sd->mark_struct();
          }
          ACE_DEBUG((LM_DEBUG,
            ACE_TEXT("    Data Type: %C\n"),
            a->name().c_str()));
          dissectors_to_add[a->name()] = sd;
        }
      }
      catch(std::runtime_error & e) {
        ACE_DEBUG ((LM_DEBUG,
                    ACE_TEXT("error parsing itl: %C\n"),
                    e.what()));
      }

      if (no_dcps_data_types) {
        ACE_DEBUG((LM_WARNING,
                   ACE_TEXT(
                     "%C has no types defined as a primary DCPS data types.\n"),
                   filename));
        primary_dissectors = dissectors_to_add; // Evaluate All Dissectors
      }

      // Build WS Namespace and Fields
      try {
        // First Pass
        for (
          Dissector_Map::iterator i = primary_dissectors.begin();
          i != primary_dissectors.end();
          i++
        ) {
          // Get Namespace from name
          Sample_Base::push_idl_name(i->first);

          // Create WS Fields off this namespace location
          Sample_Dissector & dissector = *i->second;
          dissector.init_ws_proto_tree(true); // first pass, evaluate tree
          Sample_Base::clear_ns();
        }

        // Second Pass
        for (
          Dissector_Map::iterator i = primary_dissectors.begin();
          i != primary_dissectors.end();
          i++
        ) {
          // Get Namespace from name
          Sample_Base::push_idl_name(i->first);

          // second pass, collect fields to pass to Wireshark
          i->second->init_ws_proto_tree(false);
          Sample_Base::clear_ns();
        }

      } catch (const Sample_Dissector_Error & e) {
        ACE_DEBUG((LM_ERROR,
          ACE_TEXT("Error during dissector initialization of %C: %C\n"),
          filename, e.what()
        ));

        // Any cleanup work would be done here

        return;
      }

      // Add to pool of dissectors
      for (
        Dissector_Map::iterator i = primary_dissectors.begin();
        i != primary_dissectors.end();
        i++
      ) {
        dissectors_[i->first] = i->second;
      }

#endif
    }

    void Sample_Manager::init()
    {
      hf_array_ = NULL;

#ifndef NO_ITL
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
#endif // NO_ITL
    }

    Sample_Dissector* Sample_Manager::find(const char *repo_id)
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

      Dissector_Map::const_iterator dpos = dissectors_.find(key);
      if (dpos != dissectors_.end()) {
        return dpos->second;
      } else {
        return 0;
      }
    }

    void Sample_Manager::add_protocol_field(const hf_register_info& field) {
      hf_vector_.push_back(field);
    }

    size_t Sample_Manager::number_of_fields() {
      return hf_vector_.size();
    }

    hf_register_info* Sample_Manager::fields_array() {
      if (hf_array_ == NULL) {
        hf_array_ = new hf_register_info[number_of_fields()];
        std::copy(hf_vector_.begin(), hf_vector_.end(), &hf_array_[0]);
      }
      return hf_array_;
    }
  }
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL
