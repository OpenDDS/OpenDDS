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
namespace {

struct Predicate {
  explicit Predicate(const PublicationId& writer_id) : writer_id_(writer_id) {}
  PublicationId writer_id_;
  bool operator() (const RcHandle<WriterInfo>& info) const {
    return writer_id_ == info->writer_id();
  }
};

}

// Class to cleanup associations scheduled for removal
template <typename T>
class RemoveAssociationSweeper : public ReactorInterceptor {
public:
  RemoveAssociationSweeper(ACE_Reactor* reactor,
                           ACE_thread_t owner,
                           T* reader);

  void schedule_timer(RcHandle<WriterInfo>& info, bool callback);
  ReactorInterceptor::CommandPtr cancel_timer(RcHandle<WriterInfo>& info);
  void cancel_timer(const PublicationId& writer_id);

  // Arg will be PublicationId
  int handle_timeout(const ACE_Time_Value& current_time, const void* arg);

  virtual bool reactor_is_shut_down() const
  {
    return TheServiceParticipant->is_shut_down();
  }


  RcHandle<WriterInfo> remove_info(WriterInfo* info);
private:
  ~RemoveAssociationSweeper();


  WeakRcHandle<T> reader_;
  OPENDDS_VECTOR(RcHandle<WriterInfo>) info_set_;

  struct TimerArg : PoolAllocationBase {
    TimerArg(RcHandle<RemoveAssociationSweeper<T> > sweeper,
             RcHandle<WriterInfo> info)
      : sweeper_(sweeper)
      , info_(info)
    { }

    WeakRcHandle<RemoveAssociationSweeper<T> > sweeper_;
    RcHandle<WriterInfo> info_;
  };

  class CommandBase : public Command, public TimerArg {
  public:
    CommandBase(RcHandle<RemoveAssociationSweeper<T> > sweeper,
                RcHandle<WriterInfo> info)
      : TimerArg(sweeper, info)
    { }
  };

  class ScheduleCommand : public CommandBase {
  public:
    ScheduleCommand(RcHandle<RemoveAssociationSweeper<T> > sweeper,
                    RcHandle<WriterInfo> info)
      : CommandBase(sweeper, info)
    { }
    virtual void execute();
  };

  class CancelCommand : public CommandBase {
  public:
    CancelCommand(RcHandle<RemoveAssociationSweeper<T> > sweeper,
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
void RemoveAssociationSweeper<T>::schedule_timer(RcHandle<WriterInfo>& info, bool callback)
{
  info->set_scheduled_for_removal(callback, TimeDuration(10));
  execute_or_enqueue(make_rch<ScheduleCommand>(rchandle_from(this), info));
}

template <typename T>
ReactorInterceptor::CommandPtr
RemoveAssociationSweeper<T>::cancel_timer(RcHandle<WriterInfo>& info)
{
  info->unset_scheduled_for_removal();
  return execute_or_enqueue(make_rch<CancelCommand>(rchandle_from(this), info));
}

template <typename T>
void
RemoveAssociationSweeper<T>::cancel_timer(const PublicationId& writer_id)
{
  RcHandle<WriterInfo> info;
  {
    ACE_Guard<ACE_Thread_Mutex> guard(mutex_);
    OPENDDS_VECTOR(RcHandle<WriterInfo>)::iterator pos = std::find_if(info_set_.begin(), info_set_.end(), Predicate(writer_id));
    if (pos != info_set_.end()) {
      info = *pos;
    }
  }
  if (info) {
    cancel_timer(info);
  }
}

template <typename T>
RcHandle<WriterInfo> RemoveAssociationSweeper<T>::remove_info(WriterInfo* info)
{
  RcHandle<WriterInfo> result;

  ACE_Guard<ACE_Thread_Mutex> guard(mutex_);

  if (info_set_.empty()) {
    return result;
  }

  OPENDDS_VECTOR(RcHandle<WriterInfo>)::iterator itr, last = --info_set_.end();
  // find the RcHandle holds the pointer info in info_set_
  // and then swap the found element with the last element in the
  // info_set_ vector so  we can delete it.
  for (itr = info_set_.begin(); itr != info_set_.end(); ++itr){
    if (itr->in() == info) {
      if (itr != last) {
        std::swap(*itr, info_set_.back());
      }
      result = info_set_.back();
      info_set_.pop_back();
      return result;
    }
  }
  return result;
}

template <typename T>
int RemoveAssociationSweeper<T>::handle_timeout(
    const ACE_Time_Value& ,
    const void* arg)
{
  unique_ptr<const TimerArg> ta(reinterpret_cast<const TimerArg*>(arg));
  if (!ta) {
    return 0;
  }

  RcHandle<WriterInfo> info = remove_info(ta->info_.get());
  if (!info) {
    return 0;
  }

  info->clear_remove_association_timer();
  const PublicationId pub_id = info->writer_id();

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
  RcHandle<RemoveAssociationSweeper<T> > sweeper = this->sweeper_.lock();
  if (!sweeper) {
    return;
  }

  CommandBase::info_->schedule_remove_association_timer(sweeper->reactor(), sweeper.in(), new TimerArg(sweeper, CommandBase::info_));
  sweeper->info_set_.push_back(CommandBase::info_);

  if (DCPS_debug_level) {
    ACE_DEBUG((LM_INFO,
      ACE_TEXT("(%P|%t) RemoveAssociationSweeper::ScheduleCommand::execute() - ")
      ACE_TEXT("Scheduled sweeper %@\n"),
      CommandBase::info_.in()));
  }
}

template <typename T>
void RemoveAssociationSweeper<T>::CancelCommand::execute()
{
  RcHandle<RemoveAssociationSweeper<T> > sweeper = this->sweeper_.lock();
  if (!sweeper) {
    return;
  }

  const void* arg = 0;
  CommandBase::info_->cancel_remove_association_timer(sweeper->reactor(), sweeper.in(), &arg);
  sweeper->remove_info(CommandBase::info_.in());
  const TimerArg* ta = reinterpret_cast<const TimerArg*>(arg);
  delete ta;

  if (DCPS_debug_level) {
    ACE_DEBUG((LM_INFO, ACE_TEXT("(%P|%t) RemoveAssociationSweeper::CancelCommand::execute() - ")
      ACE_TEXT("Unscheduled sweeper %@\n"),
      CommandBase::info_.in()));
  }
}
//End RemoveAssociationSweeper

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_REMOVEASSOCIATIONSWEEPER_H  */
