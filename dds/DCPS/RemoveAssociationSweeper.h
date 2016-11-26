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
  void cancel_timer(OpenDDS::DCPS::RcHandle<OpenDDS::DCPS::WriterInfo>& info);

  // Arg will be PublicationId
  int handle_timeout(const ACE_Time_Value& current_time, const void* arg);

  virtual bool reactor_is_shut_down() const
  {
    return TheServiceParticipant->is_shut_down();
  }

private:
  ~RemoveAssociationSweeper();

  T* reader_;

  class CommandBase : public Command {
  public:
    CommandBase(RemoveAssociationSweeper<T>* sweeper,
                OpenDDS::DCPS::RcHandle<OpenDDS::DCPS::WriterInfo>& info)
      : sweeper_ (sweeper)
      , info_(info)
    { }

  protected:
    RemoveAssociationSweeper<T>* sweeper_;
    OpenDDS::DCPS::RcHandle<OpenDDS::DCPS::WriterInfo> info_;
  };

  class ScheduleCommand : public CommandBase {
  public:
    ScheduleCommand(RemoveAssociationSweeper<T>* sweeper,
                    OpenDDS::DCPS::RcHandle<OpenDDS::DCPS::WriterInfo>& info)
      : CommandBase(sweeper, info)
    { }
    virtual void execute();
  };

  class CancelCommand : public CommandBase {
  public:
    CancelCommand(RemoveAssociationSweeper<T>* sweeper,
                  OpenDDS::DCPS::RcHandle<OpenDDS::DCPS::WriterInfo>& info)
      : CommandBase(sweeper, info)
    { }
    virtual void execute();
  };
};
//Starting RemoveAssociationSweeper
template <typename T>
RemoveAssociationSweeper<T>::RemoveAssociationSweeper(ACE_Reactor* reactor,
                                                   ACE_thread_t owner,
                                                   T* reader)
  : ReactorInterceptor (reactor, owner)
  , reader_(reader)
{ }

template <typename T>
RemoveAssociationSweeper<T>::~RemoveAssociationSweeper()
{ }

template <typename T>
void RemoveAssociationSweeper<T>::schedule_timer(OpenDDS::DCPS::RcHandle<OpenDDS::DCPS::WriterInfo>& info, bool callback)
{
  info->scheduled_for_removal_ = true;
  info->notify_lost_ = callback;
  ACE_Time_Value ten_seconds(10);
  info->removal_deadline_ = ACE_OS::gettimeofday() + ten_seconds;
  ScheduleCommand c(this, info);
  execute_or_enqueue(c);
}

template <typename T>
void RemoveAssociationSweeper<T>::cancel_timer(OpenDDS::DCPS::RcHandle<OpenDDS::DCPS::WriterInfo>& info)
{
  info->scheduled_for_removal_ = false;
  info->removal_deadline_ = ACE_Time_Value::zero;
  CancelCommand c(this, info);
  execute_or_enqueue(c);
}

template <typename T>
int RemoveAssociationSweeper<T>::handle_timeout(
    const ACE_Time_Value& ,
    const void* arg)
{
  WriterInfo* const info =
    const_cast<WriterInfo*>(reinterpret_cast<const WriterInfo*>(arg));
  info->remove_association_timer_ = WriterInfo::NO_TIMER;
  const PublicationId pub_id = info->writer_id_;

  info->_remove_ref();

  if (DCPS_debug_level >= 1) {
    GuidConverter sub_repo(reader_->get_repo_id());
    GuidConverter pub_repo(pub_id);
    ACE_DEBUG((LM_INFO, "((%P|%t)) RemoveAssociationSweeper::handle_timeout reader: %C waiting on writer: %C\n",
               OPENDDS_STRING(sub_repo).c_str(),
               OPENDDS_STRING(pub_repo).c_str()));
  }

  reader_->remove_or_reschedule(pub_id);
  return 0;
}

template <typename T>
void RemoveAssociationSweeper<T>::ScheduleCommand::execute()
{
  static const ACE_Time_Value two_seconds(2);

  //Pass pointer to writer info for timer to use, must decrease ref count when canceling timer
  const void* arg = reinterpret_cast<const void*>(this->info_.in());
  this->info_->_add_ref();

  this->info_->remove_association_timer_ = this->sweeper_->reactor()->schedule_timer(this->sweeper_,
                                                                       arg,
                                                                       two_seconds);
  if (DCPS_debug_level) {
    ACE_DEBUG((LM_INFO, "(%P|%t) RemoveAssociationSweeper::ScheduleCommand::execute() - Scheduled sweeper %d\n", this->info_->remove_association_timer_));
  }
}

template <typename T>
void RemoveAssociationSweeper<T>::CancelCommand::execute()
{
  if (this->info_->remove_association_timer_ != WriterInfo::NO_TIMER) {
      this->sweeper_->reactor()->cancel_timer(this->info_->remove_association_timer_);
    if (DCPS_debug_level) {
      ACE_DEBUG((LM_INFO, "(%P|%t) RemoveAssociationSweeper::CancelCommand::execute() - Unscheduled sweeper %d\n", this->info_->remove_association_timer_));
    }
    this->info_->remove_association_timer_ = WriterInfo::NO_TIMER;
    this->info_->_remove_ref();
  }
}
//End RemoveAssociationSweeper

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif /* OPENDDS_DCPS_REMOVEASSOCIATIONSWEEPER_H  */
