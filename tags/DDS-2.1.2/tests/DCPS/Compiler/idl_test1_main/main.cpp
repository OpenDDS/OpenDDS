#include "../idl_test1_lib/FooDefC.h"
#include "ace/ACE.h"
#include "ace/Log_Msg.h"
#include <map>
#include "tao/CDR.h"

// this test tests the -Gdcps generated code for type XyZ::Foo from idl_test1_lib.
int ACE_TMAIN (int argc, ACE_TCHAR *argv[])
{
  int failed = false;
  bool dump_buffer = false;
 
  if (argc > 1) dump_buffer = true;

  {
  Xyz::StructAUnion sau;
  sau.sau_f1._d(Xyz::redx);
  sau.sau_f1.rsv((const char*) "joe");
  // size = union descr/4 + string length/4 + string contents/3
  if (_dcps_find_size(sau) != 4+4+3)
    {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("StructAUnion find_size failed with = %d\n"),
        _dcps_find_size(sau)));
      failed = true;
    }
  }
  {
  Xyz::AStringSeq ass;
  ass.length(2); //4 for seq length
  ass[0] = CORBA::string_dup ("four"); //4+4 strlen & string
  ass[1] = CORBA::string_dup ("five5"); //4+5 strlen + string
  size_t size_ass = _dcps_find_size(ass);
  if (size_ass != 21)
    {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("AStringSeq find_size failed with = %d ; expecting 21\n"),
        size_ass));
    }
  }
  {
  Xyz::ArrayOfShortsSeq ash;
  ash.length(2); //4 for seq length + 5*2 for arry *2 length
  size_t size_ash = _dcps_find_size(ash);
  if (size_ash != 24)
    {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("ArrayOfShortsSeq find_size failed with = %d ; expecting 24\n"),
        size_ash));
    }
  }

  {
  Xyz::StructContainingArrayOfAStruct aas;
  aas.f1[0].v2s.length(2); //4 for v1 + 4 for length seq + 2*2
  aas.f1[1].v2s.length(1); //4 for v1 + 4 for length seq + 2
  aas.f1[2].v2s.length(0); //4 for v1 + 4 for length seq + 0
  size_t size_aas = _dcps_find_size(aas);
  if (size_aas != 30)
    {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("StructContainingArrayOfAStruct find_size failed with = %d ; expecting 30\n"),
        size_aas));
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
  size_t size_aas = _dcps_find_size(aas);
  if (size_aas != 42)
    {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("StructContainingArrayOfAStructSeq find_size failed ")
        ACE_TEXT("with = %d ; expecting 42\n"),
        size_aas));
    }
  }

  {
  Xyz::StructOfArrayOfArrayOfShorts2 aas;
  size_t size_aas = _dcps_find_size(aas);
  if (size_aas != 18)
    {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("StructContainingArrayOfAStructSeq find_size failed ")
        ACE_TEXT("with = %d ; expecting 18\n"),
        size_aas));
    }

  CORBA::Short count = 0;
  for (int i = 0; i < 3 ; i++) {
    for (int j = 0; j < 2 ; j++) {
      aas.f1[i][j] = count++;
    }
  }

  ACE_Message_Block *mb = new ACE_Message_Block(size_aas);
  TAO::DCPS::Serializer ss(mb);

  if (false == ss << aas)
    {
      ACE_ERROR((LM_ERROR, "Serializing StructOfArrayOfArrayOfShorts2 failed\n"));
      failed = true;
    }

  // Reset the chain back to the beginning.
  // This is needed when the buffer size = find_size(my_foo) because
  // the the serialize method set current_ the next block in the chain 
  // which is nil; so deserializing will fail.
  ss.add_chain(mb) ;

  Xyz::StructOfArrayOfArrayOfShorts2 aas2;
  if (false == ss >> aas2)
    {
      ACE_ERROR((LM_ERROR, "Deserializing StructOfArrayOfArrayOfShorts2 failed\n"));
      failed = true;
    }
  else 
    {
      count = 0;
      for (int i = 0; i < 3 ; i++) {
        for (int j = 0; j < 2 ; j++) {
          if (aas.f1[i][j] != count++)
            {
              ACE_ERROR((LM_ERROR,
                ACE_TEXT("arrayOfArray[%d][%d] failed serailization\n"),
                  i,j));
              failed = true;
            }
        }
      }
    }

    mb->release (); // don't leak memrory
  }

  Xyz::AStruct a_struct;
  if (!_dcps_has_key(a_struct))
    {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("_dcps_has_key(Xyz::AStruct) returned false when expecting true.\n")
        ));
    }

  Xyz::StructContainingArrayOfAStructSeq ascaoass;
  if (_dcps_has_key(ascaoass))
    {
      ACE_ERROR((LM_ERROR,
        ACE_TEXT("_dcps_has_key(Xyz::StructContainingArrayOfAStructSeq) returned true when expecting false.\n")
        ));
    }

  Xyz::Foo my_foo;

  // greenx < redx < bluex
  const Xyz::ColorX orig_color = Xyz::greenx;

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
  my_foo.ushrtseq.length(2); //+4+2*2 >> 28
  my_foo.ushrtseq[0] = 7;
  my_foo.ushrtseq[1] = 11;
  my_foo.theString = CORBA::string_dup ("four");

  // seq lenghts theStruct +4+4 theStructSeq +4 theString +4 + structArray3*8 = 40
  // theUnion defaults to short so +4+2 = 6

  const size_t expected_max_marshaled_size = 135;
  const size_t expected_find_size = 28+40+6 + 4 /*string assigned */;
  const CORBA::Boolean expected_bounded = false;

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
  foo2.theString = CORBA::string_dup ("four");

  std::map<Xyz::Foo, Xyz::Foo*, FooKeyLessThan> foomap;

  if (_dcps_has_key(my_foo))
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

  size_t ms = _dcps_max_marshaled_size(my_foo);
  CORBA::Boolean bounded = _tao_is_bounded_size(my_foo);
  size_t cs = _dcps_find_size(my_foo);

  ACE_DEBUG((LM_DEBUG,"_max_marshaled_size(my_foo) => %d\n", ms));
  ACE_DEBUG((LM_DEBUG,"_tao_is_bounded_size(my_foo) => %d\n", bounded));
  ACE_DEBUG((LM_DEBUG,"_dcps_find_size(my_foo) => %d\n", cs));

  if (bounded != expected_bounded)
    {
      ACE_ERROR((LM_ERROR, "_tao_is_bounded_size(Foo) failed - expected %d got %d\n",
        expected_bounded, bounded));
      failed = true;
    }
  if (bounded && ms != expected_max_marshaled_size)
    {
      ACE_ERROR((LM_ERROR,
        "_tao_max_marshaled_size(Foo) returned %d when was expecting %d\n",
        ms, expected_max_marshaled_size));
      failed = true;
    }
  if (!bounded && cs != expected_find_size)
    {
      ACE_ERROR((LM_ERROR,
        "_dcps_find_size(Foo) returned %d when was expecting %d\n",
        cs, expected_find_size));
      failed = true;
    }

  TAO_OutputCDR cdr;
  if (false == (cdr << my_foo))
    {
      ACE_ERROR((LM_ERROR, "TAO_OutputCDR << failed\n"));
      failed = true;
    }


  ACE_TCHAR ebuffer[512] ; ebuffer[0] = ACE_TEXT('\0') ;
  ACE_TCHAR obuffer[512] ; obuffer[0] = ACE_TEXT('\0') ;

  // test serializing

  const size_t buff_size = bounded ? ms : cs;
  ACE_Message_Block *mb = new ACE_Message_Block(buff_size);
  TAO::DCPS::Serializer ss(mb);

  if (dump_buffer) 
    {
      ACE::format_hexdump( mb->rd_ptr(), mb->length(), ebuffer, 
        sizeof(ebuffer)) ;
      ACE_DEBUG((LM_DEBUG, 
        ACE_TEXT("BEFORE WRITING, LENGTH: %d, BUFFER:\n%s\n"), 
        mb->length(), ebuffer));
    }

  if (false == ss << my_foo)
    {
      ACE_ERROR((LM_ERROR, "Serializing failed\n"));
      failed = true;
    }

  if (dump_buffer) 
    {
      ACE::format_hexdump( mb->rd_ptr(), mb->length(), ebuffer, 
        sizeof(ebuffer)) ;
      ACE_DEBUG((LM_DEBUG, 
        ACE_TEXT("AFTER WRITING, LENGTH: %d, BUFFER:\n%s\n"), 
        mb->length(), ebuffer));
    }

  // Reset the chain back to the beginning.
  // This is needed when the buffer size = find_size(my_foo) because
  // the the serialize method set current_ the next block in the chain 
  // which is nil; so deserializing will fail.
  ss.add_chain(mb) ;

  Xyz::Foo ss_foo;
  if (false == ss >> ss_foo)
    {
      ACE_ERROR((LM_ERROR, "Deserializing failed\n"));
      failed = true;
    }

  if (dump_buffer) 
    {
      ACE::format_hexdump( mb->rd_ptr(), mb->length(), 
        ebuffer, sizeof(ebuffer)) ;
      ACE_DEBUG((LM_DEBUG, 
        ACE_TEXT("AFTER READING, LENGTH: %d, BUFFER:\n%s\n"), 
        mb->length(), ebuffer));

      ACE::format_hexdump( (char*)&(my_foo.key), sizeof(ACE_CDR::ULong), 
        ebuffer, sizeof(ebuffer)) ;
      ACE::format_hexdump( (char*)&(ss_foo.key), sizeof(ACE_CDR::ULong), 
        obuffer, sizeof(obuffer)) ;
      ACE_ERROR((LM_ERROR,
                ACE_TEXT("key (expected:\n%s, observed:\n%s)\n"),
                ebuffer, obuffer
              ));
      ACE::format_hexdump( (char*)&(my_foo.x), sizeof(ACE_CDR::Float), 
        ebuffer, sizeof(ebuffer)) ;
      ACE::format_hexdump( (char*)&(ss_foo.x), sizeof(ACE_CDR::Float), 
        obuffer, sizeof(obuffer)) ;
      ACE_ERROR((LM_ERROR,
                ACE_TEXT("x (expected:\n%s, observed:\n%s)\n"),
                ebuffer, obuffer
              ));
      ACE::format_hexdump( (char*)&(my_foo.y), sizeof(ACE_CDR::Float), 
        ebuffer, sizeof(ebuffer)) ;
      ACE::format_hexdump( (char*)&(ss_foo.y), sizeof(ACE_CDR::Float), 
        obuffer, sizeof(obuffer)) ;
      ACE_ERROR((LM_ERROR,
                ACE_TEXT("y (expected:\n%s, observed:\n%s)\n"),
                ebuffer, obuffer
              ));
      ACE::format_hexdump( (char*)&(my_foo.xcolor), sizeof(ACE_CDR::UShort), ebuffer, sizeof(ebuffer)) ;
      ACE::format_hexdump( (char*)&(ss_foo.xcolor), sizeof(ACE_CDR::UShort), obuffer, sizeof(obuffer)) ;
      ACE_ERROR((LM_ERROR,
                ACE_TEXT("xcolor (expected:\n%s, observed:\n%s)\n"),
                ebuffer, obuffer
              ));
      ACE::format_hexdump( (char*)&(my_foo.octer), sizeof(ACE_CDR::Octet), ebuffer, sizeof(ebuffer)) ;
      ACE::format_hexdump( (char*)&(ss_foo.octer), sizeof(ACE_CDR::Octet), obuffer, sizeof(obuffer)) ;
      ACE_ERROR((LM_ERROR,
                ACE_TEXT("octer (expected:\n%s, observed:\n%s)\n"),
                ebuffer, obuffer
              ));
    }

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

  mb->release(); // don't leak memory!

  if (failed)
    ACE_ERROR((LM_ERROR, "%s FAILED!\n", argv[0]));
  else
    ACE_DEBUG((LM_ERROR, "%s PASSED\n", argv[0]));

  return failed; // let the test framework know it failed
}
