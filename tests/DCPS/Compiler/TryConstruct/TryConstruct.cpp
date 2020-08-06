/***
 * 
 * ORDER OF IDL MATTERS
 * DO NOT ADD MEMBERS ANYWHERE BUT THE END OF THE OBJECT
 * OR YOU WILL BREAK THE TEST
 ***/
#include "TryConstructTypeSupportImpl.h"

#include <dds/DCPS/XTypes/TypeObject.h>
#include <gtest/gtest.h>

using namespace OpenDDS::XTypes;
using namespace OpenDDS::DCPS;
const Encoding xcdr1(Encoding::KIND_XCDR1, ENDIAN_BIG);
TEST(TestMutableStruct, flags_match)
{
  //strings
  EXPECT_EQ(getMinimalTypeObject<TryCon_Mutable_Message_xtag>()
        .minimal.struct_type.member_seq[0].common.member_flags, TryConstructDiscardValue);
  EXPECT_EQ(getMinimalTypeObject<TryCon_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[1].common.member_flags, TryConstructUseDefaultValue);
  EXPECT_EQ(getMinimalTypeObject<TryCon_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[2].common.member_flags, TryConstructTrimValue);
  //wstrings
  EXPECT_EQ(getMinimalTypeObject<TryCon_Mutable_Message_xtag>()
        .minimal.struct_type.member_seq[3].common.member_flags, TryConstructDiscardValue);
  EXPECT_EQ(getMinimalTypeObject<TryCon_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[4].common.member_flags, TryConstructUseDefaultValue);
  EXPECT_EQ(getMinimalTypeObject<TryCon_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[5].common.member_flags, TryConstructTrimValue);
  //shortsequnbound
  EXPECT_EQ(getMinimalTypeObject<TryCon_Mutable_Message_xtag>()
        .minimal.struct_type.member_seq[6].common.member_flags, TryConstructDiscardValue);
  EXPECT_EQ(getMinimalTypeObject<TryCon_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[7].common.member_flags, TryConstructUseDefaultValue);
  EXPECT_EQ(getMinimalTypeObject<TryCon_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[8].common.member_flags, TryConstructTrimValue);
  //shortseqbound
  EXPECT_EQ(getMinimalTypeObject<TryCon_Mutable_Message_xtag>()
        .minimal.struct_type.member_seq[9].common.member_flags, TryConstructDiscardValue);
  EXPECT_EQ(getMinimalTypeObject<TryCon_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[10].common.member_flags, TryConstructUseDefaultValue);
  EXPECT_EQ(getMinimalTypeObject<TryCon_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[11].common.member_flags, TryConstructTrimValue);
  //unsignedshortsequnbound
  EXPECT_EQ(getMinimalTypeObject<TryCon_Mutable_Message_xtag>()
        .minimal.struct_type.member_seq[12].common.member_flags, TryConstructDiscardValue);
  EXPECT_EQ(getMinimalTypeObject<TryCon_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[13].common.member_flags, TryConstructUseDefaultValue);
  EXPECT_EQ(getMinimalTypeObject<TryCon_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[14].common.member_flags, TryConstructTrimValue);
  //unsignedshortseqbound
  EXPECT_EQ(getMinimalTypeObject<TryCon_Mutable_Message_xtag>()
        .minimal.struct_type.member_seq[15].common.member_flags, TryConstructDiscardValue);
  EXPECT_EQ(getMinimalTypeObject<TryCon_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[16].common.member_flags, TryConstructUseDefaultValue);
  EXPECT_EQ(getMinimalTypeObject<TryCon_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[17].common.member_flags, TryConstructTrimValue);
  //enumsequnbound
  EXPECT_EQ(getMinimalTypeObject<TryCon_Mutable_Message_xtag>()
        .minimal.struct_type.member_seq[18].common.member_flags, TryConstructDiscardValue);
  EXPECT_EQ(getMinimalTypeObject<TryCon_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[19].common.member_flags, TryConstructUseDefaultValue);
  EXPECT_EQ(getMinimalTypeObject<TryCon_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[20].common.member_flags, TryConstructTrimValue);
  //enumseqbound
  EXPECT_EQ(getMinimalTypeObject<TryCon_Mutable_Message_xtag>()
        .minimal.struct_type.member_seq[21].common.member_flags, TryConstructDiscardValue);
  EXPECT_EQ(getMinimalTypeObject<TryCon_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[22].common.member_flags, TryConstructUseDefaultValue);
  EXPECT_EQ(getMinimalTypeObject<TryCon_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[23].common.member_flags, TryConstructTrimValue);
  //stringsequnbound
  EXPECT_EQ(getMinimalTypeObject<TryCon_Mutable_Message_xtag>()
        .minimal.struct_type.member_seq[24].common.member_flags, TryConstructDiscardValue);
  EXPECT_EQ(getMinimalTypeObject<TryCon_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[25].common.member_flags, TryConstructUseDefaultValue);
  EXPECT_EQ(getMinimalTypeObject<TryCon_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[26].common.member_flags, TryConstructTrimValue);
  //stringseqbound
  EXPECT_EQ(getMinimalTypeObject<TryCon_Mutable_Message_xtag>()
        .minimal.struct_type.member_seq[27].common.member_flags, TryConstructDiscardValue);
  EXPECT_EQ(getMinimalTypeObject<TryCon_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[28].common.member_flags, TryConstructUseDefaultValue);
  EXPECT_EQ(getMinimalTypeObject<TryCon_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[29].common.member_flags, TryConstructTrimValue);
  //widestringsequnbound
  EXPECT_EQ(getMinimalTypeObject<TryCon_Mutable_Message_xtag>()
        .minimal.struct_type.member_seq[30].common.member_flags, TryConstructDiscardValue);
  EXPECT_EQ(getMinimalTypeObject<TryCon_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[31].common.member_flags, TryConstructUseDefaultValue);
  EXPECT_EQ(getMinimalTypeObject<TryCon_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[32].common.member_flags, TryConstructTrimValue);
  //widestringseqbound
  EXPECT_EQ(getMinimalTypeObject<TryCon_Mutable_Message_xtag>()
        .minimal.struct_type.member_seq[33].common.member_flags, TryConstructDiscardValue);
  EXPECT_EQ(getMinimalTypeObject<TryCon_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[34].common.member_flags, TryConstructUseDefaultValue);
  EXPECT_EQ(getMinimalTypeObject<TryCon_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[35].common.member_flags, TryConstructTrimValue);
  //structsequnbound
  EXPECT_EQ(getMinimalTypeObject<TryCon_Mutable_Message_xtag>()
        .minimal.struct_type.member_seq[36].common.member_flags, TryConstructDiscardValue);
  EXPECT_EQ(getMinimalTypeObject<TryCon_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[37].common.member_flags, TryConstructUseDefaultValue);
  EXPECT_EQ(getMinimalTypeObject<TryCon_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[38].common.member_flags, TryConstructTrimValue);
  //structseqbound
  EXPECT_EQ(getMinimalTypeObject<TryCon_Mutable_Message_xtag>()
        .minimal.struct_type.member_seq[39].common.member_flags, TryConstructDiscardValue);
  EXPECT_EQ(getMinimalTypeObject<TryCon_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[40].common.member_flags, TryConstructUseDefaultValue);
  EXPECT_EQ(getMinimalTypeObject<TryCon_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[41].common.member_flags, TryConstructTrimValue);
  //shortarraysequnbound
  EXPECT_EQ(getMinimalTypeObject<TryCon_Mutable_Message_xtag>()
        .minimal.struct_type.member_seq[42].common.member_flags, TryConstructDiscardValue);
  EXPECT_EQ(getMinimalTypeObject<TryCon_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[43].common.member_flags, TryConstructUseDefaultValue);
  EXPECT_EQ(getMinimalTypeObject<TryCon_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[44].common.member_flags, TryConstructTrimValue);
  //shortarrayseqbound
  EXPECT_EQ(getMinimalTypeObject<TryCon_Mutable_Message_xtag>()
        .minimal.struct_type.member_seq[45].common.member_flags, TryConstructDiscardValue);
  EXPECT_EQ(getMinimalTypeObject<TryCon_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[46].common.member_flags, TryConstructUseDefaultValue);
  EXPECT_EQ(getMinimalTypeObject<TryCon_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[47].common.member_flags, TryConstructTrimValue);
  //seqshortsequnboundunbound
  EXPECT_EQ(getMinimalTypeObject<TryCon_Mutable_Message_xtag>()
        .minimal.struct_type.member_seq[48].common.member_flags, TryConstructDiscardValue);
  EXPECT_EQ(getMinimalTypeObject<TryCon_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[49].common.member_flags, TryConstructUseDefaultValue);
  EXPECT_EQ(getMinimalTypeObject<TryCon_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[50].common.member_flags, TryConstructTrimValue);
  //seqshortsequnboundbound
  EXPECT_EQ(getMinimalTypeObject<TryCon_Mutable_Message_xtag>()
        .minimal.struct_type.member_seq[51].common.member_flags, TryConstructDiscardValue);
  EXPECT_EQ(getMinimalTypeObject<TryCon_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[52].common.member_flags, TryConstructUseDefaultValue);
  EXPECT_EQ(getMinimalTypeObject<TryCon_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[53].common.member_flags, TryConstructTrimValue);
  //seqshortseqboundunbound
  EXPECT_EQ(getMinimalTypeObject<TryCon_Mutable_Message_xtag>()
        .minimal.struct_type.member_seq[54].common.member_flags, TryConstructDiscardValue);
  EXPECT_EQ(getMinimalTypeObject<TryCon_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[55].common.member_flags, TryConstructUseDefaultValue);
  EXPECT_EQ(getMinimalTypeObject<TryCon_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[56].common.member_flags, TryConstructTrimValue);
  //seqshortseqboundbound
  EXPECT_EQ(getMinimalTypeObject<TryCon_Mutable_Message_xtag>()
        .minimal.struct_type.member_seq[57].common.member_flags, TryConstructDiscardValue);
  EXPECT_EQ(getMinimalTypeObject<TryCon_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[58].common.member_flags, TryConstructUseDefaultValue);
  EXPECT_EQ(getMinimalTypeObject<TryCon_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[59].common.member_flags, TryConstructTrimValue);
  //unionsequnbound
  EXPECT_EQ(getMinimalTypeObject<TryCon_Mutable_Message_xtag>()
        .minimal.struct_type.member_seq[60].common.member_flags, TryConstructDiscardValue);
  EXPECT_EQ(getMinimalTypeObject<TryCon_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[61].common.member_flags, TryConstructUseDefaultValue);
  EXPECT_EQ(getMinimalTypeObject<TryCon_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[62].common.member_flags, TryConstructTrimValue);
  //unionseqbound
  EXPECT_EQ(getMinimalTypeObject<TryCon_Mutable_Message_xtag>()
        .minimal.struct_type.member_seq[63].common.member_flags, TryConstructDiscardValue);
  EXPECT_EQ(getMinimalTypeObject<TryCon_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[64].common.member_flags, TryConstructUseDefaultValue);
  EXPECT_EQ(getMinimalTypeObject<TryCon_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[65].common.member_flags, TryConstructTrimValue);
  //nestedstruct
  EXPECT_EQ(getMinimalTypeObject<TryCon_Mutable_Message_xtag>()
        .minimal.struct_type.member_seq[66].common.member_flags, TryConstructDiscardValue);
  EXPECT_EQ(getMinimalTypeObject<TryCon_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[67].common.member_flags, TryConstructUseDefaultValue);
  EXPECT_EQ(getMinimalTypeObject<TryCon_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[68].common.member_flags, TryConstructTrimValue);
  //nestedunion
  EXPECT_EQ(getMinimalTypeObject<TryCon_Mutable_Message_xtag>()
        .minimal.struct_type.member_seq[69].common.member_flags, TryConstructDiscardValue);
  EXPECT_EQ(getMinimalTypeObject<TryCon_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[70].common.member_flags, TryConstructUseDefaultValue);
  EXPECT_EQ(getMinimalTypeObject<TryCon_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[71].common.member_flags, TryConstructTrimValue);
  //shortarray
  EXPECT_EQ(getMinimalTypeObject<TryCon_Mutable_Message_xtag>()
        .minimal.struct_type.member_seq[72].common.member_flags, TryConstructDiscardValue);
  EXPECT_EQ(getMinimalTypeObject<TryCon_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[73].common.member_flags, TryConstructUseDefaultValue);
  EXPECT_EQ(getMinimalTypeObject<TryCon_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[74].common.member_flags, TryConstructTrimValue);
  //enumtype
  EXPECT_EQ(getMinimalTypeObject<TryCon_Mutable_Message_xtag>()
        .minimal.struct_type.member_seq[75].common.member_flags, TryConstructDiscardValue);
  EXPECT_EQ(getMinimalTypeObject<TryCon_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[76].common.member_flags, TryConstructUseDefaultValue);
  EXPECT_EQ(getMinimalTypeObject<TryCon_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[77].common.member_flags, TryConstructTrimValue);
  //octet (byte)
  EXPECT_EQ(getMinimalTypeObject<TryCon_Mutable_Message_xtag>()
        .minimal.struct_type.member_seq[78].common.member_flags, TryConstructDiscardValue);
  EXPECT_EQ(getMinimalTypeObject<TryCon_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[79].common.member_flags, TryConstructUseDefaultValue);
  EXPECT_EQ(getMinimalTypeObject<TryCon_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[80].common.member_flags, TryConstructTrimValue);
  //boolean
  EXPECT_EQ(getMinimalTypeObject<TryCon_Mutable_Message_xtag>()
        .minimal.struct_type.member_seq[81].common.member_flags, TryConstructDiscardValue);
  EXPECT_EQ(getMinimalTypeObject<TryCon_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[82].common.member_flags, TryConstructUseDefaultValue);
  EXPECT_EQ(getMinimalTypeObject<TryCon_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[83].common.member_flags, TryConstructTrimValue);
  //short
  EXPECT_EQ(getMinimalTypeObject<TryCon_Mutable_Message_xtag>()
        .minimal.struct_type.member_seq[84].common.member_flags, TryConstructDiscardValue);
  EXPECT_EQ(getMinimalTypeObject<TryCon_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[85].common.member_flags, TryConstructUseDefaultValue);
  EXPECT_EQ(getMinimalTypeObject<TryCon_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[86].common.member_flags, TryConstructTrimValue);
  //unsigned short
  EXPECT_EQ(getMinimalTypeObject<TryCon_Mutable_Message_xtag>()
        .minimal.struct_type.member_seq[87].common.member_flags, TryConstructDiscardValue);
  EXPECT_EQ(getMinimalTypeObject<TryCon_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[88].common.member_flags, TryConstructUseDefaultValue);
  EXPECT_EQ(getMinimalTypeObject<TryCon_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[89].common.member_flags, TryConstructTrimValue);
}
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
    Message_Block_Ptr data(new ACE_Message_Block(serialized_size(xcdr1, sent)));

    // Serialize and Compare CDR
    {
      Serializer serializer(data.get(), xcdr1);
      EXPECT_TRUE(serializer << sent);
    }

    // Deserialize and Compare C++ Values
    {
      Serializer serializer(data.get(), xcdr1);
      EXPECT_TRUE(serializer >> actual);
    }
    EXPECT_FALSE(strcmp(actual.str20_d.in(), expected.str20_d.in()));
    EXPECT_FALSE(strcmp(actual.str20_ud.in(), expected.str20_ud.in()));
    EXPECT_FALSE(strcmp(actual.str20_t.in(), expected.str20_t.in()));
  }
  {
    sent.str64_d = "abcdefghijklmnopqrstuvwxyz";
    Message_Block_Ptr data(new ACE_Message_Block(serialized_size(xcdr1, sent)));

    // Serialize and Compare CDR
    {
      Serializer serializer(data.get(), xcdr1);
      EXPECT_TRUE(serializer << sent);
    }

    // Deserialize and Compare C++ Values
    {
      Serializer serializer(data.get(), xcdr1);
      EXPECT_FALSE(serializer >> actual);
    }
  }
}
TEST(TestTryCon, DISCARD) 
{
  {
    TryCon::DiscardStructString1 sent;
    sent.str64_d = "abcdefghijklmnopqrstuvwxyz";
    TryCon::DiscardStructString2 actual;

    {
      Message_Block_Ptr data(new ACE_Message_Block(serialized_size(xcdr1, sent)));

      // Serialize and Compare CDR
      {
        Serializer serializer(data.get(), xcdr1);
        EXPECT_TRUE(serializer << sent);
      }

      // Deserialize and Compare C++ Values
      {
        Serializer serializer(data.get(), xcdr1);
        EXPECT_FALSE(serializer >> actual);
      }
    }
  }
  {
    TryCon::DiscardStructWString1 sent;
    sent.wstr64_d = L"abcdefghijklmnopqrstuvwxyz";
    TryCon::DiscardStructWString2 actual;

    {
      Message_Block_Ptr data(new ACE_Message_Block(serialized_size(xcdr1, sent)));

      // Serialize and Compare CDR
      {
        Serializer serializer(data.get(), xcdr1);
        EXPECT_TRUE(serializer << sent);
      }

      // Deserialize and Compare C++ Values
      {
        Serializer serializer(data.get(), xcdr1);
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
      Message_Block_Ptr data(new ACE_Message_Block(serialized_size(xcdr1, sent)));

      // Serialize and Compare CDR
      {
        Serializer serializer(data.get(), xcdr1);
        EXPECT_TRUE(serializer << sent);
      }

      // Deserialize and Compare C++ Values
      {
        Serializer serializer(data.get(), xcdr1);
        EXPECT_FALSE(serializer >> actual);
      }
    }
  }
  {
    TryCon::DiscardStructArray1 sent;
    sent.sa[0] = "abcdefghijklmnopqrstuvwxyz";
    TryCon::DiscardStructArray2 actual;

    {
      Message_Block_Ptr data(new ACE_Message_Block(serialized_size(xcdr1, sent)));

      // Serialize and Compare CDR
      {
        Serializer serializer(data.get(), xcdr1);
        EXPECT_TRUE(serializer << sent);
      }

      // Deserialize and Compare C++ Values
      {
        Serializer serializer(data.get(), xcdr1);
        EXPECT_FALSE(serializer >> actual);
      }
    }
  }
}
//because nested structs can force default behavior on all members,
//this is the easiest place to test use_default for every type
TEST(TestTryCon, USE_DEFAULT)
{
  TryCon::NestedStructTest1 sent;
  sent.ns.str64_d = "abcdefghijklmnopqrstuvwxyz";
  sent.ns.str64_ud = "abcdefghijklmnopqrstuvwxyz";
  sent.ns.str64_t = "abcdefghijklmnopqrstuvwxyz";
  sent.ns.wstr64_ud = L"☺";
  sent.ns.psu_ud.length(1);
  sent.ns.psb_ud.length(1);
  sent.ns.upsu_ud.length(1);
  sent.ns.upsb_ud.length(1);
  sent.ns.esu_ud.length(1);
  sent.ns.esb_ud.length(1);
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
  sent.ns.usu_ud.length(1);
  sent.ns.usb_ud.length(1);
  sent.ns.ns_ud.str64_d = "HELLO";
  sent.ns.ns_ud.str64_ud = "WORLD";
  sent.ns.ns_ud.str64_t = "GOODBYE";  
  sent.ns.nu_ud._d(VALUE1);
  sent.ns.nu_ud.u_b(false);
  for (ACE_INT16 i = 0; i < 10; i++) sent.ns.sa_mud[i] = 0;
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
  expected.ns.psu_ud.length(0);
  expected.ns.psb_ud.length(0);
  expected.ns.upsu_ud.length(0);
  expected.ns.upsb_ud.length(0);
  expected.ns.esu_ud.length(0);
  expected.ns.esb_ud.length(0);
  expected.ns.strsu_ud.length(0);
  expected.ns.strsb_ud.length(0);
  expected.ns.wstrsu_ud.length(0);
  expected.ns.wstrsb_ud.length(0);
  expected.ns.ssu_ud.length(0);
  expected.ns.ssb_ud.length(0);
  expected.ns.sasu_ud.length(0);
  expected.ns.sasb_ud.length(0);
  expected.ns.sssuu_ud.length(0);
  expected.ns.sssub_ud.length(0);
  expected.ns.sssbu_ud.length(0);
  expected.ns.sssbb_ud.length(0);
  expected.ns.usu_ud.length(0);
  expected.ns.usb_ud.length(0);
  expected.ns.ns_ud.str64_d = "";
  expected.ns.ns_ud.str64_ud = "";
  expected.ns.ns_ud.str64_t = "";  
  expected.ns.nu_ud._d(VALUE1);
  expected.ns.nu_ud.u_b(false);
  for (ACE_INT16 i = 0; i < 10; i++) expected.ns.sa_mud[i] = 0;
  expected.ns.e_ud = VALUE1;
  expected.ns.by_ud = 0x00;
  expected.ns.bo_ud = false;
  expected.ns.s_ud = 0;
  expected.ns.us_ud = 0;
  TryCon::NestedStructTest2 actual;

  {
    Message_Block_Ptr data(new ACE_Message_Block(serialized_size(xcdr1, sent)));

    // Serialize and Compare CDR
    {
      Serializer serializer(data.get(), xcdr1);
      EXPECT_TRUE(serializer << sent);
    }

    // Deserialize and Compare C++ Values
    {
      Serializer serializer(data.get(), xcdr1);
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
    EXPECT_EQ(actual.ns.usu_ud.length(), expected.ns.usu_ud.length());
    EXPECT_EQ(actual.ns.usb_ud.length(), expected.ns.usb_ud.length());
    EXPECT_FALSE(strcmp(actual.ns.ns_ud.str64_d.in(), expected.ns.ns_ud.str64_d.in()));
    EXPECT_FALSE(strcmp(actual.ns.ns_ud.str64_ud.in(), expected.ns.ns_ud.str64_ud.in()));
    EXPECT_FALSE(strcmp(actual.ns.ns_ud.str64_t.in(), expected.ns.ns_ud.str64_t.in()));  
    EXPECT_EQ(actual.ns.nu_ud._d(), expected.ns.nu_ud._d());
    EXPECT_EQ(actual.ns.nu_ud.u_b(), expected.ns.nu_ud.u_b());
    for (ACE_INT16 i = 0; i < 10; i++) {
      EXPECT_EQ(actual.ns.sa_mud[i], expected.ns.sa_mud[i]);
    }
    EXPECT_EQ(actual.ns.e_ud, expected.ns.e_ud);
    EXPECT_EQ(actual.ns.by_ud, expected.ns.by_ud);
    EXPECT_EQ(actual.ns.bo_ud, expected.ns.bo_ud);
    EXPECT_EQ(actual.ns.s_ud, expected.ns.s_ud);
    EXPECT_EQ(actual.ns.us_ud, expected.ns.us_ud);
  }
}
//test all the trims in the same place
//because nested structs can force default behavior on all members,
//this is the easiest place to test use_default for every type
TEST(TestTryCon, TRIM)
{
  TryCon::TrimStruct1 sent;
  sent.str64_t = "abcdefghijklmnopqrstuvwxyz";
  sent.wstr64_t = L"abcdefghijklmnopqrstuvwxyz";
  sent.psu_t.length(3);
  for (ACE_INT16 i = 0; i < 3; i++) {
    sent.psu_t[i] = 1;
  }
  sent.psb_t.length(3);
  for (ACE_INT16 i = 0; i < 3; i++) {
    sent.psb_t[i] = 1;
  }
  sent.upsu_t.length(3);
  for (ACE_INT16 i = 0; i < 3; i++) {
    sent.upsu_t[i] = 1;
  }
  sent.upsb_t.length(3);
  for (ACE_INT16 i = 0; i < 3; i++) {
    sent.upsb_t[i] = 1;
  }
  sent.esu_t.length(3);
  for (ACE_INT16 i = 0; i < 3; i++) {
    sent.esu_t[i] = VALUE1;
  }
  sent.esb_t.length(3);
  for (ACE_INT16 i = 0; i < 3; i++) {
    sent.esb_t[i] = VALUE1;
  }
  sent.strsu_t.length(3);
  for (ACE_INT16 i = 0; i < 3; i++) {
    sent.strsu_t[i] = "HELLOWORLD";
  }
  sent.strsb_t.length(3);
  for (ACE_INT16 i = 0; i < 3; i++) {
    sent.strsb_t[i] = "HELLOWORLD";
  }
  sent.wstrsu_t.length(3);
  for (ACE_INT16 i = 0; i < 3; i++) {
    sent.wstrsu_t[i] = L"☺";
  }
  sent.wstrsb_t.length(3);
  for (ACE_INT16 i = 0; i < 3; i++) {
    sent.wstrsb_t[i] = L"☺";
  }
  sent.ssu_t.length(3);
  for (ACE_INT16 i = 0; i < 3; i++) {
    sent.ssu_t[i].str64_t = "abcdefghijklmnopqrstuvwxyz";
  }
  sent.ssb_t.length(3);
  for (ACE_INT16 i = 0; i < 3; i++) {
    sent.ssb_t[i].str64_t = "abcdefghijklmnopqrstuvwxyz";
  }
  sent.sasu_t.length(3);
  for (ACE_INT16 i = 0; i < 3; i++) {
    for (ACE_INT16 j = 0; j < 10; j++) {
      sent.sasu_t[i][j] = 1;
    }
  }
  sent.sasb_t.length(3);
  for (ACE_INT16 i = 0; i < 3; i++) {
    for (ACE_INT16 j = 0; j < 10; j++) {
      sent.sasb_t[i][j] = 1;
    }
  }
  sent.sssuu_t.length(3);
  for (ACE_INT16 i = 0; i < 3; i++) {
    sent.sssuu_t[i].length(3);
    sent.sssuu_t[i][0] = 1;
    sent.sssuu_t[i][1] = 1;
    sent.sssuu_t[i][2] = 1;
  }
  sent.sssub_t.length(3);
  for (ACE_INT16 i = 0; i < 3; i++) {
    sent.sssub_t[i].length(3);
    sent.sssub_t[i][0] = 1;
    sent.sssub_t[i][1] = 1;
    sent.sssub_t[i][2] = 1;
  }
  sent.sssbu_t.length(3);
  for (ACE_INT16 i = 0; i < 3; i++) {
    sent.sssbu_t[i].length(3);
    sent.sssbu_t[i][0] = 1;
    sent.sssbu_t[i][1] = 1;
    sent.sssbu_t[i][2] = 1;
  }
  sent.sssbb_t.length(3);
  for (ACE_INT16 i = 0; i < 3; i++) {
    sent.sssbb_t[i].length(3);
    sent.sssbb_t[i][0] = 1;
    sent.sssbb_t[i][1] = 1;
    sent.sssbb_t[i][2] = 1;
  }
  sent.usu_t.length(3);
  for (ACE_INT16 i = 0; i < 3; i++) {
    sent.usu_t[i]._d(VALUE1);
    sent.usu_t[i].u_b(false);
  }
  sent.usb_t.length(3);
  for (ACE_INT16 i = 0; i < 3; i++) {
    sent.usb_t[i]._d(VALUE1);
    sent.usb_t[i].u_b(false);
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
  expected.usu_t.length(3);
  expected.usb_t.length(2);
  TryCon::TrimStruct2 actual;

  {
    Message_Block_Ptr data(new ACE_Message_Block(serialized_size(xcdr1, sent)));

    // Serialize and Compare CDR
    {
      Serializer serializer(data.get(), xcdr1);
      EXPECT_TRUE(serializer << sent);
    }

    // Deserialize and Compare C++ Values
    {
      Serializer serializer(data.get(), xcdr1);
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
    EXPECT_EQ(actual.usu_t.length(), expected.usu_t.length());
    EXPECT_EQ(actual.usb_t.length(), expected.usb_t.length());
  }
}
int main(int argc, char* argv[])
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
