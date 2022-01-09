/*
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

struct ThreadStatusManager;

/**
 * @class ThreadMonitor
 *
 * @brief Defines the means of tracking thread utilization by measuring
 * time spent in event handling vs idle
 */
class OpenDDS_Dcps_Export ThreadMonitor
{
public:
  virtual ~ThreadMonitor();

  struct UpdateMode {
    bool implicit_;
    bool idle_;
    bool stacked_;
   };

  virtual size_t thread_count();

  virtual void summarize();

  virtual void report();

  virtual void preset(ThreadStatusManager*, const char*);

  /**
   * identify a thread as being busy or idle at a specific time.
   */
  virtual void update(UpdateMode, const char* = "");

  /**
   * retrieve the most recently measured thread business as a percentage
   * of the measured time span. Range is 0.0 to 100.0
   */
  virtual double get_utilization(const char* alias) const;

  static ThreadMonitor* installed_monitor_;

  /**
   * @class ThreadMonitor::GreenLight
   *
   * @brief Create an instance of a Green Light to indicate that the owning
   * thread is busy. The thread will then be timestamped as idle upon
   * destruction of the green light object.
   */
  class OpenDDS_Dcps_Export GreenLight
  {
  public:
    explicit GreenLight(const char* alias, bool initial = false);
    ~GreenLight();
  private:
    bool is_initial_;
  };

  /**
   * @class ThreadMonitor::RedLight
   *
   * @brief For finer thread load monitoring, use a RedLight object in
   * places where a greenlit thread is about to block.
   */
  class OpenDDS_Dcps_Export RedLight
  {
  public:
    explicit RedLight(const char* alias, bool final = false);
    ~RedLight();
  private:
    bool is_final_;
  };
};

} // namespace DCPS
} // namespace OpenDDS

OPENDDS_END_VERSIONED_NAMESPACE_DECL

#endif // OPENDDS_DCPS_THREAD_MONITOR_H_
