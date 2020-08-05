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

TEST(TestMutableStruct, flags_match)
{
  //strings
  EXPECT_EQ(getMinimalTypeObject<Messenger_Mutable_Message_xtag>()
        .minimal.struct_type.member_seq[0].common.member_flags, TryConstructDiscardValue);
  EXPECT_EQ(getMinimalTypeObject<Messenger_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[1].common.member_flags, TryConstructUseDefaultValue);
  EXPECT_EQ(getMinimalTypeObject<Messenger_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[2].common.member_flags, TryConstructTrimValue);
  //wstrings
  EXPECT_EQ(getMinimalTypeObject<Messenger_Mutable_Message_xtag>()
        .minimal.struct_type.member_seq[3].common.member_flags, TryConstructDiscardValue);
  EXPECT_EQ(getMinimalTypeObject<Messenger_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[4].common.member_flags, TryConstructUseDefaultValue);
  EXPECT_EQ(getMinimalTypeObject<Messenger_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[5].common.member_flags, TryConstructTrimValue);
  //shortsequnbound
  EXPECT_EQ(getMinimalTypeObject<Messenger_Mutable_Message_xtag>()
        .minimal.struct_type.member_seq[6].common.member_flags, TryConstructDiscardValue);
  EXPECT_EQ(getMinimalTypeObject<Messenger_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[7].common.member_flags, TryConstructUseDefaultValue);
  EXPECT_EQ(getMinimalTypeObject<Messenger_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[8].common.member_flags, TryConstructTrimValue);
  //shortseqbound
  EXPECT_EQ(getMinimalTypeObject<Messenger_Mutable_Message_xtag>()
        .minimal.struct_type.member_seq[9].common.member_flags, TryConstructDiscardValue);
  EXPECT_EQ(getMinimalTypeObject<Messenger_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[10].common.member_flags, TryConstructUseDefaultValue);
  EXPECT_EQ(getMinimalTypeObject<Messenger_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[11].common.member_flags, TryConstructTrimValue);
  //unsignedshortsequnbound
  EXPECT_EQ(getMinimalTypeObject<Messenger_Mutable_Message_xtag>()
        .minimal.struct_type.member_seq[12].common.member_flags, TryConstructDiscardValue);
  EXPECT_EQ(getMinimalTypeObject<Messenger_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[13].common.member_flags, TryConstructUseDefaultValue);
  EXPECT_EQ(getMinimalTypeObject<Messenger_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[14].common.member_flags, TryConstructTrimValue);
  //unsignedshortseqbound
  EXPECT_EQ(getMinimalTypeObject<Messenger_Mutable_Message_xtag>()
        .minimal.struct_type.member_seq[15].common.member_flags, TryConstructDiscardValue);
  EXPECT_EQ(getMinimalTypeObject<Messenger_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[16].common.member_flags, TryConstructUseDefaultValue);
  EXPECT_EQ(getMinimalTypeObject<Messenger_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[17].common.member_flags, TryConstructTrimValue);
  //enumsequnbound
  EXPECT_EQ(getMinimalTypeObject<Messenger_Mutable_Message_xtag>()
        .minimal.struct_type.member_seq[18].common.member_flags, TryConstructDiscardValue);
  EXPECT_EQ(getMinimalTypeObject<Messenger_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[19].common.member_flags, TryConstructUseDefaultValue);
  EXPECT_EQ(getMinimalTypeObject<Messenger_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[20].common.member_flags, TryConstructTrimValue);
  //enumseqbound
  EXPECT_EQ(getMinimalTypeObject<Messenger_Mutable_Message_xtag>()
        .minimal.struct_type.member_seq[21].common.member_flags, TryConstructDiscardValue);
  EXPECT_EQ(getMinimalTypeObject<Messenger_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[22].common.member_flags, TryConstructUseDefaultValue);
  EXPECT_EQ(getMinimalTypeObject<Messenger_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[23].common.member_flags, TryConstructTrimValue);
  //stringsequnbound
  EXPECT_EQ(getMinimalTypeObject<Messenger_Mutable_Message_xtag>()
        .minimal.struct_type.member_seq[24].common.member_flags, TryConstructDiscardValue);
  EXPECT_EQ(getMinimalTypeObject<Messenger_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[25].common.member_flags, TryConstructUseDefaultValue);
  EXPECT_EQ(getMinimalTypeObject<Messenger_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[26].common.member_flags, TryConstructTrimValue);
  //stringseqbound
  EXPECT_EQ(getMinimalTypeObject<Messenger_Mutable_Message_xtag>()
        .minimal.struct_type.member_seq[27].common.member_flags, TryConstructDiscardValue);
  EXPECT_EQ(getMinimalTypeObject<Messenger_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[28].common.member_flags, TryConstructUseDefaultValue);
  EXPECT_EQ(getMinimalTypeObject<Messenger_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[29].common.member_flags, TryConstructTrimValue);
  //widestringsequnbound
  EXPECT_EQ(getMinimalTypeObject<Messenger_Mutable_Message_xtag>()
        .minimal.struct_type.member_seq[30].common.member_flags, TryConstructDiscardValue);
  EXPECT_EQ(getMinimalTypeObject<Messenger_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[31].common.member_flags, TryConstructUseDefaultValue);
  EXPECT_EQ(getMinimalTypeObject<Messenger_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[32].common.member_flags, TryConstructTrimValue);
  //widestringseqbound
  EXPECT_EQ(getMinimalTypeObject<Messenger_Mutable_Message_xtag>()
        .minimal.struct_type.member_seq[33].common.member_flags, TryConstructDiscardValue);
  EXPECT_EQ(getMinimalTypeObject<Messenger_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[34].common.member_flags, TryConstructUseDefaultValue);
  EXPECT_EQ(getMinimalTypeObject<Messenger_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[35].common.member_flags, TryConstructTrimValue);
  //structsequnbound
  EXPECT_EQ(getMinimalTypeObject<Messenger_Mutable_Message_xtag>()
        .minimal.struct_type.member_seq[36].common.member_flags, TryConstructDiscardValue);
  EXPECT_EQ(getMinimalTypeObject<Messenger_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[37].common.member_flags, TryConstructUseDefaultValue);
  EXPECT_EQ(getMinimalTypeObject<Messenger_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[38].common.member_flags, TryConstructTrimValue);
  //structseqbound
  EXPECT_EQ(getMinimalTypeObject<Messenger_Mutable_Message_xtag>()
        .minimal.struct_type.member_seq[39].common.member_flags, TryConstructDiscardValue);
  EXPECT_EQ(getMinimalTypeObject<Messenger_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[40].common.member_flags, TryConstructUseDefaultValue);
  EXPECT_EQ(getMinimalTypeObject<Messenger_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[41].common.member_flags, TryConstructTrimValue);
  //shortarraysequnbound
  EXPECT_EQ(getMinimalTypeObject<Messenger_Mutable_Message_xtag>()
        .minimal.struct_type.member_seq[42].common.member_flags, TryConstructDiscardValue);
  EXPECT_EQ(getMinimalTypeObject<Messenger_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[43].common.member_flags, TryConstructUseDefaultValue);
  EXPECT_EQ(getMinimalTypeObject<Messenger_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[44].common.member_flags, TryConstructTrimValue);
  //shortarrayseqbound
  EXPECT_EQ(getMinimalTypeObject<Messenger_Mutable_Message_xtag>()
        .minimal.struct_type.member_seq[45].common.member_flags, TryConstructDiscardValue);
  EXPECT_EQ(getMinimalTypeObject<Messenger_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[46].common.member_flags, TryConstructUseDefaultValue);
  EXPECT_EQ(getMinimalTypeObject<Messenger_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[47].common.member_flags, TryConstructTrimValue);
  //seqshortsequnboundunbound
  EXPECT_EQ(getMinimalTypeObject<Messenger_Mutable_Message_xtag>()
        .minimal.struct_type.member_seq[48].common.member_flags, TryConstructDiscardValue);
  EXPECT_EQ(getMinimalTypeObject<Messenger_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[49].common.member_flags, TryConstructUseDefaultValue);
  EXPECT_EQ(getMinimalTypeObject<Messenger_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[50].common.member_flags, TryConstructTrimValue);
  //seqshortsequnboundbound
  EXPECT_EQ(getMinimalTypeObject<Messenger_Mutable_Message_xtag>()
        .minimal.struct_type.member_seq[51].common.member_flags, TryConstructDiscardValue);
  EXPECT_EQ(getMinimalTypeObject<Messenger_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[52].common.member_flags, TryConstructUseDefaultValue);
  EXPECT_EQ(getMinimalTypeObject<Messenger_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[53].common.member_flags, TryConstructTrimValue);
  //seqshortseqboundunbound
  EXPECT_EQ(getMinimalTypeObject<Messenger_Mutable_Message_xtag>()
        .minimal.struct_type.member_seq[54].common.member_flags, TryConstructDiscardValue);
  EXPECT_EQ(getMinimalTypeObject<Messenger_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[55].common.member_flags, TryConstructUseDefaultValue);
  EXPECT_EQ(getMinimalTypeObject<Messenger_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[56].common.member_flags, TryConstructTrimValue);
  //seqshortseqboundbound
  EXPECT_EQ(getMinimalTypeObject<Messenger_Mutable_Message_xtag>()
        .minimal.struct_type.member_seq[57].common.member_flags, TryConstructDiscardValue);
  EXPECT_EQ(getMinimalTypeObject<Messenger_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[58].common.member_flags, TryConstructUseDefaultValue);
  EXPECT_EQ(getMinimalTypeObject<Messenger_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[59].common.member_flags, TryConstructTrimValue);
  //unionsequnbound
  EXPECT_EQ(getMinimalTypeObject<Messenger_Mutable_Message_xtag>()
        .minimal.struct_type.member_seq[60].common.member_flags, TryConstructDiscardValue);
  EXPECT_EQ(getMinimalTypeObject<Messenger_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[61].common.member_flags, TryConstructUseDefaultValue);
  EXPECT_EQ(getMinimalTypeObject<Messenger_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[62].common.member_flags, TryConstructTrimValue);
  //unionseqbound
  EXPECT_EQ(getMinimalTypeObject<Messenger_Mutable_Message_xtag>()
        .minimal.struct_type.member_seq[63].common.member_flags, TryConstructDiscardValue);
  EXPECT_EQ(getMinimalTypeObject<Messenger_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[64].common.member_flags, TryConstructUseDefaultValue);
  EXPECT_EQ(getMinimalTypeObject<Messenger_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[65].common.member_flags, TryConstructTrimValue);
  //nestedstruct
  EXPECT_EQ(getMinimalTypeObject<Messenger_Mutable_Message_xtag>()
        .minimal.struct_type.member_seq[66].common.member_flags, TryConstructDiscardValue);
  EXPECT_EQ(getMinimalTypeObject<Messenger_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[67].common.member_flags, TryConstructUseDefaultValue);
  EXPECT_EQ(getMinimalTypeObject<Messenger_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[68].common.member_flags, TryConstructTrimValue);
  //nestedunion
  EXPECT_EQ(getMinimalTypeObject<Messenger_Mutable_Message_xtag>()
        .minimal.struct_type.member_seq[69].common.member_flags, TryConstructDiscardValue);
  EXPECT_EQ(getMinimalTypeObject<Messenger_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[70].common.member_flags, TryConstructUseDefaultValue);
  EXPECT_EQ(getMinimalTypeObject<Messenger_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[71].common.member_flags, TryConstructTrimValue);
  //shortarray
  EXPECT_EQ(getMinimalTypeObject<Messenger_Mutable_Message_xtag>()
        .minimal.struct_type.member_seq[72].common.member_flags, TryConstructDiscardValue);
  EXPECT_EQ(getMinimalTypeObject<Messenger_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[73].common.member_flags, TryConstructUseDefaultValue);
  EXPECT_EQ(getMinimalTypeObject<Messenger_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[74].common.member_flags, TryConstructTrimValue);
  //enumtype
  EXPECT_EQ(getMinimalTypeObject<Messenger_Mutable_Message_xtag>()
        .minimal.struct_type.member_seq[75].common.member_flags, TryConstructDiscardValue);
  EXPECT_EQ(getMinimalTypeObject<Messenger_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[76].common.member_flags, TryConstructUseDefaultValue);
  EXPECT_EQ(getMinimalTypeObject<Messenger_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[77].common.member_flags, TryConstructTrimValue);
  //octet (byte)
  EXPECT_EQ(getMinimalTypeObject<Messenger_Mutable_Message_xtag>()
        .minimal.struct_type.member_seq[78].common.member_flags, TryConstructDiscardValue);
  EXPECT_EQ(getMinimalTypeObject<Messenger_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[79].common.member_flags, TryConstructUseDefaultValue);
  EXPECT_EQ(getMinimalTypeObject<Messenger_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[80].common.member_flags, TryConstructTrimValue);
  //boolean
  EXPECT_EQ(getMinimalTypeObject<Messenger_Mutable_Message_xtag>()
        .minimal.struct_type.member_seq[81].common.member_flags, TryConstructDiscardValue);
  EXPECT_EQ(getMinimalTypeObject<Messenger_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[82].common.member_flags, TryConstructUseDefaultValue);
  EXPECT_EQ(getMinimalTypeObject<Messenger_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[83].common.member_flags, TryConstructTrimValue);
  //short
  EXPECT_EQ(getMinimalTypeObject<Messenger_Mutable_Message_xtag>()
        .minimal.struct_type.member_seq[84].common.member_flags, TryConstructDiscardValue);
  EXPECT_EQ(getMinimalTypeObject<Messenger_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[85].common.member_flags, TryConstructUseDefaultValue);
  EXPECT_EQ(getMinimalTypeObject<Messenger_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[86].common.member_flags, TryConstructTrimValue);
  //unsigned short
  EXPECT_EQ(getMinimalTypeObject<Messenger_Mutable_Message_xtag>()
        .minimal.struct_type.member_seq[87].common.member_flags, TryConstructDiscardValue);
  EXPECT_EQ(getMinimalTypeObject<Messenger_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[88].common.member_flags, TryConstructUseDefaultValue);
  EXPECT_EQ(getMinimalTypeObject<Messenger_Mutable_Message_xtag>()
      .minimal.struct_type.member_seq[89].common.member_flags, TryConstructTrimValue);
}

int main(int argc, char* argv[])
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
