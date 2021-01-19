#include <FooDefTypeSupportImpl.h>

#include <dds/DCPS/Message_Block_Ptr.h>

#include <tao/CDR.h>

#include <ace/ACE.h>
#include <ace/Log_Msg.h>

#include <map>

using OpenDDS::DCPS::Encoding;
using OpenDDS::DCPS::Serializer;
using OpenDDS::DCPS::serialized_size;
using OpenDDS::DCPS::Message_Block_Ptr;
using OpenDDS::DCPS::SerializedSizeBound;

const Encoding encoding(Encoding::KIND_UNALIGNED_CDR);

// this test tests the opendds_idl generated code for type XyZ::Foo from idl_test1_lib.
int ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  bool failed = false;
  bool dump_buffer = false;

  if (argc > 1) dump_buffer = true;

  {
    Xyz::StructAUnion sau;
    sau.sau_f1._d(Xyz::redx);
    sau.sau_f1.rsv("joe");
    const size_t expected_size =
      4 + // union discriminator
      4 + // string length
      4; // string contents
    const size_t actual_size = serialized_size(encoding, sau);
    if (expected_size != actual_size) {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("ERROR: StructAUnion expected serialized_size to return %B, ")
        ACE_TEXT("but it returned %B.\n"),
        expected_size, actual_size));
      failed = true;
    }
  }

  {
    Xyz::AStringSeq ass;
    ass.length(2);
    ass[0] = "four";
    ass[1] = "five5";
    const size_t expected_size =
      4 + // seq length
      4 + 5 + // seq[0]
      4 + 6; // seq[1]
    size_t actual_size = serialized_size(encoding, ass);
    if (expected_size != actual_size) {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("ERROR: AStringSeq expected serialized_size to return ")
        ACE_TEXT("%B, but it returned %B.\n"),
        expected_size, actual_size));
      failed = true;
    }
  }

  {
    const unsigned seqlen = 2;
    Xyz::ArrayOfShortsSeq ash;
    ash.length(seqlen);
    const size_t expected_size =
      4 + // seq length
      5 * 2 * seqlen; // ArrayOfShorts * seqlen
    size_t actual_size = serialized_size(encoding, ash);
    if (expected_size != actual_size) {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("ERROR: ArrayOfShortsSeq expected serialized_size to return ")
        ACE_TEXT("%B, but it returned %B.\n"),
        expected_size, actual_size));
      failed = true;
    }
  }

  {
    Xyz::StructContainingArrayOfAStruct aas;
    aas.f1[0].v2s.length(2);
    aas.f1[1].v2s.length(1);
    aas.f1[2].v2s.length(0);
    const size_t expected_size =
      4 + // f1[0].v1
      4 + // f1[0].v2s length
      2 * 2 + // f1[0].v2s contents
      4 + // f1[1].v1
      4 + // f1[1].v2s length
      2 * 1 + // f1[1].v2s contents
      4 + // f1[2].v1
      4 + // f1[2].v2s length
      2 * 0; // f1[2].v2s contents
    size_t actual_size = serialized_size(encoding, aas);
    if (expected_size != actual_size) {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("ERROR: StructContainingArrayOfAStruct expected ")
        ACE_TEXT("serialized_size to return %B, but it returned %B.\n"),
        expected_size, actual_size));
      failed = true;
    }
  }

  {
    Xyz::StructContainingArrayOfAStructSeq aas;
    aas.f1[0].length(1);
    aas.f1[0][0].v2s.length(2);
    aas.f1[1].length(1);
    aas.f1[1][0].v2s.length(1);
    aas.f1[2].length(1);
    aas.f1[2][0].v2s.length(0);
    const size_t expected_size =
      // f1[0]
        4 + // length
        // [0]
          4 + // v1
          // v2s
            4 + // length
            2 * 2 + // contents
      // f1[1]
        4 + // length
        // [0]
          4 + // v1
          // v2s
            4 + // length
            2 * 1 + // contents
      // f1[2]
        4 + // length
        // [0]
          4 + // v1
          // v2s
            4 + // length
            2 * 0; // contents
    size_t actual_size = serialized_size(encoding, aas);
    if (expected_size != actual_size) {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("ERROR: StructContainingArrayOfAStructSeq expected ")
        ACE_TEXT("serialized_size to return %B, but it returned %B.\n"),
        expected_size, actual_size));
      failed = true;
    }
  }

  {
    N1::FwdDeclSameNamespaceStructs fwddeclstructs;
    fwddeclstructs.length(2);
    fwddeclstructs[0].v1 = -5;
    fwddeclstructs[1].v1 = 43;

    Message_Block_Ptr b(new ACE_Message_Block(100000));

    Serializer serializer(b.get(), encoding);

    if (!(serializer << fwddeclstructs)) {
      ACE_ERROR((LM_ERROR, "FwdDeclSameNamespaceStructs serialization failed\n"));
      failed = true;
    }

    Serializer deserializer(b.get(), encoding);
    N1::FwdDeclSameNamespaceStructs fwddeclstructs2;
    if (!(serializer >> fwddeclstructs2)) {
      ACE_ERROR((LM_ERROR, "FwdDeclSameNamespaceStructs deserialization failed\n"));
      failed = true;
    }

    if (fwddeclstructs2.length() != 2) {
      ACE_ERROR((LM_ERROR, "FwdDeclSameNamespaceStructs Size failed\n"));
      failed = true;
    }

    if (fwddeclstructs2[0].v1 != -5) {
      ACE_ERROR((LM_ERROR, "FwdDeclSameNamespaceStructs array index 0 failed\n"));
      failed = true;
    }

    if (fwddeclstructs2[1].v1 != 43) {
      ACE_ERROR((LM_ERROR, "FwdDeclSameNamespaceStructs array index 1 failed\n"));
      failed = true;
    }
  }

  {
    N2::FwdDeclDiffNamespaceStructs fwddeclstructs;
    fwddeclstructs.length(2);
    fwddeclstructs[0].v1 = -5;
    fwddeclstructs[1].v1 = 43;

    Message_Block_Ptr b(new ACE_Message_Block(100000));

    Serializer serializer(b.get(), encoding);
    if (!(serializer << fwddeclstructs)) {
      ACE_ERROR((LM_ERROR, "FwdDeclSameNamespaceStructs serialization failed\n"));
      failed = true;
    }

    Serializer deserializer(b.get(), encoding);
    N2::FwdDeclStructSeqWrapper fwddeclstructs2;
    if (!(serializer >> fwddeclstructs2)) {
      ACE_ERROR((LM_ERROR, "FwdDeclSameNamespaceStructs deserialization failed\n"));
      failed = true;
    }

    if (fwddeclstructs2.wrapped.length() != 2) {
      ACE_ERROR((LM_ERROR, "FwdDeclDiffNamespaceStructs Size failed\n"));
      failed = true;
    }

    if (fwddeclstructs2.wrapped[0].v1 != -5) {
      ACE_ERROR((LM_ERROR, "FwdDeclDiffNamespaceStructs array index 0 failed\n"));
      failed = true;
    }

    if (fwddeclstructs2.wrapped[1].v1 != 43) {
      ACE_ERROR((LM_ERROR, "FwdDeclDiffNamespaceStructs array index 1 failed\n"));
      failed = true;
    }
  }

  {
    Xyz::StructOfArrayOfArrayOfShorts2 aas;
    const size_t expected_size = 18;
    const size_t actual_size = serialized_size(encoding, aas);
    if (actual_size != expected_size) {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("StructOfArrayOfArrayOfShorts2 serialized_size failed ")
        ACE_TEXT("with = %B ; expecting %B\n"),
        actual_size, expected_size));
      failed = true;
    }

    CORBA::Short count = 0;
    for (int i = 0; i < 3 ; i++) {
      for (int j = 0; j < 2 ; j++) {
        aas.f1[i][j] = count++;
      }
    }

    ACE_Message_Block mb(actual_size);
    Serializer ss(&mb, encoding);

    if (!(ss << aas)) {
      ACE_ERROR((LM_ERROR, "Serializing StructOfArrayOfArrayOfShorts2 failed\n"));
      failed = true;
    }

    Serializer ss2(&mb, encoding);

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
      ACE_TEXT("gen_has_key(Xyz::AStruct) returned false when expecting true.\n")
      ));
  }

  if (OpenDDS::DCPS::DDSTraits<Xyz::StructContainingArrayOfAStructSeq>::gen_has_key()) {
    ACE_ERROR((LM_ERROR,
      ACE_TEXT("gen_has_key(Xyz::StructContainingArrayOfAStructSeq) returned true when expecting false.\n")
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

  const SerializedSizeBound expected_bound;
  const size_t expected_size = 79;

  const SerializedSizeBound actual_bound =
    OpenDDS::DCPS::MarshalTraits<Xyz::Foo>::serialized_size_bound(encoding);
  const size_t actual_size = serialized_size(encoding, my_foo);

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
  Serializer ss(&mb, encoding);
  Serializer ss2(&mb, encoding);

  Xyz::Foo ss_foo;
  try {
    ACE_TCHAR ebuffer[512];
    ebuffer[0] = ACE_TEXT('\0');
    ACE_TCHAR obuffer[512];
    obuffer[0] = ACE_TEXT('\0');

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
    failed = true;
  }

  ACE_ERROR((LM_ERROR, "%s %C\n", argv[0], failed ? "FAILED" : "PASSED"));

  return failed; // let the test framework know it failed
}
