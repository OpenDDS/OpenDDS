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

#include <cstring>

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <string>

namespace OpenDDS
{
  namespace DCPS
  {


    EntityBase::EntityBase(std::string &n, EntityContext *p)
      : name_ (n),
        fqname_ (),
        idl_name_ (),
        parent_ (p)
    {
      if (p->name_ == "")
        {
          fqname_ = name_;
          idl_name_ = name_;
        }
      else
        {
          fqname_ = parent_->fqname_ + '\\' + name_;
          idl_name_ = parent_->idl_name_ + "::" + name_;
        }
    }

    EntityBase *
    EntityContext::node_at (std::string &path)
    {
      EntityBase *node = 0;
      size_t seppos = path.find ('\\');
      EntityMap::iterator iter;
      if (seppos == std::string::npos)
        {
          iter = this->children_.find(path);
          if (iter != this->children_.end())
            node = iter->second;
        }
      else
        {
          std::string basepath = path.substr (0,seppos);
          iter = this->children_.find (basepath);
          if (iter != this->children_.end())
            {
              node = iter->second;
              EntityContext *ctx = dynamic_cast<EntityContext *>(node);
              if (ctx != 0)
                {
                  std::string remainder = path.substr (seppos + 1);
                  return ctx->node_at (remainder);
                }
            }
        }
      return node;
    }

    //--------------------------------------------------------------------
    Sample_Manager
    Sample_Manager::instance_;

    Sample_Manager &
    Sample_Manager::instance ()
    {
      return instance_;
    }

    void
    Sample_Manager::find_config_sections (ACE_Configuration *config,
                                          const ACE_Configuration_Section_Key &base,
                                          EntityContext *parent)
    {
      ACE_TString section;
      for (int ndx = 0;
           config->enumerate_sections (base, ndx, section) == 0;
           ndx ++)
        {
          ACE_DEBUG ((LM_DEBUG,"at %d got section '%s'\n",
                      ndx, section.c_str()));

          ACE_Configuration_Section_Key key;
          int status =
            config->open_section (base, section.c_str(), 0, key);

          if (status != 0)
            {
              ACE_DEBUG ((LM_DEBUG, "could not open section %s\n",
                          section.c_str()));
              continue;
            }

          std::string name(section.c_str());
          if (config->enumerate_sections (key, 0, section) != 0)
            {
              ACE_DEBUG ((LM_DEBUG, "found leaf %s\n", name.c_str()));
              EntityBase *leaf  = new EntityNode (name,parent, config);
              parent->children_[name] = leaf;
            }
          else
            {
              ACE_DEBUG ((LM_DEBUG, "found context %s\n", name.c_str()));
              EntityBase *base = parent->node_at(name);
              EntityContext *ctx;
              if (base == 0)
                {
                  ctx = new EntityContext (name, parent);
                  parent->children_[name] = ctx;
                }
              else
                {
                  ctx = dynamic_cast<EntityContext *>(base);
                }
              find_config_sections (config, key, ctx);
            }
        }
    }

    void
    Sample_Manager::load_entities (EntityContext *entities)
    {
      for (EntityMap::iterator iter = entities->children_.begin();
           iter != entities->children_.end();
           iter++)
        {
          EntityBase *base = iter->second;
          EntityContext *ctx = dynamic_cast<EntityContext *>(base);
          if (ctx != 0)
            {
              this->load_entities (ctx);
            }
          else
            {
              EntityNode *node = dynamic_cast<EntityNode *>(base);
              if (node->dissector_ == 0)
                this->build_type (node);
            }
        }
    }

    void
    Sample_Manager::init_from_file (const ACE_TCHAR *filename)
    {
      ACE_Configuration_Heap *heap = new ACE_Configuration_Heap;
      ACE_Ini_ImpExp imp(*heap);

      heap->open();
      imp.import_config (filename);

      ACE_Configuration *config = heap;

      this->find_config_sections (config,
                                  config->root_section(),
                                  this->entities_);
    }

    void
    Sample_Manager::init ()
    {
      this->builtin_types_["boolean"] = Sample_Field::Boolean;
      this->builtin_types_["char"] = Sample_Field::Char;
      this->builtin_types_["octet"] = Sample_Field::Octet;
      this->builtin_types_["wchar"] = Sample_Field::WChar;
      this->builtin_types_["short"] = Sample_Field::Short;
      this->builtin_types_["unsigned short"] = Sample_Field::UShort;
      this->builtin_types_["long"] = Sample_Field::Long;
      this->builtin_types_["unsigned long"] = Sample_Field::ULong;
      this->builtin_types_["long long"] = Sample_Field::LongLong;
      this->builtin_types_["unsigned long long"] = Sample_Field::ULongLong;
      this->builtin_types_["float"] = Sample_Field::Float;
      this->builtin_types_["double"] = Sample_Field::Double;
      this->builtin_types_["long double"] = Sample_Field::LongDouble;
      this->builtin_types_["string"] = Sample_Field::String;
      this->builtin_types_["wstring"] = Sample_Field::WString;

      this->entities_ = new EntityContext;

      // - from env get directory for config files use "." by default
      // - use dirent to iterate over all *.ini files in config dir
      // - for each file load the configuration and iterate over sections
      //   - a section label defines a dissectable kind and typeid
      //   - "kind" is ("struct", "seq", "array", "enum", "union", "alias")
      //   -

      const ACE_TCHAR *ini_dir = ACE_OS::getenv ("OPENDDS_DISSECTORS");
      if (ini_dir == 0 || ACE_OS::strlen (ini_dir) == 0)
        ini_dir = ".";
      ACE_DEBUG ((LM_DEBUG,"reading files from %s\n", ini_dir));

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
          std::string path = std::string (ini_dir) + ACE_DIRECTORY_SEPARATOR_CHAR + name;
          this->init_from_file (path.c_str());
        }

      this->load_entities (this->entities_);
    }

    Sample_Dissector *
    Sample_Manager::find (const char *repo_id)
    {
      std::string key(repo_id);
      // some requests come in for the "TypeSupport" helper object,
      // If this is a type suppor tobject, pull that off as well as the
      // version identifier
      std::string typesupport ("TypeSupport");
      size_t pos = key.find (typesupport);
      if (pos == std::string::npos)
        {
          pos = key.rfind (':');
        }
      key = key.substr (0,pos);

      if (key.substr(0,4).compare ("IDL:") == 0)
        {
          key = key.substr (4);
        }
      pos = 0;
      while ((pos = key.find ('/',pos)) != std::string::npos)
        {
          key[pos++] = '\\';
        }
      ACE_DEBUG ((LM_DEBUG,"find: repo_id = %s, key = %s\n", repo_id, key.c_str()));
      Sample_Dissector *d = this->fqfind (key, entities_);
      return d;
    }

    //----------------------------------------------------------------------

    void
    Sample_Manager::build_type (EntityNode *node)
    {
      ACE_Configuration_Section_Key key;
      int status =
        node->config_->open_section (node->config_->root_section(),
                                     node->fqname_.c_str(),
                                     0,
                                     key);
      if (status != 0)
        {
          ACE_DEBUG ((LM_DEBUG,
                      "build_type could not open section for node %s\n",
                      node->fqname_.c_str()));
          return;
        }

      std::string label = node->name_ + ".kind";
      ACE_TString kind;
      status =
        node->config_->get_string_value (key,label.c_str(), kind);
      if (status != 0)
        {
          ACE_DEBUG ((LM_DEBUG, "section %s does not have a kind value\n",
                      node->name_.c_str()));
          return;
        }

      label = node->name_ + ".repoid";
      ACE_TString tid;
      status =
        node->config_->get_string_value (key,label.c_str(), tid);
      if (status != 0)
        {
          ACE_DEBUG ((LM_DEBUG, "section %s does not have a repoid value\n",
                      node->name_.c_str()));
          return;
        }
      node->type_id_ = tid.c_str();

      if (kind.compare ("struct") == 0)
        {
          ACE_DEBUG ((LM_DEBUG, "section %s is a struct\n",
                      node->name_.c_str()));
          build_struct (node, key);
          return;
        }
      else if (kind.compare ("sequence") == 0)
        {
          ACE_DEBUG ((LM_DEBUG, "section %s is a sequence\n",
                      node->name_.c_str()));
          build_sequence (node, key);
          return;
        }
      else if (kind.compare ("array") == 0)
        {
          ACE_DEBUG ((LM_DEBUG, "section %s is an array\n",
                      node->name_.c_str()));
          build_array (node, key);
          return;
        }
      else if (kind.compare ("enum") == 0)
        {
          ACE_DEBUG ((LM_DEBUG, "section %s is an enum\n",
                      node->name_.c_str()));
          build_enum (node, key);
          return;
        }
      else if (kind.compare ("union") == 0)
        {
          ACE_DEBUG ((LM_DEBUG, "section %s is a union\n",
                      node->name_.c_str()));
          build_union (node, key);
          return;
        }
      else if (kind.compare ("alias") == 0)
        {
          ACE_DEBUG ((LM_DEBUG, "section %s is an alias\n",
                      node->name_.c_str()));
          build_alias (node, key);
          return;
        }
      else
        {
          ACE_DEBUG ((LM_DEBUG, "section %s is unknown kind %s\n",
                      node->name_.c_str(),kind.c_str()));
          return;
        }
    }

    Sample_Dissector *
    Sample_Manager::fqfind (std::string idlname, EntityContext *context)
    {
      EntityContext *ctx = context;
      size_t pos = idlname.find ("::");
      if (pos == 0)
        {
          ctx = this->entities_;
          idlname = idlname.substr(2);
          pos = idlname.find ("::");
        }
      std::string path;
      bool addsep = false;
      while (pos != std::string::npos)
        {
          if (addsep)
            path += '\\';
          path += idlname.substr (0,pos);
          idlname = idlname.substr (pos + 2);
          addsep = true;
          pos = idlname.find ("::");
        }
      if (addsep)
        path += '\\';
      path += idlname;

      // find the node associated with the name
      //
      EntityBase *base = 0;
      while (base == 0 && ctx != 0)
        {
          base = ctx->node_at(path);
          ctx = ctx->parent_;
        }
      if (base == 0)
        {
          ACE_DEBUG ((LM_DEBUG,"could not find node for %s\n", path.c_str()));
          return 0;
        }
      EntityNode *node = dynamic_cast<EntityNode *>(base);
      if (node->dissector_ == 0)
        {
          build_type (node);
        }
      return node->dissector_;
    }

    void
    Sample_Manager::build_struct (EntityNode *node,
                                  const ACE_Configuration_Section_Key &key)
    {
      node->dissector_ = new Sample_Dissector (node->type_id_.c_str(),
                                               node->name_.c_str());
      Sample_Field *f = 0;

      std::string label = node->name_ + ".order";
      ACE_TString order;
      if (node->config_->get_string_value (key,label.c_str(),order))
        return;

      std::string tokenizer = order.c_str();
      size_t pos = tokenizer.find (' ');
      std::string p =
        (pos == std::string::npos) ? tokenizer : tokenizer.substr (0,pos);
      while (!p.empty())
        {

          ACE_TString type_str;
          node->config_->get_string_value (key, p.c_str(), type_str);
          std::string kind(type_str.c_str());

          BuiltinTypeMap::iterator iter = builtin_types_.find(kind);
          if (iter != builtin_types_.end())
            f = node->dissector_->add_field (iter->second, p.c_str());
          else
            {
              Sample_Dissector *value = this->fqfind (kind, node->parent_);
              if (value != 0)
                f = node->dissector_->add_field (value, p.c_str());
            }


          if (pos == std::string::npos)
            p.clear();
          else
            {
              tokenizer = tokenizer.substr (pos+1);
              pos = tokenizer.find(' ');
              p = (pos == std::string::npos) ?
                tokenizer :
                tokenizer.substr (0,pos);
            }
        }

    }

    void
    Sample_Manager::build_sequence (EntityNode *node,
                                    const ACE_Configuration_Section_Key &key)
    {
      std::string label = node->name_ + ".element";
      ACE_TString type_str;
      if (node->config_->get_string_value (key,label.c_str(),type_str))
        return;

      Sample_Dissector *sequence = 0;
      std::string kind(type_str.c_str());
      BuiltinTypeMap::iterator iter = builtin_types_.find(kind);
      if (iter != builtin_types_.end())
        {
          sequence = new Sample_Sequence (node->type_id_.c_str(), iter->second);
        }
      else
        {
          Sample_Dissector *value = this->fqfind (kind, node->parent_);
          if (value != 0)
            sequence = new Sample_Sequence (node->type_id_.c_str(), value);
        }
      node->dissector_ = sequence;
    }


    void
    Sample_Manager::build_alias (EntityNode *node,
                                 const ACE_Configuration_Section_Key &key)
    {
      std::string label = node->name_ + ".base";
      ACE_TString type_str;
      if (node->config_->get_string_value (key, label.c_str(),type_str))
        return;

      Sample_Dissector *alias = 0;
      std::string kind(type_str.c_str());
      BuiltinTypeMap::iterator iter = builtin_types_.find(kind);
      if (iter != builtin_types_.end())
        {
          alias = new Sample_Alias (node->type_id_.c_str(), iter->second);
        }
      else
        {
          Sample_Dissector *value = this->fqfind (kind, node->parent_);
          if (value != 0)
            alias = new Sample_Alias (node->type_id_.c_str(), value);
        }
      node->dissector_ = alias;
    }


    void
    Sample_Manager::build_array (EntityNode *node,
                                 const ACE_Configuration_Section_Key &key)
    {
      std::string label = node->name_ + ".size";
      u_int size = 0;
      if (node->config_->get_integer_value (key,label.c_str(),size))
        return;

      label = node->name_ + ".element";
      ACE_TString type_str;
      if (node->config_->get_string_value (key,label.c_str(),type_str))
        return;

      Sample_Dissector *array = 0;
      std::string kind(type_str.c_str());
      BuiltinTypeMap::iterator iter = builtin_types_.find(kind);
      if (iter != builtin_types_.end())
        {
          array = new Sample_Array (node->type_id_.c_str(),
                                    (size_t)size,
                                    iter->second);
        }
      else
        {
          Sample_Dissector *value = this->fqfind (kind, node->parent_);
          if (value != 0)
            array = new Sample_Array (node->type_id_.c_str(),
                                      (size_t)size,
                                      value);
        }
      node->dissector_ = array;
    }

    void
    Sample_Manager::build_enum (EntityNode *node,
                                const ACE_Configuration_Section_Key &key)
    {
      std::string label = node->name_ + ".order";
      ACE_TString order;
      if (node->config_->get_string_value (key,label.c_str(),order))
        return;

      Sample_Enum *sample =
        new Sample_Enum (node->type_id_.c_str());

      std::string tokenizer = order.c_str();
      size_t pos = tokenizer.find (' ');
      std::string p =
        (pos == std::string::npos) ? tokenizer : tokenizer.substr (0,pos);
      while (!p.empty())
        {
          sample->add_value (p.c_str());
          if (pos == std::string::npos)
            p.clear();
          else
            {
              tokenizer = tokenizer.substr (pos+1);
              pos = tokenizer.find(' ');
              p = (pos == std::string::npos) ?
                tokenizer :
                tokenizer.substr (0,pos);
            }
        }
      node->dissector_ = sample;
    }

    void
    Sample_Manager::build_union (EntityNode *node,
                                 const ACE_Configuration_Section_Key &key)
    {
      std::string label = node->name_ + ".order";
      ACE_TString order;
      ACE_TString case_name;
      ACE_TString case_type;
      Sample_Dissector *value = 0;
      Sample_Field *f = 0;

      if (node->config_->get_string_value (key,label.c_str(),order))
        return;

      label = node->name_ + ".discriminator";
      if (node->config_->get_string_value (key,label.c_str(),case_type))
        return;

      Sample_Union *s_union = new Sample_Union (node->type_id_.c_str());

      BuiltinTypeMap::iterator iter =
        builtin_types_.find(std::string(case_type.c_str()));
      if (iter != builtin_types_.end())
        {
          s_union->discriminator (iter->second);
        }
      value = fqfind (std::string(case_type.c_str()), node->parent_);
      if (value != 0)
        s_union->discriminator (value);

      label = "default.type";
      if (node->config_->get_string_value (key,label.c_str(),case_type) == 0)
        {
          label = "default.name";
          node->config_->get_string_value (key, label.c_str(), case_name);

          iter = builtin_types_.find(std::string(case_type.c_str()));
          if (iter != builtin_types_.end())
            f = new Sample_Field (iter->second, case_name.c_str());
          else
            {
              value = fqfind (std::string(case_type.c_str()), node->parent_);
              if (value != 0)
                f = new Sample_Field (value, case_name.c_str());
            }
          s_union->add_default (f);
        }

      Switch_Case *sc = 0;
      bool ranged = false;
      std::string tokenizer = order.c_str();
      size_t pos = tokenizer.find (' ');
      std::string p =
        (pos == std::string::npos) ? tokenizer : tokenizer.substr (0,pos);
      while (!p.empty())
        {
          label = p + ".type";
          bool new_range =
            (node->config_->get_string_value (key,
                                              label.c_str(),
                                              case_type) != 0);
          if (!new_range)
            {
              label = p + ".name";
              node->config_->get_string_value (key,
                                               label.c_str(),
                                               case_name);
            }


          f = 0;
          if (!new_range)
            {
              iter = builtin_types_.find(std::string(case_type.c_str()));
              if (iter != builtin_types_.end())
                f = new Sample_Field (iter->second, case_name.c_str());
              else
                {
                 value = fqfind (std::string(case_type.c_str()), node->parent_);
                  if (value != 0)
                    f = new Sample_Field (value, case_name.c_str());
                }
            }
          if (ranged)
            {
              sc = sc->add_range (p.c_str(), f);
            }
          else
            {
              sc = sc == 0 ?
                s_union->add_case (p.c_str(), f) :
                sc->chain (p.c_str(), f);
            }
          ranged = new_range;
          if (pos == std::string::npos)
            p.clear();
          else
            {
              tokenizer = tokenizer.substr (pos+1);
              pos = tokenizer.find(' ');
              p = (pos == std::string::npos) ?
                tokenizer :
                tokenizer.substr (0,pos);
            }
        }

      node->dissector_ = s_union;
    }

  }
}
