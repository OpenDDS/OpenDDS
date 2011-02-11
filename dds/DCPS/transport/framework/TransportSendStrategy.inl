/*
 * $Id$
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "ThreadSynch.h"

ACE_INLINE void
OpenDDS::DCPS::TransportSendStrategy::send_start()
{
  DBG_ENTRY_LVL("TransportSendStrategy","send_start",6);

  GuardType guard(this->lock_);

  if (!this->link_released_)
    ++this->start_counter_;
}

ACE_INLINE void
OpenDDS::DCPS::TransportSendStrategy::link_released(bool flag)
{
  DBG_ENTRY_LVL("TransportSendStrategy","link_released",6);

  GuardType guard(this->lock_);
  this->link_released_ = flag;
}

ACE_INLINE void
OpenDDS::DCPS::TransportSendStrategy::relink(bool)
{
  DBG_ENTRY_LVL("TransportSendStrategy","relink",6);
  // The subsclass needs implement this function for re-establishing
  // the link upon send failure.
}

ACE_INLINE void
OpenDDS::DCPS::TransportSendStrategy::suspend_send()
{
  DBG_ENTRY_LVL("TransportSendStrategy","suspend_send",6);
  GuardType guard(this->lock_);

  if (this->mode_ != MODE_TERMINATED && this->mode_ != MODE_SUSPEND) {
    this->mode_before_suspend_ = this->mode_;
    this->mode_ = MODE_SUSPEND;
  }
}

ACE_INLINE void
OpenDDS::DCPS::TransportSendStrategy::resume_send()
{
  DBG_ENTRY_LVL("TransportSendStrategy","resume_send",6);
  GuardType guard(this->lock_);

  // If this send strategy is reused when the connection is reestablished, then
  // we need re-initialize the mode_ and mode_before_suspend_.
  if (this->mode_ == MODE_TERMINATED) {
    this->header_.length_ = 0;
    this->pkt_chain_ = 0;
    this->header_complete_ = 0;
    this->start_counter_ = 0;
    this->mode_ = MODE_DIRECT;
    this->mode_before_suspend_ = MODE_NOT_SET;
    this->num_delayed_notifications_ = 0;

  } else if (this->mode_ == MODE_SUSPEND) {
    this->mode_ = this->mode_before_suspend_;
    this->mode_before_suspend_ = MODE_NOT_SET;

  } else {
    ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: TransportSendStrategy::resume_send  The suspend or terminate"
               " is not called previously.\n"));
  }
}

ACE_INLINE const char*
OpenDDS::DCPS::TransportSendStrategy::mode_as_str(SendMode mode)
{
  static const char* SendModeStr[] = { "MODE_NOT_SET",
                                       "MODE_DIRECT",
                                       "MODE_QUEUE",
                                       "MODE_SUSPEND",
                                       "MODE_TERMINATED",
                                       "UNKNOWN"
                                     };

  return SendModeStr [mode];
}

ACE_INLINE bool
OpenDDS::DCPS::TransportSendStrategy::isDirectMode()
{
  return this->mode_ == MODE_DIRECT;
}

ACE_INLINE ssize_t
OpenDDS::DCPS::TransportSendStrategy::send_bytes(const iovec iov[],
                                                 int n,
                                                 int& /*bp*/)
{
  return send_bytes_i(iov, n);
}

ACE_INLINE ACE_HANDLE
OpenDDS::DCPS::TransportSendStrategy::get_handle()
{
  return ACE_INVALID_HANDLE;
}


ACE_INLINE void
OpenDDS::DCPS::TransportSendStrategy::transport_shutdown ()
{
  this->transport_shutdown_ = true;
}


