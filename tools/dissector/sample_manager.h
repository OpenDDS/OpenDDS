// -*- C++ -*-
/*
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
#include "dds/DdsDcpsGuidTypeSupportImpl.h"
#include "dds/DCPS/transport/framework/TransportHeader.h"

#include "sample_dissector.h"

#include <string>
#include <map>
#include <vector>
#include <forward_list>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS
{
  namespace DCPS
  {
    /*
     * The Sample_Manager is a singleton which contains a map
     * of Sample_Dissectors keyed to type identifiers. This singleton is
     * used by the greater packet-opendds dissector to find type-specific
     * dissectors.
     */
    class dissector_Export Sample_Manager
    {
    public:
      /// Clean up Header Fields
      ~Sample_Manager();

      static Sample_Manager &instance();

      void init ();
      Sample_Dissector *find (const char *repo_id);

      /// Wireshark Namespace to set the payload in
      static const std::string payload_namespace;

      /// Add sample field to register later.
      void add_protocol_field(
        int * hf_index,
        const std::string & full_name, const std::string & short_name,
        enum ftenum ft, field_display_e fd = BASE_NONE
      );

      /// Add a premade hf_register_info struct to register later
      void add_protocol_field(hf_register_info field);

      /// What is passed to wireshark
      hf_register_info * fields_array();
      size_t number_of_fields();

    private:
      static Sample_Manager instance_;

      typedef std::map<std::string, Sample_Dissector*> DissectorsType;
      DissectorsType dissectors_;
      std::vector<hf_register_info> hf_vector_;
      hf_register_info * hf_array_ = NULL;
      /// Dynamic Field Names (Long and Short) to be deleted later
      std::forward_list<char *> field_names_;  

      void init_from_file (const ACE_TCHAR *filename);
    };
  }
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif //  _SAMPLE_MANAGE_H_
