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
    class dissector_Export Sample_Base;


    class Sample_Field
    {
    public:
      const char   *data_;

      const char   *format_;
      guint         len_;
      Sample_Base  *nested_;
      Sample_Field *next_;

      Sample_Field (const char *f, gint l);
      Sample_Field (const char *f, Sample_Base *n);
      ~Sample_Field ();

      guint length ();
    };

    class dissector_Export Sample_Base
    {
    public:
      Sample_Base ();
      Sample_Base (const char *type_id);
      ~Sample_Base ();

      virtual void dissect (tvbuff_t *, packet_info *, proto_tree *, gint&);

      virtual guint compute_length (const char *data);
      virtual guint length () const;
      const char * typeId() const;

      void add_field (Sample_Field *field);

    protected:
      virtual size_t dissect_i (tvbuff_t *, packet_info *, proto_tree *, gint);

      guint fixed_length_;
      gint ett_payload_;
      gint proto_;
      bool use_subtree_;

    private:
      char *typeId_;

      Sample_Field *field_;
      Sample_Field *last_field_;
    };

    class dissector_Export Sample_String : public Sample_Base
    {
    public:
      Sample_String () : Sample_Base ("string") { use_subtree_ = false; }

      virtual guint compute_length (const char *data);
      virtual guint length () const;
    protected:
      virtual size_t dissect_i (tvbuff_t *, packet_info *, proto_tree *, gint);
    };



    class dissector_Export Sample_Sequence : public Sample_Base
    {
    };

    class dissector_Export Sample_Enum : public Sample_Base
    {
    };

    class dissector_Export Sample_Union : public Sample_Base
    {
    };

    typedef ACE_Hash_Map_Manager <const char *, Sample_Base *, ACE_Null_Mutex> SampleDissectorMap;


    class dissector_Export Sample_Dissector_Manager
    {
    public:
      static Sample_Dissector_Manager &instance();
      void init ();

      void add (Sample_Base &dissector);
      Sample_Base *find (const char *data_name);

    private:
      static Sample_Dissector_Manager
instance_;
      SampleDissectorMap dissectors_;

    };

#if 0

#pragma DCPS_DATA_TYPE "LocationInfo"
#pragma DCPS_DATA_KEY "LocationInfo flight_id1"
#pragma DCPS_DATA_KEY "LocationInfo flight_id2"
struct LocationInfo {
  unsigned long flight_id1;
  unsigned long flight_id2;
  long x;
  long y;
  long z;
};

#endif

    class dissector_Export LocationInfo_Dissector : public Sample_Base
    {
    public:
      LocationInfo_Dissector ();
      ~LocationInfo_Dissector ();

//      virtual void dissect (tvbuff_t *, packet_info *, proto_tree *, gint &);

    private:
//       Sample_Field *field_;

//       gint ett_payload_;
//       gint proto_;
    };

#if 0

#pragma DCPS_DATA_TYPE "PlanInfo"
#pragma DCPS_DATA_KEY "PlanInfo flight_id1"
#pragma DCPS_DATA_KEY "PlanInfo flight_id2"
struct PlanInfo {
  unsigned long flight_id1;
  unsigned long flight_id2;
  string flight_name;
  string tailno;
};

#endif


    class dissector_Export PlanInfo_Dissector : public Sample_Base
    {
    public:
      PlanInfo_Dissector ()
        : Sample_Base ("IDL:PlanInfoTypeSupport:1.0")
        {
          Sample_Dissector_Manager::instance().add (*this);
        }

      ~PlanInfo_Dissector ();

//       virtual void dissect (tvbuff_t *, packet_info *, proto_tree *, gint &);

    };

#if 0

#pragma DCPS_DATA_TYPE "MoreInfo"
#pragma DCPS_DATA_KEY "MoreInfo flight_id1"
struct MoreInfo {
  unsigned long flight_id1;
  string more;
};

#endif

    class dissector_Export MoreInfo_Dissector : public Sample_Base
    {
    public:
      MoreInfo_Dissector ()
        : Sample_Base ("IDL:MoreInfoTypeSupport:1.0")
        {
          Sample_Dissector_Manager::instance().add (*this);
        }

      ~MoreInfo_Dissector ();

//       virtual void dissect (tvbuff_t *, packet_info *, proto_tree *, gint &);

    };

#if 0

#pragma DCPS_DATA_TYPE "UnrelatedInfo"
// testing cross-joins, this has no keys
struct UnrelatedInfo {
  string misc;
};

#endif


    class dissector_Export UnrelatedInfo_Dissector : public Sample_Base
    {
    public:
      UnrelatedInfo_Dissector ()
        : Sample_Base ("IDL:UnrelatedInfoTypeSupport:1.0")
        {
          Sample_Dissector_Manager::instance().add (*this);
        }

      ~UnrelatedInfo_Dissector ();

//       virtual void dissect (tvbuff_t *, packet_info *, proto_tree *, gint &);

    };

#if 0

#pragma DCPS_DATA_TYPE "Resulting"
#pragma DCPS_DATA_KEY "Resulting flight_id1"
#pragma DCPS_DATA_KEY "Resulting flight_id2"
struct Resulting {
  unsigned long flight_id1;
  unsigned long flight_id2;
  string flight_name;
  long x;
  long y;
  long height;
  string more;
  string misc;
};

#endif

    class dissector_Export Resulting_Dissector : public Sample_Base
    {
    public:
      Resulting_Dissector ()
        : Sample_Base ("IDL:ResultingTypeSupport:1.0")
        {
          Sample_Dissector_Manager::instance().add (*this);
        }

      ~Resulting_Dissector ();

//       virtual void dissect (tvbuff_t *, packet_info *, proto_tree *, gint &);

    };



  }
}

#endif //  _SAMPLE_BASE_H_
