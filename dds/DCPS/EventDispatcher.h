/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_EVENTDISPATCHER_H
#define OPENDDS_DCPS_EVENTDISPATCHER_H

#include "Definitions.h"
#include "RcObject.h"
#include "TimeTypes.h"

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS
{
namespace DCPS
{

struct OpenDDS_Dcps_Export EventBase : public virtual RcObject
{
  virtual ~EventBase();

  virtual void handle_event() = 0;
  virtual void handle_error();
  virtual void handle_cancel();

  void operator()();
};
typedef RcHandle<EventBase> EventBase_rch;

template <typename Delegate>
class PmfEvent : public EventBase
{
public:
  typedef void (Delegate::*PMF)();

  PmfEvent(RcHandle<Delegate> delegate, PMF function)
    : delegate_(delegate)
    , function_(function)
  {}

  void handle_event()
  {
    RcHandle<Delegate> handle = delegate_.lock();
    if (handle) {
      ((*handle).*function_)();
    }
  }

private:
  WeakRcHandle<Delegate> delegate_;
  PMF function_;
};

template <typename Delegate>
class PmfNowEvent : public EventBase
{
public:
  typedef void (Delegate::*PMF)(const MonotonicTimePoint&);

  PmfNowEvent(RcHandle<Delegate> delegate, PMF function)
    : delegate_(delegate)
    , function_(function)
  {}

  void handle_event()
  {
    RcHandle<Delegate> handle = delegate_.lock();
    if (handle) {
      ((*handle).*function_)(MonotonicTimePoint::now());
    }
  }

private:
  WeakRcHandle<Delegate> delegate_;
  PMF function_;
};

class OpenDDS_Dcps_Export EventDispatcher : public virtual RcObject
{
public:

  EventDispatcher();
  virtual ~EventDispatcher();

  virtual void shutdown(bool immediate = false) = 0;

  virtual bool dispatch(EventBase_rch event) = 0;

  virtual long schedule(EventBase_rch event, const MonotonicTimePoint& expiration = MonotonicTimePoint::now()) = 0;

  virtual size_t cancel(long id) = 0;

};
typedef RcHandle<EventDispatcher> EventDispatcher_rch;

} // DCPS
} // OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_DCPS_EVENTDISPATCHER_H
