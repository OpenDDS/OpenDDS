// -*- C++ -*-
//
// $Id$
#ifndef UPDATERECEIVER__T_CPP
#define UPDATERECEIVER__T_CPP

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "DcpsInfo_pch.h"
#include "dds/DCPS/debug.h"
#include "UpdateReceiver_T.h"
#include "UpdateProcessor_T.h"

namespace OpenDDS { namespace Federator {

template< class DataType>
UpdateReceiver< DataType>::UpdateReceiver( UpdateProcessor< DataType>& processor)
 : processor_( processor),
   stop_( false),
   workAvailable_( this->lock_)
{
  if( OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) UpdateReceiver::UpdateReceiver()\n")
    ));
  }

  // Always execute the thread.
  this->open(0);
}

template< class DataType>
UpdateReceiver< DataType>::~UpdateReceiver()
{
  if( OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) UpdateReceiver::~UpdateReceiver()\n")
    ));
  }

  // Cleanly terminate.
  this->close();
}

template< class DataType>
int
UpdateReceiver< DataType>::open(void*)
{
  if( OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) UpdateReceiver::open()\n")
    ));
  }

  // Run as a separate thread.
  return this->activate();
}

template< class DataType>
int
UpdateReceiver< DataType>::close( u_long /* flags */)
{
  if( OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) UpdateReceiver::close()\n")
    ));
  }

  // Stop the thread and return after it has finalized.
  this->stop();
  this->wait();
  return 0;
}

template< class DataType>
void
UpdateReceiver< DataType>::stop()
{
  if( OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) UpdateReceiver::stop()\n")
    ));
  }

  // Indicate the thread should stop and get its attention.
  this->stop_ = true;
  this->workAvailable_.signal();
}

template< class DataType>
void
UpdateReceiver< DataType>::put( DataType* sample, ::DDS::SampleInfo* info)
{
  if( OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) UpdateReceiver::put()\n")
    ));
  }

  { // Protect the queue.
    ACE_GUARD( ACE_SYNCH_MUTEX, guard, this->lock_);
    this->queue_.push_back( DataInfo( sample, info));
  }
  this->workAvailable_.signal();
}

template< class DataType>
int
UpdateReceiver< DataType>::svc()
{
  if( OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) UpdateReceiver::svc()\n")
    ));
  }

  // Continue until we are synchronously terminated.
  while( this->stop_ == false) {
    { // Block until there is work to do.
      ACE_GUARD_RETURN( ACE_SYNCH_MUTEX, guard, this->lock_, 0);
      while( this->queue_.size() == 0) {
        // This releases the lock while we block.
        this->workAvailable_.wait();

        // We were asked to stop instead.
        if( this->stop_) {
          return 0;
        }
      }
    }

    if( ::OpenDDS::DCPS::DCPS_debug_level > 0) {
      ACE_DEBUG((LM_DEBUG,
        ACE_TEXT("(%P|%t) UpdateReceiver::svc() - ")
        ACE_TEXT("processing a sample.\n")
      ));
    }

    // Delegate actual processing to the publication manager.
    this->processor_.processSample(
      this->queue_.front().first,
      this->queue_.front().second
    );

    { // Remove the completed work.
      ACE_GUARD_RETURN( ACE_SYNCH_MUTEX, guard, this->lock_, 0);
      delete this->queue_.front().first;
      delete this->queue_.front().second;
      this->queue_.pop_front();
    }
  }
  return 0;
}

}} // End namespace OpenDDS::Federator

#endif /* UPDATERECEIVER__T_CPP */

