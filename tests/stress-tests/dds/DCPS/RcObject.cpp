/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include <dds/DCPS/RcObject.h>
#include <dds/DCPS/PoolAllocator.h>
#include <dds/DCPS/ServiceEventDispatcher.h>

#include <ace/Barrier.h>

#include <gtest/gtest.h>

namespace {
  struct TestObject : public virtual OpenDDS::DCPS::RcObject
  {
    TestObject(const std::string& payload)
      : payload_(payload)
    {}
    const std::string payload_;
  };
  typedef OpenDDS::DCPS::RcHandle<TestObject> TestObject_rch;
  typedef OpenDDS::DCPS::WeakRcHandle<TestObject> TestObject_wrch;

  struct TestConfig_WrchLock_vs_RchReset : public virtual OpenDDS::DCPS::RcObject
  {
    const size_t test_size_;
    OPENDDS_VECTOR(TestObject_rch) rch_vec_;
    OPENDDS_VECTOR(TestObject_wrch) wrch_vec_;
    ACE_Thread_Barrier barrier_;

    TestConfig_WrchLock_vs_RchReset()
      : test_size_(500000u)
      , rch_vec_(test_size_)
      , wrch_vec_(test_size_)
      , barrier_(2)
    {
      for (size_t i = 0; i < test_size_; ++i) {
        rch_vec_[i] = OpenDDS::DCPS::make_rch<TestObject>("payload");
        wrch_vec_[i] = rch_vec_[i];
      }
    }

    void do_locks()
    {
      barrier_.wait();
      for (size_t i = 0; i < test_size_; ++i) {
        {
          TestObject_rch temp(wrch_vec_[i].lock());
        }
        barrier_.wait();
      }
    }

    void do_resets()
    {
      barrier_.wait();
      for (size_t i = 0; i < test_size_; ++i) {
        rch_vec_[i].reset();
        barrier_.wait();
      }
    }
  };
}

TEST(dds_DCPS_RcObject, WRCH_lock_vs_RCH_reset)
{
  using namespace OpenDDS::DCPS;

  RcHandle<TestConfig_WrchLock_vs_RchReset> config = make_rch<TestConfig_WrchLock_vs_RchReset>();
  RcHandle<EventDispatcher> dispatcher = make_rch<ServiceEventDispatcher>(2);

  dispatcher->dispatch(make_rch<PmfEvent<TestConfig_WrchLock_vs_RchReset> >(config, &TestConfig_WrchLock_vs_RchReset::do_locks));
  dispatcher->dispatch(make_rch<PmfEvent<TestConfig_WrchLock_vs_RchReset> >(config, &TestConfig_WrchLock_vs_RchReset::do_resets));

  dispatcher->shutdown();
}
