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

ACE_THR_FUNC_RETURN run(void* arg) {
  if (arg) {
    ACE_Atomic_Op<ACE_Thread_Mutex, size_t>* count = reinterpret_cast<ACE_Atomic_Op<ACE_Thread_Mutex, size_t>*>(arg);
    ++(*count);
  }
  return 0;
}

} // (anonymous) namespace

TEST(dds_DCPS_ThreadPool, NoArgConstructor)
{
  {
    OpenDDS::DCPS::ThreadPool pool(2, run);
  }
}

TEST(dds_DCPS_ThreadPool, ArgConstructorZero)
{
  ACE_Atomic_Op<ACE_Thread_Mutex, size_t> count(0u);
  {
    OpenDDS::DCPS::ThreadPool pool(0u, run, &count);
  }
  EXPECT_EQ(count, 0u);
}

TEST(dds_DCPS_ThreadPool, ArgConstructorOne)
{
  ACE_Atomic_Op<ACE_Thread_Mutex, size_t> count(0u);
  {
    OpenDDS::DCPS::ThreadPool pool(1u, run, &count);
  }
  EXPECT_EQ(count, 1u);
}

TEST(dds_DCPS_ThreadPool, ArgConstructorFour)
{
  ACE_Atomic_Op<ACE_Thread_Mutex, size_t> count(0u);
  {
    OpenDDS::DCPS::ThreadPool pool(4u, run, &count);
  }
  EXPECT_EQ(count, 4u);
}

TEST(dds_DCPS_ThreadPool, ArgConstructorSixteen)
{
  ACE_Atomic_Op<ACE_Thread_Mutex, size_t> count(0u);
  {
    OpenDDS::DCPS::ThreadPool pool(16u, run, &count);
  }
  EXPECT_EQ(count, 16u);
}
