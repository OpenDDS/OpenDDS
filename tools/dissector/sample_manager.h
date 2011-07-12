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

namespace OpenDDS
{
  namespace DCPS
  {
    typedef ACE_Hash_Map_Manager <const char *, Sample_Dissector *, ACE_Null_Mutex> SampleDissectorMap;

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

    //-------------------------------------------------------
    // Temporary type-specific dissectors for testing

      Sample_Dissector *make_LocationInfo_Dissector ();
      Sample_Dissector *make_PlanInfo_Dissector ();
      Sample_Dissector *make_MoreInfo_Dissector ();
      Sample_Dissector *make_UnrelatedInfo_Dissector ();
      Sample_Dissector *make_Resulting_Dissector ();

      Sample_Dissector *make_Message_Dissector ();
      Sample_Dissector *make_Message2_Dissector ();

      Sample_Dissector *make_A_Dissector();
      Sample_Dissector *make_ShortArray_Dissector();
      Sample_Dissector *make_ArrayOfShortArray_Dissector();
      Sample_Dissector *make_StructSeq_Dissector();
      Sample_Dissector *make_MyEnum_Dissector();
      Sample_Dissector *make_MyUnion_Dissector();

      Sample_Dissector *make_Source_Dissector();
      Sample_Dissector *make_Target_Dissector();

    };
  }
}

#endif //  _SAMPLE_MANAGE_H_
