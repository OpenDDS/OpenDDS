/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <dds/DCPS/Barrier.h>

#include <ace/OS_NS_unistd.h>

#include <gtest/gtest.h>

namespace
{

ACE_THR_FUNC_RETURN run(void* arg) {
  OpenDDS::DCPS::Barrier* barrier = reinterpret_cast<OpenDDS::DCPS::Barrier*>(arg);
  barrier->wait();
  return 0;
}

} // (anonymous) namespace

TEST(dds_DCPS_Barrier, DefaultConstructor)
{
  OpenDDS::DCPS::Barrier barrier(1u);
  barrier.wait();
}

TEST(dds_DCPS_Barrier, ConstructorFour)
{
  OpenDDS::DCPS::Barrier barrier(4);

  ACE_hthread_t ids_[4];

  for (size_t i = 0; i < 4; ++i) {
    ACE_Thread::spawn(run, &barrier, THR_NEW_LWP | THR_JOINABLE, 0, &(ids_[i]));
  }

  for (size_t i = 0; i < 4; ++i) {
    ACE_Thread::join(ids_[i], 0);
  }
}

TEST(dds_DCPS_Barrier, ConstructorFourPlusOne)
{
  OpenDDS::DCPS::Barrier barrier(5);

  ACE_hthread_t ids_[4];

  for (size_t i = 0; i < 4; ++i) {
    ACE_Thread::spawn(run, &barrier, THR_NEW_LWP | THR_JOINABLE, 0, &(ids_[i]));
  }

  barrier.wait();

  for (size_t i = 0; i < 4; ++i) {
    ACE_Thread::join(ids_[i], 0);
  }
}
