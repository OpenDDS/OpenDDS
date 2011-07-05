// -*- C++ -*-
/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef _SAMPLE_BASE_H_
#define _SAMPLE_BASE_H_


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

namespace OpenDDS
{
  namespace DCPS
  {

    class dissector_Export Sample_Base
    {
    public:
      Sample_Base (const char *type_id);
      ~Sample_Base ();

      virtual void dissect (tvbuff_t *, packet_info *, proto_tree *) = 0;

      const char * typeId() const;
      const RepoId& publication () const;
      void publication (const DCPS::RepoId& );

    private:
      char *typeId_;
      RepoId publication_;
    };



    typedef ACE_Hash_Map_Manager <const char *, Sample_Base *, ACE_Null_Mutex> SampleDissectorMap;


    class Sample_Dissector_Manager
    {
    public:
      void add (Sample_Base &dissector);
      Sample_Base *find (const char *data_name);

    private:
      SampleDissectorMap dissectors_;

    };

  }
}

#endif //  _SAMPLE_BASE_H_
