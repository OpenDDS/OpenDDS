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
      Sample_Dissector *dummy = make_LocationInfo_Dissector();
      dummy = make_PlanInfo_Dissector();
      dummy = make_MoreInfo_Dissector();
      dummy = make_UnrelatedInfo_Dissector();
      dummy = make_Resulting_Dissector();

      dummy = make_Message_Dissector();
      dummy = make_Message2_Dissector();

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

    Sample_Dissector *
    Sample_Manager::make_LocationInfo_Dissector ()
    {
      Sample_Dissector *sample =  new Sample_Dissector;

      sample->init ("IDL:LocationInfoTypeSupport:1.0", "LocationInfo");

      Sample_Field *f =
        sample->add_field (Sample_Field::ULong, "flight_id1");
      f = f->chain (Sample_Field::Long, "flight_id2");
      f = f->chain (Sample_Field::Long, "x");
      f = f->chain (Sample_Field::Long, "y");
      f = f->chain (Sample_Field::Long, "z");
      return sample;
    }

    Sample_Dissector *
    Sample_Manager::make_PlanInfo_Dissector ()
    {
      Sample_Dissector *sample = new Sample_Dissector;
      sample->init ("IDL:PlanInfoTypeSupport:1.0", "PlanInfo");
      Sample_Field *f =
        sample->add_field (Sample_Field::ULong, "flight_id1");
      f = f->chain (Sample_Field::Long, "flight_id2");
      f = f->chain (new Sample_String, "flight_name");
      f = f->chain (new Sample_String, "tailno");
      return sample;
    }

    Sample_Dissector *
    Sample_Manager::make_MoreInfo_Dissector ()
    {
      Sample_Dissector *sample = new Sample_Dissector;
      sample->init ("IDL:MoreInfoTypeSupport:1.0", "MoreInfo");
      Sample_Field *f =
        sample->add_field (Sample_Field::ULong, "flight_id1");
      f = f->chain (new Sample_String, "more");
      return sample;
    }

    Sample_Dissector *
    Sample_Manager::make_UnrelatedInfo_Dissector ()
    {
      Sample_Dissector *sample = new Sample_Dissector;
      sample->init ("IDL:UnrelatedInfoTypeSupport:1.0", "UnrelatedInfo");
      sample->add_field (new Sample_String, "misc");
      return sample;
    }

    Sample_Dissector *
    Sample_Manager::make_Resulting_Dissector ()
    {
      Sample_Dissector *sample = new Sample_Dissector;
      sample->init ("IDL:ResultingTypeSupport:1.0", "Resulting");
      Sample_Field *f =
        sample->add_field (Sample_Field::ULong, "flight_id1");
      f = f->chain (Sample_Field::Long, "flight_id2");
      f = f->chain (new Sample_String, "flight_name");
      f = f->chain (Sample_Field::Long, "x");
      f = f->chain (Sample_Field::Long, "y");
      f = f->chain (Sample_Field::Long, "height");
      f = f->chain (new Sample_String, "more");
      sample->add_field (new Sample_String, "misc");
      return sample;
    }

    //---------------------------------------------------------------------

    Sample_Dissector *
    Sample_Manager::make_Message_Dissector ()
    {
      Sample_Dissector *sample = new Sample_Dissector;

      sample->init ("IDL:Messenger/MessageTypeSupport:1.0",
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
        sample->add_field (new Sample_String, "from");
      f = f->chain (new Sample_String, "subject");
      f = f->chain (Sample_Field::Long, "subject_id");
      f = f->chain (new Sample_String, "text");
      f = f->chain (Sample_Field::Long, "count");

      Sample_Dissector *seq = new Sample_Sequence(Sample_Field::Bool);
      f = f->chain (seq, "bool_seq");

      seq = new Sample_Sequence(Sample_Field::LongDouble);
      f = f->chain (seq,"longdouble_seq");

      seq = new Sample_Sequence(Sample_Field::Short);
      f = f->chain (seq,"short_seq");

      seq = new Sample_Sequence (Sample_Field::UShort);
      f = f->chain (seq,"ushort_seq");

      seq = new Sample_Sequence(Sample_Field::Char);
      f = f->chain (seq,"char_seq");

      seq = new Sample_Sequence(Sample_Field::LongLong);
      f = f->chain (seq,"longlong_seq");

      seq = new Sample_Sequence (new Sample_String);
      f = f->chain (seq,"string_seq");

      seq = new Sample_Sequence(Sample_Field::WChar);
      f = f->chain (seq,"wchar_seq");

      seq = new Sample_Sequence(Sample_Field::Double);
      f = f->chain (seq,"double_seq");

      seq = new Sample_Sequence(Sample_Field::Long);
      f = f->chain (seq,"long_seq");

      seq = new Sample_Sequence(Sample_Field::LongLong);
      f = f->chain (seq,"ulonglong_seq");

      seq = new Sample_Sequence(new Sample_WString);
      f = f->chain (seq,"wstring_seq");

      seq = new Sample_Sequence(Sample_Field::Float);
      f = f->chain (seq,"float_seq");

      seq = new Sample_Sequence(Sample_Field::Octet);
      f = f->chain (seq,"octet_seq");

      seq = new Sample_Sequence(Sample_Field::ULong);
      f = f->chain (seq,"ulong_seq");

      seq = new Sample_Sequence(Sample_Field::Long);
      f = f->chain (seq,"outside_long_seq");
      return sample;
    }

    Sample_Dissector *
    Sample_Manager::make_Message2_Dissector ()
    {
      Sample_Dissector *sample = new Sample_Dissector;
      sample->init ("IDL:Messenger2/Message2TypeSupport:1.0",
                    "Messenger2::Message2");

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
        sample->add_field (new Sample_String, "from");
      f = f->chain (new Sample_String, "subject");
      f = f->chain (Sample_Field::Long, "subject_id");
      f = f->chain (new Sample_String, "text");
      f = f->chain (Sample_Field::Long, "count");

      Sample_Sequence *seq = new Sample_Sequence;
      seq->element()->add_field (Sample_Field::Bool,"bool_seq");
      f = f->chain (seq, "bool_seq");

      seq = new Sample_Sequence;
      seq->element()->add_field (Sample_Field::LongDouble, "longdouble_seq");
      f = f->chain (seq,"longdouble_seq");

      seq = new Sample_Sequence;
      seq->element()->add_field (Sample_Field::Short,"short_seq");
      f = f->chain (seq,"short_seq");

      seq = new Sample_Sequence;
      seq->element()->add_field (Sample_Field::UShort,"ushort_seq");
      f = f->chain (seq,"ushort_seq");

      seq = new Sample_Sequence;
      seq->element()->add_field (Sample_Field::Char,"char_seq");
      f = f->chain (seq,"char_seq");

      seq = new Sample_Sequence;
      seq->element()->add_field (Sample_Field::LongLong,"longlong_seq");
      f = f->chain (seq,"longlong_seq");

      seq = new Sample_Sequence;
      seq->element()->add_field (new Sample_String, "");
      f = f->chain (seq,"string_seq");

      seq = new Sample_Sequence;
      seq->element()->add_field (Sample_Field::WChar,"wchar_seq");
      f = f->chain (seq,"wchar_seq");

      seq = new Sample_Sequence;
      seq->element()->add_field (Sample_Field::Double,"double_seq");
      f = f->chain (seq,"double_seq");

      seq = new Sample_Sequence;
      seq->element()->add_field (Sample_Field::Long,"long_seq");
      f = f->chain (seq,"long_seq");

      seq = new Sample_Sequence;
      seq->element()->add_field (Sample_Field::LongLong,"ulonglong_seq");
      f = f->chain (seq,"ulonglong_seq");

      seq = new Sample_Sequence;
      seq->element()->add_field (new Sample_WString, "");
      f = f->chain (seq,"wstring_seq");

      seq = new Sample_Sequence;
      seq->element()->add_field (Sample_Field::Float,"float_seq");
      f = f->chain (seq,"float_seq");

      seq = new Sample_Sequence;
      seq->element()->add_field (Sample_Field::Octet,"octet_seq");
      f = f->chain (seq,"octet_seq");

      seq = new Sample_Sequence;
      seq->element()->add_field (Sample_Field::ULong,"ulong_seq");
      f = f->chain (seq,"ulong_seq");

      return sample;
    }

    //------------------------------------------------------------------------
    // From MetaStructTest.idl
    //
    // struct A {
    //   string s;
    //   long l;
    // };

    Sample_Dissector *
    Sample_Manager::make_A_Dissector ()
    {
      Sample_Dissector *sample = new Sample_Dissector;
      sample->init ("IDL:A:1.0","A");

      Sample_Field *f =
        sample->add_field (new Sample_String,"bs");
      f = f->chain (Sample_Field::Long, "l");
      return sample;
    }

    // typedef short ShortArray[3];

    Sample_Dissector *
    Sample_Manager::make_ShortArray_Dissector ()
    {
      Sample_Array *sample = new Sample_Array (3);
      sample->init("IDL:ShortArray:1.0","");

      sample->element()->add_field (Sample_Field::Short, "");
      return sample;
    }

    // typedef ShortArray ArrayOfShortArray[4];

    Sample_Dissector *
    Sample_Manager::make_ArrayOfShortArray_Dissector ()
    {
      Sample_Array *sample = new Sample_Array (4);
      sample->init("IDL:ArrayOfShortArray:1.0","");

      Sample_Dissector *field = this->find ("IDL:ShortArray:1.0");
      if (field == 0)
        field = this->make_ShortArray_Dissector();
      sample->element()->add_field (field,"shortArray");
      return sample;
    }

    // typedef sequence<A> StructSeq;

    Sample_Dissector *
    Sample_Manager::make_StructSeq_Dissector ()
    {
      Sample_Sequence *sample = new Sample_Sequence;
      sample->init("IDL:StructSeq:1.0","");

      Sample_Dissector *field = this->find ("IDL:A:1.0");
      if (field == 0)
        field = this->make_A_Dissector();
      sample->element()->add_field (field,"element");
      return sample;
    }

    // enum MyEnum {b, d, as, sa, ss, other1, other2};

    Sample_Dissector *
    Sample_Manager::make_MyEnum_Dissector ()
    {
      Sample_Enum *sample = new Sample_Enum;
      sample->init("IDL:MyEnum:1.0","");

      Sample_Field *n = sample->add_value ("b");
      n->chain (Sample_Field::Enumeration, "d");
      n->chain (Sample_Field::Enumeration, "as");
      n->chain (Sample_Field::Enumeration, "sa");
      n->chain (Sample_Field::Enumeration, "ss");
      n->chain (Sample_Field::Enumeration, "other1");
      n->chain (Sample_Field::Enumeration, "other2");

      return sample;
    }


    // union MyUnion switch (MyEnum) {
    // case b: boolean u_b;
    // case d: double u_d;
    // case as: A u_as;
    // case sa: ShortArray u_sa;
    // case ss: StructSeq u_ss;
    // default: float u_f;
    // };

    Sample_Dissector *
    Sample_Manager::make_MyUnion_Dissector ()
    {
      Sample_Union *sample = new Sample_Union;
      Sample_Dissector *_d = this->find ("IDL:MyEnum:1.0");
      if (_d == 0)
        _d = this->make_MyEnum_Dissector();
      sample->discriminator (_d);
      sample->add_case (new Sample_Field (Sample_Field::Enumeration, "b"),
                        new Sample_Field (Sample_Field::Bool, "u_b"));
      sample->add_case (new Sample_Field (Sample_Field::Enumeration, "d"),
                        new Sample_Field (Sample_Field::Double, "u_d"));
      Sample_Dissector *field = this->find ("IDL:A:1.0");
      if (field == 0)
        field = this->make_A_Dissector();
      sample->add_case (new Sample_Field (Sample_Field::Enumeration, "as"),
                        new Sample_Field (field, "u_as"));
      if ((field = this->find ("IDL:ShortArray:1.0")) == 0)
        field = this->make_ShortArray_Dissector();
      sample->add_case (new Sample_Field (Sample_Field::Enumeration, "sa"),
                        new Sample_Field (field, "u_sa"));
      if ((field = this->find ("IDL:StructSeq:1.0")) == 0)
        field = this->make_StructSeq_Dissector();
      sample->add_case (new Sample_Field (Sample_Field::Enumeration, "ss"),
                        new Sample_Field (field, "u_ss"));
      sample->add_default (new Sample_Field (Sample_Field::Float, "u_f"));

      return sample;
    }

    // struct Source {
    //   A rhs_a;
    //   ShortArray rhs_sa;
    //   ArrayOfShortArray rhs_asa;
    //   StructSeq rhs_ss;
    //   MyEnum rhs_e;
    //   MyUnion rhs_u;
    // };

    Sample_Dissector *
    Sample_Manager::make_Source_Dissector ()
    {
      Sample_Dissector *sample = new Sample_Dissector ();
      sample->init("IDL:Source:1.0");

      Sample_Dissector *field = this->find ("IDL:A:1.0");
      if (field == 0)
        field = this->make_A_Dissector();
      Sample_Field *f = sample->add_field (field, "rhs_a");
      if ((field = this->find ("IDL:ShortArray:1.0")) == 0)
        field = this->make_ShortArray_Dissector();
      f->chain (field, "rhs_sa");
      if ((field = this->find ("IDL:ArrayOfShortArray:1.0")) == 0)
        field = this->make_ArrayOfShortArray_Dissector();
      f->chain (field, "rhs_asa");
      if ((field = this->find ("IDL:StructSeq:1.0")) == 0)
        field = this->make_StructSeq_Dissector();
      f->chain (field, "rhs_ss");
      if ((field = this->find ("IDL:MyEnum:1.0")) == 0)
        field = this->make_MyEnum_Dissector();
      f->chain (field, "rhs_e");
      if ((field = this->find ("IDL:MyUnion:1.0")) == 0)
        field = this->make_MyUnion_Dissector();
      f->chain (field, "rhs_u");

      return sample;
    }

    // struct Target {
    //   A lhs_a;
    //   ShortArray lhs_sa;
    //   ArrayOfShortArray lhs_asa;
    //   StructSeq lhs_ss;
    //   MyEnum lhs_e;
    //   MyUnion lhs_u;
    // };

    Sample_Dissector *
    Sample_Manager::make_Target_Dissector ()
    {
      Sample_Dissector *sample = new Sample_Dissector;
      sample->init("IDL:Target:1.0");

      Sample_Dissector *field = this->find ("IDL:A:1.0");
      if (field == 0)
        field = this->make_A_Dissector();
      Sample_Field *f = sample->add_field (field, "lhs_a");
      if ((field = this->find ("IDL:ShortArray:1.0")) == 0)
        field = this->make_ShortArray_Dissector();
      f->chain (field, "lhs_sa");
      if ((field = this->find ("IDL:ArrayOfShortArray:1.0")) == 0)
        field = this->make_ArrayOfShortArray_Dissector();
      f->chain (field, "lhs_asa");
      if ((field = this->find ("IDL:StructSeq:1.0")) == 0)
        field = this->make_StructSeq_Dissector();
      f->chain (field, "lhs_ss");
      if ((field = this->find ("IDL:MyEnum:1.0")) == 0)
        field = this->make_MyEnum_Dissector();
      f->chain (field, "lhs_e");
      if ((field = this->find ("IDL:MyUnion:1.0")) == 0)
        field = this->make_MyUnion_Dissector();
      f->chain (field, "lhs_u");

      return sample;
    }


  }
}
