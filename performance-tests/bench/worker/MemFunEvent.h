#pragma once

#include <dds/DCPS/EventDispatcher.h>

#include <memory>

namespace Bench {

/**
 * MemFunEvent is a helper class for adapting void member funtions of existing
 * classes into dispatchable events
 */
template <typename Delegate>
class MemFunEvent : public OpenDDS::DCPS::EventBase {
public:
  typedef void (Delegate::*PMF)();

  MemFunEvent(std::shared_ptr<Delegate> delegate, PMF function)
    : delegate_(delegate)
    , function_(function)
  {}

  void handle_event()
  {
    std::shared_ptr<Delegate> handle = delegate_.lock();
    if (handle) {
      ((*handle).*function_)();
    }
  }

private:
  std::weak_ptr<Delegate> delegate_;
  PMF function_;
};

}
