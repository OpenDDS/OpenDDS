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

#include <cstring>

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <string>

namespace OpenDDS
{
  namespace DCPS
  {

    Sample_Manager
    Sample_Manager::instance_;

    Sample_Manager &
    Sample_Manager::instance ()
    {
      return instance_;
    }

    void
    Sample_Manager::init ()
    {
      Sample_Dissector *dummy = new LocationInfo_Dissector;
      dummy = new PlanInfo_Dissector;
      dummy = new MoreInfo_Dissector;
      dummy = new UnrelatedInfo_Dissector;
      dummy = new Resulting_Dissector;

      dummy = new Message_Dissector;
      dummy = new Message2_Dissector;

      ACE_UNUSED_ARG (dummy);
    };

    void
    Sample_Manager::add (Sample_Dissector &d)
    {
      const char *key = d.typeId();
      ACE_DEBUG ((LM_DEBUG,"Adding new dissector for %s\n",key));

      dissectors_.bind(key,&d);
    }

    Sample_Dissector *
    Sample_Manager::find (const char *data_name)
    {
      Sample_Dissector *result = 0;
      dissectors_.find (data_name, result);
      return result;
    }

   //----------------------------------------------------------------------

    LocationInfo_Dissector::LocationInfo_Dissector ()
    {

      this->init ("IDL:LocationInfoTypeSupport:1.0",
                  "LocationInfo");
#if 0
      Sample_Field *f =
        this->add_field ("flight_id1: %u", 4);
      f = f->chain ("flight_id2: %u", 4);
      f = f->chain ("x: %d", 4);
      f = f->chain ("y: %d", 4);
      f = f->chain ("z: %d", 4);
#else
      Sample_Field *f =
        this->add_field (Sample_Field::ULong, "flight_id1");
      f = f->chain (Sample_Field::Long, "flight_id2");
      f = f->chain (Sample_Field::Long, "x");
      f = f->chain (Sample_Field::Long, "y");
      f = f->chain (Sample_Field::Long, "z");
#endif
    }

    PlanInfo_Dissector::PlanInfo_Dissector ()
    {
      this->init ("IDL:PlanInfoTypeSupport:1.0",
                  "PlanInfo");

      Sample_Field *f =
        this->add_field (new Sample_Field ("flight_id1: %u", 4));
      f = f->chain (new Sample_Field ("flight_id2: %u", 4));
      f = f->chain (new Sample_Field (new Sample_String ("flight_name")));
      f = f->chain (new Sample_Field (new Sample_String ("tailno")));
    }

    MoreInfo_Dissector::MoreInfo_Dissector ()
    {

      this->init ("IDL:MoreInfoTypeSupport:1.0",
                  "MoreInfo");

      Sample_Field *f =
        this->add_field (new Sample_Field ("flight_id1: %u", 4));
      f = f->chain (new Sample_Field (new Sample_String ("more")));
    }

    UnrelatedInfo_Dissector::UnrelatedInfo_Dissector ()
    {


      this->init ("IDL:UnrelatedInfoTypeSupport:1.0",
                  "UnrelatedInfo");

      this->add_field (new Sample_String ("misc"));

    }

    Resulting_Dissector::Resulting_Dissector ()
    {
      this->init ("IDL:ResultingTypeSupport:1.0",
                  "Resulting");

      Sample_Field *f =
        this->add_field ("flight_id1: %u", 4);
      f = f->chain ("flight_id2: %u", 4);
      f = f->chain (new Sample_String ("flight_name"));
      f = f->chain ("x: %d", 4);
      f = f->chain ("y: %d", 4);
      f = f->chain ("height: %d", 4);
      f = f->chain (new Sample_String ("more"));
      f = f->chain (new Sample_String ("misc"));

    }

    //---------------------------------------------------------------------


    Message_Dissector::Message_Dissector ()
    {
      this->init ("IDL:Messenger/MessageTypeSupport:1.0",
                  "Messenger::Message");
//   struct Message {
//     string from;
//     string subject;
//     long subject_id;
//     string text;
//     long   count;
//     CORBA::BooleanSeq    bool_seq;
//     CORBA::LongDoubleSeq longdouble_seq;
//     CORBA::ShortSeq      short_seq;
//     CORBA::UShortSeq     ushort_seq;
//     CORBA::CharSeq       char_seq;
//     CORBA::LongLongSeq   longlong_seq;
//     CORBA::StringSeq     string_seq;
//     CORBA::WCharSeq      wchar_seq;
//     CORBA::DoubleSeq     double_seq;
//     CORBA::LongSeq       long_seq;
//     CORBA::ULongLongSeq  ulonglong_seq;
//     CORBA::WStringSeq    wstring_seq;
//     CORBA::FloatSeq      float_seq;
//     CORBA::OctetSeq      octet_seq;
//     CORBA::ULongSeq      ulong_seq;
//     Messenger2::LongSeq  outside_long_seq;
//   };
      Sample_Field *f =
        this->add_field (new Sample_String ("from"));
      f = f->chain (new Sample_String ("subject"));
      f = f->chain ("subject_id: %d", 4);
      f = f->chain (new Sample_String ("text"));
      f = f->chain ("count: %d", 4);
#if 0
      Sample_Sequence *seq = new Sample_Sequence("bool_seq");
      seq->element()->add_field ("%u", sizeof (bool));
      f = f->chain (seq);

      seq = new Sample_Sequence("longdouble_seq");
      seq->element()->add_field ("%24.16LG", sizeof (ACE_CDR::LongDouble));
      f = f->chain (seq);

      seq = new Sample_Sequence("short_seq");
      seq->element()->add_field ("%d", sizeof (ACE_CDR::Short));
      f = f->chain (seq);

      seq = new Sample_Sequence("ushort_seq");
      seq->element()->add_field ("%u", sizeof (ACE_CDR::Short));
      f = f->chain (seq);

      seq = new Sample_Sequence("char_seq");
      seq->element()->add_field ("%c", sizeof (ACE_CDR::Char));
      f = f->chain (seq);

      seq = new Sample_Sequence("longlong_seq");
      seq->element()->add_field ("%lld", sizeof (ACE_CDR::LongLong));
      f = f->chain (seq);

      seq = new Sample_Sequence("string_seq");
      seq->element()->add_field (new Sample_String (""));
      f = f->chain (seq);

      seq = new Sample_Sequence("wchar_seq");
      seq->element()->add_field ("%lc", sizeof (ACE_CDR::WChar));
      f = f->chain (seq);

      seq = new Sample_Sequence("double_seq");
      seq->element()->add_field ("%g", sizeof (ACE_CDR::Double));
      f = f->chain (seq);

      seq = new Sample_Sequence("long_seq");
      seq->element()->add_field ("%ld", sizeof (ACE_CDR::Long));
      f = f->chain (seq);

      seq = new Sample_Sequence("ulonglong_seq");
      seq->element()->add_field ("%llu", sizeof (ACE_CDR::LongLong));
      f = f->chain (seq);

      seq = new Sample_Sequence("wstring_seq");
      seq->element()->add_field (new Sample_WString (""));
      f = f->chain (seq);

      seq = new Sample_Sequence("float_seq");
      seq->element()->add_field ("%f", sizeof (ACE_CDR::Float));
      f = f->chain (seq);

      seq = new Sample_Sequence("octet_seq");
      seq->element()->add_field ("%x", sizeof (ACE_CDR::Char));
      f = f->chain (seq);

      seq = new Sample_Sequence("ulong_seq");
      seq->element()->add_field ("%lu", sizeof (ACE_CDR::Long));
      f = f->chain (seq);

      seq = new Sample_Sequence("outside_long_seq");
      seq->element()->add_field ("%ld", sizeof (ACE_CDR::Long));
      f = f->chain (seq);
#else
      Sample_Sequence *seq = new Sample_Sequence("bool_seq");
      seq->element()->add_field (Sample_Field::Bool,"element");
      f = f->chain (seq);

      seq = new Sample_Sequence("longdouble_seq");
      seq->element()->add_field (Sample_Field::LongDouble, "element");
      f = f->chain (seq);

      seq = new Sample_Sequence("short_seq");
      seq->element()->add_field ("%d", sizeof (ACE_CDR::Short));
      f = f->chain (seq);

      seq = new Sample_Sequence("ushort_seq");
      seq->element()->add_field ("%u", sizeof (ACE_CDR::Short));
      f = f->chain (seq);

      seq = new Sample_Sequence("char_seq");
      seq->element()->add_field ("%c", sizeof (ACE_CDR::Char));
      f = f->chain (seq);

      seq = new Sample_Sequence("longlong_seq");
      seq->element()->add_field ("%lld", sizeof (ACE_CDR::LongLong));
      f = f->chain (seq);

      seq = new Sample_Sequence("string_seq");
      seq->element()->add_field (new Sample_String (""));
      f = f->chain (seq);

      seq = new Sample_Sequence("wchar_seq");
      seq->element()->add_field ("%lc", sizeof (ACE_CDR::WChar));
      f = f->chain (seq);

      seq = new Sample_Sequence("double_seq");
      seq->element()->add_field ("%g", sizeof (ACE_CDR::Double));
      f = f->chain (seq);

      seq = new Sample_Sequence("long_seq");
      seq->element()->add_field ("%ld", sizeof (ACE_CDR::Long));
      f = f->chain (seq);

      seq = new Sample_Sequence("ulonglong_seq");
      seq->element()->add_field ("%llu", sizeof (ACE_CDR::LongLong));
      f = f->chain (seq);

      seq = new Sample_Sequence("wstring_seq");
      seq->element()->add_field (new Sample_WString (""));
      f = f->chain (seq);

      seq = new Sample_Sequence("float_seq");
      seq->element()->add_field ("%f", sizeof (ACE_CDR::Float));
      f = f->chain (seq);

      seq = new Sample_Sequence("octet_seq");
      seq->element()->add_field ("%x", sizeof (ACE_CDR::Char));
      f = f->chain (seq);

      seq = new Sample_Sequence("ulong_seq");
      seq->element()->add_field ("%lu", sizeof (ACE_CDR::Long));
      f = f->chain (seq);

      seq = new Sample_Sequence("outside_long_seq");
      seq->element()->add_field ("%ld", sizeof (ACE_CDR::Long));
      f = f->chain (seq);
#endif
    }

    Message2_Dissector::Message2_Dissector ()
    {

      this->init ("IDL:Messenger/MessageTypeSupport:1.0",
                  "Messenger::Message");


//     typedef sequence <long> LongSeq;

//   struct Message2 {
//     string from;
//     string subject;
//     long subject_id;
//     string text;
//     long   count;
//     CORBA::BooleanSeq    bool_seq;
//     CORBA::LongDoubleSeq longdouble_seq;
//     CORBA::ShortSeq      short_seq;
//     CORBA::UShortSeq     ushort_seq;
//     CORBA::CharSeq       char_seq;
//     CORBA::LongLongSeq   longlong_seq;
//     CORBA::StringSeq     string_seq;
//     CORBA::WCharSeq      wchar_seq;
//     CORBA::DoubleSeq     double_seq;
//     CORBA::LongSeq       long_seq;
//     CORBA::ULongLongSeq  ulonglong_seq;
//     CORBA::WStringSeq    wstring_seq;
//     CORBA::FloatSeq      float_seq;
//     CORBA::OctetSeq      octet_seq;
//     CORBA::ULongSeq      ulong_seq;
//   };

      Sample_Field *f =
        this->add_field (new Sample_String ("from"));
      f = f->chain (new Sample_String ("subject"));
      f = f->chain ("subject_id: %d", 4);
      f = f->chain (new Sample_String ("text"));
      f = f->chain ("count: %d", 4);

      Sample_Sequence *seq = new Sample_Sequence("bool_seq");
      seq->element()->add_field ("%u", sizeof (bool));
      f = f->chain (seq);

      seq = new Sample_Sequence("longdouble_seq");
      seq->element()->add_field ("%24.16LG", sizeof (ACE_CDR::LongDouble));
      f = f->chain (seq);

      seq = new Sample_Sequence("short_seq");
      seq->element()->add_field ("%d", sizeof (ACE_CDR::Short));
      f = f->chain (seq);

      seq = new Sample_Sequence("ushort_seq");
      seq->element()->add_field ("%u", sizeof (ACE_CDR::Short));
      f = f->chain (seq);

      seq = new Sample_Sequence("char_seq");
      seq->element()->add_field ("%c", sizeof (ACE_CDR::Char));
      f = f->chain (seq);

      seq = new Sample_Sequence("longlong_seq");
      seq->element()->add_field ("%lld", sizeof (ACE_CDR::LongLong));
      f = f->chain (seq);

      seq = new Sample_Sequence("string_seq");
      seq->element()->add_field (new Sample_String (""));
      f = f->chain (seq);

      seq = new Sample_Sequence("wchar_seq");
      seq->element()->add_field ("%lc", sizeof (ACE_CDR::WChar));
      f = f->chain (seq);

      seq = new Sample_Sequence("double_seq");
      seq->element()->add_field ("%g", sizeof (ACE_CDR::Double));
      f = f->chain (seq);

      seq = new Sample_Sequence("long_seq");
      seq->element()->add_field ("%ld", sizeof (ACE_CDR::Long));
      f = f->chain (seq);

      seq = new Sample_Sequence("ulonglong_seq");
      seq->element()->add_field ("%llu", sizeof (ACE_CDR::LongLong));
      f = f->chain (seq);

      seq = new Sample_Sequence("wstring_seq");
      seq->element()->add_field (new Sample_WString (""));
      f = f->chain (seq);

      seq = new Sample_Sequence("float_seq");
      seq->element()->add_field ("%f", sizeof (ACE_CDR::Float));
      f = f->chain (seq);

      seq = new Sample_Sequence("octet_seq");
      seq->element()->add_field ("%x", sizeof (ACE_CDR::Char));
      f = f->chain (seq);

      seq = new Sample_Sequence("ulong_seq");
      seq->element()->add_field ("%lu", sizeof (ACE_CDR::Long));
      f = f->chain (seq);

    }

  }
}
