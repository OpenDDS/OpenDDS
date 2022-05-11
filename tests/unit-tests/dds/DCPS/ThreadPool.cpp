/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <dds/DCPS/ThreadPool.h>

#include <ace/OS_NS_unistd.h>

#include <gtest/gtest.h>

namespace
{

ACE_THR_FUNC_RETURN inc_count(void* arg) {
  if (arg) {
    ACE_Atomic_Op<ACE_Thread_Mutex, size_t>* count = reinterpret_cast<ACE_Atomic_Op<ACE_Thread_Mutex, size_t>*>(arg);
    ++(*count);
  }
  return 0;
}

ACE_THR_FUNC_RETURN check_membership(void* arg) {
  if (arg) {
    OpenDDS::DCPS::ThreadPool& pool = *reinterpret_cast<OpenDDS::DCPS::ThreadPool*>(arg);
    EXPECT_TRUE(pool.contains(ACE_Thread::self()));
  }
  return 0;
}

} // (anonymous) namespace

TEST(dds_DCPS_ThreadPool, NoArgConstructor)
{
  {
    OpenDDS::DCPS::ThreadPool pool(2, inc_count); // no arg means nothing to increment
  }
}

TEST(dds_DCPS_ThreadPool, ArgConstructorZero)
{
  ACE_Atomic_Op<ACE_Thread_Mutex, size_t> count(0u);
  {
    OpenDDS::DCPS::ThreadPool pool(0u, inc_count, &count);
  }
  EXPECT_EQ(count, 0u);
}

TEST(dds_DCPS_ThreadPool, ArgConstructorOne)
{
  ACE_Atomic_Op<ACE_Thread_Mutex, size_t> count(0u);
  {
    OpenDDS::DCPS::ThreadPool pool(1u, inc_count, &count);
  }
  EXPECT_EQ(count, 1u);
}

TEST(dds_DCPS_ThreadPool, ArgConstructorFour)
{
  ACE_Atomic_Op<ACE_Thread_Mutex, size_t> count(0u);
  {
    OpenDDS::DCPS::ThreadPool pool(4u, inc_count, &count);
  }
  EXPECT_EQ(count, 4u);
}

TEST(dds_DCPS_ThreadPool, ArgConstructorSixteen)
{
  ACE_Atomic_Op<ACE_Thread_Mutex, size_t> count(0u);
  {
    OpenDDS::DCPS::ThreadPool pool(16u, inc_count, &count);
  }
  EXPECT_EQ(count, 16u);
}

TEST(dds_DCPS_ThreadPool, CheckMembership)
{
  {
    OpenDDS::DCPS::ThreadPool pool(4, check_membership, &pool);
    EXPECT_FALSE(pool.contains(ACE_Thread::self()));
  }
}
