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
      init_base_sequences ();
      init_for_MultiTopicTest();
      //init_for_CorbaSeqTest();
      init_for_MetaStructTest();
      init_for_QueryConditionTest();
#if 0
      make_LocationInfo_Dissector();
      make_PlanInfo_Dissector();
      make_MoreInfo_Dissector();
      make_UnrelatedInfo_Dissector();
      make_Resulting_Dissector();

      make_Message_Dissector();
      make_Message2_Dissector();

      make_A_Dissector();
      make_ShortArray_Dissector ();
      make_ArrayOfShortArray_Dissector ();
      make_StructSeq_Dissector ();
      make_MyEnum_Dissector ();
      make_MyUnion_Dissector ();
      make_Source_Dissector ();
      make_Target_Dissector ();
#endif
    };

    void
    Sample_Manager::add (Sample_Dissector &d)
    {
      std::string &key = d.typeId();
      ACE_DEBUG ((LM_DEBUG,"Adding new dissector for %s\n",key.c_str()));

      dissectors_.bind(key.c_str(),&d);
    }

    Sample_Dissector *
    Sample_Manager::find (const char *data_name)
    {
      std::string key(data_name);
      std::string typesupport ("TypeSupport");
      size_t ts = key.find (typesupport);
      if (ts != std::string::npos)
        {
          std::string version = key.substr (ts + typesupport.length());
          key = key.substr (0,ts);
          key.append (version);
          ACE_DEBUG ((LM_DEBUG, "looking for new key= %s\n", key.c_str()));
        }

      Sample_Dissector *result = 0;
      dissectors_.find (key.c_str(), result);
      return result;
    }

   //----------------------------------------------------------------------

    void
    Sample_Manager::init_base_sequences ()
    {
      new Sample_Sequence("IDL:CORBA/BooleanSeq:1.0",
                          Sample_Field::Boolean);
      new Sample_Sequence("IDL:CORBA/CharSeq:1.0",
                          Sample_Field::Char);
      new Sample_Sequence("IDL:CORBA/OctetSeq:1.0",
                          Sample_Field::Octet);

      new Sample_Sequence("IDL:CORBA/WCharSeq:1.0",
                          Sample_Field::WChar);
      new Sample_Sequence("IDL:CORBA/ShortSeq:1.0",
                          Sample_Field::Short);
      new Sample_Sequence("IDL:CORBA/UShortSeq:1.0",
                          Sample_Field::UShort);

      new Sample_Sequence("IDL:CORBA/LongSeq:1.0",
                          Sample_Field::Long);
      new Sample_Sequence("IDL:CORBA/ULongSeq:1.0",
                          Sample_Field::ULong);
      new Sample_Sequence("IDL:CORBA/LongLongSeq:1.0",
                          Sample_Field::LongLong);
      new Sample_Sequence("IDL:CORBA/ULongLongSeq:1.0",
                          Sample_Field::ULongLong);

      new Sample_Sequence("IDL:CORBA/FloatSeq:1.0",
                          Sample_Field::Float);
      new Sample_Sequence("IDL:CORBA/DoubleSeq:1.0",
                          Sample_Field::Double);
      new Sample_Sequence("IDL:CORBA/LongDoubleSeq:1.0",
                          Sample_Field::LongDouble);

      new Sample_Sequence("IDL:CORBA/StringSeq:1.0",
                          Sample_Field::String);
      new Sample_Sequence("IDL:CORBA/WStringSeq:1.0",
                          Sample_Field::WString);
    }

    //----------------------------------------------------------------------
    // from multiTopic test
#if 0
    void
    Sample_Manager::make_LocationInfo_Dissector ()
    {
      Sample_Dissector *sample =
        new Sample_Dissector ("IDL:LocationInfo:1.0", "LocationInfo");
      Sample_Field *f =
        sample->add_field (Sample_Field::ULong, "flight_id1");
      f = f->chain (Sample_Field::Long, "flight_id2");
      f = f->chain (Sample_Field::Long, "x");
      f = f->chain (Sample_Field::Long, "y");
      f = f->chain (Sample_Field::Long, "z");
    }

    void
    Sample_Manager::make_PlanInfo_Dissector ()
    {
      Sample_Dissector *sample =
        new Sample_Dissector ("IDL:PlanInfo:1.0", "PlanInfo");
      Sample_Field *f =
        sample->add_field (Sample_Field::ULong, "flight_id1");
      f = f->chain (Sample_Field::Long, "flight_id2");
      f = f->chain (Sample_Field::String, "flight_name");
      f = f->chain (Sample_Field::String, "tailno");
    }

    void
    Sample_Manager::make_MoreInfo_Dissector ()
    {
      Sample_Dissector *sample =
        new Sample_Dissector ("IDL:MoreInfo:1.0", "MoreInfo");
      Sample_Field *f =
        sample->add_field (Sample_Field::ULong, "flight_id1");
      f = f->chain (Sample_Field::String, "more");
    }

    void
    Sample_Manager::make_UnrelatedInfo_Dissector ()
    {
      Sample_Dissector *sample =
        new Sample_Dissector ("IDL:UnrelatedInfo:1.0", "UnrelatedInfo");
      sample->add_field (Sample_Field::String, "misc");
    }

    void
    Sample_Manager::make_Resulting_Dissector ()
    {
      Sample_Dissector *sample =
        new Sample_Dissector ("IDL:Resulting:1.0", "Resulting");
      Sample_Field *f =
        sample->add_field (Sample_Field::ULong, "flight_id1");
      f = f->chain (Sample_Field::Long, "flight_id2");
      f = f->chain (Sample_Field::String, "flight_name");
      f = f->chain (Sample_Field::Long, "x");
      f = f->chain (Sample_Field::Long, "y");
      f = f->chain (Sample_Field::Long, "height");
      f = f->chain (Sample_Field::String, "more");
      f = f->chain (Sample_Field::String, "misc");
    }
#else
    void
    Sample_Manager::init_for_MultiTopicTest ()
    {
      Sample_Dissector *sample =
        new Sample_Dissector ("IDL:LocationInfo:1.0", "LocationInfo");
      Sample_Field *f =
        sample->add_field (Sample_Field::ULong, "flight_id1");
      f = f->chain (Sample_Field::Long, "flight_id2");
      f = f->chain (Sample_Field::Long, "x");
      f = f->chain (Sample_Field::Long, "y");
      f = f->chain (Sample_Field::Long, "z");

      sample =
        new Sample_Dissector ("IDL:PlanInfo:1.0", "PlanInfo");
      f = sample->add_field (Sample_Field::ULong, "flight_id1");
      f = f->chain (Sample_Field::Long, "flight_id2");
      f = f->chain (Sample_Field::String, "flight_name");
      f = f->chain (Sample_Field::String, "tailno");

      sample =
        new Sample_Dissector ("IDL:MoreInfo:1.0", "MoreInfo");
      f = sample->add_field (Sample_Field::ULong, "flight_id1");
      f = f->chain (Sample_Field::String, "more");

      sample =
        new Sample_Dissector ("IDL:UnrelatedInfo:1.0", "UnrelatedInfo");
      sample->add_field (Sample_Field::String, "misc");

      sample =
        new Sample_Dissector ("IDL:Resulting:1.0", "Resulting");
      f = sample->add_field (Sample_Field::ULong, "flight_id1");
      f = f->chain (Sample_Field::Long, "flight_id2");
      f = f->chain (Sample_Field::String, "flight_name");
      f = f->chain (Sample_Field::Long, "x");
      f = f->chain (Sample_Field::Long, "y");
      f = f->chain (Sample_Field::Long, "height");
      f = f->chain (Sample_Field::String, "more");
      f = f->chain (Sample_Field::String, "misc");
    }
#endif

    //---------------------------------------------------------------------
#if 0
    void
    Sample_Manager::make_Message_Dissector ()
    {
      Sample_Dissector *sample =
        new Sample_Dissector ("IDL:Messenger/Message:1.0",
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
        sample->add_field (Sample_Field::String, "from");
      f = f->chain (Sample_Field::String, "subject");
      f = f->chain (Sample_Field::Long, "subject_id");
      f = f->chain (Sample_Field::String, "text");
      f = f->chain (Sample_Field::Long, "count");
      f = f->chain (find ("IDL:CORBA/BooleanSeq:1.0"), "bool_seq");
      f = f->chain (find ("IDL:CORBA/LongDoubleSeq:1.0"),"longdouble_seq");
      f = f->chain (find ("IDL:CORBA/ShortSeq:1.0"), "short_seq");
      f = f->chain (find ("IDL:CORBA/UShortSeq:1.0"), "ushort_seq");
      f = f->chain (find ("IDL:CORBA/CharSeq:1.0"), "char_seq");
      f = f->chain (find ("IDL:CORBA/LongLongSeq:1.0"), "longlong_seq");
      f = f->chain (find ("IDL:CORBA/StringSeq:1.0"), "string_seq");
      f = f->chain (find ("IDL:CORBA/WCharSeq:1.0"), "wchar_seq");
      f = f->chain (find ("IDL:CORBA/DoubleSeq:1.0"), "double_seq");
      f = f->chain (find ("IDL:CORBA/LongSeq:1.0"), "long_seq");
      f = f->chain (find ("IDL:CORBA/ULongLongSeq:1.0"), "ulonglong_seq");
      f = f->chain (find ("IDL:CORBA/WStringSeq:1.0"), "wstring_seq");
      f = f->chain (find ("IDL:CORBA/FloatSeq:1.0"), "float_seq");
      f = f->chain (find ("IDL:CORBA/OctetSeq:1.0"), "octet_seq");
      f = f->chain (find ("IDL:CORBA/ULongSeq:1.0"), "ulong_seq");

      // this should be Messaenger2::LongSeq;
      f = f->chain (find ("IDL:CORBA/LongSeq:1.0"), "outside_long_seq");
    }

    void
    Sample_Manager::make_Message2_Dissector ()
    {
      Sample_Dissector *sample =
        new Sample_Dissector ("IDL:Messenger2/Message2:1.0",
                              "Messenger2::Message2");

      Sample_Field *f =
        sample->add_field (Sample_Field::String, "from");
      f = f->chain (Sample_Field::String, "subject");
      f = f->chain (Sample_Field::Long, "subject_id");
      f = f->chain (Sample_Field::String, "text");
      f = f->chain (Sample_Field::Long, "count");
      f = f->chain (find("IDL:CORBA/BooleanSeq:1.0"), "bool_seq");
      f = f->chain (find ("IDL:CORBA/LongDoubleSeq:1.0"),"longdouble_seq");
      f = f->chain (find ("IDL:CORBA/ShortSeq:1.0"), "short_seq");
      f = f->chain (find ("IDL:CORBA/UShortSeq:1.0"), "ushort_seq");
      f = f->chain (find ("IDL:CORBA/CharSeq:1.0"), "char_seq");
      f = f->chain (find ("IDL:CORBA/LongLongSeq:1.0"), "longlong_seq");
      f = f->chain (find ("IDL:CORBA/StringSeq:1.0"), "string_seq");
      f = f->chain (find ("IDL:CORBA/WCharSeq:1.0"), "wchar_seq");
      f = f->chain (find ("IDL:CORBA/DoubleSeq:1.0"), "double_seq");
      f = f->chain (find ("IDL:CORBA/LongSeq:1.0"), "long_seq");
      f = f->chain (find ("IDL:CORBA/ULongLongSeq:1.0"), "ulonglong_seq");
      f = f->chain (find ("IDL:CORBA/WStringSeq:1.0"), "wstring_seq");
      f = f->chain (find ("IDL:CORBA/FloatSeq:1.0"), "float_seq");
      f = f->chain (find ("IDL:CORBA/OctetSeq:1.0"), "octet_seq");
      f = f->chain (find ("IDL:CORBA/ULongSeq:1.0"), "ulong_seq");
    }
#else
    void
    Sample_Manager::init_for_CorbaSeqTest ()
    {
      Sample_Dissector *sample =
        new Sample_Dissector ("IDL:Messenger/Message:1.0",
                              "Messenger::Message");
      Sample_Field *f =
        sample->add_field (Sample_Field::String, "from");
      f = f->chain (Sample_Field::String, "subject");
      f = f->chain (Sample_Field::Long, "subject_id");
      f = f->chain (Sample_Field::String, "text");
      f = f->chain (Sample_Field::Long, "count");
      f = f->chain (find ("IDL:CORBA/BooleanSeq:1.0"), "bool_seq");
      f = f->chain (find ("IDL:CORBA/LongDoubleSeq:1.0"),"longdouble_seq");
      f = f->chain (find ("IDL:CORBA/ShortSeq:1.0"), "short_seq");
      f = f->chain (find ("IDL:CORBA/UShortSeq:1.0"), "ushort_seq");
      f = f->chain (find ("IDL:CORBA/CharSeq:1.0"), "char_seq");
      f = f->chain (find ("IDL:CORBA/LongLongSeq:1.0"), "longlong_seq");
      f = f->chain (find ("IDL:CORBA/StringSeq:1.0"), "string_seq");
      f = f->chain (find ("IDL:CORBA/WCharSeq:1.0"), "wchar_seq");
      f = f->chain (find ("IDL:CORBA/DoubleSeq:1.0"), "double_seq");
      f = f->chain (find ("IDL:CORBA/LongSeq:1.0"), "long_seq");
      f = f->chain (find ("IDL:CORBA/ULongLongSeq:1.0"), "ulonglong_seq");
      f = f->chain (find ("IDL:CORBA/WStringSeq:1.0"), "wstring_seq");
      f = f->chain (find ("IDL:CORBA/FloatSeq:1.0"), "float_seq");
      f = f->chain (find ("IDL:CORBA/OctetSeq:1.0"), "octet_seq");
      f = f->chain (find ("IDL:CORBA/ULongSeq:1.0"), "ulong_seq");

      // this should be Messaenger2::LongSeq;
      f = f->chain (find ("IDL:CORBA/LongSeq:1.0"), "outside_long_seq");

      sample =
        new Sample_Dissector ("IDL:Messenger2/Message2:1.0",
                              "Messenger2::Message2");

      f = sample->add_field (Sample_Field::String, "from");
      f = f->chain (Sample_Field::String, "subject");
      f = f->chain (Sample_Field::Long, "subject_id");
      f = f->chain (Sample_Field::String, "text");
      f = f->chain (Sample_Field::Long, "count");
      f = f->chain (find("IDL:CORBA/BooleanSeq:1.0"), "bool_seq");
      f = f->chain (find ("IDL:CORBA/LongDoubleSeq:1.0"),"longdouble_seq");
      f = f->chain (find ("IDL:CORBA/ShortSeq:1.0"), "short_seq");
      f = f->chain (find ("IDL:CORBA/UShortSeq:1.0"), "ushort_seq");
      f = f->chain (find ("IDL:CORBA/CharSeq:1.0"), "char_seq");
      f = f->chain (find ("IDL:CORBA/LongLongSeq:1.0"), "longlong_seq");
      f = f->chain (find ("IDL:CORBA/StringSeq:1.0"), "string_seq");
      f = f->chain (find ("IDL:CORBA/WCharSeq:1.0"), "wchar_seq");
      f = f->chain (find ("IDL:CORBA/DoubleSeq:1.0"), "double_seq");
      f = f->chain (find ("IDL:CORBA/LongSeq:1.0"), "long_seq");
      f = f->chain (find ("IDL:CORBA/ULongLongSeq:1.0"), "ulonglong_seq");
      f = f->chain (find ("IDL:CORBA/WStringSeq:1.0"), "wstring_seq");
      f = f->chain (find ("IDL:CORBA/FloatSeq:1.0"), "float_seq");
      f = f->chain (find ("IDL:CORBA/OctetSeq:1.0"), "octet_seq");
      f = f->chain (find ("IDL:CORBA/ULongSeq:1.0"), "ulong_seq");
    }

#endif

    //------------------------------------------------------------------------
    // From MetaStructTest.idl

#if 0
    //
    // struct A {
    //   string s;
    //   long l;
    // };

    void
    Sample_Manager::make_A_Dissector ()
    {
      Sample_Dissector *sample =
        new Sample_Dissector ("IDL:A:1.0","A");
      Sample_Field *f =
        sample->add_field (Sample_Field::String,"bs");
      f = f->chain (Sample_Field::Long, "l");
    }

    // typedef short ShortArray[3];

    void
    Sample_Manager::make_ShortArray_Dissector ()
    {
      new Sample_Array ("IDL:ShortArray:1.0",3, Sample_Field::Short);
    }

    // typedef ShortArray ArrayOfShortArray[4];

    void
    Sample_Manager::make_ArrayOfShortArray_Dissector ()
    {
      new Sample_Array ("IDL:ArrayOfShortArray:1.0", 4,
                        find ("IDL:ShortArray:1.0") );
    }

    // typedef sequence<A> StructSeq;

    void
    Sample_Manager::make_StructSeq_Dissector ()
    {
      new Sample_Sequence ("IDL:StructSeq:1.0",find ("IDL:A:1.0"));
    }

    // enum MyEnum {b, d, as, sa, ss, other1, other2};

    void
    Sample_Manager::make_MyEnum_Dissector ()
    {
      Sample_Enum *sample =
        new Sample_Enum("IDL:MyEnum:1.0");

      Sample_Field *n = sample->add_value ("b");
      n->chain (Sample_Field::Enumeration, "d");
      n->chain (Sample_Field::Enumeration, "as");
      n->chain (Sample_Field::Enumeration, "sa");
      n->chain (Sample_Field::Enumeration, "ss");
      n->chain (Sample_Field::Enumeration, "other1");
      n->chain (Sample_Field::Enumeration, "other2");
    }


    // union MyUnion switch (MyEnum) {
    // case b: boolean u_b;
    // case d: double u_d;
    // case as: A u_as;
    // case sa: ShortArray u_sa;
    // case ss: StructSeq u_ss;
    // default: float u_f;
    // };

    void
    Sample_Manager::make_MyUnion_Dissector ()
    {
      Sample_Union *sample = new Sample_Union ("IDL:MyUnion:1.0");
      sample->discriminator (find ("IDL:MyEnum:1.0"));
      sample->add_case ("b", new Sample_Field (Sample_Field::Boolean, "u_b"));
      sample->add_case ("d", new Sample_Field (Sample_Field::Double, "u_d"));
      sample->add_case ("as",
                        new Sample_Field (find ("IDL:A:1.0"), "u_as"));
      sample->add_case ("sa",
                        new Sample_Field (find ("IDL:ShortArray:1.0"), "u_sa"));
      sample->add_case ("ss",
                        new Sample_Field (find ("IDL:StructSeq:1.0"), "u_ss"));
      sample->add_default (new Sample_Field (Sample_Field::Float, "u_f"));
    }

    // struct Source {
    //   A rhs_a;
    //   ShortArray rhs_sa;
    //   ArrayOfShortArray rhs_asa;
    //   StructSeq rhs_ss;
    //   MyEnum rhs_e;
    //   MyUnion rhs_u;
    // };

    void
    Sample_Manager::make_Source_Dissector ()
    {
      Sample_Dissector *sample =
        new Sample_Dissector ("IDL:Source:1.0", "Source");
      Sample_Field *f =
        sample->add_field (find ("IDL:A:1.0"), "rhs_a");
      f->chain (find ("IDL:ShortArray:1.0"), "rhs_sa");
      f->chain (find ("IDL:ArrayOfShortArray:1.0"), "rhs_asa");
      f->chain (find ("IDL:StructSeq:1.0"), "rhs_ss");
      f->chain (find ("IDL:MyEnum:1.0"), "rhs_e");
      f->chain (find ("IDL:MyUnion:1.0"), "rhs_u");
    }

    // struct Target {
    //   A lhs_a;
    //   ShortArray lhs_sa;
    //   ArrayOfShortArray lhs_asa;
    //   StructSeq lhs_ss;
    //   MyEnum lhs_e;
    //   MyUnion lhs_u;
    // };

    void
    Sample_Manager::make_Target_Dissector ()
    {
      Sample_Dissector *sample =
        new Sample_Dissector ("IDL:Target:1.0", "Target");
      Sample_Field *f =
        sample->add_field (find ("IDL:A:1.0"), "lhs_a");
      f->chain (find ("IDL:ShortArray:1.0"), "lhs_sa");
      f->chain (find ("IDL:ArrayOfShortArray:1.0"), "lhs_asa");
      f->chain (find ("IDL:StructSeq:1.0"), "lhs_ss");
      f->chain (find ("IDL:MyEnum:1.0"), "lhs_e");
      f->chain (find ("IDL:MyUnion:1.0"), "lhs_u");
    }
#else
    void
    Sample_Manager::init_for_MetaStructTest()
    {
      Sample_Dissector *sample =
        new Sample_Dissector ("IDL:A:1.0","A");
      Sample_Field *f =
        sample->add_field (Sample_Field::String,"bs");
      f = f->chain (Sample_Field::Long, "l");

      new Sample_Array ("IDL:ShortArray:1.0",3, Sample_Field::Short);

      new Sample_Array ("IDL:ArrayOfShortArray:1.0", 4,
                        find ("IDL:ShortArray:1.0") );

      new Sample_Sequence ("IDL:StructSeq:1.0",find ("IDL:A:1.0"));

      Sample_Enum *s_enum =
        new Sample_Enum("IDL:MyEnum:1.0");

      f = s_enum->add_value ("b");
      f->chain (Sample_Field::Enumeration, "d");
      f->chain (Sample_Field::Enumeration, "as");
      f->chain (Sample_Field::Enumeration, "sa");
      f->chain (Sample_Field::Enumeration, "ss");
      f->chain (Sample_Field::Enumeration, "other1");
      f->chain (Sample_Field::Enumeration, "other2");

      Sample_Union *s_union = new Sample_Union ("IDL:MyUnion:1.0");
      s_union->discriminator (find ("IDL:MyEnum:1.0"));
      Switch_Case *sc =
        s_union->add_case ("b", new Sample_Field (Sample_Field::Boolean, "u_b"));
      sc->chain ("d", new Sample_Field (Sample_Field::Double, "u_d"));
      sc->chain ("as",
                 new Sample_Field (find ("IDL:A:1.0"), "u_as"));
      sc->chain ("sa",
                 new Sample_Field (find ("IDL:ShortArray:1.0"), "u_sa"));
      sc->chain ("ss",
                 new Sample_Field (find ("IDL:StructSeq:1.0"), "u_ss"));
      s_union->add_default (new Sample_Field (Sample_Field::Float, "u_f"));

      sample =
        new Sample_Dissector ("IDL:Source:1.0", "Source");
      f = sample->add_field (find ("IDL:A:1.0"), "rhs_a");
      f->chain (find ("IDL:ShortArray:1.0"), "rhs_sa");
      f->chain (find ("IDL:ArrayOfShortArray:1.0"), "rhs_asa");
      f->chain (find ("IDL:StructSeq:1.0"), "rhs_ss");
      f->chain (find ("IDL:MyEnum:1.0"), "rhs_e");
      f->chain (find ("IDL:MyUnion:1.0"), "rhs_u");

      sample =
        new Sample_Dissector ("IDL:Target:1.0", "Target");
      f = sample->add_field (find ("IDL:A:1.0"), "lhs_a");
      f->chain (find ("IDL:ShortArray:1.0"), "lhs_sa");
      f->chain (find ("IDL:ArrayOfShortArray:1.0"), "lhs_asa");
      f->chain (find ("IDL:StructSeq:1.0"), "lhs_ss");
      f->chain (find ("IDL:MyEnum:1.0"), "lhs_e");
      f->chain (find ("IDL:MyUnion:1.0"), "lhs_u");
    }

#endif

    void
    Sample_Manager::init_for_QueryConditionTest()
    {
//   enum X {
//     A, B, C
//   };

      Sample_Enum *s_enum =
        new Sample_Enum("IDL:Messenger/X:1.0");
      s_enum->add_value ("A");
      s_enum->add_value ("B");
      s_enum->add_value ("C");


//   struct Nested {
//     X value;
//   };

      Sample_Dissector *sample =
        new Sample_Dissector ("IDL:Messenger/Nested:1.0",
                              "Messenger::Nested");
      sample->add_field (find ("IDL:Messenger/X:1.0"), "value");

//   struct Message {
//     long key;
//     string name;
//     Nested nest;
//   };

      sample =
        new Sample_Dissector ("IDL:Messenger/Message:1.0",
                              "Messenger::Message");
      sample->add_field (Sample_Field::Long,"key");
      sample->add_field (Sample_Field::String,"name");
      sample->add_field (find ("IDL:Messenger/Nested:1.0"),"nest");
    }


  }
}
