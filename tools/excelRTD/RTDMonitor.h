/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */
#ifndef RTDMONITOR_H
#define RTDMONITOR_H

#include <string>
#include <list>

namespace Monitor {

class RTDMonitorImpl;

/// Accessor class so the Excel RTD server can utilize the same
/// backend as the stand-alone Monitor app.
class RTDMonitor {
  public:
    RTDMonitor(int argc = 0,  char* argv[] = 0);
    ~RTDMonitor();

    bool newRepo(const std::string& ior);
    int getColumnCount() const;
    std::string getNodeValue(const std::string& nodeId, int column) const;
    std::string getNodeColor(const std::string& nodeId, int column) const;
    bool isNodeValid(const std::string& nodeId) const;

    static void shutdown();

    std::list<std::string> getNodeChildren(const std::string& parentId) const;

  private:
    RTDMonitorImpl* impl_;
};

} // End of namespace Monitor

#endif /* RTDMONITOR_H */
