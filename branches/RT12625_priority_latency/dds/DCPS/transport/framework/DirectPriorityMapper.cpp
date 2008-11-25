// -*- C++ -*-
//
// $Id$

#include "DCPS/DdsDcps_pch.h"
#include "DCPS/debug.h"
#include "ace/Log_Msg.h"

#include "DirectPriorityMapper.h"

#if !defined (__ACE_INLINE__)
#include "DirectPriorityMapper.inl"
#endif /* __ACE_INLINE__ */

// NOTE: This might make a call to sched_get_priority_min(SCHED_RR) if appropriate.
const short
OpenDDS::DCPS::DirectPriorityMapper::thread_min_ = ACE_THR_PRI_RR_MIN;

// NOTE: This might make a call to sched_get_priority_max(SCHED_RR) if appropriate.
const short
OpenDDS::DCPS::DirectPriorityMapper::thread_max_ = ACE_THR_PRI_RR_MAX;

OpenDDS::DCPS::DirectPriorityMapper::~DirectPriorityMapper()
{
}

short
OpenDDS::DCPS::DirectPriorityMapper::codepoint() const
{
  short value;

  // We know that the DiffServ codepoints range from a low number to a
  // high number, with the high number being a higher priority - which
  // is the ordering that the TRANSPORT_PRIORIY value has.
  if( this->priority() > this->dscp_max_) {
    value = this->dscp_max_;
  }

  if( this->priority() < this->dscp_min_) {
    value = this->dscp_min_;
  }

  value = this->priority();

  if( OpenDDS::DCPS::DCPS_debug_level > 4) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) DirectPriorityMapper:codepoint() - ")
      ACE_TEXT("mapped TRANSPORT_PRIORITY value %d ")
      ACE_TEXT("to codepoint %d.\n"),
      this->priority(),
      value
    ));
  }
  return value;
}

short
OpenDDS::DCPS::DirectPriorityMapper::thread_priority() const
{
  static const int direction = (this->thread_max_ < this->thread_min_)? -1: 1;
  static const int range = direction * (this->thread_max_ - this->thread_min_);

  short value = this->thread_min_ + direction * this->priority();

  if( this->priority() < 0) {
    value = this->thread_min_;
  }

  if( this->priority() > range) {
    value = this->thread_max_;
  }

  if( OpenDDS::DCPS::DCPS_debug_level > 4) {
    ACE_DEBUG((LM_DEBUG,
      ACE_TEXT("(%P|%t) DirectPriorityMapper:thread_priority() - ")
      ACE_TEXT("mapped TRANSPORT_PRIORITY value %d ")
      ACE_TEXT("to thread priority %d.\n"),
      this->priority(),
      value
    ));
  }
  return value;
}

