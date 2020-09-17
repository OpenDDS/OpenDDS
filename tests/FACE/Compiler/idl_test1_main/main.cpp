#include "../idl_test1_lib/FooDefTypeSupportImpl.h"

#include <dds/DCPS/Message_Block_Ptr.h>
#include <dds/Version.h>

#include <ace/ACE.h>
#include <ace/Log_Msg.h>

#include <map>
#include <cstring>

using OpenDDS::DCPS::Encoding;
using OpenDDS::DCPS::SerializedSizeBound;

const Encoding encoding_plain_native(Encoding::KIND_XCDR1);
const Encoding encoding_unaligned_native(Encoding::KIND_UNALIGNED_CDR);

namespace {
  unsigned int bcd(unsigned int i)
  {
    return i / 10 * 16 + i % 10;
  }

  unsigned int convert_version(int maj, int min, int micro)
  {
    return bcd(static_cast<unsigned int>(maj)) << 16
      | bcd(static_cast<unsigned int>(min)) << 8
      | bcd(static_cast<unsigned int>(micro));
  }
}

// this test tests the opendds_idl generated code for type XyZ::Foo from idl_test1_lib.
int ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  int failed = false;
  bool dump_buffer = false;

  const unsigned int vers = convert_version(DDS_MAJOR_VERSION,
                                            DDS_MINOR_VERSION,
                                            DDS_MICRO_VERSION);
  if (vers != dds_version) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("Expected dds_version 0x%06x, actual 0x%06x\n"),
      dds_version, vers));
    failed = true;
  }

  if (argc > 1) dump_buffer = true;

  {
    Xyz::AStringSeq ass;
    ass.length(2); //4 for seq length
    ass[0] = "four"; //4+5 strlen + string
    ass[1] = "five5"; //4+6 strlen + string
    size_t size_ass = serialized_size(encoding_plain_native, ass);
    if (size_ass != 26) {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("AStringSeq find_size failed with = %B ; expecting 23\n"),
        size_ass));
      failed = true;
    }
  }
  {
    Xyz::ArrayOfShortsSeq ash;
    ash.length(2); //4 for seq length + 5*2 for arry *2 length
    size_t size_ash = serialized_size(encoding_plain_native, ash);
    if (size_ash != 24) {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("ArrayOfShortsSeq find_size failed with = %B ; expecting 24\n"),
        size_ash));
      failed = true;
    }
  }
  {
    Xyz::StructContainingArrayOfAStruct aas;
    aas.f1[0].v2s.length(2); //4 for v1 + 4 for length seq + 2*2
    aas.f1[1].v2s.length(1); //4 for v1 + 4 for length seq + 2
    aas.f1[2].v2s.length(0); //4 for v1 + 4 for length seq + 0
    size_t size_aas = serialized_size(encoding_plain_native, aas);
    if (size_aas != 32) {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("StructContainingArrayOfAStruct find_size failed with = %B ; expecting 30\n"),
        size_aas));
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
    size_t size_aas = serialized_size(encoding_plain_native, aas);
    if (size_aas != 44) {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("StructContainingArrayOfAStructSeq find_size failed ")
        ACE_TEXT("with = %B ; expecting 42\n"),
        size_aas));
      failed = true;
    }
  }
  {
  N1::FwdDeclSameNamespaceStructs fwddeclstructs;
  fwddeclstructs.length(2);
  fwddeclstructs[0].v1 = -5;
  fwddeclstructs[1].v1 = 43;
  OpenDDS::DCPS::Message_Block_Ptr b(new ACE_Message_Block(100000));
  OpenDDS::DCPS::Serializer serializer(b.get(), encoding_plain_native);

  serializer << fwddeclstructs;

  OpenDDS::DCPS::Serializer deserializer(b.get(), encoding_plain_native);
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
  OpenDDS::DCPS::Message_Block_Ptr b (new ACE_Message_Block( 100000));
  OpenDDS::DCPS::Serializer serializer(b.get(), encoding_unaligned_native);

  serializer << fwddeclstructs;

  OpenDDS::DCPS::Serializer deserializer(b.get(), encoding_unaligned_native);
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
    size_t size_aas = serialized_size(encoding_plain_native, aas);
    if (size_aas != 18) {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("StructOfArrayOfArrayOfShorts2 find_size failed ")
        ACE_TEXT("with = %B ; expecting 18\n"),
        size_aas));
      failed = true;
    }

    ACE_CDR::Short count = 0;
    for (int i = 0; i < 3 ; i++) {
      for (int j = 0; j < 2 ; j++) {
        aas.f1[i][j] = count++;
      }
    }

    ACE_Message_Block mb(size_aas);
    OpenDDS::DCPS::Serializer ss(&mb, encoding_unaligned_native);

    if (!(ss << aas)) {
      ACE_ERROR((LM_ERROR, "Serializing StructOfArrayOfArrayOfShorts2 failed\n"));
      failed = true;
    }

    OpenDDS::DCPS::Serializer ss2(&mb, encoding_unaligned_native);

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
  my_foo.theUnion.rsv("a string"); // (discriminator + string length + string data + null) +4+4+8+1 = 90

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

  const SerializedSizeBound expected_bound;
  const size_t expected_size = 90;

  const SerializedSizeBound actual_bound =
    OpenDDS::DCPS::MarshalTraits<Xyz::Foo>::serialized_size_bound(encoding_unaligned_native);
  const size_t actual_size = serialized_size(encoding_unaligned_native, my_foo);

  ACE_DEBUG((LM_DEBUG, "serialized_size_bound => %C\n", actual_bound.to_string().c_str()));
  ACE_DEBUG((LM_DEBUG, "serialized_size => %B\n", actual_size));

  if (actual_bound != expected_bound) {
    ACE_ERROR((LM_ERROR,
      "serialized_size_bound failed: expected %C got %C\n",
      expected_bound.to_string().c_str(), actual_bound.to_string().c_str()));
    failed = true;
  }

  if (actual_size != expected_size) {
    ACE_ERROR((LM_ERROR,
      "serialized_size(my_foo) failed: returned %B when was expecting %B\n",
      actual_size, expected_size));
    failed = true;
  }

  // test serializing

  ACE_Message_Block mb(actual_size);
  OpenDDS::DCPS::Serializer ss(&mb, encoding_unaligned_native);
  OpenDDS::DCPS::Serializer ss2(&mb, encoding_unaligned_native);

  Xyz::Foo ss_foo;
  try {
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
    } else if (0 != std::strcmp(ss_foo.theString, my_foo.theString)) {
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
    ACE_ERROR((LM_ERROR, "%s PASSED\n", argv[0]));

  return failed; // let the test framework know it failed
}
