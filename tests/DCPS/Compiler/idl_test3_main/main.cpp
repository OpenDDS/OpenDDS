#include "../idl_test3_lib/FooDefTypeSupportImpl.h"
#include "../idl_test3_lib/FooDef2TypeSupportImpl.h"
#include "../idl_test3_lib/FooDef3TypeSupportImpl.h"
#include "ace/ACE.h"
#include "ace/Log_Msg.h"
#include <map>
#include "tao/CDR.h"

int failed = false;
bool dump_buffer = false;

#define DONT_CHECK_CS 0
#define DONT_CHECK_MS 0

template<typename FOO>
int try_marshaling(const FOO &in_foo, FOO &out_foo,
                   size_t expected_ms,
                   size_t expected_cs,
                   const char* name)
{
  CORBA::Boolean bounded = OpenDDS::DCPS::gen_is_bounded_size(in_foo);
  size_t ms = OpenDDS::DCPS::gen_max_marshaled_size(in_foo);
  size_t cs = OpenDDS::DCPS::gen_find_size(in_foo);

  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("%C: OpenDDS::DCPS::gen_is_bounded_size(foo) => %d\n"),
    name, bounded));
  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("%C: _max_marshaled_size(my_foo) => %d\n"),
    name, ms));
  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("%C: OpenDDS::DCPS::gen_find_size(my_foo) => %d\n"),
    name, cs));

  // NOTE:_max_marshaled_size is not always > for unbounded.
  if (bounded && ms < cs)
    {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("%C: OpenDDS::DCPS::gen_max_marshaled_size(foo) %d < OpenDDS::DCPS::gen_find_size(foo) %d\n"),
        name, ms, cs));
      failed = true;
      return false;
    }

  if (expected_cs != DONT_CHECK_CS && cs != expected_cs)
    {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("%C: OpenDDS::DCPS::gen_find_size(foo) got %d but expected %d\n"),
        name, cs, expected_cs ));
      failed = true;
      return false;
    }

  if (expected_ms != DONT_CHECK_MS && ms != expected_ms)
    {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("%C: OpenDDS::DCPS::gen_max_marshaled_size(foo) got %d but expected %d\n"),
        name, ms, expected_ms ));
      failed = true;
      return false;
    }

  // testing with OpenDDS::DCPS::gen_find_size is a stronger test
  const size_t buff_size = cs; // bounded ? ms : cs;
  ACE_Message_Block *mb = new ACE_Message_Block(buff_size);

  TAO_OutputCDR cdr;
  if (false == (cdr << in_foo))
    {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("%C: TAO_OutputCDR << failed\n"), name));
      failed = true;
    }


  ACE_TCHAR ebuffer[51200] ; ebuffer[0] = ACE_TEXT('\0') ;

  OpenDDS::DCPS::Serializer ss(mb);

  if (dump_buffer)
    {
      ACE::format_hexdump( mb->rd_ptr(), mb->length(), ebuffer,
        sizeof(ebuffer)) ;
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("%C: BEFORE WRITING, LENGTH: %d, BUFFER:\n%s\n"),
        name, mb->length(), ebuffer));
    }

  if (false == ss << in_foo)
    {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("%C: Serializing failed\n"), name));
      failed = true;
      return false;
    }

  if (dump_buffer)
    {
      ACE::format_hexdump( mb->rd_ptr(), mb->length(), ebuffer,
        sizeof(ebuffer)) ;
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("%C: AFTER WRITING, LENGTH: %d, BUFFER:\n%s\n"),
        name, mb->length(), ebuffer));
    }

  OpenDDS::DCPS::Serializer ss2(mb);

  if (false == ss2 >> out_foo)
    {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("%C: Deserializing failed\n"), name));
      failed = true;
      mb->release(); // don't leak memory!
      return false;
    }

  mb->release(); // don't leak memory!

  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("%C: try_marshaling PASSED\n"), name));
  return true;
}

// this test tests the -Gdcps generated code for type XyZ::Foo from foo_test1_lib.
int ACE_TMAIN (int argc, ACE_TCHAR *argv[])
{

  if (argc > 1) dump_buffer = true;

  // ARRAYS
  const CORBA::ULong ARRAY_LEN = 5;
  { //=====================================================================
    Xyz::StructOfArrayOfBoolean val;
    for (CORBA::ULong ii =0; ii < ARRAY_LEN; ii++)
      {
        val.f[ii] = (ii % 2 == 0) ? true : false;
      }
    Xyz::StructOfArrayOfBoolean val_out;
    if (try_marshaling<Xyz::StructOfArrayOfBoolean>(val, val_out,
                            DONT_CHECK_MS, ARRAY_LEN, "Xyz::ArrayOfBoolean"))
      {
        for (CORBA::ULong ii =0; ii < ARRAY_LEN;ii++)
          {
            if (val_out.f[ii] != ((ii % 2 == 0) ? true : false))
              {
                ACE_ERROR((LM_ERROR,
                  ACE_TEXT("Xyz::ArrayOfBoolean: marshaling comparison failure\n")));
                failed = true;
              }
          }
      }
  }
  { //=====================================================================
    Xyz::StructOfArrayOfString val;
    val.f[0] = CORBA::string_dup("I");
    val.f[1] = CORBA::string_dup("hope");
    val.f[2] = CORBA::string_dup("this");
    val.f[3] = CORBA::string_dup("works");
    val.f[4] = CORBA::string_dup("");
    Xyz::StructOfArrayOfString val_out;
    if (try_marshaling<Xyz::StructOfArrayOfString>(val, val_out,
                            DONT_CHECK_MS, ARRAY_LEN*5+14, "Xyz::StructOfArrayOfString"))
      {
        for (CORBA::ULong ii =0; ii < ARRAY_LEN;ii++)
          {
            if (strcmp(val.f[ii], val_out.f[ii]))
              {
                ACE_ERROR((LM_ERROR,
                  ACE_TEXT("Xyz::StructOfArrayOfString: marshaling comparison failure\n")));
                failed = true;
              }
          }
      }
  }
  { //=====================================================================
    Xyz::StructOfArrayOfChar val;
    for (CORBA::ULong ii =0; ii < ARRAY_LEN; ii++)
      {
        val.f[ii] = static_cast<char>(65+ii);
      }
    Xyz::StructOfArrayOfChar val_out;
    if (try_marshaling<Xyz::StructOfArrayOfChar>(val, val_out,
                            DONT_CHECK_MS, ARRAY_LEN, "Xyz::StructOfArrayOfChar"))
      {
        for (CORBA::ULong ii =0; ii < ARRAY_LEN;ii++)
          {
            if (val_out.f[ii] != static_cast<char>(65+ii))
              {
                ACE_ERROR((LM_ERROR,
                  ACE_TEXT("Xyz::StructOfArrayOfChar: marshaling comparison failure\n")));
                failed = true;
              }
          }
      }
  }
  { //=====================================================================
    Xyz::StructOfArrayOfOctet val;
    for (CORBA::ULong ii =0; ii < ARRAY_LEN; ii++)
      {
        val.f[ii] = ii;
      }
    Xyz::StructOfArrayOfOctet val_out;
    if (try_marshaling<Xyz::StructOfArrayOfOctet>(val, val_out,
                            DONT_CHECK_MS, ARRAY_LEN, "Xyz::StructOfArrayOfOctet"))
      {
        for (CORBA::Octet ii =0; ii < ARRAY_LEN;ii++)
          {
            if (val_out.f[ii] != ii)
              {
                ACE_ERROR((LM_ERROR,
                  ACE_TEXT("Xyz::StructOfArrayOfOctet: marshaling comparison failure\n")));
                failed = true;
              }
          }
      }
  }
  { //=====================================================================
    Xyz::StructOfArrayOfLong val;
    for (CORBA::ULong ii =0; ii < ARRAY_LEN; ii++)
      {
        val.f[ii] = static_cast<CORBA::Long>(ii);
      }
    Xyz::StructOfArrayOfLong val_out;
    if (try_marshaling<Xyz::StructOfArrayOfLong>(val, val_out,
                            DONT_CHECK_MS, 4*ARRAY_LEN, "Xyz::StructOfArrayOfLong"))
      {
        for (CORBA::ULong ii =0; ii < ARRAY_LEN;ii++)
          {
            if (val_out.f[ii] != static_cast<CORBA::Long>(ii))
              {
                ACE_ERROR((LM_ERROR,
                  ACE_TEXT("Xyz::StructOfArrayOfLong: marshaling comparison failure\n")));
                failed = true;
              }
          }
      }
  }
  { //=====================================================================
    Xyz::StructOfArrayOfAnEnum val;
    for (CORBA::ULong ii =0; ii < ARRAY_LEN; ii++)
      {
        val.f[ii] = (ii % 2 == 0) ? Xyz::greenx : Xyz::bluex;
      }
    Xyz::StructOfArrayOfAnEnum val_out;
    if (try_marshaling<Xyz::StructOfArrayOfAnEnum>(val, val_out,
                            DONT_CHECK_MS, 4*ARRAY_LEN, "Xyz::StructOfArrayOfAnEnum"))
      {
        for (CORBA::ULong ii =0; ii < ARRAY_LEN;ii++)
          {
            if (val_out.f[ii] != ((ii % 2 == 0) ? Xyz::greenx : Xyz::bluex))
              {
                ACE_ERROR((LM_ERROR,
                  ACE_TEXT("Xyz::StructOfArrayOfAnEnum: marshaling comparison failure\n")));
                failed = true;
              }
          }
      }
  }

  // ARRAYS OF ARRAYS
  const CORBA::ULong AofA_LEN = 7;
  { //=====================================================================
    Xyz::StructOfArrayOfArrayOfLong val;
    for (CORBA::ULong jj =0; jj < AofA_LEN; jj++)
      for (CORBA::ULong ii =0; ii < ARRAY_LEN; ii++)
        {
          val.f[jj][ii] = ii+jj*ARRAY_LEN;
        }
    Xyz::StructOfArrayOfArrayOfLong val_out;
    if (try_marshaling<Xyz::StructOfArrayOfArrayOfLong>(val, val_out,
                            DONT_CHECK_MS, 4*ARRAY_LEN*AofA_LEN, "Xyz::ArrayOfArrayOfLong"))
      {
        for (CORBA::ULong jj =0; jj < AofA_LEN; jj++)
          for (CORBA::ULong ii =0; ii < ARRAY_LEN; ii++)
            {
            if (val_out.f[jj][ii] != CORBA::Long(ii+jj*ARRAY_LEN))
              {
                ACE_ERROR((LM_ERROR,
                  ACE_TEXT("Xyz::ArrayOfArrayOfLong: marshaling comparison failure\n")));
                failed = true;
              }
          }
      }
  }


  // SEQUENCES

  const CORBA::ULong SEQ_LEN = 5;
  const CORBA::ULong SEQ_LEN_SIZE = 4;

  { //=====================================================================
    Xyz::StructOfSeqOfBoolean val;
    val.field.length (SEQ_LEN);
    for (CORBA::ULong ii =0; ii < SEQ_LEN; ii++)
      {
        val.field[ii] = (ii % 2 == 0) ? true : false;
      }
    Xyz::StructOfSeqOfBoolean val_out;
    if (try_marshaling<Xyz::StructOfSeqOfBoolean>(val, val_out,
                            DONT_CHECK_MS, SEQ_LEN_SIZE+SEQ_LEN, "Xyz::StructOfSeqOfBoolean"))
      {
        for (CORBA::ULong ii =0; ii < SEQ_LEN;ii++)
          {
            if (val_out.field[ii] != ((ii % 2 == 0) ? true : false))
              {
                ACE_ERROR((LM_ERROR,
                  ACE_TEXT("Xyz::StructOfSeqOfBoolean: marshaling comparison failure\n")));
                failed = true;
              }
          }
      }
  }
  { //=====================================================================
    Xyz::StructOfSeqOfString val;
    val.field.length (SEQ_LEN);
    val.field[0] = CORBA::string_dup("I");
    val.field[1] = CORBA::string_dup("hope");
    val.field[2] = CORBA::string_dup("this");
    val.field[3] = CORBA::string_dup("works");
    val.field[4] = CORBA::string_dup("");
    Xyz::StructOfSeqOfString val_out;
    if (try_marshaling<Xyz::StructOfSeqOfString>(val, val_out,
                            DONT_CHECK_MS, SEQ_LEN_SIZE+SEQ_LEN*5+14, "Xyz::StructOfSeqOfString"))
      {
        for (CORBA::ULong ii =0; ii < SEQ_LEN;ii++)
          {
            if (strcmp(val.field[ii], val_out.field[ii]))
              {
                ACE_ERROR((LM_ERROR,
                  ACE_TEXT("Xyz::StructOfSeqOfString: marshaling comparison failure\n")));
                failed = true;
              }
          }
      }
  }
  { //=====================================================================
    Xyz::StructOfSeqOfChar val;
    val.field.length (SEQ_LEN);
    for (CORBA::ULong ii =0; ii < SEQ_LEN; ii++)
      {
        val.field[ii] = static_cast<char>(65+ii);
      }
    Xyz::StructOfSeqOfChar val_out;
    if (try_marshaling<Xyz::StructOfSeqOfChar>(val, val_out,
                            DONT_CHECK_MS, SEQ_LEN_SIZE+SEQ_LEN, "Xyz::StructOfSeqOfChar"))
      {
        for (CORBA::ULong ii =0; ii < SEQ_LEN;ii++)
          {
            if (val_out.field[ii] != static_cast<char>(65+ii))
              {
                ACE_ERROR((LM_ERROR,
                  ACE_TEXT("Xyz::StructOfSeqOfChar: marshaling comparison failure\n")));
                failed = true;
              }
          }
      }
  }
  { //=====================================================================
    Xyz::StructOfSeqOfOctet val;
    val.field.length (SEQ_LEN);
    for (CORBA::ULong ii =0; ii < SEQ_LEN; ii++)
      {
        val.field[ii] = ii;
      }
    Xyz::StructOfSeqOfOctet val_out;
    if (try_marshaling<Xyz::StructOfSeqOfOctet>(val, val_out,
                            DONT_CHECK_MS, SEQ_LEN_SIZE+SEQ_LEN, "Xyz::StructOfSeqOfOctet"))
      {
        for (CORBA::Octet ii =0; ii < SEQ_LEN;ii++)
          {
            if (val_out.field[ii] != ii)
              {
                ACE_ERROR((LM_ERROR,
                  ACE_TEXT("Xyz::StructOfSeqOfOctet: marshaling comparison failure\n")));
                failed = true;
              }
          }
      }
  }
  { //=====================================================================
    Xyz::StructOfSeqOfLong val;
    val.field.length (SEQ_LEN);
    for (CORBA::ULong ii =0; ii < SEQ_LEN; ii++)
      {
        val.field[ii] = ii;
      }
    Xyz::StructOfSeqOfLong val_out;
    if (try_marshaling<Xyz::StructOfSeqOfLong>(val, val_out,
                            DONT_CHECK_MS, SEQ_LEN_SIZE+4*SEQ_LEN, "Xyz::StructOfSeqOfLong"))
      {
        for (CORBA::ULong ii =0; ii < SEQ_LEN;ii++)
          {
            if (val_out.field[ii] != CORBA::Long(ii))
              {
                ACE_ERROR((LM_ERROR,
                  ACE_TEXT("Xyz::StructOfSeqOfLong: marshaling comparison failure\n")));
                failed = true;
              }
          }
      }
  }
  { //=====================================================================
    Xyz::StructOfSeqOfAnEnum val;
    val.field.length (SEQ_LEN);
    for (CORBA::ULong ii =0; ii < SEQ_LEN; ii++)
      {
        val.field[ii] = (ii % 2 == 0) ? Xyz::greenx : Xyz::bluex;
      }
    Xyz::StructOfSeqOfAnEnum val_out;
    if (try_marshaling<Xyz::StructOfSeqOfAnEnum>(val, val_out,
                            DONT_CHECK_MS, SEQ_LEN_SIZE+4*SEQ_LEN, "Xyz::StructOfSeqOfAnEnum"))
      {
        for (CORBA::ULong ii =0; ii < SEQ_LEN;ii++)
          {
            if (val_out.field[ii] != ((ii % 2 == 0) ? Xyz::greenx : Xyz::bluex))
              {
                ACE_ERROR((LM_ERROR,
                  ACE_TEXT("Xyz::StructOfSeqOfAnEnum: marshaling comparison failure\n")));
                failed = true;
              }
          }
      }
  }

  // ARRAYS OF SEQUENCES
  const CORBA::ULong AofS_LEN = 6;
  { //=====================================================================
    Xyz::StructOfArrayOfSeqOfLong val;
    for (CORBA::ULong jj =0; jj < AofS_LEN; jj++)
      {
        val.f[jj].length (SEQ_LEN);
        for (CORBA::ULong ii =0; ii < SEQ_LEN; ii++)
            {
              val.f[jj][ii] = ii+jj*ARRAY_LEN;
            }
      }
    Xyz::StructOfArrayOfSeqOfLong val_out;
    if (try_marshaling<Xyz::StructOfArrayOfSeqOfLong>(val, val_out,
                            DONT_CHECK_MS, 4*ARRAY_LEN*AofS_LEN + 4*AofS_LEN, "Xyz::ArrayOfSeqOfLong"))
      {
        for (CORBA::ULong jj =0; jj < AofS_LEN; jj++)
          for (CORBA::ULong ii =0; ii < ARRAY_LEN; ii++)
            {
            if (val_out.f[jj][ii] != CORBA::Long(ii+jj*ARRAY_LEN))
              {
                ACE_ERROR((LM_ERROR,
                  ACE_TEXT("Xyz::ArrayOfSeqOfLong: marshaling comparison failure\n")));
                failed = true;
              }
          }
      }
  }

  // SEQUENCE OF SEQUENCES
  const CORBA::ULong SofS_LEN = 4;
  { //=====================================================================
    Xyz::StructOfSeqOfSeqOfLong val;
    val.field.length(SofS_LEN);
    for (CORBA::ULong jj =0; jj < SofS_LEN; jj++)
      {
        val.field[jj].length (SEQ_LEN);
        for (CORBA::ULong ii =0; ii < SEQ_LEN; ii++)
            {
              val.field[jj][ii] = CORBA::Long(ii+jj*ARRAY_LEN);
            }
      }
    Xyz::StructOfSeqOfSeqOfLong val_out;
    if (try_marshaling<Xyz::StructOfSeqOfSeqOfLong>(val, val_out,
                            DONT_CHECK_MS,
                            // longs + inner lengths + outter length
                            4*SEQ_LEN*SofS_LEN + 4*SofS_LEN + 4,
                            "Xyz::StructOfSeqOfSeqOfLong"))
      {
        for (CORBA::ULong jj =0; jj < SofS_LEN; jj++)
          for (CORBA::ULong ii =0; ii < ARRAY_LEN; ii++)
            {
            if (val_out.field[jj][ii] != CORBA::Long(ii+jj*ARRAY_LEN))
              {
                ACE_ERROR((LM_ERROR,
                  ACE_TEXT("Xyz::StructOfSeqOfSeqOfLong: marshaling comparison failure\n")));
                failed = true;
              }
          }
      }
  }

  // OTHER

  { //=====================================================================
    Xyz::StructAUnion val;
    val.sau_f1._d(Xyz::redx);
    val.sau_f1.rv(CORBA::string_dup("joe"));
    // size = union descr/4 + string length/4 + string contents/4
    Xyz::StructAUnion val_out;
    if (try_marshaling<Xyz::StructAUnion>(val, val_out,
                            DONT_CHECK_MS, 4+4+4, "Xyz::StructAUnion"))
      {
         if (strcmp(val.sau_f1.rv (), val_out.sau_f1.rv ()))
           {
             ACE_ERROR((LM_ERROR,
               ACE_TEXT("Xyz::StructAUnion: marshaling comparison failure\n")));
             failed = true;
           }
      }
  }

  { //=====================================================================
    Xyz::StructOfSeqOfString val;
    val.field.length(2); //4 for seq length
    val.field[0] = CORBA::string_dup("four"); //4+5 strlen + string
    val.field[1] = CORBA::string_dup("five5"); //4+6 strlen + string
    Xyz::StructOfSeqOfString val_out;
    if (try_marshaling<Xyz::StructOfSeqOfString>(val, val_out,
                            DONT_CHECK_MS, 23, "Xyz::StructOfSeqOfString"))
      {
         if (strcmp(val.field[1], val_out.field[1]))
           {
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
  my_foo.x = 0.99f; //+4 > 8
  my_foo.y = 0.11f; //+4 > 12
  my_foo.octer = 0x13; //+1 > 13
  my_foo.xcolor = orig_color; //+4 > 17
  // syntax error my_foo.ooo = {0xff, 0x80, 0x3d};
  my_foo.ooo[0] = 0xff; //+3 > 20
  my_foo.ooo[1] = 0x80;
  my_foo.ooo[2] = 0x3d;
  my_foo.theString = CORBA::string_dup("four");
  //  ACE_DEBUG((LM_DEBUG, "thestruct.f60.length() = %d\n",
  //           my_foo.thestruct.f60.length()));
  my_foo.theUnion._d(Xyz::bluex); // !!!! Unions are invalid unless set!!!
  Xyz::SeqOfLong asol;
  asol.length(2);
  asol[0] = 77;
  asol[1] = 88;
  my_foo.theUnion.bv(asol);// !!!! Unions are invalid unless set!!!
  my_foo.theSeqOfUnion.length(2);
  my_foo.theSeqOfUnion[0]._d(Xyz::redx);
  my_foo.theSeqOfUnion[0].rv(CORBA::string_dup("Berkley"));
  my_foo.theSeqOfUnion[1]._d(Xyz::greenx);
  Xyz::AStruct as;
  as.f2 = 3.14F;
  my_foo.theSeqOfUnion[1].gv(as);

  // seq lengths theStruct +4+4 theStructSeq +4 theString +4 + structArray3*8 = 40
  // theUnion defaults to short so +4+2 = 6

  //const size_t expected_max_marshaled_size = 135;
  //const size_t expected_find_size = 28+40+6 + 4 /*string assigned */;
  //const CORBA::Boolean expected_bounded = false;

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
  foo2.theString = CORBA::string_dup("four");


  std::map<Xyz::Foo, Xyz::Foo*, Xyz::OpenDDSGenerated::Foo_KeyLessThan> foomap;

  if (OpenDDS::DCPS::gen_has_key(my_foo))
    {
      foomap[my_foo] = &my_foo;
      foomap[foo2] = &foo2;
      // foo2 and my_foo should have mapped to the same place
      if (foomap[my_foo] != &foo2)
        {
          ACE_ERROR((LM_ERROR, "FooKeyLessThan failed with map\n"));
          failed = true;
        }

      my_foo.key = 77;
      my_foo.xcolor = Xyz::redx;
      foomap[my_foo] = &my_foo;
      if (foomap[foo2] != &foo2)
        {
          ACE_ERROR((LM_ERROR, "FooKeyLessThan failed with map - 2\n"));
          failed = true;
        }

      if (foomap[my_foo]->key != 77)
        {
          ACE_ERROR((LM_ERROR, "FooKeyLessThan failed with map - 3a\n"));
          failed = true;
        }
      if (foomap[my_foo]->xcolor != Xyz::redx)
        {
          ACE_ERROR((LM_ERROR, "FooKeyLessThan failed with map - 3b\n"));
          failed = true;
        }
      if (foomap[foo2]->xcolor != orig_color)
        {
          ACE_ERROR((LM_ERROR, "FooKeyLessThan failed with map - 4\n"));
          failed = true;
        }
    }
  else
    ACE_DEBUG((LM_DEBUG, "NOTE: _dcps_has_key(foo) returned false\n"));


  Xyz::Foo ss_foo;
  if (try_marshaling<Xyz::Foo>(my_foo, ss_foo,
                           DONT_CHECK_MS, DONT_CHECK_CS, "Xyz::Foo"))
    {

      if (ss_foo.key != my_foo.key)
        {
          ACE_ERROR((LM_ERROR, ACE_TEXT("Failed to serialize key\n")));
          failed = true;
        }
      else if (ss_foo.x != my_foo.x)
        {
          ACE_ERROR((LM_ERROR, ACE_TEXT("Failed to serialize x\n")));
          failed = true;
        }
      else if (ss_foo.y != my_foo.y)
        {
          ACE_ERROR((LM_ERROR, ACE_TEXT("Failed to serialize y\n")));
          failed = true;
        }
      else if (ss_foo.xcolor != my_foo.xcolor)
        {
          ACE_ERROR((LM_ERROR, ACE_TEXT("Failed to serialize xcolor\n")));
          failed = true;
        }
      else if (ss_foo.octer != my_foo.octer)
        {
          ACE_ERROR((LM_ERROR, ACE_TEXT("Failed to serialize octer\n")));
          failed = true;
        }
      else if (0 != strcmp(ss_foo.theString.in (), my_foo.theString.in ()))
        {
          ACE_ERROR((LM_ERROR,
            ACE_TEXT("Failed to serialize theString \"%C\" => \"%C\"\n"),
            my_foo.theString.in (), ss_foo.theString.in ()));
          failed = true;
        }
      else if (ss_foo.ooo[0] != my_foo.ooo[0]
            || ss_foo.ooo[1] != my_foo.ooo[1]
            || ss_foo.ooo[2] != my_foo.ooo[2])
        {
          ACE_ERROR((LM_ERROR, "Failed to serialize ooo\n"));
          failed = true;
        }
/*
      else if (ss_foo.theUnion.bv()[1] != my_foo.theUnion.bv()[1])
        {
          ACE_ERROR((LM_ERROR, "Failed to serialize foo.theUnion.bv()[1]\n"));
          failed = true;
        }
*/
/*
      //else if (0 != strcmp(ss_foo.theSeqOfUnion[0].rv(), my_foo.theSeqOfUnion[0].rv()))
      //  {
      //    ACE_ERROR((LM_ERROR, "Failed to serialize foo.theSeqOfUnion[0].rv()\n"));
      //    failed = true;
      //  }
      //else if (ss_foo.theSeqOfUnion[1].gv().f2 != my_foo.theSeqOfUnion[1].gv().f2)
      //  {
      //    ACE_ERROR((LM_ERROR, "Failed to serialize foo.theSeqOfUnion[1].gv().f2\n"));
      //    failed = true;
        }
      else if (ss_foo.theSeqOfUnion[1].gv().f2 != my_foo.theSeqOfUnion[1].gv().f2)
        {
          ACE_ERROR((LM_ERROR, "Failed to serialize foo.theSeqOfUnion[1].gv().f2\n"));
          failed = true;
        }
*/
    }

  if (failed)
    ACE_ERROR((LM_ERROR, "%s FAILED!\n", argv[0]));
  else
    ACE_DEBUG((LM_ERROR, "%s PASSED\n", argv[0]));

  return failed; // let the test framework know it failed
}
