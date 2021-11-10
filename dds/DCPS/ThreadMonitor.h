/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#ifndef OPENDDS_DCPS_THREAD_MONITOR_H
#define OPENDDS_DCPS_THREAD_MONITOR_H

#include "dcps_export.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
#pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

OPENDDS_BEGIN_VERSIONED_NAMESPACE_DECL

namespace OpenDDS {
namespace DCPS {

/**
 * @class Thread_Monitor
 *
 * @brief Defines the means of tracking thread utilization by measuring
 * time spent in event handling vs idle
 */
  class OpenDDS_Dcps_Export Thread_Monitor
  {
  public:
    virtual ~Thread_Monitor()
    {
    }

    enum UpdateMode {
      EXPLICIT_IDLE,
      EXPLICIT_BUSY,
      IMPLICIT_IDLE,
      IMPLICIT_BUSY
    };

    virtual void update(UpdateMode, const char* = "")
    {
    }

    static Thread_Monitor* installed_monitor_;

    /**
     * @class Thread_Monitor::Green_Light
     *
     * @brief Create an instance of a Green Light to indicate that the owning
     * thread is busy. The thread will then be timestamped as idle upon
     * destruction of the green light object.
     */
    class Green_Light
    {
    public:
      Green_Light(const char* alias = "")
      {
        if (installed_monitor_) {
          installed_monitor_->update(EXPLICIT_BUSY, alias);
        }
      }

      ~Green_Light(void)
      {
        if (installed_monitor_) {
          installed_monitor_->update(IMPLICIT_IDLE);
        }
      }
    };

    /**
     * @class Thread_Monitor::Red_Light
     *
     * @brief For finer thread load monitoring, use a Red_Light object in
     * places where a greenlit thread is about to block.
     */
    class Red_Light
    {
    public:
      Red_Light(const char* alias = "")
      {
        if (installed_monitor_) {
          installed_monitor_->update(EXPLICIT_IDLE, alias);
        }
      }

      ~Red_Light(void)
      {
        if (installed_monitor_) {
          installed_monitor_->update(IMPLICIT_BUSY);
        }
      }
    };
  };

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_DCPS_THREAD_MONITOR_H_
