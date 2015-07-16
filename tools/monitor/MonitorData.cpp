/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "MonitorData.h"
#include "Options.h"
#include "MonitorTask.h"
#include "MonitorDataModel.h"
#include "MonitorDataStorage.h"

#include "dds/DCPS/Service_Participant.h"
#include "dds/monitor/monitorC.h"

Monitor::MonitorData::MonitorData( const Options& options, MonitorDataModel* model, bool mapExistingIORKeys)
 : enabled_( true),
   options_( options),
   model_( model),
   dataSource_( 0),
   storage_( new MonitorDataStorage( this))
{
  this->dataSource_ = new MonitorTask( this->storage_, this->options_, mapExistingIORKeys);
  this->dataSource_->start();
}

Monitor::MonitorData::~MonitorData()
{
  this->disable();
  delete this->dataSource_;
  delete this->storage_;
}

void
Monitor::MonitorData::disable()
{
  this->enabled_ = false;
}

void
Monitor::MonitorData::getIorList( QList<QString>& iorList)
{
  for( MonitorTask::IorKeyMap::const_iterator
       location = this->dataSource_->iorKeyMap().begin();
       location != this->dataSource_->iorKeyMap().end();
       ++location) {
    iorList.append( QString( QObject::tr( location->first.c_str())));
  }
}

bool
Monitor::MonitorData::setRepoIor( const QString& ior)
{
  if( !this->enabled_) {
    return false;
  }

  // Replace call to toStdString() with toLocal8Bit().constData()
  // to avoid QString-related aborts under windows.
  QByteArray ior_arr = ior.toLocal8Bit();
  const char* ior_str = ior_arr.constData();

  // Return successfully if the requested repository is already the
  // active repository.
  if( this->storage_->activeIor() == ior_str) {
    return true;
  }

  // Set the repository IOR.
  MonitorTask::RepoKey key
    = this->dataSource_->setRepoIor( ior_str);

  // Clear existing data.
  this->clearData();

  // Switch to the new repository.
  if( this->dataSource_->setActiveRepo( key)) {
    this->storage_->activeIor() = ior_str;
    return true;

  } else {
    return false;
  }
}

bool
Monitor::MonitorData::removeRepo( const QString& ior)
{
  if( !this->enabled_) {
    return false;
  }

  // Replace call to toStdString() with toLocal8Bit().constData()
  // to avoid QString-related aborts under windows.
  QByteArray ior_arr = ior.toLocal8Bit();
  const char* ior_str = ior_arr.constData();

  // Check if this is the currently monitored repository, and clear its
  // data if it is in the view.
  if( this->storage_->activeIor() == ior_str) {
    this->clearData();
  }

  // Look up the index for this IOR and remove it from the service.
  return true;
}

bool
Monitor::MonitorData::clearData()
{
  if( !this->enabled_) {
    return false;
  }

  // Copy out the previous header settings.
  QList< QVariant> list;
  int cols = this->model_->columnCount();
  for( int index = 0; index < cols; ++index) {
     QString value
       = this->model_->headerData( index, Qt::Horizontal).toString();
     list << value;
  }

  // Clear the mappings.
  this->storage_->reset();

  // Create a new tree to install fresh data into.
  TreeNode* root = new TreeNode( list);

  // Install the empty tree into the data model.
  this->model_->newRoot( root);

  // Clear the active IOR field.
  this->storage_->activeIor() = std::string();

  return true;
}

bool
Monitor::MonitorData::stopInstrumentation()
{
  if( !this->enabled_) {
    return false;
  }
  this->dataSource_->stopInstrumentation();

  return true;
}

bool
Monitor::MonitorData::getBuiltinTopicData(
  const OpenDDS::DCPS::GUID_t&      id,
  DDS::ParticipantBuiltinTopicData& data
)
{
  return this->dataSource_->getBuiltinTopicData( id, data);
}

bool
Monitor::MonitorData::getBuiltinTopicData(
  const OpenDDS::DCPS::GUID_t& id,
  DDS::TopicBuiltinTopicData&  data
)
{
  return this->dataSource_->getBuiltinTopicData( id, data);
}

bool
Monitor::MonitorData::getBuiltinTopicData(
  const OpenDDS::DCPS::GUID_t&      id,
  DDS::PublicationBuiltinTopicData& data
)
{
  return this->dataSource_->getBuiltinTopicData( id, data);
}

bool
Monitor::MonitorData::getBuiltinTopicData(
  const OpenDDS::DCPS::GUID_t&       id,
  DDS::SubscriptionBuiltinTopicData& data
)
{
  return this->dataSource_->getBuiltinTopicData( id, data);
}

Monitor::TreeNode*
Monitor::MonitorData::modelRoot()
{
  return this->model_->getNode( QModelIndex());
}

void
Monitor::MonitorData::updated( TreeNode* node, int column)
{
  this->model_->updated( node, column);
}

void
Monitor::MonitorData::updated( TreeNode* left, int lcol, TreeNode* right, int rcol)
{
  this->model_->updated( left, lcol, right, rcol);
}

void
Monitor::MonitorData::changed()
{
  this->model_->changed();
}

