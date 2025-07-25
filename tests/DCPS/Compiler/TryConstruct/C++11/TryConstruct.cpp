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
  sent.str64_d("abc");
  sent.str64_ud("abcdefghijklmnopqrstuvwxyz");
  sent.str64_t("abcdefghijklmnopqrstuvwxyz");
  TryCon::StringTest2 expected;
  expected.str20_d("abc");
  expected.str20_ud("");
  expected.str20_t("abcdefghijklmnopqrst");
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
    ASSERT_EQ(actual.str20_d(), expected.str20_d());
    ASSERT_EQ(actual.str20_ud(), expected.str20_ud());
    ASSERT_EQ(actual.str20_t(), expected.str20_t());
  }
  {
    sent.str64_d("abcdefghijklmnopqrstuvwxyz");
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
    sent.str64_d("abcdefghijklmnopqrstuvwxyz");
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
    sent.wstr64_d(L"abcdefghijklmnopqrstuvwxyz");
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
    sent.ns().str64_d("abcdefghijklmnopqrstuvwxyz");
    sent.ns().str64_ud("abcdefghijklmnopqrstuvwxyz");
    sent.ns().str64_t("abcdefghijklmnopqrstuvwxyz");
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
    sent.sa()[0] = "abcdefghijklmnopqrstuvwxyz";
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
    sent.ss64().resize(1);
    sent.ss64()[0] = "abcdefghijklmnopqrstuvwxyz";
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
    sent.saa64()[0] = "abcdefghijklmnopqrstuvwxyz";
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
    sent.ssa64().resize(1);
    sent.ssa64()[0] = "abcdefghijklmnopqrstuvwxyz";
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
  sent.ns().str64_d("abcdefghijklmnopqrstuvwxyz");
  sent.ns().str64_ud("abcdefghijklmnopqrstuvwxyz");
  sent.ns().str64_t("abcdefghijklmnopqrstuvwxyz");
  sent.ns().wstr64_ud(L"☺");
  sent.ns().psu_ud().resize(1);
  sent.ns().psb_ud().resize(1);
  sent.ns().upsu_ud().resize(1);
  sent.ns().upsb_ud().resize(1);
  sent.ns().esu_ud().resize(1);
  sent.ns().esb_ud().resize(1);
  sent.ns().strsu_ud().resize(1);
  sent.ns().strsb_ud().resize(1);
  sent.ns().wstrsu_ud().resize(1);
  sent.ns().wstrsb_ud().resize(1);
  sent.ns().ssu_ud().resize(1);
  sent.ns().ssb_ud().resize(1);
  sent.ns().sasu_ud().resize(1);
  sent.ns().sasb_ud().resize(1);
  sent.ns().sssuu_ud().resize(1);
  sent.ns().sssub_ud().resize(1);
  sent.ns().sssbu_ud().resize(1);
  sent.ns().sssbb_ud().resize(1);
  sent.ns().ns_ud().str64_d("HELLO");
  sent.ns().ns_ud().str64_ud("WORLD");
  sent.ns().ns_ud().str64_t("GOODBYE");
  for (size_t i = 0; i < 10; ++i) sent.ns().sa_mud()[i] = 0;
  sent.ns().e_ud() = EnumType::VALUE1;
  sent.ns().by_ud() = 0x00;
  sent.ns().bo_ud() = false;
  sent.ns().s_ud() = 5;
  sent.ns().us_ud() = 5;
  TryCon::NestedStructTest2 expected;
  expected.ns().str20_d("");
  expected.ns().str20_ud("");
  expected.ns().str20_t("");
  expected.ns().wstr64_ud(L"");
  expected.ns().ns_ud().str64_d("");
  expected.ns().ns_ud().str64_ud("");
  expected.ns().ns_ud().str64_t("");
  for (size_t i = 0; i < 10; ++i) expected.ns().sa_mud()[i] = 0;
  expected.ns().e_ud() = EnumType::VALUE1;
  expected.ns().by_ud() = 0x00;
  expected.ns().bo_ud() = false;
  expected.ns().s_ud() = 0;
  expected.ns().us_ud() = 0;
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
    ASSERT_EQ(actual.ns().str20_d(), expected.ns().str20_d());
    ASSERT_EQ(actual.ns().str20_ud(), expected.ns().str20_ud());
    ASSERT_EQ(actual.ns().str20_t(), expected.ns().str20_t());
    ASSERT_EQ(actual.ns().wstr64_ud(), expected.ns().wstr64_ud());
    EXPECT_EQ(actual.ns().psu_ud().size() , expected.ns().psu_ud().size());
    EXPECT_EQ(actual.ns().psb_ud().size(), expected.ns().psb_ud().size());
    EXPECT_EQ(actual.ns().upsu_ud().size(), expected.ns().upsu_ud().size());
    EXPECT_EQ(actual.ns().upsb_ud().size(), expected.ns().upsb_ud().size());
    EXPECT_EQ(actual.ns().esu_ud().size(), expected.ns().esu_ud().size());
    EXPECT_EQ(actual.ns().esb_ud().size(), expected.ns().esb_ud().size());
    EXPECT_EQ(actual.ns().strsu_ud().size(), expected.ns().strsu_ud().size());
    EXPECT_EQ(actual.ns().strsb_ud().size(), expected.ns().strsb_ud().size());
    EXPECT_EQ(actual.ns().wstrsu_ud().size(), expected.ns().wstrsu_ud().size());
    EXPECT_EQ(actual.ns().wstrsb_ud().size(), expected.ns().wstrsb_ud().size());
    EXPECT_EQ(actual.ns().ssu_ud().size(), expected.ns().ssu_ud().size());
    EXPECT_EQ(actual.ns().ssb_ud().size(), expected.ns().ssb_ud().size());
    EXPECT_EQ(actual.ns().sasu_ud().size(), expected.ns().sasu_ud().size());
    EXPECT_EQ(actual.ns().sasb_ud().size(), expected.ns().sasb_ud().size());
    EXPECT_EQ(actual.ns().sssuu_ud().size(), expected.ns().sssuu_ud().size());
    EXPECT_EQ(actual.ns().sssub_ud().size(), expected.ns().sssub_ud().size());
    EXPECT_EQ(actual.ns().sssbu_ud().size(), expected.ns().sssbu_ud().size());
    EXPECT_EQ(actual.ns().sssbb_ud().size(), expected.ns().sssbb_ud().size());
    ASSERT_EQ(actual.ns().ns_ud().str64_d(), expected.ns().ns_ud().str64_d());
    ASSERT_EQ(actual.ns().ns_ud().str64_ud(), expected.ns().ns_ud().str64_ud());
    ASSERT_EQ(actual.ns().ns_ud().str64_t(), expected.ns().ns_ud().str64_t());
    for (size_t i = 0; i < 10; ++i) {
      EXPECT_EQ(actual.ns().sa_mud()[i], expected.ns().sa_mud()[i]);
    }
    EXPECT_EQ(actual.ns().e_ud(), expected.ns().e_ud());
    EXPECT_EQ(actual.ns().by_ud(), expected.ns().by_ud());
    EXPECT_EQ(actual.ns().bo_ud(), expected.ns().bo_ud());
    EXPECT_EQ(actual.ns().s_ud(), expected.ns().s_ud());
    EXPECT_EQ(actual.ns().us_ud(), expected.ns().us_ud());
  }
}

TEST(StructandSeq, TRIM)
{
  TryCon::TrimStruct1 sent;
  sent.str64_t("abcdefghijklmnopqrstuvwxyz");
  sent.wstr64_t(L"abcdefghijklmnopqrstuvwxyz");
  sent.psu_t().resize(3);
  for (size_t i = 0; i < 3; ++i) {
    sent.psu_t()[i] = 1;
  }
  sent.psb_t().resize(3);
  for (size_t i = 0; i < 3; ++i) {
    sent.psb_t()[i] = 1;
  }
  sent.upsu_t().resize(3);
  for (size_t i = 0; i < 3; ++i) {
    sent.upsu_t()[i] = 1;
  }
  sent.upsb_t().resize(3);
  for (size_t i = 0; i < 3; ++i) {
    sent.upsb_t()[i] = 1;
  }
  sent.esu_t().resize(3);
  for (size_t i = 0; i < 3; ++i) {
    sent.esu_t()[i] = EnumType::VALUE1;
  }
  sent.esb_t().resize(3);
  for (size_t i = 0; i < 3; ++i) {
    sent.esb_t()[i] = EnumType::VALUE1;
  }
  sent.strsu_t().resize(3);
  for (size_t i = 0; i < 3; ++i) {
    sent.strsu_t()[i] = "HELLOWORLD";
  }
  sent.strsb_t().resize(3);
  for (size_t i = 0; i < 3; ++i) {
    sent.strsb_t()[i] = "HELLOWORLD";
  }
  sent.wstrsu_t().resize(3);
  for (size_t i = 0; i < 3; ++i) {
    sent.wstrsu_t()[i] = L"☺";
  }
  sent.wstrsb_t().resize(3);
  for (size_t i = 0; i < 3; ++i) {
    sent.wstrsb_t()[i] = L"☺";
  }
  sent.ssu_t().resize(3);
  for (size_t i = 0; i < 3; ++i) {
    sent.ssu_t()[i].str64_t("abcdefghijklmnopqrstuvwxyz");
  }
  sent.ssb_t().resize(3);
  for (size_t i = 0; i < 3; ++i) {
    sent.ssb_t()[i].str64_t("abcdefghijklmnopqrstuvwxyz");
  }
  sent.sasu_t().resize(3);
  for (size_t i = 0; i < 3; ++i) {
    for (size_t j = 0; j < 10; j++) {
      sent.sasu_t()[i][j] = 1;
    }
  }
  sent.sasb_t().resize(3);
  for (size_t i = 0; i < 3; ++i) {
    for (size_t j = 0; j < 10; j++) {
      sent.sasb_t()[i][j] = 1;
    }
  }
  sent.sssuu_t().resize(3);
  for (size_t i = 0; i < 3; ++i) {
    sent.sssuu_t()[i].resize(3);
    sent.sssuu_t()[i][0] = 1;
    sent.sssuu_t()[i][1] = 1;
    sent.sssuu_t()[i][2] = 1;
  }
  sent.sssub_t().resize(3);
  for (size_t i = 0; i < 3; ++i) {
    sent.sssub_t()[i].resize(3);
    sent.sssub_t()[i][0] = 1;
    sent.sssub_t()[i][1] = 1;
    sent.sssub_t()[i][2] = 1;
  }
  sent.sssbu_t().resize(3);
  for (size_t i = 0; i < 3; ++i) {
    sent.sssbu_t()[i].resize(3);
    sent.sssbu_t()[i][0] = 1;
    sent.sssbu_t()[i][1] = 1;
    sent.sssbu_t()[i][2] = 1;
  }
  sent.sssbb_t().resize(3);
  for (size_t i = 0; i < 3; ++i) {
    sent.sssbb_t()[i].resize(3);
    sent.sssbb_t()[i][0] = 1;
    sent.sssbb_t()[i][1] = 1;
    sent.sssbb_t()[i][2] = 1;
  }
  TryCon::TrimStruct2 expected;
  expected.str20_t("abcdefghijklmnopqrst");
  expected.wstr20_t(L"abcdefghijklmnopqrst");
  expected.psu_t().resize(3);
  expected.psb_t().resize(2);
  expected.upsu_t().resize(3);
  expected.upsb_t().resize(2);
  expected.esu_t().resize(3);
  expected.esb_t().resize(2);
  expected.strsu_t().resize(3);
  expected.strsb_t().resize(2);
  expected.wstrsu_t().resize(3);
  expected.wstrsb_t().resize(2);
  expected.ssu_t().resize(3);
  expected.ssu_t()[0].str20_t("abcdefghijklmnopqrst");
  expected.ssb_t().resize(2);
  expected.ssb_t()[0].str20_t("abcdefghijklmnopqrst");
  expected.sasu_t().resize(3);
  expected.sasb_t().resize(2);
  expected.sssuu_t().resize(3);
  expected.sssuu_t()[0].resize(3);
  expected.sssub_t().resize(2);
  expected.sssub_t()[0].resize(3);
  expected.sssbu_t().resize(3);
  expected.sssbu_t()[0].resize(2);
  expected.sssbb_t().resize(2);
  expected.sssbb_t()[0].resize(2);
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
    ASSERT_EQ(actual.str20_t(), expected.str20_t());
    ASSERT_EQ(actual.wstr20_t(), expected.wstr20_t());
    EXPECT_EQ(actual.psu_t().size(), expected.psu_t().size());
    EXPECT_EQ(actual.psb_t().size(), expected.psb_t().size());
    EXPECT_EQ(actual.upsu_t().size(), expected.upsu_t().size());
    EXPECT_EQ(actual.upsb_t().size(), expected.upsb_t().size());
    EXPECT_EQ(actual.esu_t().size(), expected.esu_t().size());
    EXPECT_EQ(actual.esb_t().size(), expected.esb_t().size());
    EXPECT_EQ(actual.strsu_t().size(), expected.strsu_t().size());
    EXPECT_EQ(actual.strsb_t().size(), expected.strsb_t().size());
    EXPECT_EQ(actual.wstrsu_t().size(), expected.wstrsu_t().size());
    EXPECT_EQ(actual.wstrsb_t().size(), expected.wstrsb_t().size());
    EXPECT_EQ(actual.ssu_t().size(), expected.ssu_t().size());
    ASSERT_EQ(actual.ssu_t()[0].str20_t(), expected.ssu_t()[0].str20_t());
    EXPECT_EQ(actual.ssb_t().size(), expected.ssb_t().size());
    ASSERT_EQ(actual.ssb_t()[0].str20_t(), expected.ssb_t()[0].str20_t());
    EXPECT_EQ(actual.sasu_t().size(), expected.sasu_t().size());
    EXPECT_EQ(actual.sasb_t().size(), expected.sasb_t().size());
    EXPECT_EQ(actual.sssuu_t().size(), expected.sssuu_t().size());
    EXPECT_EQ(actual.sssuu_t()[0].size(), expected.sssuu_t()[0].size());
    EXPECT_EQ(actual.sssub_t().size(), expected.sssub_t().size());
    EXPECT_EQ(actual.sssub_t()[0].size(), expected.sssub_t()[0].size());
    EXPECT_EQ(actual.sssbu_t().size(), expected.sssbu_t().size());
    EXPECT_EQ(actual.sssbu_t()[0].size(), expected.sssbu_t()[0].size());
    EXPECT_EQ(actual.sssbb_t().size(), expected.sssbb_t().size());
    EXPECT_EQ(actual.sssbb_t()[0].size(), expected.sssbb_t()[0].size());
  }
}

TEST(AnonSequence, Trim)
{
  {
    TryCon::AnonSeqStruct sent;
    sent.AnonEnumSeqBound().resize(3);
    for (size_t i = 0; i < 3; ++i) {
      sent.AnonEnumSeqBound()[i] = EnumType::VALUE1;
    }
    sent.AnonShortSeqBound().resize(3);
    for (size_t i = 0; i < 3; ++i) {
      sent.AnonShortSeqBound()[0] = 5;
    }
    sent.AnonUnsignedShortSeqBound().resize(3);
    for (size_t i = 0; i < 3; ++i) {
      sent.AnonUnsignedShortSeqBound()[i] = static_cast<uint16_t>(i);
    }
    sent.AnonStringSeqBound().resize(3);
    for (size_t i = 0; i < 3; ++i) {
      sent.AnonStringSeqBound()[i] = "abcdefghijklmnopqrstuvwxyz";
    }
    sent.AnonWideStringSeqBound().resize(3);
    for (size_t i = 0; i < 3; ++i) {
      sent.AnonWideStringSeqBound()[i] = L"abcdefghijklmnopqrstuvwxyz";
    }
    sent.AnonShortArraySeqBound().resize(3);
    for (size_t i = 0; i < 3; ++i) {
      for (size_t j = 0; j < 2; j++) {
        sent.AnonShortArraySeqBound()[i][j] = static_cast<uint16_t>(j);
      }
    }
    ShortSeqUnbound ssu;
    ssu.resize(3);
    for (size_t i = 0; i < 3; ++i) {
      ssu[i] = i;
    }
    ShortSeqBound2 ssb;
    ssu.resize(3);
    for (size_t i = 0; i < 3; ++i) {
      ssu[i] = i;
    }
    sent.AnonShortSeqUnboundUnbound().resize(3);
    for (size_t i = 0; i < 3; ++i) {
      sent.AnonShortSeqUnboundUnbound()[i] = ssu;
    }
    sent.AnonSeqShortSeqUnboundBound().resize(3);
    for (size_t i = 0; i < 3; ++i) {
      sent.AnonSeqShortSeqUnboundBound()[i] = ssu;
    }
    sent.AnonSeqShortSeqBoundUnbound().resize(3);
    for (size_t i = 0; i < 3; ++i) {
      sent.AnonSeqShortSeqBoundUnbound()[i] = ssb;
    }
    sent.AnonSeqShortSeqBoundBound().resize(3);
    for (size_t i = 0; i < 3; ++i) {
      sent.AnonSeqShortSeqBoundBound()[i] = ssb;
    }

    TryCon::AnonSeqStructTrim expected;
    expected.AnonEnumSeqBound().resize(2);
    for (size_t i = 0; i < 2; ++i) {
      expected.AnonEnumSeqBound()[i] = EnumType::VALUE1;
    }
    expected.AnonShortSeqBound().resize(2);
    for (size_t i = 0; i < 2; ++i) {
      expected.AnonShortSeqBound()[0] = 5;
    }
    expected.AnonUnsignedShortSeqBound().resize(2);
    for (size_t i = 0; i < 2; ++i) {
      expected.AnonUnsignedShortSeqBound()[i] = i;
    }
    expected.AnonStringSeqBound().resize(2);
    for (size_t i = 0; i < 2; ++i) {
      expected.AnonStringSeqBound()[i] = "abcdefghijklmnopqrst";
    }
    expected.AnonWideStringSeqBound().resize(2);
    for (size_t i = 0; i < 2; ++i) {
      expected.AnonWideStringSeqBound()[i] = L"abcdefghijklmnopqrst";
    }
    expected.AnonShortArraySeqBound().resize(2);
    for (size_t i = 0; i < 2; ++i) {
      for (size_t j = 0; j < 2; j++) {
        expected.AnonShortArraySeqBound()[i][j] = j;
      }
    }
    ShortSeqUnbound ssu_2;
    ssu.resize(3);
    for (size_t i = 0; i < 2; ++i) {
      ssu[i] = i;
    }
    ShortSeqBound ssb_2;
    ssu.resize(2);
    for (size_t i = 0; i < 2; ++i) {
      ssu[i] = i;
    }
    expected.AnonShortSeqUnboundUnbound().resize(3);
    for (size_t i = 0; i < 3; ++i) {
      expected.AnonShortSeqUnboundUnbound()[i] = ssu_2;
    }
    expected.AnonSeqShortSeqUnboundBound().resize(2);
    for (size_t i = 0; i < 2; ++i) {
      expected.AnonSeqShortSeqUnboundBound()[i] = ssu_2;
    }
    expected.AnonSeqShortSeqBoundUnbound().resize(3);
    for (size_t i = 0; i < 3; ++i) {
      expected.AnonSeqShortSeqBoundUnbound()[i] = ssb_2;
    }
    expected.AnonSeqShortSeqBoundBound().resize(2);
    for (size_t i = 0; i < 2; ++i) {
      expected.AnonSeqShortSeqBoundBound()[i] = ssb_2;
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
    EXPECT_EQ(actual.AnonEnumSeqBound().size(), expected.AnonEnumSeqBound().size());
    EXPECT_EQ(actual.AnonShortSeqBound().size(), expected.AnonShortSeqBound().size());
    EXPECT_EQ(actual.AnonUnsignedShortSeqBound().size(), expected.AnonUnsignedShortSeqBound().size());
    EXPECT_EQ(actual.AnonStringSeqBound().size(), expected.AnonStringSeqBound().size());
    EXPECT_EQ(actual.AnonWideStringSeqBound().size(), expected.AnonWideStringSeqBound().size());
    EXPECT_EQ(actual.AnonShortArraySeqBound().size(), expected.AnonShortArraySeqBound().size());
    EXPECT_EQ(actual.AnonShortSeqUnboundUnbound().size(), expected.AnonShortSeqUnboundUnbound().size());
    EXPECT_EQ(actual.AnonSeqShortSeqUnboundBound().size(), expected.AnonSeqShortSeqUnboundBound().size());
    EXPECT_EQ(actual.AnonSeqShortSeqBoundUnbound().size(), expected.AnonSeqShortSeqBoundUnbound().size());
    EXPECT_EQ(actual.AnonSeqShortSeqBoundBound().size(), expected.AnonSeqShortSeqBoundBound().size());
  }
}

TEST(AnonSequence, USE_DEFAULT)
{
  {
    TryCon::AnonSeqStruct sent;
    sent.AnonEnumSeqBound().resize(3);
    for (size_t i = 0; i < 3; ++i) {
      sent.AnonEnumSeqBound()[i] = EnumType::VALUE1;
    }
    sent.AnonShortSeqBound().resize(3);
    for (size_t i = 0; i < 3; ++i) {
      sent.AnonShortSeqBound()[0] = 5;
    }
    sent.AnonUnsignedShortSeqBound().resize(3);
    for (size_t i = 0; i < 3; ++i) {
      sent.AnonUnsignedShortSeqBound()[i] = i;
    }
    sent.AnonStringSeqBound().resize(3);
    for (size_t i = 0; i < 3; ++i) {
      sent.AnonStringSeqBound()[i] = "abcdefghijklmnopqrstuvwxyz";
    }
    sent.AnonWideStringSeqBound().resize(3);
    for (size_t i = 0; i < 3; ++i) {
      sent.AnonWideStringSeqBound()[i] = L"abcdefghijklmnopqrstuvwxyz";
    }
    sent.AnonShortArraySeqBound().resize(3);
    for (size_t i = 0; i < 3; ++i) {
      for (size_t j = 0; j < 2; j++) {
        sent.AnonShortArraySeqBound()[i][j] = j;
      }
    }
    ShortSeqUnbound ssu;
    ssu.resize(3);
    for (size_t i = 0; i < 3; ++i) {
      ssu[i] = i;
    }
    ShortSeqBound2 ssb;
    ssu.resize(3);
    for (size_t i = 0; i < 3; ++i) {
      ssu[i] = i;
    }
    sent.AnonShortSeqUnboundUnbound().resize(3);
    for (size_t i = 0; i < 3; ++i) {
      sent.AnonShortSeqUnboundUnbound()[i] = ssu;
    }
    sent.AnonSeqShortSeqUnboundBound().resize(3);
    for (size_t i = 0; i < 3; ++i) {
      sent.AnonSeqShortSeqUnboundBound()[i] = ssu;
    }
    sent.AnonSeqShortSeqBoundUnbound().resize(3);
    for (size_t i = 0; i < 3; ++i) {
      sent.AnonSeqShortSeqBoundUnbound()[i] = ssb;
    }
    sent.AnonSeqShortSeqBoundBound().resize(3);
    for (size_t i = 0; i < 3; ++i) {
      sent.AnonSeqShortSeqBoundBound()[i] = ssb;
    }

    TryCon::AnonSeqStructDefault expected;
    expected.AnonShortSeqUnboundUnbound().resize(3);
    expected.AnonSeqShortSeqBoundUnbound().resize(3);

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
    EXPECT_EQ(actual.AnonEnumSeqBound().size(), expected.AnonEnumSeqBound().size());
    EXPECT_EQ(actual.AnonShortSeqBound().size(), expected.AnonShortSeqBound().size());
    EXPECT_EQ(actual.AnonUnsignedShortSeqBound().size(), expected.AnonUnsignedShortSeqBound().size());
    EXPECT_EQ(actual.AnonStringSeqBound().size(), expected.AnonStringSeqBound().size());
    EXPECT_EQ(actual.AnonWideStringSeqBound().size(), expected.AnonWideStringSeqBound().size());
    EXPECT_EQ(actual.AnonShortArraySeqBound().size(), expected.AnonShortArraySeqBound().size());
    EXPECT_EQ(actual.AnonShortSeqUnboundUnbound().size(), expected.AnonShortSeqUnboundUnbound().size());
    EXPECT_EQ(actual.AnonSeqShortSeqUnboundBound().size(), expected.AnonSeqShortSeqUnboundBound().size());
    EXPECT_EQ(actual.AnonSeqShortSeqBoundUnbound().size(), expected.AnonSeqShortSeqBoundUnbound().size());
    EXPECT_EQ(actual.AnonSeqShortSeqBoundBound().size(), expected.AnonSeqShortSeqBoundBound().size());
  }
}

TEST(AnonArray, TRIM)
{
  {
    TryCon::AnonArrStruct sent;
    for (size_t i = 0; i < 3; ++i) {
      sent.AnonStringArr()[i] = "abcdefghijklmnopqrstuvwxyz";
    }
    for (size_t i = 0; i < 3; ++i) {
      sent.AnonWideStringArr()[i] = L"abcdefghijklmnopqrstuvwxyz";
    }
    for (size_t i = 0; i < 3; ++i) {
      for (size_t j = 0; j < 10; j++) {
        sent.AnonArrayArr()[i][j] = "abcdefghijklmnopqrstuvwxyz";
      }
    }
    ShortSeqBound2 temp_seq;
    temp_seq.resize(3);
    for (size_t i = 0; i < 3; ++i) {
      temp_seq[i] = i;
    }
    for (size_t i = 0; i < 3; ++i) {
      sent.AnonArrShortSeqBound()[i] = temp_seq;
    }

    TryCon::AnonArrStructTrim expected;
    for (size_t i = 0; i < 3; ++i) {
      expected.AnonStringArr()[i] = "abcdefghijklmnopqrst";
    }
    for (size_t i = 0; i < 3; ++i) {
      expected.AnonWideStringArr()[i] = L"abcdefghijklmnopqrst";
    }
    for (size_t i = 0; i < 3; ++i) {
      for (size_t j = 0; j < 10; j++) {
        expected.AnonArrayArr()[i][j] = "abcdefghijklmnopqrst";
      }
    }
    for (size_t i = 0; i < 3; ++i) {
      expected.AnonArrShortSeqBound()[i].resize(2);
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
    for (size_t i = 0; i < 3; ++i) {
      ASSERT_EQ(actual.AnonStringArr()[i], expected.AnonStringArr()[i]);
      ASSERT_EQ(actual.AnonWideStringArr()[i], expected.AnonWideStringArr()[i]);
      for (size_t j = 0; j < 10; j++) {
        ASSERT_EQ(actual.AnonArrayArr()[i][j], expected.AnonArrayArr()[i][j]);
      }
      EXPECT_EQ(actual.AnonArrShortSeqBound()[i].size(), expected.AnonArrShortSeqBound()[i].size());
    }
  }
}

TEST(AnonArray, USE_DEFAULT)
{
  {
    TryCon::AnonArrStruct sent;
    for (size_t i = 0; i < 3; ++i) {
      sent.AnonStringArr()[i] = "abcdefghijklmnopqrstuvwxyz";
    }
    for (size_t i = 0; i < 3; ++i) {
      sent.AnonWideStringArr()[i] = L"abcdefghijklmnopqrstuvwxyz";
    }
    for (size_t i = 0; i < 3; ++i) {
      for (size_t j = 0; j < 10; j++) {
        sent.AnonArrayArr()[i][j] = "abcdefghijklmnopqrstuvwxyz";
      }
    }
    ShortSeqBound2 temp_seq;
    temp_seq.resize(3);
    for (size_t i = 0; i < 3; ++i) {
      temp_seq[i] = i;
    }
    for (size_t i = 0; i < 3; ++i) {
      sent.AnonArrShortSeqBound()[i] = temp_seq;
    }

    TryCon::AnonArrStructUseDefault expected;
    for (size_t i = 0; i < 3; ++i) {
      expected.AnonStringArr()[i] = "";
    }
    for (size_t i = 0; i < 3; ++i) {
      expected.AnonWideStringArr()[i] = L"";
    }
    for (size_t i = 0; i < 3; ++i) {
      for (size_t j = 0; j < 10; j++) {
        expected.AnonArrayArr()[i][j] = "";
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
    for (size_t i = 0; i < 3; ++i) {
      ASSERT_EQ(actual.AnonStringArr()[i], expected.AnonStringArr()[i]);
      ASSERT_EQ(actual.AnonWideStringArr()[i], expected.AnonWideStringArr()[i]);
      for (size_t j = 0; j < 10; j++) {
        ASSERT_EQ(actual.AnonArrayArr()[i][j], expected.AnonArrayArr()[i][j]);
      }
      EXPECT_EQ(actual.AnonArrShortSeqBound()[i].size(), expected.AnonArrShortSeqBound()[i].size());
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
    temp_seq.resize(3);
    for (size_t i = 0; i < 3; ++i) {
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
    sent.e_d(EnumType2::BExtra);
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
    sent._d(EnumType2::BExtra);
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
    for (size_t i = 0; i < 10; ++i) {
      temp_arr[i] = "abcdefghijklmnopqrstuvwxyz";
    }
    sent.stra_ud(temp_arr);
    TryCon::DefaultUnion expected;
    expected._d(7);
    str64Array temp_arr2;
    for (size_t i = 0; i < 3; ++i) {
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
      ASSERT_EQ(expected.stra_ud()[0], actual.stra_ud()[0]);
    }
  }
  {
    TryCon::BaseUnion sent;
    sent._d(10);
    StringSeqBound2 temp_seq;
    temp_seq.resize(3);
    for (size_t i = 0; i < 3; ++i) {
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
      ASSERT_EQ(expected.strs_ud().size(), actual.strs_ud().size());
    }
  }
  {
    TryCon::BaseUnion sent;
    sent._d(13);
    sent.e_ud(EnumType2::BExtra);
    TryCon::DefaultUnion expected;
    expected._d(13);
    expected.e_ud(EnumType::VALUE1);
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
    sent._d(EnumType2::BExtra);
    sent.s4(5);
    TryCon::DefaultDiscrimUnion expected;
    expected._d(EnumType::VALUE1);
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
        ASSERT_EQ(expected.str_t(), actual.str_t());
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
      ASSERT_EQ(expected.wstr_t(), actual.wstr_t());
    }
  }
  {
    TryCon::BaseUnion sent;
    sent._d(8);
    str64Array temp_arr;
    for (size_t i = 0; i < 10; ++i) {
      temp_arr[i] = "abcdefghijklmnopqrstuvwxyz";
    }
    sent.stra_t(temp_arr);
    TryCon::TrimUnion expected;
    expected._d(8);
    str64Array temp_arr2;
    for (size_t i = 0; i < 3; ++i) {
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
      ASSERT_EQ(expected.stra_t()[0], actual.stra_t()[0]);
    }
  }
  {
    TryCon::BaseUnion sent;
    sent._d(11);
    StringSeqBound2 temp_seq;
    temp_seq.resize(3);
    for (size_t i = 0; i < 3; ++i) {
      temp_seq[i] = "abcdefghijklmnopqrstuvwxyz";
    }
    sent.strs_t(temp_seq);
    TryCon::TrimUnion expected;
    expected._d(11);
    StringSeqBound temp_seq2;
    temp_seq2.resize(2);
    for (size_t i = 0; i < 2; ++i) {
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
      ASSERT_EQ(expected.strs_t().size(), actual.strs_t().size());
    }
  }
}

int main(int argc, char* argv[])
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
