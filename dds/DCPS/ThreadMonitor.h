/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_THREAD_MONITOR_H
#define OPENDDS_DCPS_THREAD_MONITOR_H

#include <string>

namespace OpenDDS {
namespace DCPS {

/**
 * @class ACE_Thread_Monitor
 *
 * @brief Defines the means of tracking thread utilization by measuring
 * time spent in event handling vs idle
 */
  class Thread_Monitor
  {
  public:
    enum UpdateMode {
      EXPLICIT_IDLE,
      EXPLICIT_BUSY,
      IMPLICIT_IDLE,
      IMPLICIT_BUSY
    };
    virtual void update(UpdateMode, const char* = "")
    {
    }

    static Thread_Monitor noop_monitor_;
    static Thread_Monitor* installed_monitor_;

    class Green_Light
    {
    public:
      Green_Light(const char* alias = "")
      {
        if (installed_monitor_ != nullptr) {
          installed_monitor_->update(EXPLICIT_BUSY, alias);
        }
      }

      ~Green_Light(void)
      {
        if (installed_monitor_ != nullptr) {
          installed_monitor_->update(IMPLICIT_IDLE);
        }
      }
    };

    class Red_Light
    {
    public:
      Red_Light(const char* alias = "")
      {
        if (installed_monitor_ != nullptr) {
          installed_monitor_->update(EXPLICIT_IDLE, alias);
        }
      }

      ~Red_Light(void)
      {
        if (installed_monitor_ != nullptr) {
          installed_monitor_->update(IMPLICIT_BUSY);
        }
      }
    };
  };
}
}

#endif // OPENDDS_DCPS_THREAD_MONITOR_H_
