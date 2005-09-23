// -*- C++ -*-
//
// $Id$

#include "DCPS/DdsDcps_pch.h"
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
DataBlockLockPool::get_lock ()
{
  DataBlockLock* lock = &(pool_[iterator_++]);
  if (iterator_ >= size_)
    {
      iterator_ = iterator_ % size_;
    }
  return lock;
}


void
DataBlockLockPool::return_lock (DataBlockLockPool::DataBlockLock * lock)
{
  ACE_UNUSED_ARG(lock);
  // Do Nothing.
}

