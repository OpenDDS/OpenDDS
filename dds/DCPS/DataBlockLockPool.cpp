/*
 * $Id$
 *
 * Copyright 2009 Object Computing, Inc.
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "DCPS/DdsDcps_pch.h" //Only the _pch include should start with DCPS/
#include "DataBlockLockPool.h"

DataBlockLockPool::DataBlockLockPool(size_t size)
  : pool_(size),
    size_(size),
    iterator_(0)
{
}

DataBlockLockPool::~DataBlockLockPool()
{
}

DataBlockLockPool::DataBlockLock *
DataBlockLockPool::get_lock()
{
  DataBlockLock* lock = &(pool_[iterator_++]);

  if (iterator_ >= size_) {
    iterator_ = iterator_ % size_;
  }

  return lock;
}

void
DataBlockLockPool::return_lock(DataBlockLockPool::DataBlockLock * /* lock */)
{
  // Do Nothing.
}
