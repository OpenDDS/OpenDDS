/*
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DISSECTOR_SAMPLE_MANAGER_H_
#define OPENDDS_DISSECTOR_SAMPLE_MANAGER_H_

#include "sample_dissector.h"
#include "ws_common.h"
#include "dissector_export.h"
#include "ws-wrapper-headers/packet-tcp.h"

#include <dds/DCPS/DataSampleHeader.h>
#include <dds/DCPS/transport/framework/TransportHeader.h>

#include <dds/DdsDcpsGuidTypeSupportImpl.h>

#include <epan/value_string.h>
#include <epan/ipproto.h>

#include <glib.h>
#include <gmodule.h>

#include <string>
#include <map>
#include <vector>
#include <list>

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS
{
  namespace DCPS
  {
    /// Wireshark Namespace to set the payload in
    const std::string payload_namespace = "opendds.sample.payload";

    /// Namespace for arrays and sequence elements
    const std::string element_namespace = "_e";

    /**
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

      static Sample_Manager& instance();

      void init();
      Sample_Dissector* find(const char* repo_id);

      /// Add a hf_register_info struct to register later
      void add_protocol_field(const hf_register_info& field);

      /// Field information to be passed to wireshark
      hf_register_info* fields_array();

      /// Number of fields in the fields_array_
      size_t number_of_fields();

    private:
      static Sample_Manager instance_;

      Dissector_Map dissectors_;

      std::vector<hf_register_info> hf_vector_;
      hf_register_info* hf_array_;

      void init_from_file(const ACE_TCHAR* filename);
    };
  }
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif
