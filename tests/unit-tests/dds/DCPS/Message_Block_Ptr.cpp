#include <dds/DCPS/Message_Block_Ptr.h>

#include <gtest/gtest.h>

using namespace OpenDDS::DCPS;

TEST(dds_DCPS_Message_Block_Shared_Ptr, copy)
{
  Message_Block_Shared_Ptr mbsp1(new ACE_Message_Block);
  EXPECT_EQ(1, mbsp1->reference_count());

  {
    Message_Block_Shared_Ptr mbsp2(mbsp1);
    EXPECT_EQ(2, mbsp1->reference_count());
    EXPECT_EQ(2, mbsp2->reference_count());

    mbsp2 = mbsp1;
    EXPECT_EQ(2, mbsp1->reference_count());
    EXPECT_EQ(2, mbsp2->reference_count());
  }

  EXPECT_EQ(1, mbsp1->reference_count());

  {
    Message_Block_Shared_Ptr mbsp2(new ACE_Message_Block);
    EXPECT_EQ(1, mbsp1->reference_count());
    EXPECT_EQ(1, mbsp2->reference_count());

    mbsp2 = mbsp1;
    EXPECT_EQ(2, mbsp1->reference_count());
    EXPECT_EQ(2, mbsp2->reference_count());
  }

  EXPECT_EQ(1, mbsp1->reference_count());
}

#ifdef ACE_HAS_CPP11
TEST(dds_DCPS_Lockable_Message_Block_Ptr, ctor)
{
  Lockable_Message_Block_Ptr def;
  EXPECT_EQ(nullptr, def.get());

  Lockable_Message_Block_Ptr nolock(nullptr);
  EXPECT_EQ(Lockable_Message_Block_Ptr::Lock_Policy::No_Lock, nolock.lock_policy());

  Lockable_Message_Block_Ptr lock(new ACE_Message_Block, Lockable_Message_Block_Ptr::Lock_Policy::Use_Lock);
  EXPECT_EQ(Lockable_Message_Block_Ptr::Lock_Policy::Use_Lock, lock.lock_policy());
  EXPECT_NE(nullptr, lock->locking_strategy());

  Message_Block_Shared_Ptr mbsp(new ACE_Message_Block);
  Lockable_Message_Block_Ptr upgrade(mbsp);
  EXPECT_EQ(2, mbsp->reference_count());
  EXPECT_EQ(2, upgrade->reference_count());
  EXPECT_EQ(nullptr, upgrade->locking_strategy());

  {
    Lockable_Message_Block_Ptr copy(upgrade);
    EXPECT_EQ(3, mbsp->reference_count());
    EXPECT_EQ(3, upgrade->reference_count());
    EXPECT_EQ(3, copy->reference_count());

    copy = lock;
    EXPECT_EQ(2, mbsp->reference_count());
    EXPECT_EQ(2, upgrade->reference_count());
    EXPECT_NE(upgrade.get(), copy.get());
    EXPECT_EQ(2, copy->reference_count());
    EXPECT_EQ(2, lock->reference_count());
  }

  EXPECT_EQ(1, lock->reference_count());
}

TEST(dds_DCPS_Lockable_Message_Block_Ptr, cont_unlocked)
{
  Lockable_Message_Block_Ptr inner(new ACE_Message_Block);
  Lockable_Message_Block_Ptr outer(new ACE_Message_Block);
  outer.lockable_cont(inner);
  EXPECT_NE(nullptr, outer->cont());
  EXPECT_EQ(outer->cont()->data_block(), inner->data_block());
  EXPECT_EQ(nullptr, outer->locking_strategy());
  EXPECT_EQ(2, inner->reference_count());
  EXPECT_EQ(1, outer->reference_count());
}

TEST(dds_DCPS_Lockable_Message_Block_Ptr, cont_locked)
{
  Lockable_Message_Block_Ptr inner(new ACE_Message_Block, Lockable_Message_Block_Ptr::Lock_Policy::Use_Lock);
  Lockable_Message_Block_Ptr outer(new ACE_Message_Block);
  const auto orig_lock = inner->locking_strategy();
  outer.lockable_cont(inner);
  EXPECT_NE(nullptr, outer->cont());
  EXPECT_EQ(outer->cont()->data_block(), inner->data_block());
  EXPECT_EQ(orig_lock, outer->locking_strategy());
  EXPECT_EQ(nullptr, inner->locking_strategy());
  EXPECT_EQ(2, inner->reference_count());
  EXPECT_EQ(1, outer->reference_count());
}
#endif
