/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_DATABLOCKLOCKPOOL_H
#define OPENDDS_DCPS_DATABLOCKLOCKPOOL_H

#include "Atomic.h"
#include "dcps_export.h"
#include "PoolAllocationBase.h"

#include <ace/Lock_Adapter_T.h>
#include <ace/Thread_Mutex.h>
#include <ace/Containers_T.h>

/**
 * @class DataBlockLockPool
 *
 * @brief Holds and distributes locks to be used for locking
 * ACE_Data_Blocks.  Currently it does not require returning the
 * locks.
 *
 * @NOTE: The lock returned is not guaranteed to be unique.
 *
 * @NOTE: This class is thread safe.
 */
class OpenDDS_Dcps_Export DataBlockLockPool : public OpenDDS::DCPS::PoolAllocationBase {
public:
  typedef ACE_Lock_Adapter<ACE_Thread_Mutex> DataBlockLock;

  DataBlockLockPool(unsigned long size)
    : pool_(size),
      size_(size),
      iterator_(0)
  {
  }

  ~DataBlockLockPool() { }

  DataBlockLock * get_lock() {
    const unsigned long index = iterator_++ % size_;
    return &(pool_[index]);
  }

private:
  typedef ACE_Array<DataBlockLock> Pool;

  Pool pool_;
  const unsigned long size_;
  /// Counter used to track which lock to give out next (modulus size_)
  OpenDDS::DCPS::Atomic<unsigned long> iterator_;
};

#endif /* DATABLOCKLOCKPOOL_H  */
