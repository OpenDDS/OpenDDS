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

    int
    Sample_Manager::get_string_value (ACE_Configuration *config,
                                      const ACE_Configuration_Section_Key &base,
                                      const std::string &label,
                                      std::string &value)
    {
      ACE_TString tval;
      int result=
        config->get_string_value (base,
                                  ACE_TEXT_CHAR_TO_TCHAR(label.c_str()),
                                  tval);
      if (result == 0)
        value = ACE_TEXT_ALWAYS_CHAR (tval.c_str());
      return result;
    }

    int
    Sample_Manager::get_int_value (ACE_Configuration *config,
                                   const ACE_Configuration_Section_Key &base,
                                   const std::string &label,
                                   u_int &value)
    {
      return config->get_integer_value (base,
                                        ACE_TEXT_CHAR_TO_TCHAR(label.c_str()),
                                        value);
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
          ACE_Configuration_Section_Key key;
          int status =
            config->open_section (base, section.c_str(), 0, key);

          if (status != 0)
            {
              continue;
            }

          std::string name(ACE_TEXT_ALWAYS_CHAR (section.c_str()));
          if (config->enumerate_sections (key, 0, section) != 0)
            {
              EntityBase *leaf  = new EntityNode (name,parent, config);
              parent->children_[name] = leaf;
            }
          else
            {
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

      this->builtin_types_["CORBA::Boolean"] = Sample_Field::Boolean;
      this->builtin_types_["CORBA::Char"] = Sample_Field::Char;
      this->builtin_types_["CORBA::Octet"] = Sample_Field::Octet;
      this->builtin_types_["CORBA::Wchar"] = Sample_Field::WChar;
      this->builtin_types_["CORBA::Short"] = Sample_Field::Short;
      this->builtin_types_["CORBA::UShort"] = Sample_Field::UShort;
      this->builtin_types_["CORBA::Long"] = Sample_Field::Long;
      this->builtin_types_["CORBA::ULong"] = Sample_Field::ULong;
      this->builtin_types_["CORBA::LongLong"] = Sample_Field::LongLong;
      this->builtin_types_["CORBA::ULongLong"] = Sample_Field::ULongLong;
      this->builtin_types_["CORBA::Float"] = Sample_Field::Float;
      this->builtin_types_["CORBA::Double"] = Sample_Field::Double;
      this->builtin_types_["CORBA::LongDouble"] = Sample_Field::LongDouble;
      this->builtin_types_["CORBA::String"] = Sample_Field::String;
      this->builtin_types_["CORBA::Wstring"] = Sample_Field::WString;

      this->builtin_types_["char *"] = Sample_Field::String;

      this->entities_ = new EntityContext;

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
          if (len < 5) // must be at least "x.ini"
            continue;
          const ACE_TCHAR *ext_pos = ACE_OS::strstr (name, ACE_TEXT(".ini"));
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
      Sample_Dissector *d = this->fqfind (key, entities_);
      return d;
    }

    //----------------------------------------------------------------------

    void
    Sample_Manager::build_type (EntityNode *node)
    {
      ACE_Configuration_Section_Key key;
      const ACE_TCHAR * tname = ACE_TEXT_CHAR_TO_TCHAR(node->fqname_.c_str());
      int status =
        node->config_->open_section (node->config_->root_section(),
                                     tname,
                                     0,
                                     key);
      if (status != 0)
        {
          ACE_DEBUG ((LM_DEBUG,
                      ACE_TEXT("build_type could not open section for node %s\n"),
                      node->fqname_.c_str()));
          return;
        }

      std::string label = node->name_ + ".kind";
      std::string kind;
      status = get_string_value (node->config_, key, label, kind);
      if (status != 0)
        {
          ACE_DEBUG ((LM_DEBUG,
                      ACE_TEXT("section %s does not have a kind value\n"),
                      node->name_.c_str()));
          return;
        }

      label = node->name_ + ".repoid";
      std::string tid;
      status =
        get_string_value (node->config_, key,label, tid);
      if (status != 0)
        {
          ACE_DEBUG ((LM_DEBUG,
                      ACE_TEXT("section %s does not have a repoid value\n"),
                      node->name_.c_str()));
          return;
        }
      node->type_id_ = tid;

      if (kind.compare (ACE_TEXT("struct")) == 0)
        {
          build_struct (node, key);
          return;
        }
      else if (kind.compare (ACE_TEXT("sequence")) == 0)
        {
          build_sequence (node, key);
          return;
        }
      else if (kind.compare (ACE_TEXT("array")) == 0)
        {
          build_array (node, key);
          return;
        }
      else if (kind.compare (ACE_TEXT("enum")) == 0)
        {
          build_enum (node, key);
          return;
        }
      else if (kind.compare (ACE_TEXT("union")) == 0)
        {
          build_union (node, key);
          return;
        }
      else if (kind.compare (ACE_TEXT("alias")) == 0)
        {
          build_alias (node, key);
          return;
        }
      else
        {
          ACE_DEBUG ((LM_DEBUG, ACE_TEXT("section %s is unknown kind %s\n"),
                      node->name_.c_str(),kind.c_str()));
          return;
        }
    }

    Sample_Dissector *
    Sample_Manager::fqfind (const std::string &name, EntityContext *context)
    {
      EntityContext *ctx = context;
      std::string idlname = name;
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
      node->dissector_ = new Sample_Dissector (node->type_id_,
                                               node->name_);
      Sample_Field *f = 0;

      std::string label = node->name_ + ".order";
      std::string order;
      if (get_string_value (node->config_, key, label, order))
        return;

      size_t pos = order.find (' ');
      std::string p =
        (pos == std::string::npos) ? order : order.substr (0,pos);
      while (!p.empty())
        {

          std::string kind;
          get_string_value (node->config_, key, p, kind);

          BuiltinTypeMap::iterator iter = builtin_types_.find(kind);
          if (iter != builtin_types_.end())
            f = node->dissector_->add_field (iter->second, p);
          else
            {
              Sample_Dissector *value = this->fqfind (kind, node->parent_);
              if (value != 0)
                f = node->dissector_->add_field (value, p);
            }

          if (pos == std::string::npos)
            p.clear();
          else
            {
              order = order.substr (pos+1);
              pos = order.find(' ');
              p = (pos == std::string::npos) ?
                order :
                order.substr (0,pos);
            }
        }

    }

    void
    Sample_Manager::build_sequence (EntityNode *node,
                                    const ACE_Configuration_Section_Key &key)
    {
      std::string label = node->name_ + ".element";
      std::string kind;
      if (get_string_value (node->config_, key, label, kind))
        return;

      Sample_Dissector *sequence = 0;
      BuiltinTypeMap::iterator iter = builtin_types_.find(kind);
      if (iter != builtin_types_.end())
        {
          sequence = new Sample_Sequence (node->type_id_, iter->second);
        }
      else
        {
          Sample_Dissector *value = this->fqfind (kind, node->parent_);
          if (value != 0)
            sequence = new Sample_Sequence (node->type_id_, value);
        }
      node->dissector_ = sequence;
    }


    void
    Sample_Manager::build_alias (EntityNode *node,
                                 const ACE_Configuration_Section_Key &key)
    {
      std::string label = node->name_ + ".base";
      std::string kind;
      if (get_string_value (node->config_, key, label, kind))
        return;

      Sample_Dissector *alias = 0;
      BuiltinTypeMap::iterator iter = builtin_types_.find(kind);
      if (iter != builtin_types_.end())
        {
          alias = new Sample_Alias (node->type_id_, iter->second);
        }
      else
        {
          Sample_Dissector *value = this->fqfind (kind, node->parent_);
          if (value != 0)
            alias = new Sample_Alias (node->type_id_, value);
        }
      node->dissector_ = alias;
    }


    void
    Sample_Manager::build_array (EntityNode *node,
                                 const ACE_Configuration_Section_Key &key)
    {
      std::string label = node->name_ + ".size";
      u_int size = 0;
      if (get_int_value (node->config_, key, label, size))
        return;

      label = node->name_ + ".element";
      std::string kind;
      if (get_string_value (node->config_, key, label, kind))
        return;

      Sample_Dissector *array = 0;
      BuiltinTypeMap::iterator iter = builtin_types_.find(kind);
      if (iter != builtin_types_.end())
        {
          array = new Sample_Array (node->type_id_,
                                    (size_t)size,
                                    iter->second);
        }
      else
        {
          Sample_Dissector *value = this->fqfind (kind, node->parent_);
          if (value != 0)
            array = new Sample_Array (node->type_id_,
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
      std::string order;
      if (get_string_value (node->config_, key, label, order))
        return;

      Sample_Enum *sample = new Sample_Enum (node->type_id_);

      size_t pos = order.find (' ');
      std::string p =
        (pos == std::string::npos) ? order : order.substr (0,pos);
      while (!p.empty())
        {
          sample->add_value (p);
          if (pos == std::string::npos)
            p.clear();
          else
            {
              order = order.substr (pos+1);
              pos = order.find(' ');
              p = (pos == std::string::npos) ?
                order :
                order.substr (0,pos);
            }
        }
      node->dissector_ = sample;
    }

    void
    Sample_Manager::build_union (EntityNode *node,
                                 const ACE_Configuration_Section_Key &key)
    {
      std::string label = node->name_ + ".order";
      std::string order;
      std::string case_name;
      std::string case_type;
      Sample_Dissector *value = 0;
      Sample_Field *f = 0;

      if (get_string_value (node->config_, key, label, order))
        return;

      label = node->name_ + ".discriminator";
      if (get_string_value (node->config_, key, label, case_type))
        return;

      Sample_Union *s_union = new Sample_Union (node->type_id_);

      BuiltinTypeMap::iterator iter = builtin_types_.find(case_type);
      if (iter != builtin_types_.end())
        {
          s_union->discriminator (iter->second);
        }
      else
        {
          value = fqfind (case_type, node->parent_);
          if (value != 0)
            s_union->discriminator (value);
          else
            {
              ACE_DEBUG ((LM_DEBUG,
                          ACE_TEXT ("build union could not find ")
                          ACE_TEXT ("discriminator type %s\n"),
                          case_type.c_str()));
            }
        }

      label = "default.type";
      if (get_string_value (node->config_, key, label, case_type) == 0)
        {
          label = "default.name";
          get_string_value (node->config_, key, label, case_name);

          iter = builtin_types_.find(case_type);
          if (iter != builtin_types_.end())
            f = new Sample_Field (iter->second, case_name);
          else
            {
              value = fqfind (case_type, node->parent_);
              if (value != 0)
                f = new Sample_Field (value, case_name);
            }
          s_union->add_default (f);
        }

      Switch_Case *sc = 0;
      bool ranged = false;
      size_t pos = order.find (' ');
      std::string p =
        (pos == std::string::npos) ? order : order.substr (0,pos);
      while (!p.empty())
        {
          label = p + ".type";
          bool new_range =
            (get_string_value (node->config_, key,
                                              label,
                                              case_type) != 0);
          if (!new_range)
            {
              label = p + ".name";
              get_string_value (node->config_, key,
                                               label,
                                               case_name);
            }


          f = 0;
          if (!new_range)
            {
              iter = builtin_types_.find(case_type);
              if (iter != builtin_types_.end())
                f = new Sample_Field (iter->second, case_name);
              else
                {
                  value = fqfind (case_type, node->parent_);
                  if (value != 0)
                    f = new Sample_Field (value, case_name);
                }
            }
          if (ranged)
            {
              sc = sc->add_range (p, f);
            }
          else
            {
              sc = sc == 0 ?
                s_union->add_case (p, f) :
                sc->chain (p, f);
            }
          ranged = new_range;
          if (pos == std::string::npos)
            p.clear();
          else
            {
              order = order.substr (pos+1);
              pos = order.find(' ');
              p = (pos == std::string::npos) ?
                order :
                order.substr (0,pos);
            }
        }

      node->dissector_ = s_union;
    }

  }
}
