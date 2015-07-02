/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "RTDMonitor.h"

#include "MonitorData.h"
#include "MonitorDataModel.h"
#include "Options.h"
#include "TreeNode.h"
#include <QtCore/QPersistentModelIndex>

#include "ace/Argv_Type_Converter.h"
#include "ace/Service_Config.h"
#include "dds/DCPS/Service_Participant.h"

#include <set>

// Note calls to QObject::toStdString() have been replaced with QObject::toLocal8Bit().constData()
// to avoid QString-related aborts under windows.

namespace Monitor {

///
/// Beginning of RTDMonitorImpl
///

class RTDMonitorImpl {

  public:
    RTDMonitorImpl(int argc, ACE_TCHAR **argv)
       : options_( argc, argv)
    {
      if (!TheServiceParticipant->monitor_) {
        // Initialize the service and consume the ACE+TAO+DDS arguments.
        TheParticipantFactoryWithArgs(argc, argv);
      }

      this->model_ = new MonitorDataModel();
      this->dataSource_ = new MonitorData( this->options_, this->model_, false);
    }

    ~RTDMonitorImpl()
    {
      this->dataSource_->disable();
      delete this->dataSource_;
      delete this->model_;
    }

    static void shutdown()
    {
      // Clean up the service resources.
      if (!TheServiceParticipant->monitor_) {
        TheServiceParticipant->shutdown();
      }
    }

    bool newRepo( const std::string& ior)
    {
      QString ior_str(ior.c_str());

      if (!ior.empty()) {
        if ( !this->dataSource_->setRepoIor( ior_str)) {
          this->dataSource_->removeRepo( ior_str);
          return false;
        }
      } else {
        this->dataSource_->stopInstrumentation();
        this->dataSource_->clearData();
        this->nodeIndexes_.clear();
      }
      return true;
    }

    bool isNodeValid( const std::string& id) const
    {
      return getNode(id) != NULL;
    }

    std::string getNodeValue(const std::string& nodeId, int column) const
    {
      TreeNode* node = getNode(nodeId);
      if (!node || column >= node->width()) {
        return "#N/A";
      }

      return node->column(column).toString().toLocal8Bit().constData();
    }

    std::string getNodeColor(const std::string& nodeId, int column) const
    {
      TreeNode* node = getNode(nodeId);
      if (!node || column >= node->width()) {
        return "";
      }

      return node->color(column).toString().toLocal8Bit().constData();
    }

    int getColumnCount() const
    {
      return this->dataSource_->modelRoot()->width();
    }

    std::list<std::string> getNodeChildren(const std::string& parentId) const
    {
      std::list<std::string> children;
      if (parentId.empty()) {
        if (this->dataSource_->modelRoot() != NULL) {
          QPersistentModelIndex pindex(this->model_->index(this->dataSource_->modelRoot(), 0));
          this->nodeIndexes_.insert(pindex);

          // Convert uint64 internal id to string.
          QString qstr = QString::number(pindex.internalId());

          children.push_back(qstr.toLocal8Bit().constData());
        }
      } else {
        TreeNode* parentNode = getNode(parentId);
          if (parentNode != NULL) {
          QList<TreeNode*> childNodes = parentNode->children();
          for (int i = 0; i < childNodes.count(); ++i) {
            QPersistentModelIndex pindex(this->model_->index(childNodes[i], 0));
            this->nodeIndexes_.insert(pindex);

            // Convert uint64 internal id to string.
            QString qstr = QString::number(pindex.internalId());

            children.push_back(qstr.toLocal8Bit().constData());
          }
        }
      }
      return children;
    }

    typedef std::set<QPersistentModelIndex> IndexList;

  private:
    Options           options_;
    MonitorDataModel* model_;
    MonitorData*      dataSource_;
    mutable IndexList nodeIndexes_;

    TreeNode* getNode( const std::string& id) const
    {
      QString qstr(id.c_str());
      qint64 internalId = qstr.toLongLong();

      for (IndexList::iterator iIndex = this->nodeIndexes_.begin();
           iIndex != this->nodeIndexes_.end(); ++iIndex) {
        QPersistentModelIndex index = *iIndex;
        if (index.isValid()) {
          if (index.internalId() == internalId) {
            return this->model_->getNode(index, false);
          }
        } else {
          this->nodeIndexes_.erase(iIndex);
          break;
        }
      }
      return NULL;
    }
};

///
/// End of RTDMonitorImpl
///

RTDMonitor::RTDMonitor(int argc, char* argv[])
{
  ACE_Argv_Type_Converter converter(argc, argv);

  impl_ = new RTDMonitorImpl(converter.get_argc(), converter.get_TCHAR_argv());
}

RTDMonitor::~RTDMonitor()
{
  delete impl_;
}

void
RTDMonitor::shutdown()
{
  RTDMonitorImpl::shutdown();
}

bool
RTDMonitor::newRepo(const std::string& ior)
{
  return impl_->newRepo(ior);
}

int
RTDMonitor::getColumnCount() const
{
  return impl_->getColumnCount();
}

bool
RTDMonitor::isNodeValid(const std::string& nodeId) const
{
  return impl_->isNodeValid(nodeId);
}

std::string
RTDMonitor::getNodeValue(const std::string& nodeId, int column) const
{
  return impl_->getNodeValue(nodeId, column);
}

std::string
RTDMonitor::getNodeColor(const std::string& nodeId, int column) const
{
  return impl_->getNodeColor(nodeId, column);
}

std::list<std::string>
RTDMonitor::getNodeChildren(const std::string& parentId) const
{
  return impl_->getNodeChildren(parentId);
}

} // End of namespace Monitor
