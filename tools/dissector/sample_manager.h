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

    };

    //-------------------------------------------------------
    // Temporary type-specific dissectors for testing

    class dissector_Export LocationInfo_Dissector : public Sample_Dissector
    {
    public:
      LocationInfo_Dissector ();
    };

    class dissector_Export PlanInfo_Dissector : public Sample_Dissector
    {
    public:
      PlanInfo_Dissector ();
    };

    class dissector_Export MoreInfo_Dissector : public Sample_Dissector
    {
    public:
      MoreInfo_Dissector ();
    };

    class dissector_Export UnrelatedInfo_Dissector : public Sample_Dissector
    {
    public:
      UnrelatedInfo_Dissector ();
    };

    class dissector_Export Resulting_Dissector : public Sample_Dissector
    {
    public:
      Resulting_Dissector ();
    };

    class dissector_Export Message_Dissector : public Sample_Dissector
    {
    public:
      Message_Dissector ();
    };

    class dissector_Export Message2_Dissector : public Sample_Dissector
    {
    public:
      Message2_Dissector ();
    };

  }
}

#endif //  _SAMPLE_MANAGE_H_
