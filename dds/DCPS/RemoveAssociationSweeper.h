/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_REMOVEASSOCIATIONSWEEPER_H
#define OPENDDS_DCPS_REMOVEASSOCIATIONSWEEPER_H


#include "WriterInfo.h"
#include "ReactorInterceptor.h"
#include "Service_Participant.h"
#include "GuidConverter.h"
#include "TimeTypes.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

class DDS_TEST;

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

// Class to cleanup associations scheduled for removal
template <typename T>
class RemoveAssociationSweeper : public ReactorInterceptor {
public:
  RemoveAssociationSweeper(ACE_Reactor* reactor,
                           ACE_thread_t owner,
                           T* reader);

  void schedule_timer(OpenDDS::DCPS::RcHandle<OpenDDS::DCPS::WriterInfo>& info, bool callback);
  ReactorInterceptor::CommandPtr cancel_timer(OpenDDS::DCPS::RcHandle<OpenDDS::DCPS::WriterInfo>& info);

  // Arg will be PublicationId
  int handle_timeout(const ACE_Time_Value& current_time, const void* arg);

  virtual bool reactor_is_shut_down() const
  {
    return TheServiceParticipant->is_shut_down();
  }


  int remove_info(WriterInfo* const info);
private:
  ~RemoveAssociationSweeper();


  WeakRcHandle<T> reader_;
  OPENDDS_VECTOR(RcHandle<WriterInfo>) info_set_;

  class CommandBase : public Command {
  public:
    CommandBase(RemoveAssociationSweeper<T>* sweeper,
                RcHandle<WriterInfo> info)
      : sweeper_ (sweeper)
      , info_(info)
    { }

  protected:
    RemoveAssociationSweeper<T>* sweeper_;
    RcHandle<OpenDDS::DCPS::WriterInfo> info_;
  };

  class ScheduleCommand : public CommandBase {
  public:
    ScheduleCommand(RemoveAssociationSweeper<T>* sweeper,
                    RcHandle<WriterInfo> info)
      : CommandBase(sweeper, info)
    { }
    virtual void execute();
  };

  class CancelCommand : public CommandBase {
  public:
    CancelCommand(RemoveAssociationSweeper<T>* sweeper,
                  RcHandle<WriterInfo> info)
      : CommandBase(sweeper, info)
    { }
    virtual void execute();
  };

  friend class CancelCommand;
};

//Starting RemoveAssociationSweeper
template <typename T>
RemoveAssociationSweeper<T>::RemoveAssociationSweeper(ACE_Reactor* reactor,
                                                   ACE_thread_t owner,
                                                   T* reader)
  : ReactorInterceptor (reactor, owner)
  , reader_(*reader)
{ }

template <typename T>
RemoveAssociationSweeper<T>::~RemoveAssociationSweeper()
{ }

template <typename T>
void RemoveAssociationSweeper<T>::schedule_timer(OpenDDS::DCPS::RcHandle<OpenDDS::DCPS::WriterInfo>& info, bool callback)
{
  info->scheduled_for_removal_ = true;
  info->notify_lost_ = callback;
  info->removal_deadline_ = MonotonicTimePoint(MonotonicTimePoint::now() +
    std::min(info->activity_wait_period(), TimeDuration(10)));
  execute_or_enqueue(new ScheduleCommand(this, info));
}

template <typename T>
ReactorInterceptor::CommandPtr
RemoveAssociationSweeper<T>::cancel_timer(OpenDDS::DCPS::RcHandle<OpenDDS::DCPS::WriterInfo>& info)
{
  info->scheduled_for_removal_ = false;
  info->removal_deadline_ = MonotonicTimePoint::zero_value;
  return execute_or_enqueue(new CancelCommand(this, info));
}

template <typename T>
int RemoveAssociationSweeper<T>::remove_info(WriterInfo* const info)
{
  OPENDDS_VECTOR(RcHandle<WriterInfo>)::iterator itr, last = --info_set_.end();
  // find the RcHandle holds the pointer info in this->info_set_
  // and then swap the found element with the last element in the
  // info_set_ vector so  we can delete it.
  for (itr = info_set_.begin(); itr != info_set_.end(); ++itr){
    if (itr->in() == info) {
      if (itr != last) {
        std::swap(*itr, info_set_.back());
      }
      info_set_.pop_back();
      return 0;
    }
  }
  return -1;
}

template <typename T>
int RemoveAssociationSweeper<T>::handle_timeout(
    const ACE_Time_Value& ,
    const void* arg)
{
  WriterInfo* const info =
    const_cast<WriterInfo*>(reinterpret_cast<const WriterInfo*>(arg));

  {
    // info may be destroyed at this moment, we can only access it
    // if it is in the info_set_. This could happen when cancel_timer() handle it first.
    ACE_Guard<ACE_Thread_Mutex> guard(this->mutex_);
    if (this->remove_info(info) == -1)
      return 0;
  }

  info->remove_association_timer_ = WriterInfo::NO_TIMER;
  const PublicationId pub_id = info->writer_id_;

  RcHandle<T> reader = reader_.lock();
  if (!reader)
    return 0;

  if (DCPS_debug_level >= 1) {
    GuidConverter sub_repo(reader->get_repo_id());
    GuidConverter pub_repo(pub_id);
    ACE_DEBUG((LM_INFO, "((%P|%t)) RemoveAssociationSweeper::handle_timeout reader: %C waiting on writer: %C\n",
               OPENDDS_STRING(sub_repo).c_str(),
               OPENDDS_STRING(pub_repo).c_str()));
  }

  reader->remove_publication(pub_id);
  return 0;
}

template <typename T>
void RemoveAssociationSweeper<T>::ScheduleCommand::execute()
{
  /*
   * Pass pointer to writer info for timer to use, must decrease ref count when
   * canceling timer
   */
  const void* arg = reinterpret_cast<const void*>(this->info_.in());
  this->sweeper_->info_set_.push_back(this->info_);

  this->info_->remove_association_timer_ =
    this->sweeper_->reactor()->schedule_timer(
      this->sweeper_, arg,
      (this->info_->removal_deadline_ - MonotonicTimePoint::now()).value());
  if (DCPS_debug_level) {
    ACE_DEBUG((LM_INFO,
      ACE_TEXT("(%P|%t) RemoveAssociationSweeper::ScheduleCommand::execute() - ")
      ACE_TEXT("Scheduled sweeper %d\n"),
      this->info_->remove_association_timer_));
  }
}

template <typename T>
void RemoveAssociationSweeper<T>::CancelCommand::execute()
{
  if (this->info_->remove_association_timer_ != WriterInfo::NO_TIMER) {
    this->sweeper_->reactor()->cancel_timer(this->info_->remove_association_timer_);
    if (DCPS_debug_level) {
      ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) RemoveAssociationSweeper::CancelCommand::execute() - ")
        ACE_TEXT("Unscheduled sweeper %d\n"),
        this->info_->remove_association_timer_));
    }
    this->info_->remove_association_timer_ = WriterInfo::NO_TIMER;
    this->sweeper_->remove_info(this->info_.in());
  }
}
//End RemoveAssociationSweeper

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_REMOVEASSOCIATIONSWEEPER_H  */
