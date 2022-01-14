#include <DCPS/DdsDcps_pch.h> // Only the _pch include should start with DCPS/

#include "DcpsUpcalls.h"

#include "Service_Participant.h"
#include "ThreadStatusManager.h"
OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

DcpsUpcalls::DcpsUpcalls(
  DataReaderCallbacks_rch drr,
  const GUID_t& reader,
  const WriterAssociation& wa,
  bool active,
  DataWriterCallbacks_rch dwr)
  : drr_(drr)
  , reader_(reader)
  , wa_(wa)
  , active_(active)
  , dwr_(dwr)
  , reader_done_(false)
  , writer_done_(false)
  , cnd_(mtx_)
{
}

int DcpsUpcalls::svc()
{
  ThreadStatusManager& thread_status_manager = TheServiceParticipant->get_thread_status_manager();
  const TimeDuration thread_status_interval = thread_status_manager.thread_status_interval();
  const bool update_thread_status = thread_status_manager.update_thread_status();

  ThreadStatusManager::Start s(thread_status_manager, "DcpsUpcalls");

  MonotonicTimePoint expire = MonotonicTimePoint::now() + thread_status_interval;

  DataReaderCallbacks_rch drr = drr_.lock();
  if (!drr) {
    return 0;
  }
  drr->add_association(reader_, wa_, active_);

  {
    ACE_GUARD_RETURN(ACE_Thread_Mutex, g, mtx_, -1);
    reader_done_ = true;
    cnd_.notify_one();
    while (!writer_done_) {
      if (update_thread_status) {
        switch (cnd_.wait_until(expire, thread_status_manager)) {
        case CvStatus_NoTimeout:
          break;

        case CvStatus_Timeout:
          expire = MonotonicTimePoint::now() + thread_status_interval;
          break;

        case CvStatus_Error:
          if (DCPS_debug_level) {
            ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: DcpsUpcalls::svc: error in wait_utill\n"));
          }
          return -1;
        }
      } else if (cnd_.wait(thread_status_manager) == CvStatus_Error) {
        if (DCPS_debug_level) {
          ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: DcpsUpcalls::svc: error in wait\n"));
        }
        return -1;
      }
    }
  }

  return 0;
}

void DcpsUpcalls::writer_done()
{
  {
    ACE_GUARD(ACE_Thread_Mutex, g, mtx_);
    writer_done_ = true;
    cnd_.notify_one();
  }

  ThreadStatusManager& thread_status_manager = TheServiceParticipant->get_thread_status_manager();
  ThreadStatusManager::Sleeper sleeper(thread_status_manager);
  wait(); // ACE_Task_Base::wait does not accept a timeout
}

} // namespace OpenDDS
} // namespace DCPS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
