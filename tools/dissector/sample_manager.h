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
  //#include <epan/dissectors/packet-giop.h>
} // extern "C"

#include "tools/dissector/dissector_export.h"

#include "dds/DCPS/DataSampleHeader.h"
#include "dds/DCPS/RepoIdConverter.h"
#include "dds/DdsDcpsGuidTypeSupportImpl.h"
#include "dds/DCPS/transport/framework/TransportHeader.h"
#include "ace/Hash_Map_Manager.h"

#include "sample_dissector.h"


class ACE_Configuration;
class ACE_Configuration_Section_Key;

namespace OpenDDS
{
  namespace DCPS
  {

    class ModuleName
    {
    public:
      ACE_TString name_;
      ModuleName *parent_;
    };

    typedef ACE_Hash_Map_Manager <const char *, Sample_Dissector *, ACE_Null_Mutex> SampleDissectorMap;

    typedef ACE_Hash_Map_Manager <const char *, Sample_Field::IDLTypeID, ACE_Null_Mutex> BuiltinTypeMap;

    typedef ACE_Hash_Map_Manager <const char *, ModuleName *, ACE_Null_Mutex> ModuleTreeMap;

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

      void add (Sample_Dissector &dissector);
      Sample_Dissector *find (const char *data_name);

    private:

      static Sample_Manager instance_;
      SampleDissectorMap dissectors_;
      BuiltinTypeMap builtin_types_;
      ModuleTreeMap module_tree_;

      void build_type (ACE_TString &name, ACE_Configuration &config);
      void init_from_file (const ACE_TCHAR *filename);

      struct ConfigInfo {
        ACE_Configuration *config_;
        ACE_Configuration_Section_Key *key_;
        ACE_TString name_;
        ACE_TString version_;
        ACE_TString module_;
        ACE_TString kind_;
        ACE_TString type_id_;
      };

      Sample_Dissector * fqfind (ACE_TString &parent, ACE_TString &name);
      Sample_Dissector * fqfind_i (ModuleName *top_level_mn, ACE_TString &name);

      void build_fqname (ConfigInfo &info);
      void build_module (ConfigInfo &info);
      void build_struct (ConfigInfo &info);
      void build_sequence (ConfigInfo &info);
      void build_array  (ConfigInfo &info);
      void build_enum   (ConfigInfo &info);
      void build_union  (ConfigInfo &info);
      void build_alias  (ConfigInfo &info);
    };
  }
}

#endif //  _SAMPLE_MANAGE_H_
