#include "../idl_test1_lib/FooDefTypeSupportImpl.h"
#include "ace/ACE.h"
#include "ace/Log_Msg.h"
#include <map>
#include "tao/CDR.h"

namespace {
  template <typename T>
  size_t find_size(const T& data, size_t& padding)
  {
    size_t size = 0;
    padding = 0;
    OpenDDS::DCPS::gen_find_size(data, size, padding);
    return size;
  }
}

// this test tests the opendds_idl generated code for type XyZ::Foo from idl_test1_lib.
int ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  int failed = false;
  bool dump_buffer = false;

  if (argc > 1) dump_buffer = true;
  size_t padding;

  {
    Xyz::StructAUnion sau;
    sau.sau_f1._d(Xyz::redx);
    sau.sau_f1.rsv("joe");
    // size = union descr/4 + string length/4 + string contents/4
    if (find_size(sau, padding) != 4+4+4) {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("StructAUnion find_size failed with = %B\n"),
        find_size(sau, padding)));
      failed = true;
    }
    if (padding != 0) {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("StructAUnion find_size padding failed with = %B\n"),
        padding));
      failed = true;
    }
  }
  {
    Xyz::AStringSeq ass;
    ass.length(2); //4 for seq length
    ass[0] = "four"; //4+5 strlen + string
    ass[1] = "five5"; //4+6 strlen + string
    size_t size_ass = find_size(ass, padding);
    if (size_ass != 23) {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("AStringSeq find_size failed with = %B ; expecting 23\n"),
        size_ass));
      failed = true;
    }
    if (padding != 3) {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("AStringSeq find_size padding failed with = %B\n"),
        padding));
      failed = true;
    }
  }
  {
    Xyz::ArrayOfShortsSeq ash;
    ash.length(2); //4 for seq length + 5*2 for arry *2 length
    size_t size_ash = find_size(ash, padding);
    if (size_ash != 24) {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("ArrayOfShortsSeq find_size failed with = %B ; expecting 24\n"),
        size_ash));
      failed = true;
    }
    if (padding != 0) {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("ArrayOfShortsSeq find_size padding failed with = %B\n"),
        padding));
      failed = true;
    }
  }
  {
    Xyz::StructContainingArrayOfAStruct aas;
    aas.f1[0].v2s.length(2); //4 for v1 + 4 for length seq + 2*2
    aas.f1[1].v2s.length(1); //4 for v1 + 4 for length seq + 2
    aas.f1[2].v2s.length(0); //4 for v1 + 4 for length seq + 0
    size_t size_aas = find_size(aas, padding);
    if (size_aas != 30) {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("StructContainingArrayOfAStruct find_size failed with = %B ; expecting 30\n"),
        size_aas));
      failed = true;
    }
    if (padding != 2) {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("StructContainingArrayOfAStruct find_size padding failed with = %B\n"),
        padding));
      failed = true;
    }
  }
  {
    Xyz::StructContainingArrayOfAStructSeq aas;
    aas.f1[0].length(1); //4 for length
    aas.f1[0][0].v2s.length(2); //4 for v1 + 4 for length seq + 2*2
    aas.f1[1].length(1);//4 for length
    aas.f1[1][0].v2s.length(1); //4 for v1 + 4 for length seq + 2
    aas.f1[2].length(1);//4 for length
    aas.f1[2][0].v2s.length(0); //4 for v1 + 4 for length seq + 0
    size_t size_aas = find_size(aas, padding);
    if (size_aas != 42) {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("StructContainingArrayOfAStructSeq find_size failed ")
        ACE_TEXT("with = %B ; expecting 42\n"),
        size_aas));
      failed = true;
    }
    if (padding != 2) {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("StructContainingArrayOfAStructSeq find_size padding failed with = %B\n"),
        padding));
      failed = true;
    }
  }
  {
  N1::FwdDeclSameNamespaceStructs fwddeclstructs;
  fwddeclstructs.length(2);
  fwddeclstructs[0].v1 = -5;
  fwddeclstructs[1].v1 = 43;
  ACE_Message_Block* b = new ACE_Message_Block( 100000) ;
  OpenDDS::DCPS::Serializer serializer( b, false) ;

  serializer << fwddeclstructs;

  OpenDDS::DCPS::Serializer deserializer( b, false) ;
  N1::FwdDeclSameNamespaceStructs fwddeclstructs2;
  deserializer >> fwddeclstructs2;

  if (fwddeclstructs2.length() != 2)
    {
      ACE_ERROR((LM_ERROR, "FwdDeclSameNamespaceStructs Size failed\n"));
      failed = true;
    }

  if (fwddeclstructs2[0].v1 != -5)
    {
      ACE_ERROR((LM_ERROR, "FwdDeclSameNamespaceStructs array index 0 failed\n"));
      failed = true;
    }

  if (fwddeclstructs2[1].v1 != 43)
    {
      ACE_ERROR((LM_ERROR, "FwdDeclSameNamespaceStructs array index 1 failed\n"));
      failed = true;
    }
  }

  {
  N2::FwdDeclDiffNamespaceStructs fwddeclstructs;
  fwddeclstructs.length(2);
  fwddeclstructs[0].v1 = -5;
  fwddeclstructs[1].v1 = 43;
  ACE_Message_Block* b = new ACE_Message_Block( 100000) ;
  OpenDDS::DCPS::Serializer serializer( b, false) ;

  serializer << fwddeclstructs;

  OpenDDS::DCPS::Serializer deserializer( b, false) ;
  N2::FwdDeclDiffNamespaceStructs fwddeclstructs2;
  deserializer >> fwddeclstructs2;

  if (fwddeclstructs2.length() != 2)
    {
      ACE_ERROR((LM_ERROR, "FwdDeclDiffNamespaceStructs Size failed\n"));
      failed = true;
    }

  if (fwddeclstructs2[0].v1 != -5)
    {
      ACE_ERROR((LM_ERROR, "FwdDeclDiffNamespaceStructs array index 0 failed\n"));
      failed = true;
    }

  if (fwddeclstructs2[1].v1 != 43)
    {
      ACE_ERROR((LM_ERROR, "FwdDeclDiffNamespaceStructs array index 1 failed\n"));
      failed = true;
    }
  }

  {
    Xyz::StructOfArrayOfArrayOfShorts2 aas;
    size_t size_aas = find_size(aas, padding);
    if (size_aas != 18) {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("StructOfArrayOfArrayOfShorts2 find_size failed ")
        ACE_TEXT("with = %B ; expecting 18\n"),
        size_aas));
      failed = true;
    }
    if (padding != 0) {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("StructOfArrayOfArrayOfShorts2 find_size padding failed with = %B\n"),
        padding));
      failed = true;
    }

    CORBA::Short count = 0;
    for (int i = 0; i < 3 ; i++) {
      for (int j = 0; j < 2 ; j++) {
        aas.f1[i][j] = count++;
      }
    }

    ACE_Message_Block mb(size_aas);
    OpenDDS::DCPS::Serializer ss(&mb);

    if (!(ss << aas)) {
      ACE_ERROR((LM_ERROR, "Serializing StructOfArrayOfArrayOfShorts2 failed\n"));
      failed = true;
    }

    OpenDDS::DCPS::Serializer ss2(&mb);

    Xyz::StructOfArrayOfArrayOfShorts2 aas2;
    if (!(ss2 >> aas2)) {
      ACE_ERROR((LM_ERROR, "Deserializing StructOfArrayOfArrayOfShorts2 failed\n"));
      failed = true;
    } else {
      count = 0;
      for (int i = 0; i < 3 ; i++) {
        for (int j = 0; j < 2 ; j++) {
          if (aas.f1[i][j] != count++) {
            ACE_ERROR((LM_ERROR,
                       ACE_TEXT("arrayOfArray[%d][%d] failed serailization\n"),
                       i, j));
            failed = true;
          }
        }
      }
    }
  }

  if (!OpenDDS::DCPS::DDSTraits<Xyz::AStruct>::gen_has_key()) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("_dcps_has_key(Xyz::AStruct) returned false when expecting true.\n")
      ));
  }

  if (OpenDDS::DCPS::DDSTraits<Xyz::StructContainingArrayOfAStructSeq>::gen_has_key()) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("_dcps_has_key(Xyz::StructContainingArrayOfAStructSeq) returned true when expecting false.\n")
      ));
  }

  Xyz::Foo my_foo;

  // greenx < redx < bluex
  const Xyz::ColorX orig_color = Xyz::greenx;

  // -- //+# is keeping track of the find_size
  my_foo.key = 99; //4
  my_foo.octer = 0x13; //+1 = 5
  my_foo.xcolor = orig_color; //+4 = 9     {padding 3}
  my_foo.ooo[0] = 0xff; //+3 = 12
  my_foo.ooo[1] = 0x80;
  my_foo.ooo[2] = 0x3d;
  my_foo.ushrtseq.length(2); //+4+2*2 = 20 {padding +1 = 4}
  my_foo.ushrtseq[0] = 7;
  my_foo.ushrtseq[1] = 11;
  // my_foo.thestruct        //+8 = 28
  // my_foo.theStructSeq     //+4 = 32
  my_foo.theString = "four"; //+4+5 = 41
  // my_foo.structArray      //+24 = 65    {padding +3 = 7}
  my_foo.x = 0.99f; //+4 = 69
  my_foo.y = 0.11f; //+4 = 73
  // my_foo.theUnion //+6 = 79

  Xyz::Foo foo2;
  foo2.key = 99;
  foo2.x = 0.99f;
  foo2.y = 0.11f;
  foo2.octer = 0x13;
  foo2.xcolor = orig_color;
  // syntax error foo2.ooo = {0xff, 0x80, 0x3d};
  foo2.ooo[0] = 0xff;
  foo2.ooo[1] = 0x80;
  foo2.ooo[2] = 0x3d;
  foo2.ushrtseq.length(2);
  foo2.ushrtseq[0] = 7;
  foo2.ushrtseq[1] = 11;
  foo2.theString = "four";

  std::map<Xyz::Foo, Xyz::Foo*, Xyz::Foo_OpenDDS_KeyLessThan> foomap;

  if (OpenDDS::DCPS::DDSTraits<Xyz::Foo>::gen_has_key()) {
    foomap[my_foo] = &my_foo;
    foomap[foo2] = &foo2;
    // foo2 and my_foo should have mapped to the same place
    if (foomap[my_foo] != &foo2) {
      ACE_ERROR((LM_ERROR, "FooKeyLessThan failed with map\n"));
      failed = true;
    }

    my_foo.key = 77;
    my_foo.xcolor = Xyz::redx;
    foomap[my_foo] = &my_foo;
    if (foomap[foo2] != &foo2) {
      ACE_ERROR((LM_ERROR, "FooKeyLessThan failed with map - 2\n"));
      failed = true;
    }

    if (foomap[my_foo]->key != 77) {
      ACE_ERROR((LM_ERROR, "FooKeyLessThan failed with map - 3a\n"));
      failed = true;
    }

    if (foomap[my_foo]->xcolor != Xyz::redx) {
      ACE_ERROR((LM_ERROR, "FooKeyLessThan failed with map - 3b\n"));
      failed = true;
    }

    if (foomap[foo2]->xcolor != orig_color) {
      ACE_ERROR((LM_ERROR, "FooKeyLessThan failed with map - 4\n"));
      failed = true;
    }
  } else {
    ACE_DEBUG((LM_DEBUG, "NOTE: _dcps_has_key(foo) returned false\n"));
  }

  const CORBA::Boolean expected_bounded = false;
  const size_t expected_find_size = 79;
  const size_t expected_padding = 7;

  size_t ms = OpenDDS::DCPS::gen_max_marshaled_size(my_foo, false /*align*/);
  CORBA::Boolean bounded = OpenDDS::DCPS::MarshalTraits<Xyz::Foo>::gen_is_bounded_size();
  size_t cs = find_size(my_foo, padding);

  ACE_DEBUG((LM_DEBUG,"OpenDDS::DCPS::gen_max_marshaled_size(my_foo) => %B\n", ms));
  ACE_DEBUG((LM_DEBUG,"OpenDDS::DCPS::gen_is_bounded_size(my_foo) => %d\n", int(bounded)));
  ACE_DEBUG((LM_DEBUG,"OpenDDS::DCPS::gen_find_size(my_foo) => %B\n", cs));

  if (bounded != expected_bounded) {
    ACE_ERROR((LM_ERROR, "OpenDDS::DCPS::gen_is_bounded_size(Foo) failed - expected %d got %d\n",
      int(expected_bounded), int(bounded)));
    failed = true;
  }

  if (!bounded && cs != expected_find_size) {
    ACE_ERROR((LM_ERROR,
      "OpenDDS::DCPS::gen_find_size(Foo) returned %B when was expecting %B\n",
      cs, expected_find_size));
    failed = true;
  }

  if (padding != expected_padding) {
    ACE_ERROR((LM_ERROR,
      "OpenDDS::DCPS::gen_find_size(Foo) padding = %B when was expecting %B\n",
      padding, expected_padding));
    failed = true;
  }

  // test serializing

  const size_t buff_size = bounded ? ms : cs;
  ACE_Message_Block mb(buff_size);
  OpenDDS::DCPS::Serializer ss(&mb);
  TAO_OutputCDR cdr;
  OpenDDS::DCPS::Serializer ss2(&mb);

  Xyz::Foo ss_foo;
  try {
    if (!(cdr << my_foo)) {
      ACE_ERROR((LM_ERROR, "TAO_OutputCDR << failed\n"));
      failed = true;
    }

    ACE_TCHAR ebuffer[512]; ebuffer[0] = ACE_TEXT('\0');
    ACE_TCHAR obuffer[512]; obuffer[0] = ACE_TEXT('\0');


    if (dump_buffer) {
      ACE::format_hexdump(mb.rd_ptr(), mb.length(), ebuffer, sizeof(ebuffer));
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("BEFORE WRITING, LENGTH: %B, BUFFER:\n%s\n"),
        mb.length(), ebuffer));
    }

    if (!(ss << my_foo)) {
      ACE_ERROR((LM_ERROR, "Serializing failed\n"));
      failed = true;
    }

    if (dump_buffer) {
      ACE::format_hexdump(mb.rd_ptr(), mb.length(), ebuffer, sizeof(ebuffer));
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("AFTER WRITING, LENGTH: %B, BUFFER:\n%s\n"),
        mb.length(), ebuffer));
    }

    if (!(ss2 >> ss_foo)) {
      ACE_ERROR((LM_ERROR, "Deserializing failed\n"));
      failed = true;
    }

    if (dump_buffer) {
      ACE::format_hexdump(mb.rd_ptr(), mb.length(), ebuffer, sizeof(ebuffer));
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("AFTER READING, LENGTH: %B, BUFFER:\n%s\n"),
        mb.length(), ebuffer));

      ACE::format_hexdump( (char*)&(my_foo.key), sizeof(ACE_CDR::ULong),
        ebuffer, sizeof(ebuffer));
      ACE::format_hexdump( (char*)&(ss_foo.key), sizeof(ACE_CDR::ULong),
        obuffer, sizeof(obuffer));
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("key (expected:\n%s, observed:\n%s)\n"),
                 ebuffer, obuffer));
      ACE::format_hexdump( (char*)&(my_foo.x), sizeof(ACE_CDR::Float),
        ebuffer, sizeof(ebuffer));
      ACE::format_hexdump( (char*)&(ss_foo.x), sizeof(ACE_CDR::Float),
        obuffer, sizeof(obuffer));
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("x (expected:\n%s, observed:\n%s)\n"),
                 ebuffer, obuffer));
      ACE::format_hexdump( (char*)&(my_foo.y), sizeof(ACE_CDR::Float),
        ebuffer, sizeof(ebuffer));
      ACE::format_hexdump( (char*)&(ss_foo.y), sizeof(ACE_CDR::Float),
        obuffer, sizeof(obuffer));
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("y (expected:\n%s, observed:\n%s)\n"),
                 ebuffer, obuffer));
      ACE::format_hexdump( (char*)&(my_foo.xcolor), sizeof(ACE_CDR::UShort), ebuffer, sizeof(ebuffer));
      ACE::format_hexdump( (char*)&(ss_foo.xcolor), sizeof(ACE_CDR::UShort), obuffer, sizeof(obuffer));
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("xcolor (expected:\n%s, observed:\n%s)\n"),
                 ebuffer, obuffer));
      ACE::format_hexdump( (char*)&(my_foo.octer), sizeof(ACE_CDR::Octet), ebuffer, sizeof(ebuffer));
      ACE::format_hexdump( (char*)&(ss_foo.octer), sizeof(ACE_CDR::Octet), obuffer, sizeof(obuffer));
      ACE_ERROR((LM_ERROR,
                 ACE_TEXT("octer (expected:\n%s, observed:\n%s)\n"),
                 ebuffer, obuffer));
    }

    if (ss_foo.key != my_foo.key) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("Failed to serialize key\n")));
      failed = true;
    } else if (ss_foo.x != my_foo.x) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("Failed to serialize x\n")));
      failed = true;
    } else if (ss_foo.y != my_foo.y) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("Failed to serialize y\n")));
      failed = true;
    } else if (ss_foo.xcolor != my_foo.xcolor) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("Failed to serialize xcolor\n")));
      failed = true;
    } else if (ss_foo.octer != my_foo.octer) {
      ACE_ERROR((LM_ERROR, ACE_TEXT("Failed to serialize octer\n")));
      failed = true;
    } else if (0 != strcmp(ss_foo.theString.in(), my_foo.theString.in())) {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("Failed to serialize theString \"%C\" => \"%C\"\n"),
        my_foo.theString.in(), ss_foo.theString.in()));
      failed = true;
    } else if (ss_foo.ooo[0] != my_foo.ooo[0]
               || ss_foo.ooo[1] != my_foo.ooo[1]
               || ss_foo.ooo[2] != my_foo.ooo[2]) {
      ACE_ERROR((LM_ERROR, "Failed to serialize ooo\n"));
      failed = true;
    }

  } catch (const CORBA::Exception& e) {
    e._tao_print_exception("Exception caught in main():");
    return -1;
  }

  if (failed)
    ACE_ERROR((LM_ERROR, "%s FAILED!\n", argv[0]));
  else
    ACE_DEBUG((LM_ERROR, "%s PASSED\n", argv[0]));

  return failed; // let the test framework know it failed
}
