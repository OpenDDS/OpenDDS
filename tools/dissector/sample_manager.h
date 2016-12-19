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
      static Sample_Manager &instance();
      void init ();

      Sample_Dissector *find (const char *repo_id);

    private:

      static Sample_Manager instance_;
      typedef std::map<std::string, Sample_Dissector*> DissectorsType;
      DissectorsType dissectors_;
      void init_from_file (const ACE_TCHAR *filename);
    };
  }
}

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif //  _SAMPLE_MANAGE_H_
