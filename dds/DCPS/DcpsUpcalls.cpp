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
  : drr_(drr), reader_(reader), wa_(wa), active_(active), dwr_(dwr)
  , reader_done_(false), writer_done_(false), cnd_(mtx_)
  , interval_(TheServiceParticipant->get_thread_status_interval())
  , thread_status_manager_(TheServiceParticipant->get_thread_status_manager())
  , thread_key_(ThreadStatusManager::get_key("DcpsUpcalls"))
{
}

int DcpsUpcalls::svc()
{
  MonotonicTimePoint expire;
  const bool update_thread_status = thread_status_manager_ && has_timeout();
  if (update_thread_status) {
    expire = MonotonicTimePoint::now() + interval_;
  }

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
        switch (cnd_.wait_until(expire)) {
        case CvStatus_NoTimeout:
          break;

        case CvStatus_Timeout:
          {
            expire = MonotonicTimePoint::now() + interval_;
            if (DCPS_debug_level > 4) {
              ACE_DEBUG((LM_DEBUG,
                         "(%P|%t) DcpsUpcalls::svc: Updating thread status\n"));
            }

            if (!thread_status_manager_->update(thread_key_)) {
              if (DCPS_debug_level) {
                ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: DcpsUpcalls::svc: "
                  "update failed\n"));
              }
              return -1;
            }
          }
          break;

        case CvStatus_Error:
          if (DCPS_debug_level) {
            ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: DcpsUpcalls::svc: error in wait_utill\n"));
          }
          return -1;
        }
      } else if (cnd_.wait() == CvStatus_Error) {
        if (DCPS_debug_level) {
          ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: DcpsUpcalls::svc: error in wait\n"));
        }
        return -1;
      }
    }
  }

  if (update_thread_status) {
    if (DCPS_debug_level > 4) {
      ACE_DEBUG((LM_DEBUG, "(%P|%t) DcpsUpcalls::svc: "
        "Updating thread status for the last time\n"));
    }
    if (!thread_status_manager_->update(thread_key_, ThreadStatus_Finished) &&
        DCPS_debug_level) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: DcpsUpcalls::svc: final update failed\n"));
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

  const MonotonicTimePoint expire = has_timeout() ?
    MonotonicTimePoint::now() + interval_ : MonotonicTimePoint();

  wait(); // ACE_Task_Base::wait does not accept a timeout

  if (thread_status_manager_ && has_timeout() && MonotonicTimePoint::now() > expire) {
    if (DCPS_debug_level > 4) {
      ACE_DEBUG((LM_DEBUG,
                 "(%P|%t) DcpsUpcalls::writer_done: Updating thread status\n"));
    }
    if (!thread_status_manager_->update(thread_key_) && DCPS_debug_level) {
      ACE_ERROR((LM_ERROR, "(%P|%t) ERROR: DcpsUpcalls::writer_done: "
        "update failed\n"));
    }
  }
}

} // namespace OpenDDS
} // namespace DCPS

OPENDDS_END_VERSIONED_NAMESPACE_DECL
