/***
 *
 * ORDER OF IDL MATTERS
 * DO NOT ADD MEMBERS ANYWHERE BUT THE END OF THE OBJECT
 * OR YOU WILL BREAK THE TEST
 ***/
#include "TryConstructTypeSupportImpl.h"
#include "AnonTypesTypeSupportImpl.h"
#include "NestedTrimStructTypeSupportImpl.h"

#include <dds/DCPS/XTypes/TypeObject.h>
#include <gtest/gtest.h>

using namespace OpenDDS::XTypes;
using namespace OpenDDS::DCPS;
const Encoding xcdr2(Encoding::KIND_XCDR2, ENDIAN_BIG);

TEST(TestTryCon, string)
{
  TryCon::StringTest1 sent;
  sent.str64_d = "abc";
  sent.str64_ud = "abcdefghijklmnopqrstuvwxyz";
  sent.str64_t = "abcdefghijklmnopqrstuvwxyz";
  TryCon::StringTest2 expected;
  expected.str20_d = "abc";
  expected.str20_ud = "";
  expected.str20_t = "abcdefghijklmnopqrst";
  TryCon::StringTest2 actual;

  {
    Message_Block_Ptr data(new ACE_Message_Block(serialized_size(xcdr2, sent)));

    // Serialize and Compare CDR
    {
      Serializer serializer(data.get(), xcdr2);
      EXPECT_TRUE(serializer << sent);
    }

    // Deserialize and Compare C++ Values
    {
      Serializer serializer(data.get(), xcdr2);
      EXPECT_TRUE(serializer >> actual);
    }
    EXPECT_FALSE(strcmp(actual.str20_d.in(), expected.str20_d.in()));
    EXPECT_FALSE(strcmp(actual.str20_ud.in(), expected.str20_ud.in()));
    EXPECT_FALSE(strcmp(actual.str20_t.in(), expected.str20_t.in()));
  }
  {
    sent.str64_d = "abcdefghijklmnopqrstuvwxyz";
    Message_Block_Ptr data(new ACE_Message_Block(serialized_size(xcdr2, sent)));

    {
      Serializer serializer(data.get(), xcdr2);
      EXPECT_TRUE(serializer << sent);
    }

    {
      Serializer serializer(data.get(), xcdr2);
      EXPECT_FALSE(serializer >> actual);
    }
  }
}

TEST(StructandSeq, DISCARD)
{
  {
    TryCon::DiscardStructString1 sent;
    sent.str64_d = "abcdefghijklmnopqrstuvwxyz";
    TryCon::DiscardStructString2 actual;

    {
      Message_Block_Ptr data(new ACE_Message_Block(serialized_size(xcdr2, sent)));

      {
        Serializer serializer(data.get(), xcdr2);
        EXPECT_TRUE(serializer << sent);
      }

      {
        Serializer serializer(data.get(), xcdr2);
        EXPECT_FALSE(serializer >> actual);
      }
    }
  }
  {
    TryCon::DiscardStructWString1 sent;
    sent.wstr64_d = L"abcdefghijklmnopqrstuvwxyz";
    TryCon::DiscardStructWString2 actual;

    {
      Message_Block_Ptr data(new ACE_Message_Block(serialized_size(xcdr2, sent)));

      {
        Serializer serializer(data.get(), xcdr2);
        EXPECT_TRUE(serializer << sent);
      }

      {
        Serializer serializer(data.get(), xcdr2);
        EXPECT_FALSE(serializer >> actual);
      }
    }
  }
  {
    TryCon::DiscardStructStruct1 sent;
    sent.ns.str64_d = "abcdefghijklmnopqrstuvwxyz";
    sent.ns.str64_ud = "abcdefghijklmnopqrstuvwxyz";
    sent.ns.str64_t = "abcdefghijklmnopqrstuvwxyz";
    TryCon::DiscardStructStruct2 actual;

    {
      Message_Block_Ptr data(new ACE_Message_Block(serialized_size(xcdr2, sent)));

      {
        Serializer serializer(data.get(), xcdr2);
        EXPECT_TRUE(serializer << sent);
      }

      {
        Serializer serializer(data.get(), xcdr2);
        EXPECT_FALSE(serializer >> actual);
      }
    }
  }
  {
    TryCon::DiscardStructArray1 sent;
    sent.sa[0] = "abcdefghijklmnopqrstuvwxyz";
    TryCon::DiscardStructArray2 actual;

    {
      Message_Block_Ptr data(new ACE_Message_Block(serialized_size(xcdr2, sent)));

      {
        Serializer serializer(data.get(), xcdr2);
        EXPECT_TRUE(serializer << sent);
      }

      {
        Serializer serializer(data.get(), xcdr2);
        EXPECT_FALSE(serializer >> actual);
      }
    }
  }
  {
    TryCon::DiscardStructSequence1 sent;
    sent.ss64.length(1);
    sent.ss64[0] = "abcdefghijklmnopqrstuvwxyz";
    TryCon::DiscardStructSequence2 actual;

    {
      Message_Block_Ptr data(new ACE_Message_Block(serialized_size(xcdr2, sent)));

      {
        Serializer serializer(data.get(), xcdr2);
        EXPECT_TRUE(serializer << sent);
      }

      {
        Serializer serializer(data.get(), xcdr2);
        EXPECT_FALSE(serializer >> actual);
      }
    }
  }
  {
    TryCon::DiscardStructArrayAnon1 sent;
    sent.saa64[0] = "abcdefghijklmnopqrstuvwxyz";
    TryCon::DiscardStructArrayAnon2 actual;

    {
      Message_Block_Ptr data(new ACE_Message_Block(serialized_size(xcdr2, sent)));

      {
        Serializer serializer(data.get(), xcdr2);
        EXPECT_TRUE(serializer << sent);
      }

      {
        Serializer serializer(data.get(), xcdr2);
        EXPECT_FALSE(serializer >> actual);
      }
    }
  }
  {
    TryCon::DiscardStructSequenceAnon1 sent;
    sent.ssa64.length(1);
    sent.ssa64[0] = "abcdefghijklmnopqrstuvwxyz";
    TryCon::DiscardStructSequenceAnon2 actual;

    {
      Message_Block_Ptr data(new ACE_Message_Block(serialized_size(xcdr2, sent)));

      {
        Serializer serializer(data.get(), xcdr2);
        EXPECT_TRUE(serializer << sent);
      }

      {
        Serializer serializer(data.get(), xcdr2);
        EXPECT_FALSE(serializer >> actual);
      }
    }
  }
}

TEST(StructandSeq, USE_DEFAULT)
{
  TryCon::NestedStructTest1 sent;
  OpenDDS::DCPS::set_default(sent);
  sent.ns.str64_d = "abcdefghijklmnopqrstuvwxyz";
  sent.ns.str64_ud = "abcdefghijklmnopqrstuvwxyz";
  sent.ns.str64_t = "abcdefghijklmnopqrstuvwxyz";
  sent.ns.wstr64_ud = L"☺";
  sent.ns.psu_ud.length(1);
  sent.ns.psb_ud.length(1);
  sent.ns.upsu_ud.length(1);
  sent.ns.upsb_ud.length(1);
  sent.ns.esu_ud.length(1);
  sent.ns.esu_ud[0] = VALUE1;
  sent.ns.esb_ud.length(1);
  sent.ns.esb_ud[0] = VALUE1;
  sent.ns.strsu_ud.length(1);
  sent.ns.strsb_ud.length(1);
  sent.ns.wstrsu_ud.length(1);
  sent.ns.wstrsb_ud.length(1);
  sent.ns.ssu_ud.length(1);
  sent.ns.ssb_ud.length(1);
  sent.ns.sasu_ud.length(1);
  sent.ns.sasb_ud.length(1);
  sent.ns.sssuu_ud.length(1);
  sent.ns.sssub_ud.length(1);
  sent.ns.sssbu_ud.length(1);
  sent.ns.sssbb_ud.length(1);
  sent.ns.ns_ud.str64_d = "HELLO";
  sent.ns.ns_ud.str64_ud = "WORLD";
  sent.ns.ns_ud.str64_t = "GOODBYE";
  for (ACE_INT16 i = 0; i < 10; ++i) sent.ns.sa_mud[i] = 0;
  sent.ns.e_ud = VALUE1;
  sent.ns.by_ud = 0x00;
  sent.ns.bo_ud = false;
  sent.ns.s_ud = 5;
  sent.ns.us_ud = 5;
  TryCon::NestedStructTest2 expected;
  expected.ns.str20_d = "";
  expected.ns.str20_ud = "";
  expected.ns.str20_t = "";
  expected.ns.wstr64_ud = L"";
  expected.ns.ns_ud.str64_d = "";
  expected.ns.ns_ud.str64_ud = "";
  expected.ns.ns_ud.str64_t = "";
  for (ACE_INT16 i = 0; i < 10; ++i) expected.ns.sa_mud[i] = 0;
  expected.ns.e_ud = VALUE1;
  expected.ns.by_ud = 0x00;
  expected.ns.bo_ud = false;
  expected.ns.s_ud = 0;
  expected.ns.us_ud = 0;
  TryCon::NestedStructTest2 actual;

  {
    Message_Block_Ptr data(new ACE_Message_Block(serialized_size(xcdr2, sent)));

    {
      Serializer serializer(data.get(), xcdr2);
      EXPECT_TRUE(serializer << sent);
    }

    {
      Serializer serializer(data.get(), xcdr2);
      EXPECT_TRUE(serializer >> actual);
    }
    EXPECT_FALSE(strcmp(actual.ns.str20_d.in(), expected.ns.str20_d.in()));
    EXPECT_FALSE(strcmp(actual.ns.str20_ud.in(), expected.ns.str20_ud.in()));
    EXPECT_FALSE(strcmp(actual.ns.str20_t.in(), expected.ns.str20_t.in()));
    EXPECT_FALSE(wcscmp(actual.ns.wstr64_ud.in(), expected.ns.wstr64_ud.in()));
    EXPECT_EQ(actual.ns.psu_ud.length(), expected.ns.psu_ud.length());
    EXPECT_EQ(actual.ns.psb_ud.length(), expected.ns.psb_ud.length());
    EXPECT_EQ(actual.ns.upsu_ud.length(), expected.ns.upsu_ud.length());
    EXPECT_EQ(actual.ns.upsb_ud.length(), expected.ns.upsb_ud.length());
    EXPECT_EQ(actual.ns.esu_ud.length(), expected.ns.esu_ud.length());
    EXPECT_EQ(actual.ns.esb_ud.length(), expected.ns.esb_ud.length());
    EXPECT_EQ(actual.ns.strsu_ud.length(), expected.ns.strsu_ud.length());
    EXPECT_EQ(actual.ns.strsb_ud.length(), expected.ns.strsb_ud.length());
    EXPECT_EQ(actual.ns.wstrsu_ud.length(), expected.ns.wstrsu_ud.length());
    EXPECT_EQ(actual.ns.wstrsb_ud.length(), expected.ns.wstrsb_ud.length());
    EXPECT_EQ(actual.ns.ssu_ud.length(), expected.ns.ssu_ud.length());
    EXPECT_EQ(actual.ns.ssb_ud.length(), expected.ns.ssb_ud.length());
    EXPECT_EQ(actual.ns.sasu_ud.length(), expected.ns.sasu_ud.length());
    EXPECT_EQ(actual.ns.sasb_ud.length(), expected.ns.sasb_ud.length());
    EXPECT_EQ(actual.ns.sssuu_ud.length(), expected.ns.sssuu_ud.length());
    EXPECT_EQ(actual.ns.sssub_ud.length(), expected.ns.sssub_ud.length());
    EXPECT_EQ(actual.ns.sssbu_ud.length(), expected.ns.sssbu_ud.length());
    EXPECT_EQ(actual.ns.sssbb_ud.length(), expected.ns.sssbb_ud.length());
    EXPECT_FALSE(strcmp(actual.ns.ns_ud.str64_d.in(), expected.ns.ns_ud.str64_d.in()));
    EXPECT_FALSE(strcmp(actual.ns.ns_ud.str64_ud.in(), expected.ns.ns_ud.str64_ud.in()));
    EXPECT_FALSE(strcmp(actual.ns.ns_ud.str64_t.in(), expected.ns.ns_ud.str64_t.in()));
    for (ACE_INT16 i = 0; i < 10; ++i) {
      EXPECT_EQ(actual.ns.sa_mud[i], expected.ns.sa_mud[i]);
    }
    EXPECT_EQ(actual.ns.e_ud, expected.ns.e_ud);
    EXPECT_EQ(actual.ns.by_ud, expected.ns.by_ud);
    EXPECT_EQ(actual.ns.bo_ud, expected.ns.bo_ud);
    EXPECT_EQ(actual.ns.s_ud, expected.ns.s_ud);
    EXPECT_EQ(actual.ns.us_ud, expected.ns.us_ud);
  }
}

TEST(StructandSeq, TRIM)
{
  TryCon::TrimStruct1 sent;
  sent.str64_t = "abcdefghijklmnopqrstuvwxyz";
  sent.wstr64_t = L"abcdefghijklmnopqrstuvwxyz";
  sent.psu_t.length(3);
  for (ACE_INT16 i = 0; i < 3; ++i) {
    sent.psu_t[i] = 1;
  }
  sent.psb_t.length(3);
  for (ACE_INT16 i = 0; i < 3; ++i) {
    sent.psb_t[i] = 1;
  }
  sent.upsu_t.length(3);
  for (ACE_INT16 i = 0; i < 3; ++i) {
    sent.upsu_t[i] = 1;
  }
  sent.upsb_t.length(3);
  for (ACE_INT16 i = 0; i < 3; ++i) {
    sent.upsb_t[i] = 1;
  }
  sent.esu_t.length(3);
  for (ACE_INT16 i = 0; i < 3; ++i) {
    sent.esu_t[i] = VALUE1;
  }
  sent.esb_t.length(3);
  for (ACE_INT16 i = 0; i < 3; ++i) {
    sent.esb_t[i] = VALUE1;
  }
  sent.strsu_t.length(3);
  for (ACE_INT16 i = 0; i < 3; ++i) {
    sent.strsu_t[i] = "HELLOWORLD";
  }
  sent.strsb_t.length(3);
  for (ACE_INT16 i = 0; i < 3; ++i) {
    sent.strsb_t[i] = "HELLOWORLD";
  }
  sent.wstrsu_t.length(3);
  for (ACE_INT16 i = 0; i < 3; ++i) {
    sent.wstrsu_t[i] = L"☺";
  }
  sent.wstrsb_t.length(3);
  for (ACE_INT16 i = 0; i < 3; ++i) {
    sent.wstrsb_t[i] = L"☺";
  }
  sent.ssu_t.length(3);
  for (ACE_INT16 i = 0; i < 3; ++i) {
    sent.ssu_t[i].str64_t = "abcdefghijklmnopqrstuvwxyz";
  }
  sent.ssb_t.length(3);
  for (ACE_INT16 i = 0; i < 3; ++i) {
    sent.ssb_t[i].str64_t = "abcdefghijklmnopqrstuvwxyz";
  }
  sent.sasu_t.length(3);
  for (ACE_INT16 i = 0; i < 3; ++i) {
    for (ACE_INT16 j = 0; j < 10; j++) {
      sent.sasu_t[i][j] = 1;
    }
  }
  sent.sasb_t.length(3);
  for (ACE_INT16 i = 0; i < 3; ++i) {
    for (ACE_INT16 j = 0; j < 10; j++) {
      sent.sasb_t[i][j] = 1;
    }
  }
  sent.sssuu_t.length(3);
  for (ACE_INT16 i = 0; i < 3; ++i) {
    sent.sssuu_t[i].length(3);
    sent.sssuu_t[i][0] = 1;
    sent.sssuu_t[i][1] = 1;
    sent.sssuu_t[i][2] = 1;
  }
  sent.sssub_t.length(3);
  for (ACE_INT16 i = 0; i < 3; ++i) {
    sent.sssub_t[i].length(3);
    sent.sssub_t[i][0] = 1;
    sent.sssub_t[i][1] = 1;
    sent.sssub_t[i][2] = 1;
  }
  sent.sssbu_t.length(3);
  for (ACE_INT16 i = 0; i < 3; ++i) {
    sent.sssbu_t[i].length(3);
    sent.sssbu_t[i][0] = 1;
    sent.sssbu_t[i][1] = 1;
    sent.sssbu_t[i][2] = 1;
  }
  sent.sssbb_t.length(3);
  for (ACE_INT16 i = 0; i < 3; ++i) {
    sent.sssbb_t[i].length(3);
    sent.sssbb_t[i][0] = 1;
    sent.sssbb_t[i][1] = 1;
    sent.sssbb_t[i][2] = 1;
  }
  TryCon::TrimStruct2 expected;
  expected.str20_t = "abcdefghijklmnopqrst";
  expected.wstr20_t = L"abcdefghijklmnopqrst";
  expected.psu_t.length(3);
  expected.psb_t.length(2);
  expected.upsu_t.length(3);
  expected.upsb_t.length(2);
  expected.esu_t.length(3);
  expected.esb_t.length(2);
  expected.strsu_t.length(3);
  expected.strsb_t.length(2);
  expected.wstrsu_t.length(3);
  expected.wstrsb_t.length(2);
  expected.ssu_t.length(3);
  expected.ssu_t[0].str20_t = "abcdefghijklmnopqrst";
  expected.ssb_t.length(2);
  expected.ssb_t[0].str20_t = "abcdefghijklmnopqrst";
  expected.sasu_t.length(3);
  expected.sasb_t.length(2);
  expected.sssuu_t.length(3);
  expected.sssuu_t[0].length(3);
  expected.sssub_t.length(2);
  expected.sssub_t[0].length(3);
  expected.sssbu_t.length(3);
  expected.sssbu_t[0].length(2);
  expected.sssbb_t.length(2);
  expected.sssbb_t[0].length(2);
  TryCon::TrimStruct2 actual;

  {
    Message_Block_Ptr data(new ACE_Message_Block(serialized_size(xcdr2, sent)));

    {
      Serializer serializer(data.get(), xcdr2);
      EXPECT_TRUE(serializer << sent);
    }

    {
      Serializer serializer(data.get(), xcdr2);
      EXPECT_TRUE(serializer >> actual);
    }
    EXPECT_FALSE(strcmp(actual.str20_t.in(), expected.str20_t.in()));
    EXPECT_FALSE(wcscmp(actual.wstr20_t.in(), expected.wstr20_t.in()));
    EXPECT_EQ(actual.psu_t.length(), expected.psu_t.length());
    EXPECT_EQ(actual.psb_t.length(), expected.psb_t.length());
    EXPECT_EQ(actual.upsu_t.length(), expected.upsu_t.length());
    EXPECT_EQ(actual.upsb_t.length(), expected.upsb_t.length());
    EXPECT_EQ(actual.esu_t.length(), expected.esu_t.length());
    EXPECT_EQ(actual.esb_t.length(), expected.esb_t.length());
    EXPECT_EQ(actual.strsu_t.length(), expected.strsu_t.length());
    EXPECT_EQ(actual.strsb_t.length(), expected.strsb_t.length());
    EXPECT_EQ(actual.wstrsu_t.length(), expected.wstrsu_t.length());
    EXPECT_EQ(actual.wstrsb_t.length(), expected.wstrsb_t.length());
    EXPECT_EQ(actual.ssu_t.length(), expected.ssu_t.length());
    EXPECT_FALSE(strcmp(actual.ssu_t[0].str20_t.in(), expected.ssu_t[0].str20_t.in()));
    EXPECT_EQ(actual.ssb_t.length(), expected.ssb_t.length());
    EXPECT_FALSE(strcmp(actual.ssb_t[0].str20_t.in(), expected.ssb_t[0].str20_t.in()));
    EXPECT_EQ(actual.sasu_t.length(), expected.sasu_t.length());
    EXPECT_EQ(actual.sasb_t.length(), expected.sasb_t.length());
    EXPECT_EQ(actual.sssuu_t.length(), expected.sssuu_t.length());
    EXPECT_EQ(actual.sssuu_t[0].length(), expected.sssuu_t[0].length());
    EXPECT_EQ(actual.sssub_t.length(), expected.sssub_t.length());
    EXPECT_EQ(actual.sssub_t[0].length(), expected.sssub_t[0].length());
    EXPECT_EQ(actual.sssbu_t.length(), expected.sssbu_t.length());
    EXPECT_EQ(actual.sssbu_t[0].length(), expected.sssbu_t[0].length());
    EXPECT_EQ(actual.sssbb_t.length(), expected.sssbb_t.length());
    EXPECT_EQ(actual.sssbb_t[0].length(), expected.sssbb_t[0].length());
  }
}

TEST(AnonSequence, Trim)
{
  {
    TryCon::AnonSeqStruct sent;
    sent.AnonEnumSeqBound.length(3);
    for (ACE_INT16 i = 0; i < 3; ++i) {
      sent.AnonEnumSeqBound[i] = VALUE1;
    }
    sent.AnonShortSeqBound.length(3);
    for (ACE_INT16 i = 0; i < 3; ++i) {
      sent.AnonShortSeqBound[0] = 5;
    }
    sent.AnonUnsignedShortSeqBound.length(3);
    for (ACE_INT16 i = 0; i < 3; ++i) {
      sent.AnonUnsignedShortSeqBound[i] = i;
    }
    sent.AnonStringSeqBound.length(3);
    for (ACE_INT16 i = 0; i < 3; ++i) {
      sent.AnonStringSeqBound[i] = "abcdefghijklmnopqrstuvwxyz";
    }
    sent.AnonWideStringSeqBound.length(3);
    for (ACE_INT16 i = 0; i < 3; ++i) {
      sent.AnonWideStringSeqBound[i] = L"abcdefghijklmnopqrstuvwxyz";
    }
    sent.AnonShortArraySeqBound.length(3);
    for (ACE_INT16 i = 0; i < 3; ++i) {
      for (ACE_INT16 j = 0; j < 2; j++) {
        sent.AnonShortArraySeqBound[i][j] = j;
      }
    }
    ShortSeqUnbound ssu;
    ssu.length(3);
    for (ACE_INT16 i = 0; i < 3; ++i) {
      ssu[i] = i;
    }
    ShortSeqBound2 ssb;
    ssu.length(3);
    for (ACE_INT16 i = 0; i < 3; ++i) {
      ssu[i] = i;
    }
    sent.AnonShortSeqUnboundUnbound.length(3);
    for (ACE_INT16 i = 0; i < 3; ++i) {
      sent.AnonShortSeqUnboundUnbound[i] = ssu;
    }
    sent.AnonSeqShortSeqUnboundBound.length(3);
    for (ACE_INT16 i = 0; i < 3; ++i) {
      sent.AnonSeqShortSeqUnboundBound[i] = ssu;
    }
    sent.AnonSeqShortSeqBoundUnbound.length(3);
    for (ACE_INT16 i = 0; i < 3; ++i) {
      sent.AnonSeqShortSeqBoundUnbound[i] = ssb;
    }
    sent.AnonSeqShortSeqBoundBound.length(3);
    for (ACE_INT16 i = 0; i < 3; ++i) {
      sent.AnonSeqShortSeqBoundBound[i] = ssb;
    }

    TryCon::AnonSeqStructTrim expected;
    expected.AnonEnumSeqBound.length(2);
    for (ACE_INT16 i = 0; i < 2; ++i) {
      expected.AnonEnumSeqBound[i] = VALUE1;
    }
    expected.AnonShortSeqBound.length(2);
    for (ACE_INT16 i = 0; i < 2; ++i) {
      expected.AnonShortSeqBound[0] = 5;
    }
    expected.AnonUnsignedShortSeqBound.length(2);
    for (ACE_INT16 i = 0; i < 2; ++i) {
      expected.AnonUnsignedShortSeqBound[i] = i;
    }
    expected.AnonStringSeqBound.length(2);
    for (ACE_INT16 i = 0; i < 2; ++i) {
      expected.AnonStringSeqBound[i] = "abcdefghijklmnopqrst";
    }
    expected.AnonWideStringSeqBound.length(2);
    for (ACE_INT16 i = 0; i < 2; ++i) {
      expected.AnonWideStringSeqBound[i] = L"abcdefghijklmnopqrst";
    }
    expected.AnonShortArraySeqBound.length(2);
    for (ACE_INT16 i = 0; i < 2; ++i) {
      for (ACE_INT16 j = 0; j < 2; j++) {
        expected.AnonShortArraySeqBound[i][j] = j;
      }
    }
    ShortSeqUnbound ssu_2;
    ssu.length(3);
    for (ACE_INT16 i = 0; i < 2; ++i) {
      ssu[i] = i;
    }
    ShortSeqBound ssb_2;
    ssu.length(2);
    for (ACE_INT16 i = 0; i < 2; ++i) {
      ssu[i] = i;
    }
    expected.AnonShortSeqUnboundUnbound.length(3);
    for (ACE_INT16 i = 0; i < 3; ++i) {
      expected.AnonShortSeqUnboundUnbound[i] = ssu_2;
    }
    expected.AnonSeqShortSeqUnboundBound.length(2);
    for (ACE_INT16 i = 0; i < 2; ++i) {
      expected.AnonSeqShortSeqUnboundBound[i] = ssu_2;
    }
    expected.AnonSeqShortSeqBoundUnbound.length(3);
    for (ACE_INT16 i = 0; i < 3; ++i) {
      expected.AnonSeqShortSeqBoundUnbound[i] = ssb_2;
    }
    expected.AnonSeqShortSeqBoundBound.length(2);
    for (ACE_INT16 i = 0; i < 2; ++i) {
      expected.AnonSeqShortSeqBoundBound[i] = ssb_2;
    }

    TryCon::AnonSeqStructTrim actual;

    {
      Message_Block_Ptr data(new ACE_Message_Block(serialized_size(xcdr2, sent)));

      {
        Serializer serializer(data.get(), xcdr2);
        EXPECT_TRUE(serializer << sent);
      }

      {
        Serializer serializer(data.get(), xcdr2);
        EXPECT_TRUE(serializer >> actual);
      }
    }
    EXPECT_EQ(actual.AnonEnumSeqBound.length(), expected.AnonEnumSeqBound.length());
    EXPECT_EQ(actual.AnonShortSeqBound.length(), expected.AnonShortSeqBound.length());
    EXPECT_EQ(actual.AnonUnsignedShortSeqBound.length(), expected.AnonUnsignedShortSeqBound.length());
    EXPECT_EQ(actual.AnonStringSeqBound.length(), expected.AnonStringSeqBound.length());
    EXPECT_EQ(actual.AnonWideStringSeqBound.length(), expected.AnonWideStringSeqBound.length());
    EXPECT_EQ(actual.AnonShortArraySeqBound.length(), expected.AnonShortArraySeqBound.length());
    EXPECT_EQ(actual.AnonShortSeqUnboundUnbound.length(), expected.AnonShortSeqUnboundUnbound.length());
    EXPECT_EQ(actual.AnonSeqShortSeqUnboundBound.length(), expected.AnonSeqShortSeqUnboundBound.length());
    EXPECT_EQ(actual.AnonSeqShortSeqBoundUnbound.length(), expected.AnonSeqShortSeqBoundUnbound.length());
    EXPECT_EQ(actual.AnonSeqShortSeqBoundBound.length(), expected.AnonSeqShortSeqBoundBound.length());
  }
}

TEST(AnonSequence, USE_DEFAULT)
{
  {
    TryCon::AnonSeqStruct sent;
    sent.AnonEnumSeqBound.length(3);
    for (ACE_INT16 i = 0; i < 3; ++i) {
      sent.AnonEnumSeqBound[i] = VALUE1;
    }
    sent.AnonShortSeqBound.length(3);
    for (ACE_INT16 i = 0; i < 3; ++i) {
      sent.AnonShortSeqBound[0] = 5;
    }
    sent.AnonUnsignedShortSeqBound.length(3);
    for (ACE_INT16 i = 0; i < 3; ++i) {
      sent.AnonUnsignedShortSeqBound[i] = i;
    }
    sent.AnonStringSeqBound.length(3);
    for (ACE_INT16 i = 0; i < 3; ++i) {
      sent.AnonStringSeqBound[i] = "abcdefghijklmnopqrstuvwxyz";
    }
    sent.AnonWideStringSeqBound.length(3);
    for (ACE_INT16 i = 0; i < 3; ++i) {
      sent.AnonWideStringSeqBound[i] = L"abcdefghijklmnopqrstuvwxyz";
    }
    sent.AnonShortArraySeqBound.length(3);
    for (ACE_INT16 i = 0; i < 3; ++i) {
      for (ACE_INT16 j = 0; j < 2; j++) {
        sent.AnonShortArraySeqBound[i][j] = j;
      }
    }
    ShortSeqUnbound ssu;
    ssu.length(3);
    for (ACE_INT16 i = 0; i < 3; ++i) {
      ssu[i] = i;
    }
    ShortSeqBound2 ssb;
    ssu.length(3);
    for (ACE_INT16 i = 0; i < 3; ++i) {
      ssu[i] = i;
    }
    sent.AnonShortSeqUnboundUnbound.length(3);
    for (ACE_INT16 i = 0; i < 3; ++i) {
      sent.AnonShortSeqUnboundUnbound[i] = ssu;
    }
    sent.AnonSeqShortSeqUnboundBound.length(3);
    for (ACE_INT16 i = 0; i < 3; ++i) {
      sent.AnonSeqShortSeqUnboundBound[i] = ssu;
    }
    sent.AnonSeqShortSeqBoundUnbound.length(3);
    for (ACE_INT16 i = 0; i < 3; ++i) {
      sent.AnonSeqShortSeqBoundUnbound[i] = ssb;
    }
    sent.AnonSeqShortSeqBoundBound.length(3);
    for (ACE_INT16 i = 0; i < 3; ++i) {
      sent.AnonSeqShortSeqBoundBound[i] = ssb;
    }

    TryCon::AnonSeqStructDefault expected;
    expected.AnonShortSeqUnboundUnbound.length(3);
    expected.AnonSeqShortSeqBoundUnbound.length(3);

    TryCon::AnonSeqStructDefault actual;

    {
      Message_Block_Ptr data(new ACE_Message_Block(serialized_size(xcdr2, sent)));

      {
        Serializer serializer(data.get(), xcdr2);
        EXPECT_TRUE(serializer << sent);
      }

      {
        Serializer serializer(data.get(), xcdr2);
        EXPECT_TRUE(serializer >> actual);
      }
    }
    EXPECT_EQ(actual.AnonEnumSeqBound.length(), expected.AnonEnumSeqBound.length());
    EXPECT_EQ(actual.AnonShortSeqBound.length(), expected.AnonShortSeqBound.length());
    EXPECT_EQ(actual.AnonUnsignedShortSeqBound.length(), expected.AnonUnsignedShortSeqBound.length());
    EXPECT_EQ(actual.AnonStringSeqBound.length(), expected.AnonStringSeqBound.length());
    EXPECT_EQ(actual.AnonWideStringSeqBound.length(), expected.AnonWideStringSeqBound.length());
    EXPECT_EQ(actual.AnonShortArraySeqBound.length(), expected.AnonShortArraySeqBound.length());
    EXPECT_EQ(actual.AnonShortSeqUnboundUnbound.length(), expected.AnonShortSeqUnboundUnbound.length());
    EXPECT_EQ(actual.AnonSeqShortSeqUnboundBound.length(), expected.AnonSeqShortSeqUnboundBound.length());
    EXPECT_EQ(actual.AnonSeqShortSeqBoundUnbound.length(), expected.AnonSeqShortSeqBoundUnbound.length());
    EXPECT_EQ(actual.AnonSeqShortSeqBoundBound.length(), expected.AnonSeqShortSeqBoundBound.length());
  }
}

TEST(AnonArray, TRIM)
{
  {
    TryCon::AnonArrStruct sent;
    for (ACE_INT16 i = 0; i < 3; ++i) {
      sent.AnonStringArr[i] = "abcdefghijklmnopqrstuvwxyz";
    }
    for (ACE_INT16 i = 0; i < 3; ++i) {
      sent.AnonWideStringArr[i] = L"abcdefghijklmnopqrstuvwxyz";
    }
    for (ACE_INT16 i = 0; i < 3; ++i) {
      for (ACE_INT16 j = 0; j < 10; j++) {
        sent.AnonArrayArr[i][j] = "abcdefghijklmnopqrstuvwxyz";
      }
    }
    ShortSeqBound2 temp_seq;
    temp_seq.length(3);
    for (ACE_INT16 i = 0; i < 3; ++i) {
      temp_seq[i] = i;
    }
    for (ACE_INT16 i = 0; i < 3; ++i) {
      sent.AnonArrShortSeqBound[i] = temp_seq;
    }

    TryCon::AnonArrStructTrim expected;
    for (ACE_INT16 i = 0; i < 3; ++i) {
      expected.AnonStringArr[i] = "abcdefghijklmnopqrst";
    }
    for (ACE_INT16 i = 0; i < 3; ++i) {
      expected.AnonWideStringArr[i] = L"abcdefghijklmnopqrst";
    }
    for (ACE_INT16 i = 0; i < 3; ++i) {
      for (ACE_INT16 j = 0; j < 10; j++) {
        expected.AnonArrayArr[i][j] = "abcdefghijklmnopqrst";
      }
    }
    for (ACE_INT16 i = 0; i < 3; ++i) {
      expected.AnonArrShortSeqBound[i].length(2);
    }
    TryCon::AnonArrStructTrim actual;

    {
      Message_Block_Ptr data(new ACE_Message_Block(serialized_size(xcdr2, sent)));

      {
        Serializer serializer(data.get(), xcdr2);
        EXPECT_TRUE(serializer << sent);
      }

      {
        Serializer serializer(data.get(), xcdr2);
        EXPECT_TRUE(serializer >> actual);
      }
    }
    for (ACE_INT16 i = 0; i < 3; ++i) {
      ASSERT_STREQ(actual.AnonStringArr[i].in(), expected.AnonStringArr[i].in());
      ASSERT_STREQ(actual.AnonWideStringArr[i].in(), expected.AnonWideStringArr[i].in());
      for (ACE_INT16 j = 0; j < 10; j++) {
        ASSERT_STREQ(actual.AnonArrayArr[i][j].in(), expected.AnonArrayArr[i][j].in());
      }
      EXPECT_EQ(actual.AnonArrShortSeqBound[i].length(), expected.AnonArrShortSeqBound[i].length());
    }
  }
}

TEST(AnonArray, USE_DEFAULT)
{
  {
    TryCon::AnonArrStruct sent;
    for (ACE_INT16 i = 0; i < 3; ++i) {
      sent.AnonStringArr[i] = "abcdefghijklmnopqrstuvwxyz";
    }
    for (ACE_INT16 i = 0; i < 3; ++i) {
      sent.AnonWideStringArr[i] = L"abcdefghijklmnopqrstuvwxyz";
    }
    for (ACE_INT16 i = 0; i < 3; ++i) {
      for (ACE_INT16 j = 0; j < 10; j++) {
        sent.AnonArrayArr[i][j] = "abcdefghijklmnopqrstuvwxyz";
      }
    }
    ShortSeqBound2 temp_seq;
    temp_seq.length(3);
    for (ACE_INT16 i = 0; i < 3; ++i) {
      temp_seq[i] = i;
    }
    for (ACE_INT16 i = 0; i < 3; ++i) {
      sent.AnonArrShortSeqBound[i] = temp_seq;
    }

    TryCon::AnonArrStructUseDefault expected;
    for (ACE_INT16 i = 0; i < 3; ++i) {
      expected.AnonStringArr[i] = "";
    }
    for (ACE_INT16 i = 0; i < 3; ++i) {
      expected.AnonWideStringArr[i] = L"";
    }
    for (ACE_INT16 i = 0; i < 3; ++i) {
      for (ACE_INT16 j = 0; j < 10; j++) {
        expected.AnonArrayArr[i][j] = "";
      }
    }
    TryCon::AnonArrStructUseDefault actual;

    {
      Message_Block_Ptr data(new ACE_Message_Block(serialized_size(xcdr2, sent)));

      {
        Serializer serializer(data.get(), xcdr2);
        EXPECT_TRUE(serializer << sent);
      }

      {
        Serializer serializer(data.get(), xcdr2);
        EXPECT_TRUE(serializer >> actual);
      }
    }
    for (ACE_INT16 i = 0; i < 3; ++i) {
      ASSERT_STREQ(actual.AnonStringArr[i].in(), expected.AnonStringArr[i].in());
      ASSERT_STREQ(actual.AnonWideStringArr[i].in(), expected.AnonWideStringArr[i].in());
      for (ACE_INT16 j = 0; j < 10; j++) {
        ASSERT_STREQ(actual.AnonArrayArr[i][j].in(), expected.AnonArrayArr[i][j].in());
      }
      EXPECT_EQ(actual.AnonArrShortSeqBound[i].length(), expected.AnonArrShortSeqBound[i].length());
    }
  }
}

TEST(Union, DISCARD)
{
  {
    TryCon::BaseUnion sent;
    sent._d(0);
    sent.str_d("abcdefghijklmnopqrstuvwxyz");
    TryCon::DiscardUnion actual;
    {
      Message_Block_Ptr data(new ACE_Message_Block(serialized_size(xcdr2, sent)));

      {
        Serializer serializer(data.get(), xcdr2);
        EXPECT_TRUE(serializer << sent);
      }

      {
        Serializer serializer(data.get(), xcdr2);
        EXPECT_FALSE(serializer >> actual);
      }
    }
  }
  {
    TryCon::BaseUnion sent;
    sent._d(3);
    sent.wstr_d(L"abcdefghijklmnopqrstuvwxyz");
    TryCon::DiscardUnion actual;
    {
      Message_Block_Ptr data(new ACE_Message_Block(serialized_size(xcdr2, sent)));

      {
        Serializer serializer(data.get(), xcdr2);
        EXPECT_TRUE(serializer << sent);
      }

      {
        Serializer serializer(data.get(), xcdr2);
        EXPECT_FALSE(serializer >> actual);
      }
    }
  }
  {
    TryCon::BaseUnion sent;
    sent._d(6);
    str64Array temp_arr;
    temp_arr[0] = "abcdefghijklmnopqrstuvwxyz";
    sent.stra_d(temp_arr);
    TryCon::DiscardUnion actual;
    {
      Message_Block_Ptr data(new ACE_Message_Block(serialized_size(xcdr2, sent)));

      {
        Serializer serializer(data.get(), xcdr2);
        EXPECT_TRUE(serializer << sent);
      }

      {
        Serializer serializer(data.get(), xcdr2);
        EXPECT_FALSE(serializer >> actual);
      }
    }
  }
  {
    TryCon::BaseUnion sent;
    sent._d(9);
    StringSeqBound2 temp_seq;
    temp_seq.length(3);
    for (ACE_INT16 i = 0; i < 3; ++i) {
      temp_seq[i] = "abcdefghijklmnopqrstuvwxyz";
    }
    sent.strs_d(temp_seq);
    TryCon::DiscardUnion actual;
    {
      Message_Block_Ptr data(new ACE_Message_Block(serialized_size(xcdr2, sent)));

      {
        Serializer serializer(data.get(), xcdr2);
        EXPECT_TRUE(serializer << sent);
      }

      {
        Serializer serializer(data.get(), xcdr2);
        EXPECT_FALSE(serializer >> actual);
      }
    }
  }
  {
    TryCon::BaseUnion sent;
    sent._d(12);
    sent.e_d(BExtra);
    TryCon::DiscardUnion actual;
    {
      Message_Block_Ptr data(new ACE_Message_Block(serialized_size(xcdr2, sent)));

      {
        Serializer serializer(data.get(), xcdr2);
        EXPECT_TRUE(serializer << sent);
      }

      {
        Serializer serializer(data.get(), xcdr2);
        EXPECT_FALSE(serializer >> actual);
      }
    }
  }
  {
    TryCon::BaseDiscrimUnion sent;
    sent._d(BExtra);
    sent.s4(5);
    TryCon::DiscardDiscrimUnion actual;
    {
      Message_Block_Ptr data(new ACE_Message_Block(serialized_size(xcdr2, sent)));

      {
        Serializer serializer(data.get(), xcdr2);
        EXPECT_TRUE(serializer << sent);
      }

      {
        Serializer serializer(data.get(), xcdr2);
        EXPECT_FALSE(serializer >> actual);
      }
    }
  }
}

TEST(Union, USE_DEFAULT)
{
  {
    TryCon::BaseUnion sent;
    sent._d(1);
    sent.str_ud("abcdefghijklmnopqrstuvwxyz");
    TryCon::DefaultUnion expected;
    expected._d(1);
    expected.str_ud("");
    TryCon::DefaultUnion actual;
    {
      Message_Block_Ptr data(new ACE_Message_Block(serialized_size(xcdr2, sent)));

      {
        Serializer serializer(data.get(), xcdr2);
        EXPECT_TRUE(serializer << sent);
      }

      {
        Serializer serializer(data.get(), xcdr2);
        EXPECT_TRUE(serializer >> actual);
      }
        ASSERT_EQ(expected.str_ud(), actual.str_ud());
    }
  }
  {
    TryCon::BaseUnion sent;
    sent._d(4);
    sent.wstr_ud(L"abcdefghijklmnopqrstuvwxyz");
    TryCon::DefaultUnion expected;
    expected._d(4);
    expected.wstr_ud(L"");
    TryCon::DefaultUnion actual;
    {
      Message_Block_Ptr data(new ACE_Message_Block(serialized_size(xcdr2, sent)));

      {
        Serializer serializer(data.get(), xcdr2);
        EXPECT_TRUE(serializer << sent);
      }

      {
        Serializer serializer(data.get(), xcdr2);
        EXPECT_TRUE(serializer >> actual);
      }
      ASSERT_EQ(expected.wstr_ud(), actual.wstr_ud());
    }
  }
  {
    TryCon::BaseUnion sent;
    sent._d(7);
    str64Array temp_arr;
    for (ACE_INT16 i = 0; i < 10; ++i) {
      temp_arr[i] = "abcdefghijklmnopqrstuvwxyz";
    }
    sent.stra_ud(temp_arr);
    TryCon::DefaultUnion expected;
    expected._d(7);
    str64Array temp_arr2;
    for (ACE_INT16 i = 0; i < 3; ++i) {
      temp_arr2[i] = "";
    }
    expected.stra_ud(temp_arr2);
    TryCon::DefaultUnion actual;
    {
      Message_Block_Ptr data(new ACE_Message_Block(serialized_size(xcdr2, sent)));

      {
        Serializer serializer(data.get(), xcdr2);
        EXPECT_TRUE(serializer << sent);
      }

      {
        Serializer serializer(data.get(), xcdr2);
        EXPECT_TRUE(serializer >> actual);
      }
      ASSERT_STREQ(expected.stra_ud()[0].in(), actual.stra_ud()[0].in());
    }
  }
  {
    TryCon::BaseUnion sent;
    sent._d(10);
    StringSeqBound2 temp_seq;
    temp_seq.length(3);
    for (ACE_INT16 i = 0; i < 3; ++i) {
      temp_seq[i] = "abcdefghijklmnopqrstuvwxyz";
    }
    sent.strs_ud(temp_seq);
    TryCon::DefaultUnion expected;
    expected._d(10);
    StringSeqBound temp_seq2;
    expected.strs_ud(temp_seq2);
    TryCon::DefaultUnion actual;
    {
      Message_Block_Ptr data(new ACE_Message_Block(serialized_size(xcdr2, sent)));

      {
        Serializer serializer(data.get(), xcdr2);
        EXPECT_TRUE(serializer << sent);
      }

      {
        Serializer serializer(data.get(), xcdr2);
        EXPECT_TRUE(serializer >> actual);
      }
      ASSERT_EQ(expected.strs_ud().length(), actual.strs_ud().length());
    }
  }
  {
    TryCon::BaseUnion sent;
    sent._d(13);
    sent.e_ud(BExtra);
    TryCon::DefaultUnion expected;
    expected._d(13);
    expected.e_ud(VALUE1);
    TryCon::DefaultUnion actual;
    {
      Message_Block_Ptr data(new ACE_Message_Block(serialized_size(xcdr2, sent)));

      {
        Serializer serializer(data.get(), xcdr2);
        EXPECT_TRUE(serializer << sent);
      }

      {
        Serializer serializer(data.get(), xcdr2);
        EXPECT_TRUE(serializer >> actual);
      }
      ASSERT_EQ(expected.e_ud(), actual.e_ud());
    }
  }
  {
    TryCon::BaseDiscrimUnion sent;
    sent._d(BExtra);
    sent.s4(5);
    TryCon::DefaultDiscrimUnion expected;
    expected._d(VALUE1);
    expected.s1(0);
    TryCon::DefaultDiscrimUnion actual;
    {
      Message_Block_Ptr data(new ACE_Message_Block(serialized_size(xcdr2, sent)));

      {
        Serializer serializer(data.get(), xcdr2);
        EXPECT_TRUE(serializer << sent);
      }

      {
        Serializer serializer(data.get(), xcdr2);
        EXPECT_TRUE(serializer >> actual);
      }
      ASSERT_EQ(expected.s1(), actual.s1());
      ASSERT_EQ(expected._d(), actual._d());
    }
  }
}

TEST(Union, TRIM)
{
  {
    TryCon::BaseUnion sent;
    sent._d(2);
    sent.str_t("abcdefghijklmnopqrstuvwxyz");
    TryCon::TrimUnion expected;
    expected._d(2);
    expected.str_t("abcdefghijklmnopqrst");
    TryCon::TrimUnion actual;
    {
      Message_Block_Ptr data(new ACE_Message_Block(serialized_size(xcdr2, sent)));

      {
        Serializer serializer(data.get(), xcdr2);
        EXPECT_TRUE(serializer << sent);
      }

      {
        Serializer serializer(data.get(), xcdr2);
        EXPECT_TRUE(serializer >> actual);
      }
        ASSERT_STREQ(expected.str_t(), actual.str_t());
    }
  }
  {
    TryCon::BaseUnion sent;
    sent._d(5);
    sent.wstr_t(L"abcdefghijklmnopqrstuvwxyz");
    TryCon::TrimUnion expected;
    expected._d(5);
    expected.wstr_t(L"abcdefghijklmnopqrst");
    TryCon::TrimUnion actual;
    {
      Message_Block_Ptr data(new ACE_Message_Block(serialized_size(xcdr2, sent)));

      {
        Serializer serializer(data.get(), xcdr2);
        EXPECT_TRUE(serializer << sent);
      }

      {
        Serializer serializer(data.get(), xcdr2);
        EXPECT_TRUE(serializer >> actual);
      }
      ASSERT_STREQ(expected.wstr_t(), actual.wstr_t());
    }
  }
  {
    TryCon::BaseUnion sent;
    sent._d(8);
    str64Array temp_arr;
    for (ACE_INT16 i = 0; i < 10; ++i) {
      temp_arr[i] = "abcdefghijklmnopqrstuvwxyz";
    }
    sent.stra_t(temp_arr);
    TryCon::TrimUnion expected;
    expected._d(8);
    str64Array temp_arr2;
    for (ACE_INT16 i = 0; i < 3; ++i) {
      temp_arr2[i] = "abcdefghijklmnopqrst";
    }
    expected.stra_t(temp_arr2);
    TryCon::TrimUnion actual;
    {
      Message_Block_Ptr data(new ACE_Message_Block(serialized_size(xcdr2, sent)));

      {
        Serializer serializer(data.get(), xcdr2);
        EXPECT_TRUE(serializer << sent);
      }

      {
        Serializer serializer(data.get(), xcdr2);
        EXPECT_TRUE(serializer >> actual);
      }
      ASSERT_STREQ(expected.stra_t()[0].in(), actual.stra_t()[0].in());
    }
  }
  {
    TryCon::BaseUnion sent;
    sent._d(11);
    StringSeqBound2 temp_seq;
    temp_seq.length(3);
    for (ACE_INT16 i = 0; i < 3; ++i) {
      temp_seq[i] = "abcdefghijklmnopqrstuvwxyz";
    }
    sent.strs_t(temp_seq);
    TryCon::TrimUnion expected;
    expected._d(11);
    StringSeqBound temp_seq2;
    temp_seq2.length(2);
    for (ACE_INT16 i = 0; i < 2; ++i) {
      temp_seq2[i] = "abcdefghijklmnopqrstuvwxyz";
    }
    expected.strs_t(temp_seq2);
    TryCon::TrimUnion actual;
    {
      Message_Block_Ptr data(new ACE_Message_Block(serialized_size(xcdr2, sent)));

      {
        Serializer serializer(data.get(), xcdr2);
        EXPECT_TRUE(serializer << sent);
      }

      {
        Serializer serializer(data.get(), xcdr2);
        EXPECT_TRUE(serializer >> actual);
      }
      ASSERT_EQ(expected.strs_t().length(), actual.strs_t().length());
    }
  }
}

int main(int argc, char* argv[])
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
