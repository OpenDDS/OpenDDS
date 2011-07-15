/*
 * $Id$
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
#include <ace/Tokenizer_T.h>

#include <cstring>

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <string>

namespace OpenDDS
{
  namespace DCPS
  {

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
      ACE_Configuration_Heap config;
      ACE_Ini_ImpExp imp(config);

      config.open();

      imp.import_config (filename);
      //ACE_Configuration::VALUETYPE vtype;
      ACE_TString section;
      for (int ndx = 0;
           config.enumerate_sections (config.root_section(), ndx, section) == 0;
           ndx ++)
        {
          ACE_DEBUG ((LM_DEBUG,"at %d got section '%s'\n", ndx, section.c_str()));
          build_type (section, config);
        }

    }

    void
    Sample_Manager::init ()
    {
      builtin_types_.bind ("boolean", Sample_Field::Boolean);
      builtin_types_.bind ("char", Sample_Field::Char);
      builtin_types_.bind ("octet", Sample_Field::Octet);
      builtin_types_.bind ("wchar", Sample_Field::WChar);
      builtin_types_.bind ("short", Sample_Field::Short);
      builtin_types_.bind ("unsigned short", Sample_Field::UShort);
      builtin_types_.bind ("long", Sample_Field::Long);
      builtin_types_.bind ("unsigned long", Sample_Field::ULong);
      builtin_types_.bind ("long long", Sample_Field::LongLong);
      builtin_types_.bind ("unsigned long long", Sample_Field::ULongLong);
      builtin_types_.bind ("float", Sample_Field::Float);
      builtin_types_.bind ("double", Sample_Field::Double);
      builtin_types_.bind ("long double", Sample_Field::LongDouble);
      builtin_types_.bind ("string", Sample_Field::String);
      builtin_types_.bind ("wstring", Sample_Field::WString);

      // - from env get directory for config files use "." by default
      // - use dirent to iterate over all *.ini files in config dir
      // - for each file load the configuration and iterate over sections
      //   - a section label defines a dissectable kind and typeid
      //   - "kind" is ("struct", "seq", "array", "enum", "union", "alias")
      //   -

      const ACE_TCHAR *ini_dir = ACE_OS::getenv ("OPENDDS_DISSECTORS");
      if (ini_dir == 0 || ACE_OS::strlen (ini_dir) == 0)
        ini_dir = ".";

      ACE_Dirent directory (ini_dir);
      for (ACE_DIRENT *entry = directory.read();
           entry != 0;
           entry = directory.read())
        {
          const char *name = entry->d_name;
          size_t len = ACE_OS::strlen (name);
          if (len < 5) // must be at least "x.ini"
            continue;
          const char *ext_pos = ACE_OS::strstr (name, ".ini");
          if (ext_pos == 0)
            continue;
          size_t pos = (ext_pos - name);
          if (pos < len - 4)
            continue;
          init_from_file (name);
        }
    };

    void
    Sample_Manager::add (Sample_Dissector &d)
    {
      std::string &key = d.typeId();
      ACE_DEBUG ((LM_DEBUG,"Adding new dissector for %s\n",key.c_str()));
      dissectors_.bind(key.c_str(),&d);
    }

    Sample_Dissector *
    Sample_Manager::find (const char *data_name)
    {
      std::string key(data_name);
      std::string typesupport ("TypeSupport");
      size_t ts = key.find (typesupport);
      if (ts != std::string::npos)
        {
          std::string version = key.substr (ts + typesupport.length());
          key = key.substr (0,ts);
          key.append (version);
        }

      Sample_Dissector *result = 0;
      dissectors_.find (key.c_str(), result);
      return result;
    }


    //----------------------------------------------------------------------

    void
    Sample_Manager::build_type (ACE_TString &name, ACE_Configuration &config)
    {
      ConfigInfo info;
      ACE_Configuration_Section_Key key;
      info.config_ = &config;
      info.key_ = &key;
      info.name_ = name;

      int status =
        config.open_section (config.root_section(),name.c_str(),0, key);
      if (status != 0)
        {
          ACE_DEBUG ((LM_DEBUG, "build_type could not open section %s\n",
                      name.c_str()));
          return;
        }
      ACE_TString label = name + ".kind";
      status = config.get_string_value (key,label.c_str(),info.kind_);
      if (status != 0)
        {
          ACE_DEBUG ((LM_DEBUG, "section %s does not have a kind value\n",
                      name.c_str()));
          return;
        }

      if (info.kind_.compare ("module") == 0)
        {
          ACE_DEBUG ((LM_DEBUG, "calling build_module for %s\n",
                      info.name_.c_str()));
          build_module (info);
          return;
        }

      label = name + ".typeid";
      status = config.get_string_value (key,label.c_str(),info.type_id_);
      if (status != 0)
        {
          ACE_DEBUG ((LM_DEBUG, "section %s does not have a typeid value\n",
                      name.c_str()));
          return;
        }

      label = name + ".module";

      if (status != 0)
        {
          info.module_ = "";
        }
      else
        {
          build_fqname (info);
        }

      Sample_Dissector *sample = this->find (info.type_id_.c_str());
      if (sample != 0)
        {
          ACE_DEBUG ((LM_DEBUG, "Already configured a type with id %s\n",
                      info.type_id_.c_str()));
        }

      if (info.kind_.compare ("struct") == 0)
        {
          build_struct (info);
          return;
        }
      else if (info.kind_.compare ("sequence") == 0)
        {
          ACE_DEBUG ((LM_DEBUG, "section %s is a sequence\n", name.c_str()));
          return;
        }
      else if (info.kind_.compare ("array") == 0)
        {
          ACE_DEBUG ((LM_DEBUG, "section %s is an array\n", name.c_str()));
          return;
        }
      else if (info.kind_.compare ("enum") == 0)
        {
          ACE_DEBUG ((LM_DEBUG, "section %s is an enum\n", name.c_str()));
          return;
        }
      else if (info.kind_.compare ("union") == 0)
        {
          ACE_DEBUG ((LM_DEBUG, "section %s is a union\n", name.c_str()));
          return;
        }
      else if (info.kind_.compare ("alias") == 0)
        {
          ACE_DEBUG ((LM_DEBUG, "section %s is an alias\n", name.c_str()));
          return;
        }
      else
        {
          ACE_DEBUG ((LM_DEBUG, "section %s is unknown kind %s\n",
                      name.c_str(),info.kind_.c_str()));
          return;
        }
    }

    Sample_Dissector *
    Sample_Manager::fqfind_i (ModuleName *top_level_mn, ACE_TString &name)
    {
      ACE_TString fqname = name;
      for (ModuleName *mn = top_level_mn; mn != 0; mn = mn->parent_)
        {
          fqname =  mn->name_ + "/" + fqname;
        }
      fqname = "IDL:" + fqname + ":1.0";
      Sample_Dissector *sample = find (fqname.c_str());
      if (sample == 0)
        {
          if (top_level_mn != 0)
            return fqfind_i (top_level_mn->parent_, name);
        }
      return sample;
    }

    Sample_Dissector *
    Sample_Manager::fqfind (ACE_TString& parent, ACE_TString &name)
    {
      ModuleName *top_level_mn = 0;
      size_t modsep = name.rfind (';');
      if  (modsep == ACE_TString::npos)
        {
          module_tree_.find(parent.c_str(), top_level_mn);
          return fqfind_i (top_level_mn, name);
        }
      ACE_TString lname = name.substr (modsep);
      if (modsep == 1)
        {
          return fqfind_i (0,lname);
        }
      ACE_TString lmod = name.substr (0,modsep-1);
      modsep = lmod.rfind (':');
      if (modsep != ACE_TString::npos)
        {
          module_tree_.find(lmod.substr(modsep).c_str(), top_level_mn);
        }
      else
        {
          module_tree_.find(lmod.c_str(), top_level_mn);
        }
      return fqfind_i (top_level_mn, lname);
    }

    void
    Sample_Manager::build_fqname (ConfigInfo &info)
    {
      ModuleName *mn = 0;
      if (!info.module_.empty() &&
          module_tree_.find(info.module_.c_str(), mn) != 0)
        {
          this->build_type(info.module_, *info.config_);
          if (module_tree_.find (info.module_.c_str(), mn) != 0)
            {
              ACE_DEBUG ((LM_DEBUG, "build_fqname: Unable to build module tree, "
                          "parent %s of %s missing\n",
                          info.module_.c_str(), info.name_.c_str()));
              return;
            }
        }
      while (mn != 0)
        {
          info.name_ =  mn->name_ + "::" + info.name_;
          mn = mn->parent_;
        }
    }

    void
    Sample_Manager::build_module (ConfigInfo &info)
    {

      ACE_TString label = info.name_ + ".module";
      ACE_TString parent;
      ModuleName *mn = 0;
      if (info.config_->get_string_value (*info.key_,label.c_str(),parent) == 0)
        {
          if (module_tree_.find(parent.c_str(), mn) != 0)
            {
              this->build_type(parent, *info.config_);
              int result = module_tree_.find (parent.c_str(), mn);
              if (result != 0)
                {
                  ACE_DEBUG ((LM_DEBUG, "build_module: Unable to build module tree, "
                              "parent %s of %s missing, result = %d\n",
                              parent.c_str(), info.name_.c_str(), result));
                  return;
                }
            }
        }

      ModuleName *mod = new ModuleName;
      mod->name_ = info.name_;
      mod->parent_ = mn;
      module_tree_.bind (mod->name_.c_str(), mod);
    }

    void
    Sample_Manager::build_struct (ConfigInfo &info)
    {
      Sample_Dissector *sample =
        new Sample_Dissector (info.type_id_.c_str(), info.name_.c_str());
      Sample_Field *f = 0;


      ACE_TString label = info.name_ + ".order";
      ACE_TString order;
      if (info.config_->get_string_value (*info.key_,label.c_str(),order))
        return;

      ACE_Tokenizer_T<ACE_TCHAR> tok (order.rep());
      tok.delimiter_replace (' ', 0);
      for (ACE_TCHAR *p = tok.next(); p; p = tok.next())
        {
          ACE_TString type_str;
          info.config_->get_string_value (*info.key_, p, type_str);
          ACE_DEBUG ((LM_DEBUG, "field = %s is kind %s\n",
                      p, type_str.c_str()));
          Sample_Field::IDLTypeID field_type = Sample_Field::Undefined;
          if (builtin_types_.find(type_str.c_str(), field_type) == 0)
            f = sample->add_field (field_type, p);
          else
            {
              Sample_Dissector *value = fqfind (info.module_, type_str);
              if (value == 0)
                {
                  build_type (type_str, *info.config_);
                  value = fqfind (info.module_, type_str);
                }
              if (value != 0)
                f = sample->add_field (value, p);
            }
        }
    }

    void
    Sample_Manager::build_sequence (ConfigInfo &info)
    {
      ACE_TString label = info.name_ + ".element";
      ACE_TString type_str;
      if (info.config_->get_string_value (*info.key_,label.c_str(),type_str))
        return;
      Sample_Field::IDLTypeID type_id = Sample_Field::Undefined;
      if (builtin_types_.find(type_str.c_str(), type_id) == 0)
        {
          new Sample_Sequence (info.type_id_.c_str(), type_id);
          return;
        }
      Sample_Dissector *value = fqfind (info.module_, type_str);
      if (value == 0)
        {
          build_type (type_str, *info.config_);
          value = fqfind (info.module_, type_str);
        }
      if (value != 0)
        new Sample_Sequence (info.type_id_.c_str(), value);
    }

    void
    Sample_Manager::build_array (ConfigInfo &info)
    {
      ACE_TString label = info.name_ + ".size";
      u_int size = 0;
      if (info.config_->get_integer_value (*info.key_,label.c_str(),size))
        return;

      label = info.name_ + ".element";
      ACE_TString type_str;
      if (info.config_->get_string_value (*info.key_,label.c_str(),type_str))
        return;

      Sample_Field::IDLTypeID type_id = Sample_Field::Undefined;
      if (builtin_types_.find(type_str.c_str(), type_id) == 0)
        {
          new Sample_Array (info.type_id_.c_str(), (size_t)size, type_id);
          return;
        }
      Sample_Dissector *value = fqfind (info.module_, type_str);
      if (value == 0)
        {
          build_type (type_str, *info.config_);
          value = fqfind (info.module_, type_str);
        }
      if (value != 0)
        new Sample_Array (info.type_id_.c_str(), size, value);
    }

    void
    Sample_Manager::build_enum (ConfigInfo &info)
    {
      ACE_TString label = info.name_ + ".order";
      ACE_TString order;
      if (info.config_->get_string_value (*info.key_,label.c_str(),order))
        return;

      Sample_Enum *sample =
        new Sample_Enum (info.type_id_.c_str());

      ACE_Tokenizer_T<ACE_TCHAR> tok (order.rep());
      tok.delimiter_replace (' ', 0);
      for (ACE_TCHAR *p = tok.next(); p; p = tok.next())
        {
          sample->add_value (p);
        }
    }

    void
    Sample_Manager::build_union (ConfigInfo &info)
    {
      ACE_TString label = info.name_ + ".order";
      ACE_TString order;
      ACE_TString case_name;
      ACE_TString case_type;

      Sample_Field *f = 0;

      if (info.config_->get_string_value (*info.key_,label.c_str(),order))
        return;

      label = info.name_ + ".discriminator";
      if (info.config_->get_string_value (*info.key_,label.c_str(),case_type))
        return;

      Sample_Union *s_union = new Sample_Union (info.type_id_.c_str());

      Sample_Field::IDLTypeID type_id = Sample_Field::Undefined;
      if (builtin_types_.find(case_type.c_str(), type_id) == 0)
        {
          s_union->discriminator (type_id);
        }
      Sample_Dissector *value = fqfind (info.module_, case_type);
      if (value == 0)
        {
          build_type (case_type, *info.config_);
          value = fqfind (info.module_, case_type);
        }
      if (value != 0)
        s_union->discriminator (value);

      label = "default.type";
      if (info.config_->get_string_value (*info.key_,label.c_str(),case_type) == 0)
        {
          label = "default.name";
          info.config_->get_string_value (*info.key_, label.c_str(), case_name);
          if (builtin_types_.find(case_type.c_str(), type_id) == 0)
            f = new Sample_Field (type_id, case_name.c_str());
          else
            {
              Sample_Dissector *value = fqfind (info.module_, case_type);
              if (value == 0)
                {
                  build_type (case_type, *info.config_);
                  value = fqfind (info.module_, case_type);
                }
              if (value != 0)
                f = new Sample_Field (value, case_name.c_str());
            }
          s_union->add_default (f);
        }

      ACE_Tokenizer_T<ACE_TCHAR> tok (order.rep());
      tok.delimiter_replace (' ', 0);
      Switch_Case *sc = 0;
      bool ranged = false;
      for (ACE_TCHAR *p = tok.next(); p; p = tok.next())
        {
          label = ACE_TString(p) + ".type";
          bool new_range =
            (info.config_->get_string_value (*info.key_,
                                             label.c_str(),
                                             case_type) != 0);
          if (!new_range)
            {
              label = ACE_TString(p) + ".name";
              info.config_->get_string_value (*info.key_,
                                              label.c_str(),
                                              case_name);
            }

          type_id = Sample_Field::Undefined;
          f = 0;
          if (!new_range)
            {
              if (builtin_types_.find(case_type.c_str(), type_id) == 0)
                f = new Sample_Field (type_id, case_name.c_str());
              else
                {
                  Sample_Dissector *value = fqfind (info.module_, case_type);
                  if (value == 0)
                    {
                      build_type (case_type, *info.config_);
                      value = fqfind (info.module_, case_type);
                    }
                  if (value != 0)
                    f = new Sample_Field (value, case_name.c_str());
                }
            }
          if (ranged)
            {
              sc = sc->add_range (p, f);
            }
          else
            {
              sc = sc == 0 ? s_union->add_case (p, f) : sc->chain (p, f);
            }
          ranged = new_range;
        }
    }

    void
    Sample_Manager::build_alias (ConfigInfo &info)
    {
      ACE_TString label = info.name_ + ".base";
      ACE_TString type_str;
      if (info.config_->get_string_value (*info.key_,label.c_str(),type_str))
        return;
      Sample_Field::IDLTypeID type_id = Sample_Field::Undefined;
      if (builtin_types_.find(type_str.c_str(), type_id) == 0)
        {
          new Sample_Alias (info.type_id_.c_str(), type_id);
          return;
        }
      Sample_Dissector *value = fqfind (info.module_, type_str);
      if (value == 0)
        {
          build_type (type_str, *info.config_);
          value = fqfind (info.module_, type_str);
        }
      if (value != 0)
        new Sample_Alias (info.type_id_.c_str(), value);

    }
  }
}
