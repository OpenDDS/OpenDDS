/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef UPDATERECEIVER_T_CPP
#define UPDATERECEIVER_T_CPP

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "DcpsInfo_pch.h"
#include "dds/DCPS/debug.h"
#include "UpdateReceiver_T.h"
#include "UpdateProcessor_T.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace Federator {

template<class DataType>
UpdateReceiver<DataType>::UpdateReceiver(UpdateProcessor<DataType>& processor)
  : processor_(processor),
    stop_(false),
    workAvailable_(this->lock_)
{
  if (OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) UpdateReceiver::UpdateReceiver()\n")));
  }

  // Always execute the thread.
  this->open(0);
}

template<class DataType>
UpdateReceiver<DataType>::~UpdateReceiver()
{
  if (OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) UpdateReceiver::~UpdateReceiver()\n")));
  }

  // Cleanly terminate.
  this->stop();
  this->wait();
}

template<class DataType>
int
UpdateReceiver<DataType>::open(void*)
{
  if (OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) UpdateReceiver::open()\n")));
  }

  // Run as a separate thread.
  return this->activate();
}

template<class DataType>
int
UpdateReceiver<DataType>::close(u_long /* flags */)
{
  if (OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) UpdateReceiver::close()\n")));
  }

  return 0;
}

template<class DataType>
void
UpdateReceiver<DataType>::stop()
{
  if (OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) UpdateReceiver::stop()\n")));
  }

  // Indicate the thread should stop and get its attention.
  if (this->stop_)
    return;

  this->stop_ = true;
  this->workAvailable_.signal();
}

template<class DataType>
void
UpdateReceiver<DataType>::add(DataType* sample, DDS::SampleInfo* info)
{
  if (OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) UpdateReceiver::add()\n")));
  }

  { // Protect the queue.
    ACE_GUARD(ACE_SYNCH_MUTEX, guard, this->lock_);
    this->queue_.push_back(DataInfo(sample, info));

    if (OpenDDS::DCPS::DCPS_debug_level > 9) {
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) UpdateReceiver::add() - ")
                 ACE_TEXT(" %d samples waiting to process in 0x%x.\n"),
                 this->queue_.size(),
                 (void*)this));
    }
  }

  this->workAvailable_.signal();
}

template<class DataType>
int
UpdateReceiver<DataType>::svc()
{
  if (OpenDDS::DCPS::DCPS_debug_level > 0) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) UpdateReceiver::svc()\n")));
  }

  // Continue until we are synchronously terminated.
  while (this->stop_ == false) {
    { // Block until there is work to do.
      ACE_GUARD_RETURN(ACE_SYNCH_MUTEX, guard, this->lock_, 0);

      while (this->queue_.size() == 0) {
        // This releases the lock while we block.
        this->workAvailable_.wait();

        if (OpenDDS::DCPS::DCPS_debug_level > 9) {
          ACE_DEBUG((LM_DEBUG,
                     ACE_TEXT("(%P|%t) UpdateReceiver::svc() - ")
                     ACE_TEXT("wakeup in 0x%x.\n"),
                     (void*)this));
        }

        // We were asked to stop instead.
        if (this->stop_ == true) {
          if (OpenDDS::DCPS::DCPS_debug_level > 4) {
            ACE_DEBUG((LM_DEBUG,
                       ACE_TEXT("(%P|%t) UpdateReceiver::svc() - ")
                       ACE_TEXT("discontinuing processing after wakeup in 0x%x.\n"),
                       (void*)this));
          }

          return 0;
        }
      }
    }

    if (OpenDDS::DCPS::DCPS_debug_level > 0) {
      ACE_DEBUG((LM_DEBUG,
                 ACE_TEXT("(%P|%t) UpdateReceiver::svc() - ")
                 ACE_TEXT("processing a sample in 0x%x.\n"),
                 (void*)this));
    }

    // Delegate actual processing to the publication manager.
    this->processor_.processSample(
      this->queue_.front().first,
      this->queue_.front().second);

    { // Remove the completed work.
      ACE_GUARD_RETURN(ACE_SYNCH_MUTEX, guard, this->lock_, 0);
      delete this->queue_.front().first;
      delete this->queue_.front().second;
      this->queue_.pop_front();
    }
  }

  if (OpenDDS::DCPS::DCPS_debug_level > 4) {
    ACE_DEBUG((LM_DEBUG,
               ACE_TEXT("(%P|%t) UpdateReceiver::svc() - ")
               ACE_TEXT("discontinuing processing after sample complete in 0x%x.\n"),
               (void*)this));
  }

  return 0;
}

} // namespace Federator
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* UPDATERECEIVER_T_CPP */
