#include "dds/DCPS/TypeAssignability.h"

#include "gtest/gtest.h"

using namespace OpenDDS::XTypes;

/*
TEST(PrimitiveTypesTest, IsAssignable)
{
  TypeAssignability test;
  TypeIdentifierPtr ti_ptr(new TypeIdentifier, OpenDDS::DCPS::keep_count());
  ti_ptr->kind = TK_BOOLEAN;
  MinimalStructMemberSeq mem_seq;
  MinimalStructMember member(CommonStructMember(MemberId(), StructMemberFlag(), ti_ptr),
                             MinimalMemberDetail());
  mem_seq.append(member);
  MinimalStructType min_struct(StructTypeFlag(), MinimalStructHeader(), mem_seq);
  MinimalTypeObject min_toa(min_struct);
  MinimalTypeObject min_tob = min_toa;
  min_tob.struct_type.member_seq.members[0].common.member_type_id = TypeIdentifierPtr(new TypeIdentifier, OpenDDS::DCPS::keep_count());
  min_tob.struct_type.member_seq.members[0].common.member_type_id->kind = TK_INT16;

  EXPECT_TRUE(test.assignable(TypeObject(min_toa), TypeObject(min_tob)));
}
*/

TEST(PrimitiveTypesTest, IsAssignable)
{
  TypeIdentifier tia, tib;
  tia.kind = TK_BOOLEAN;
  tib.kind = TK_INT16;
  TypeAssignability test;
  EXPECT_TRUE(test.assignable(tia, tib));
}

int main(int argc, char* argv[])
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
