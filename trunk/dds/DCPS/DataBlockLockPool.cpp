// -*- C++ -*-
//
// $Id$

#include "DCPS/DdsDcps_pch.h"
#include "DataBlockLockPool.h"

DataBlockLockPool::DataBlockLockPool(size_t size)
: size_(size),
  pool_(size),
  iterator_(0)
{
/*
  // Initialize the pointers
  for (ssize_t cnt = 0; cnt < size_; cnt++)
    {
      pool_[cnt] = new DataBlockLock();
    }
*/
}

DataBlockLockPool::~DataBlockLockPool()
{
/*
  for (size_t cnt = 0; cnt < size_; cnt++)
    {
      delete pool_[cnt];
    }
*/
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
  // Do Nothing.
}

