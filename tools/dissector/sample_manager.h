// -*- C++ -*-
/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef _SAMPLE_MANAGER_H_
#define _SAMPLE_MANAGER_H_


extern "C" {

#include "config.h"

#include <glib.h>
#include <gmodule.h>

#include <epan/value_string.h>
#include <epan/ipproto.h>
#include <epan/packet.h>
#include <epan/dissectors/packet-tcp.h>
} // extern "C"

#include "tools/dissector/dissector_export.h"

#include "dds/DCPS/DataSampleHeader.h"
#include "dds/DCPS/RepoIdConverter.h"
#include "dds/DdsDcpsGuidTypeSupportImpl.h"
#include "dds/DCPS/transport/framework/TransportHeader.h"

#include "sample_dissector.h"

//#include <memory>
//#include <boost/smart_ptr/shared_ptr.hpp>
#include <string>
#include <map>

class ACE_Configuration;
class ACE_Configuration_Section_Key;

namespace OpenDDS
{
  namespace DCPS
  {

    class EntityBase;
    class EntityContext;
    class EntityNode;

    typedef std::map<std::string, EntityBase *> EntityMap;

    class EntityBase
    {
    public:
      virtual ~EntityBase() {}

      std::string name_;   // just the leaf name
      std::string fqname_; // fully qualified name [module/]name
      std::string idl_name_; // [module::]name

      EntityContext *parent_;

    protected:
      EntityBase ()
        : name_ (""),
          fqname_ (""),
          idl_name_ (""),
          parent_ (0)
      {}

      EntityBase (const std::string &n, EntityContext *p);

    };

    class EntityNode : public EntityBase
    {
    public:
      EntityNode (const std::string &n, EntityContext *p, ACE_Configuration *cfg)
        : EntityBase (n, p),
          dissector_(0),
          config_ (cfg)
      {}
      virtual ~EntityNode() {}

      Sample_Dissector  *dissector_;
      ACE_Configuration *config_;
    };

    class EntityContext : public EntityBase
    {
    public:
      EntityContext ()
        :EntityBase(),
         children_()
      {}
      EntityContext (std::string &n, EntityContext *p)
        : EntityBase (n, p),
          children_ ()
      {}

      virtual ~EntityContext() {}

      EntityBase *node_at (std::string &fqname);

      EntityMap children_;
    };

    /*
     * The Sample_Dessector_Manager is a singleton which contains a hash map
     * of Sample Base instances keyed to type identifiers. This singleton is
     * used by the greater packet-opendds dissector to find type-specific
     * dissectors.
     */
    class dissector_Export Sample_Manager
    {
    public:
      static Sample_Manager &instance();
      void init ();

      Sample_Dissector *find (const char *repo_id);

    private:

      static Sample_Manager instance_;
      EntityContext        *entities_;

      void init_from_file (const ACE_TCHAR *filename);

      void init_builtin (Sample_Field::IDLTypeID tid,
                         const std::string &idl_type,
                         const std::string &cxx_type);

      void load_entities (EntityContext *entities);

      int get_string_value (ACE_Configuration *config,
                            const ACE_Configuration_Section_Key &base,
                            const std::string &label,
                            std::string &value);

      int get_int_value (ACE_Configuration *config,
                         const ACE_Configuration_Section_Key &base,
                         const std::string &label,
                         u_int &value);

      void find_config_sections (ACE_Configuration *config,
                                 const ACE_Configuration_Section_Key &base,
                                 EntityContext *parent);

      void build_type (EntityNode *node);

      Sample_Dissector *fqfind (const std::string &name, EntityContext *ctx);

      void build_struct (EntityNode *node,
                        const ACE_Configuration_Section_Key &);
      void build_sequence (EntityNode *node,
                        const ACE_Configuration_Section_Key &);
      void build_array  (EntityNode *node,
                        const ACE_Configuration_Section_Key &);
      void build_enum   (EntityNode *node,
                        const ACE_Configuration_Section_Key &);
      void build_union  (EntityNode *node,
                        const ACE_Configuration_Section_Key &);
      void build_alias  (EntityNode *node,
                        const ACE_Configuration_Section_Key &);
    };
  }
}

#endif //  _SAMPLE_MANAGE_H_
