#include "../idl_test3_lib/FooDefTypeSupportImpl.h"
#include "../idl_test3_lib/FooDef2TypeSupportImpl.h"
#include "../idl_test3_lib/FooDef3TypeSupportImpl.h"

#include "tao/CDR.h"
#include "ace/ACE.h"
#include "ace/Log_Msg.h"

#include <map>
#include <cstring>

int failed = false;
bool dump_buffer = false;

#define DONT_CHECK_CS 0
#define DONT_CHECK_MS 0

template<typename FOO>
int try_marshaling(const FOO& in_foo, FOO& out_foo,
                   size_t expected_ms, size_t expected_cs,
                   size_t expected_pad, size_t expected_ms_align,
                   const char* name)
{
  CORBA::Boolean bounded = OpenDDS::DCPS::MarshalTraits<FOO>::gen_is_bounded_size();
  size_t ms = OpenDDS::DCPS::gen_max_marshaled_size(in_foo, false);
  size_t ms_align = OpenDDS::DCPS::gen_max_marshaled_size(in_foo, true);
  size_t cs = 0, padding = 0;
  OpenDDS::DCPS::gen_find_size(in_foo, cs, padding);

  ACE_DEBUG((LM_DEBUG,
             ACE_TEXT("%C: gen_is_bounded_size(foo) => %d\n"),
             name, int(bounded)));
  ACE_DEBUG((LM_DEBUG,
             ACE_TEXT("%C: gen_max_marshaled_size(foo, false) => %B\n"),
             name, ms));
  ACE_DEBUG((LM_DEBUG,
             ACE_TEXT("%C: gen_max_marshaled_size(foo, true) => %B\n"),
             name, ms_align));
  ACE_DEBUG((LM_DEBUG,
             ACE_TEXT("%C: gen_find_size(foo) => %B, padding %B\n"),
             name, cs, padding));

  // NOTE: gen_max_marshaled_size is not always > for unbounded.
  if (bounded && ms < cs) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("%C: gen_max_marshaled_size(foo) %B < gen_find_size(foo) %B\n"),
               name, ms, cs));
    failed = true;
    return false;
  }

  if (expected_cs != DONT_CHECK_CS && cs != expected_cs) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("%C: gen_find_size(foo) got %B but expected %B\n"),
               name, cs, expected_cs));
    failed = true;
    return false;
  }

  if (expected_ms != DONT_CHECK_MS && ms != expected_ms) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("%C: gen_max_marshaled_size(foo, false) got %B but expected %B\n"),
               name, ms, expected_ms));
    failed = true;
    return false;
  }

  if (expected_pad != padding) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("%C: gen_find_size(foo) padding got %B but expected %B\n"),
               name, padding, expected_pad));
    failed = true;
    return false;
  }

  if (bounded && expected_ms_align != ms_align) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("%C: gen_max_marshaled_size(foo, true) got %B but expected %B\n"),
               name, ms_align, expected_ms_align));
    failed = true;
    return false;
  }

  // testing with OpenDDS::DCPS::gen_find_size is a stronger test
  const size_t buff_size = cs; // bounded ? ms : cs;
  ACE_Message_Block mb(buff_size);

  TAO_OutputCDR cdr;

  if (!(cdr << in_foo)) {
    ACE_ERROR((LM_ERROR,
               ACE_TEXT("%C: TAO_OutputCDR << failed\n"), name));
    failed = true;
  }


  ACE_TCHAR ebuffer[51200];
  ebuffer[0] = ACE_TEXT('\0');

  OpenDDS::DCPS::Serializer ss(&mb);

  if (dump_buffer) {
    ACE::format_hexdump(mb.rd_ptr(), mb.length(), ebuffer, sizeof(ebuffer));
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("%C: BEFORE WRITING, LENGTH: %B, BUFFER:\n%s\n"),
               name, mb.length(), ebuffer));
  }

  if (!(ss << in_foo)) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("%C: Serializing failed\n"), name));
    failed = true;
    return false;
  }

  if (dump_buffer) {
    ACE::format_hexdump(mb.rd_ptr(), mb.length(), ebuffer, sizeof(ebuffer));
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("%C: AFTER WRITING, LENGTH: %B, BUFFER:\n%s\n"),
               name, mb.length(), ebuffer));
  }

  OpenDDS::DCPS::Serializer ss2(&mb);

  if (!(ss2 >> out_foo)) {
    ACE_ERROR((LM_ERROR, ACE_TEXT("%C: Deserializing failed\n"), name));
    failed = true;
    return false;
  }

  ACE_DEBUG((LM_DEBUG, ACE_TEXT("%C: try_marshaling PASSED\n"), name));
  return true;
}

// this test tests the -Gdcps generated code for type XyZ::Foo from foo_test1_lib.
int ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{

  if (argc > 1) dump_buffer = true;

  // ARRAYS
  const CORBA::ULong ARRAY_LEN = 5;
  {
    //=====================================================================
    Xyz::StructOfArrayOfBoolean val;

    for (CORBA::ULong ii =0; ii < ARRAY_LEN; ii++) {
      val.f[ii] = (ii % 2 == 0) ? true : false;
    }

    Xyz::StructOfArrayOfBoolean val_out;

    if (try_marshaling(val, val_out, ARRAY_LEN, ARRAY_LEN, 0, ARRAY_LEN,
                       "Xyz::StructOfArrayOfBoolean")) {
      for (CORBA::ULong ii =0; ii < ARRAY_LEN; ii++) {
        if (val_out.f[ii] != ((ii % 2 == 0) ? true : false)) {
          ACE_ERROR((LM_ERROR,
                     ACE_TEXT("Xyz::StructOfArrayOfBoolean: marshaling comparison failure\n")));
          failed = true;
        }
      }
    }
  }
  {
    //=====================================================================
    Xyz::StructOfArrayOfString val;
    val.f[0] = CORBA::string_dup("I");
    val.f[1] = CORBA::string_dup("hope");
    val.f[2] = CORBA::string_dup("this");
    val.f[3] = CORBA::string_dup("works");
    val.f[4] = CORBA::string_dup("");
    Xyz::StructOfArrayOfString val_out;

    if (try_marshaling(val, val_out, 60, ARRAY_LEN*5+14, 2+3+3+2, 60,
                       "Xyz::StructOfArrayOfString")) {
      for (CORBA::ULong ii =0; ii < ARRAY_LEN; ii++) {
        if (strcmp(val.f[ii], val_out.f[ii])) {
          ACE_ERROR((LM_ERROR,
                     ACE_TEXT("Xyz::StructOfArrayOfString: marshaling comparison failure\n")));
          failed = true;
        }
      }
    }
  }
  {
    //=====================================================================
    Xyz::StructOfArrayOfChar val;

    for (CORBA::ULong ii =0; ii < ARRAY_LEN; ii++) {
      val.f[ii] = static_cast<char>(65+ii);
    }

    Xyz::StructOfArrayOfChar val_out;

    if (try_marshaling(val, val_out, ARRAY_LEN, ARRAY_LEN, 0, ARRAY_LEN,
                       "Xyz::StructOfArrayOfChar")) {
      for (CORBA::ULong ii =0; ii < ARRAY_LEN; ii++) {
        if (val_out.f[ii] != static_cast<char>(65+ii)) {
          ACE_ERROR((LM_ERROR,
                     ACE_TEXT("Xyz::StructOfArrayOfChar: marshaling comparison failure\n")));
          failed = true;
        }
      }
    }
  }
  {
    //=====================================================================
    Xyz::StructOfArrayOfOctet val;

    for (CORBA::ULong ii =0; ii < ARRAY_LEN; ii++) {
      val.f[ii] = ii;
    }

    Xyz::StructOfArrayOfOctet val_out;

    if (try_marshaling(val, val_out, ARRAY_LEN, ARRAY_LEN, 0, ARRAY_LEN,
                       "Xyz::StructOfArrayOfOctet")) {
      for (CORBA::Octet ii =0; ii < ARRAY_LEN; ii++) {
        if (val_out.f[ii] != ii) {
          ACE_ERROR((LM_ERROR,
                     ACE_TEXT("Xyz::StructOfArrayOfOctet: marshaling comparison failure\n")));
          failed = true;
        }
      }
    }
  }
  {
    //=====================================================================
    Xyz::StructOfArrayOfLong val;

    for (CORBA::ULong ii =0; ii < ARRAY_LEN; ii++) {
      val.f[ii] = static_cast<CORBA::Long>(ii);
    }

    Xyz::StructOfArrayOfLong val_out;

    if (try_marshaling(val, val_out, 4*ARRAY_LEN, 4*ARRAY_LEN, 0, 4*ARRAY_LEN,
                       "Xyz::StructOfArrayOfLong")) {
      for (CORBA::ULong ii =0; ii < ARRAY_LEN; ii++) {
        if (val_out.f[ii] != static_cast<CORBA::Long>(ii)) {
          ACE_ERROR((LM_ERROR,
                     ACE_TEXT("Xyz::StructOfArrayOfLong: marshaling comparison failure\n")));
          failed = true;
        }
      }
    }
  }
  {
    //=====================================================================
    Xyz::StructOfArrayOfAnEnum val;

    for (CORBA::ULong ii =0; ii < ARRAY_LEN; ii++) {
      val.f[ii] = (ii % 2 == 0) ? Xyz::greenx : Xyz::bluex;
    }

    Xyz::StructOfArrayOfAnEnum val_out;

    if (try_marshaling(val, val_out, 4*ARRAY_LEN, 4*ARRAY_LEN, 0, 4*ARRAY_LEN,
                       "Xyz::StructOfArrayOfAnEnum")) {
      for (CORBA::ULong ii =0; ii < ARRAY_LEN; ii++) {
        if (val_out.f[ii] != ((ii % 2 == 0) ? Xyz::greenx : Xyz::bluex)) {
          ACE_ERROR((LM_ERROR,
                     ACE_TEXT("Xyz::StructOfArrayOfAnEnum: marshaling comparison failure\n")));
          failed = true;
        }
      }
    }
  }

  // ARRAYS OF ARRAYS
  const CORBA::ULong AofA_LEN = 7;
  {
    //=====================================================================
    Xyz::StructOfArrayOfArrayOfLong val;

    for (CORBA::ULong jj =0; jj < AofA_LEN; jj++)
      for (CORBA::ULong ii =0; ii < ARRAY_LEN; ii++) {
        val.f[jj][ii] = ii+jj*ARRAY_LEN;
      }

    Xyz::StructOfArrayOfArrayOfLong val_out;

    const size_t sz = 4*ARRAY_LEN*AofA_LEN;
    if (try_marshaling(val, val_out, sz, sz, 0, sz,
                       "Xyz::StructOfArrayOfArrayOfLong")) {
      for (CORBA::ULong jj =0; jj < AofA_LEN; jj++)
        for (CORBA::ULong ii =0; ii < ARRAY_LEN; ii++) {
          if (val_out.f[jj][ii] != CORBA::Long(ii+jj*ARRAY_LEN)) {
            ACE_ERROR((LM_ERROR,
                       ACE_TEXT("Xyz::StructOfArrayOfArrayOfLong: marshaling comparison failure\n")));
            failed = true;
          }
        }
    }
  }

  // SEQUENCES

  const CORBA::ULong SEQ_LEN = 5;
  const CORBA::ULong SEQ_LEN_SIZE = 4;

  {
    //=====================================================================
    Xyz::StructOfSeqOfBoolean val;
    val.field.length(SEQ_LEN);

    for (CORBA::ULong ii = 0; ii < SEQ_LEN; ii++) {
      val.field[ii] = (ii % 2 == 0) ? true : false;
    }

    Xyz::StructOfSeqOfBoolean val_out;

    const size_t max_sz = SEQ_LEN_SIZE + 6, sz = SEQ_LEN_SIZE + SEQ_LEN;
    if (try_marshaling(val, val_out, max_sz, sz, 0, max_sz,
                       "Xyz::StructOfSeqOfBoolean")) {
      for (CORBA::ULong ii = 0; ii < SEQ_LEN; ii++) {
        if (val_out.field[ii] != ((ii % 2 == 0) ? true : false)) {
          ACE_ERROR((LM_ERROR,
                     ACE_TEXT("Xyz::StructOfSeqOfBoolean: marshaling comparison failure\n")));
          failed = true;
        }
      }
    }
  }
  {
    //=====================================================================
    Xyz::StructOfSeqOfString val;
    val.field.length(SEQ_LEN);
    val.field[0] = CORBA::string_dup("I");
    val.field[1] = CORBA::string_dup("hope");
    val.field[2] = CORBA::string_dup("this");
    val.field[3] = CORBA::string_dup("works");
    val.field[4] = CORBA::string_dup("");
    Xyz::StructOfSeqOfString val_out;

    if (try_marshaling(val, val_out, DONT_CHECK_MS, SEQ_LEN_SIZE+SEQ_LEN*5+14,
                       2+3+3+2, DONT_CHECK_MS, "Xyz::StructOfSeqOfString")) {
      for (CORBA::ULong ii =0; ii < SEQ_LEN; ii++) {
        if (strcmp(val.field[ii], val_out.field[ii])) {
          ACE_ERROR((LM_ERROR,
                     ACE_TEXT("Xyz::StructOfSeqOfString: marshaling comparison failure\n")));
          failed = true;
        }
      }
    }
  }
  {
    //=====================================================================
    Xyz::StructOfSeqOfChar val;
    val.field.length(SEQ_LEN);

    for (CORBA::ULong ii =0; ii < SEQ_LEN; ii++) {
      val.field[ii] = static_cast<char>(65+ii);
    }

    Xyz::StructOfSeqOfChar val_out;

    const size_t max_sz = SEQ_LEN_SIZE + 6, sz = SEQ_LEN_SIZE + SEQ_LEN;
    if (try_marshaling(val, val_out, max_sz, sz, 0, max_sz,
                       "Xyz::StructOfSeqOfChar")) {
      for (CORBA::ULong ii =0; ii < SEQ_LEN; ii++) {
        if (val_out.field[ii] != static_cast<char>(65+ii)) {
          ACE_ERROR((LM_ERROR,
                     ACE_TEXT("Xyz::StructOfSeqOfChar: marshaling comparison failure\n")));
          failed = true;
        }
      }
    }
  }
  {
    //=====================================================================
    Xyz::StructOfSeqOfOctet val;
    val.field.length(SEQ_LEN);

    for (CORBA::ULong ii =0; ii < SEQ_LEN; ii++) {
      val.field[ii] = ii;
    }

    Xyz::StructOfSeqOfOctet val_out;

    const size_t max_sz = SEQ_LEN_SIZE + 6, sz = SEQ_LEN_SIZE + SEQ_LEN;
    if (try_marshaling(val, val_out, max_sz, sz, 0, max_sz,
                       "Xyz::StructOfSeqOfOctet")) {
      for (CORBA::Octet ii =0; ii < SEQ_LEN; ii++) {
        if (val_out.field[ii] != ii) {
          ACE_ERROR((LM_ERROR,
                     ACE_TEXT("Xyz::StructOfSeqOfOctet: marshaling comparison failure\n")));
          failed = true;
        }
      }
    }
  }
  {
    //=====================================================================
    Xyz::StructOfSeqOfLong val;
    val.field.length(SEQ_LEN);

    for (CORBA::ULong ii =0; ii < SEQ_LEN; ii++) {
      val.field[ii] = ii;
    }

    Xyz::StructOfSeqOfLong val_out;

    const size_t max_sz = SEQ_LEN_SIZE + 6*4, sz = SEQ_LEN_SIZE + SEQ_LEN*4;
    if (try_marshaling(val, val_out, max_sz, sz, 0, max_sz,
                       "Xyz::StructOfSeqOfLong")) {
      for (CORBA::ULong ii =0; ii < SEQ_LEN; ii++) {
        if (val_out.field[ii] != CORBA::Long(ii)) {
          ACE_ERROR((LM_ERROR,
                     ACE_TEXT("Xyz::StructOfSeqOfLong: marshaling comparison failure\n")));
          failed = true;
        }
      }
    }
  }
  {
    //=====================================================================
    Xyz::StructOfSeqOfAnEnum val;
    val.field.length(SEQ_LEN);

    for (CORBA::ULong ii = 0; ii < SEQ_LEN; ii++) {
      val.field[ii] = (ii % 2 == 0) ? Xyz::greenx : Xyz::bluex;
    }

    Xyz::StructOfSeqOfAnEnum val_out;

    const size_t sz = SEQ_LEN_SIZE + SEQ_LEN*4;
    if (try_marshaling(val, val_out, DONT_CHECK_MS, sz, 0, 0,
                       "Xyz::StructOfSeqOfAnEnum")) {
      for (CORBA::ULong ii = 0; ii < SEQ_LEN; ii++) {
        if (val_out.field[ii] != ((ii % 2 == 0) ? Xyz::greenx : Xyz::bluex)) {
          ACE_ERROR((LM_ERROR,
                     ACE_TEXT("Xyz::StructOfSeqOfAnEnum: marshaling comparison failure\n")));
          failed = true;
        }
      }
    }
  }

  // ARRAYS OF SEQUENCES
  const CORBA::ULong AofS_LEN = 6;
  {
    //=====================================================================
    Xyz::StructOfArrayOfSeqOfLong val;

    for (CORBA::ULong jj =0; jj < AofS_LEN; jj++) {
      val.f[jj].length(SEQ_LEN);

      for (CORBA::ULong ii =0; ii < SEQ_LEN; ii++) {
        val.f[jj][ii] = ii+jj*ARRAY_LEN;
      }
    }

    Xyz::StructOfArrayOfSeqOfLong val_out;

    const size_t max_sz = AofS_LEN * (SEQ_LEN_SIZE + 6 * 4),
      sz = AofS_LEN * (SEQ_LEN_SIZE + SEQ_LEN * 4);
    if (try_marshaling(val, val_out, max_sz, sz, 0, max_sz,
                       "Xyz::StructOfArrayOfSeqOfLong")) {
      for (CORBA::ULong jj =0; jj < AofS_LEN; jj++)
        for (CORBA::ULong ii =0; ii < ARRAY_LEN; ii++) {
          if (val_out.f[jj][ii] != CORBA::Long(ii+jj*ARRAY_LEN)) {
            ACE_ERROR((LM_ERROR,
                       ACE_TEXT("Xyz::StructOfArrayOfSeqOfLong: marshaling comparison failure\n")));
            failed = true;
          }
        }
    }
  }

  // SEQUENCE OF SEQUENCES
  const CORBA::ULong SofS_LEN = 4;
  {
    //=====================================================================
    Xyz::StructOfSeqOfSeqOfLong val;
    val.field.length(SofS_LEN);

    for (CORBA::ULong jj =0; jj < SofS_LEN; jj++) {
      val.field[jj].length(SEQ_LEN);

      for (CORBA::ULong ii =0; ii < SEQ_LEN; ii++) {
        val.field[jj][ii] = CORBA::Long(ii+jj*ARRAY_LEN);
      }
    }

    Xyz::StructOfSeqOfSeqOfLong val_out;

    const size_t max_sz = SEQ_LEN_SIZE + SofS_LEN * (SEQ_LEN_SIZE + 6 * 4),
      sz = SEQ_LEN_SIZE + SofS_LEN * (SEQ_LEN_SIZE + SEQ_LEN * 4);
    if (try_marshaling(val, val_out, max_sz, sz, 0, max_sz,
                       "Xyz::StructOfSeqOfSeqOfLong")) {
      for (CORBA::ULong jj =0; jj < SofS_LEN; jj++)
        for (CORBA::ULong ii =0; ii < ARRAY_LEN; ii++) {
          if (val_out.field[jj][ii] != CORBA::Long(ii+jj*ARRAY_LEN)) {
            ACE_ERROR((LM_ERROR,
                       ACE_TEXT("Xyz::StructOfSeqOfSeqOfLong: marshaling comparison failure\n")));
            failed = true;
          }
        }
    }
  }

  // OTHER
  {
    //=====================================================================
    Xyz::StructAUnion val;
    val.sau_f1._d(Xyz::redx);
    val.sau_f1.rv(CORBA::string_dup("joe"));
    // size = union descr/4 + string length/4 + string contents/4
    Xyz::StructAUnion val_out;

    if (try_marshaling(val, val_out, DONT_CHECK_MS, 4+4+4, 0, DONT_CHECK_MS,
                       "Xyz::StructAUnion")) {
        if (0 != std::strcmp(val.sau_f1.rv(), val_out.sau_f1.rv())) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("Xyz::StructAUnion: marshaling comparison failure\n")));
        failed = true;
      }
    }
  }
  {
    //=====================================================================
    Xyz::StructOfSeqOfString val;
    val.field.length(2); //4 for seq length
    val.field[0] = CORBA::string_dup("four"); //4+5 strlen + string
    val.field[1] = CORBA::string_dup("five5"); //4+6 strlen + string
    Xyz::StructOfSeqOfString val_out;

    if (try_marshaling(val, val_out, DONT_CHECK_MS, 23, 3, DONT_CHECK_MS,
                       "Xyz::StructOfSeqOfString")) {
      if (0 != std::strcmp(val.field[1], val_out.field[1])) {
        ACE_ERROR((LM_ERROR,
                   ACE_TEXT("Xyz::StructOfSeqOfString: marshaling comparison failure\n")));
        failed = true;
      }
    }
  }

  Xyz::Foo my_foo;

  // greenx < redx < bluex
  const Xyz::AnEnum orig_color = Xyz::greenx;

  // -- //+# is keeping track of the find_size
  my_foo.key = 99; //4
  my_foo.octer = 0x13; //+1 = 5
  my_foo.theOctetTypedef = 0x14; //+1 = 6
  my_foo.xcolor = orig_color; //+4 = 10           {padding 2}
  my_foo.ooo[0] = 0xff; //+3 = 13
  my_foo.ooo[1] = 0x80;
  my_foo.ooo[2] = 0x3d;
  my_foo.theString = "four"; //+9 = 22            {padding +1 = 3}
  //  AStruct thestruct;     //+885 = 907         {padding +7+123 = 133}
  //    ** see breakdown in comment at the end of this file
  //  AStructSeq theStructSeq; //+4 = 911
  //  ArrayOfAStruct structArray; //+(3*885) = 3566 {padding +4+3*123 = 506}
  my_foo.theUnion._d(Xyz::bluex); //+4 = 3570

  Xyz::SeqOfLong asol;
  asol.length(2);
  asol[0] = 77;
  asol[1] = 88;
  my_foo.theUnion.bv(asol); //+4+8 = 3582
  my_foo.theSeqOfUnion.length(2); //+4 = 3586
  my_foo.theSeqOfUnion[0]._d(Xyz::redx); //+4 = 3590
  my_foo.theSeqOfUnion[0].rv("Berkley"); //+4+8 = 3602
  my_foo.theSeqOfUnion[1]._d(Xyz::greenx); //+4 = 3606
  Xyz::AStruct as;
  as.f2 = 3.14F;
  as.f5 = 42;
  my_foo.theSeqOfUnion[1].gv(as); //+885 = 4491   {padding +123 = 629}

  //  BigUnion theBigUnion;        //+4 = 4495
  //  BigUnionSeq theSeqOfBitUnion;//+4 = 4499
  my_foo.x = 0.99f;                //+4 = 4503
  my_foo.y = 0.11f;                //+4 = 4507
  my_foo.theWChar = L'a';          //+3 = 4510
  //  wstring theWString;          //+4 = 4514    {padding +1 = 630}
  //  long double theLongDouble;  //+16 = 4530

  Xyz::Foo foo2;
  foo2.key = 99;
  foo2.x = 0.99f;
  foo2.y = 0.11f;
  foo2.octer = 0x13;
  foo2.xcolor = orig_color;
  foo2.ooo[0] = 0xff;
  foo2.ooo[1] = 0x80;
  foo2.ooo[2] = 0x3d;
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
    ACE_DEBUG((LM_DEBUG, "NOTE: gen_has_key(foo) returned false\n"));
  }

  Xyz::Foo ss_foo;

  const size_t sz = 4530 // see running totals above
#ifdef OPENDDS_SAFETY_PROFILE
    - 4 // theWString is gone
#endif
    , pad = 630;

  try {
    if (try_marshaling(my_foo, ss_foo, DONT_CHECK_MS, sz, pad, DONT_CHECK_MS,
                       "Xyz::Foo")) {
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

      } else if (ss_foo.theUnion._d() != my_foo.theUnion._d()
                 || ss_foo.theUnion.bv().length() != my_foo.theUnion.bv().length()
                 || ss_foo.theUnion.bv()[0] != my_foo.theUnion.bv()[0]
                 || ss_foo.theUnion.bv()[1] != my_foo.theUnion.bv()[1]) {
        ACE_ERROR((LM_ERROR, "Failed to serialize foo.theUnion\n"));
        failed = true;

      } else if (ss_foo.theSeqOfUnion.length() != my_foo.theSeqOfUnion.length()
                 || ss_foo.theSeqOfUnion[0]._d() != my_foo.theSeqOfUnion[0]._d()
                 || 0 != std::strcmp(ss_foo.theSeqOfUnion[0].rv(),
                                     my_foo.theSeqOfUnion[0].rv())
                 || ss_foo.theSeqOfUnion[1]._d() != my_foo.theSeqOfUnion[1]._d()
                 || ss_foo.theSeqOfUnion[1].gv().f5 !=
                    my_foo.theSeqOfUnion[1].gv().f5) {
        ACE_ERROR((LM_ERROR, "Failed to serialize foo.theSeqOfUnion\n"));
        failed = true;
      }
    }
  }
  catch (const CORBA::BAD_PARAM& ex)
  {
    ex._tao_print_exception("Exception caught in main.cpp:");
    return 1;
  }

  if (failed)
    ACE_ERROR((LM_ERROR, "%s FAILED!\n", argv[0]));
  else
    ACE_DEBUG((LM_ERROR, "%s PASSED\n", argv[0]));

  return failed; // let the test framework know it failed
}

// serialized size of AStruct (default constructed)
/*
                               Sizeof    Running    Padding
    double  f1;                     8          8
    float   f2;                     4         12
    boolean f3;                     1         13
    SevenStr f4;                    5         18          3
    char f5;                        1         19
    octet f6;                       1         20
    OctetTypedef f6a;               1         21
    long f7;                        4         25
    AnEnum f8;                      4         29
    ArrayOfBoolean f10;             5         34
    ArrayOfString f11;             25         59         15 (3+4*3)
    ArrayOfChar f12;                5         64
    ArrayOfOctet f13;               5         69
    ArrayOfLong f14;               20         89          1
    ArrayOfAnEnum f15;             20        109
    SeqOfBoolean f20;               4        113
    SeqOfString f21;                4        117
    SeqOfChar f22;                  4        121
    SeqOfOctet f23;                 4        125
    SeqOfLong f24;                  4        129
    SeqOfAnEnum f25;                4        133
    ArrayOfSeqOfBoolean f30;       24        157
    ArrayOfSeqOfString f31;        24        181
    ArrayOfSeqOfChar f32;          24        205
    ArrayOfSeqOfOctet f33;         24        229
    ArrayOfSeqOfLong f34;          24        253
    ArrayOfSeqOfAnEnum f35;        24        277
    SeqOfArrayOfBoolean f40;        4        281
    SeqOfArrayOfString f41;         4        285
    SeqOfArrayOfChar f42;           4        289
    SeqOfArrayOfOctet f43;          4        293
    SeqOfArrayOfLong f44;           4        297
    SeqOfArrayOfAnEnum f45;         4        301
    ArrayOfArrayOfBoolean f50;     35        336
    ArrayOfArrayOfString f51;     175        511        103 (1+(5*7-1)*3)
    ArrayOfArrayOfChar f52;        35        546
    ArrayOfArrayOfOctet f53;       35        581
    ArrayOfArrayOfLong f54;       140        721          1
    ArrayOfArrayOfAnEnum f55;     140        861
    SeqOfSeqOfBoolean f60;          4        865
    SeqOfSeqOfString f61;           4        869
    SeqOfSeqOfChar f62;             4        873
    SeqOfSeqOfOctet f63;            4        877
    SeqOfSeqOfLong f64;             4        881
    SeqOfSeqOfAnEnum f65;           4        885
 (total internal padding    ====>                       123)
    */
